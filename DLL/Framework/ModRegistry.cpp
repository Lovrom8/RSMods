#include "ModRegistry.hpp"

#include <algorithm>
#include <deque>
#include <exception>
#include <iomanip>
#include <string>
#include <string_view>
#include <unordered_set>
#include <utility>
#include <vector>

#include "../Log.hpp"
#include "ConflictResolver.hpp"
#include "HudRegistry.hpp"
#include "MainThreadInbox.hpp"
#include "ModContext.hpp"
#include "ResourceLedger.hpp"

namespace Framework {
	PendingRegistration* g_modPendingHead = nullptr;

	// Registered initializes into Inactive; resolution moves Inactive <-> Active. A mod leaving
	// Active reverts synchronously (there are no in-flight callbacks to wait for). Faulted is terminal.
	// The complete lifecycle contract and transition diagram live in README.md.
	enum class ModState {
		Registered,
		Inactive,
		Active,
		Faulted,
	};

	enum class DeactivationReason {
		Disabled,
		Suppressed,
		Faulted,
	};

	// Stable identity token for the registry's ledger slot. Pointer used as owner key.
	static const char registryOwner = 0;

	namespace {
		ModState TeardownTargetState(DeactivationReason reason) {
			return reason == DeactivationReason::Faulted ? ModState::Faulted : ModState::Inactive;
		}

		const char* TeardownLogOutcome(DeactivationReason reason) {
			switch (reason) {
			case DeactivationReason::Disabled:   return " disabled";
			case DeactivationReason::Suppressed: return " suppressed by a higher-priority conflicting mod";
			case DeactivationReason::Faulted:    return " faulted";
			}

			return " deactivated";
		}
	}

	struct ModRegistry::Impl {
		using Hook = void (IMod::*)(ModContext&);

		// One flag per record, positionally indexed alongside `records` and `exclusiveResourcesByMod`.
		// vector<char> rather than vector<bool> so elements are addressable, plain bytes.
		using ActivationMask = std::vector<char>;

		struct Record {
			std::unique_ptr<IMod> mod;
			ModState state = ModState::Registered;
			bool inSong = false;
		};

		bool Invoke(Record& record, Hook hook, const char* where) {
			ctx.currentMod = record.mod.get();

			try {
				(record.mod.get()->*hook)(ctx);
				return true;
			}
			catch (const std::exception& ex) {
				LOG_ERROR("[Framework] " << record.mod->Id() << "::" << where << " threw: " << ex.what() << std::endl);
			}
			catch (...) {
				LOG_ERROR("[Framework] " << record.mod->Id() << "::" << where << " threw unknown exception" << std::endl);
			}

			return false;
		}

		bool IsRequestedSafe(Record& record) {
			ctx.currentMod = record.mod.get();

			try {
				return record.mod->IsEnabled(ctx);
			}
			catch (...) {
				LOG_ERROR("[Framework] " << record.mod->Id() << "::IsEnabled threw; treating as disabled" << std::endl);
				return false;
			}
		}

		void Fault(Record& record, const char* reason) {
			LOG_ERROR("[Framework] " << record.mod->Id() << " faulted (" << reason << "); it will no longer run" << std::endl);

			record.state = ModState::Faulted;
			Commands().RemoveMod(record.mod.get());
			Hud().RemoveMod(record.mod.get());
		}

		// Best-effort revert of live game state before a mod leaves Active.
		void Revert(Record& record) {
			if (record.inSong) {
				Invoke(record, &IMod::OnSongExit, "OnSongExit");
				record.inSong = false;
			}

			Invoke(record, &IMod::OnDisabled, "OnDisabled");
		}

		// Leaves Active: revert live game state immediately (there are no render callbacks to wait
		// for), then settle into Inactive - or Faulted when the teardown is itself a fault.
		void BeginTeardown(Record& record, DeactivationReason reason) {
			Revert(record);

			// Unlike command bindings, a HUD element is meaningless once the mod stops ticking,
			// so drop it on every deactivation. The mod re-publishes on its next active tick.
			Hud().RemoveMod(record.mod.get());

			const ModState target = TeardownTargetState(reason);
			if (target == ModState::Faulted) {
				Commands().RemoveMod(record.mod.get());
			}

			record.state = target;
			LOG_INFO("[Framework] " << record.mod->Id() << TeardownLogOutcome(reason) << std::endl);
		}

		// Fault an already-active mod: best-effort revert, then terminal Faulted.
		void EscalateToFaulted(Record& record) {
			if (record.state == ModState::Active) {
				BeginTeardown(record, DeactivationReason::Faulted);
			}
		}

		void HandleCommandFaults() {
			for (const IMod* mod : Commands().TakeFaultedMods()) {
				Record* record = FindByMod(mod);
				if (!record) continue;

				// An Inactive mod (initialized, not active) has no live game state to revert, so
				// fault it straight away; an Active one gets a best-effort revert first.
				if (record->state == ModState::Inactive) {
					Fault(*record, "threw in a command");
				}
				else {
					EscalateToFaulted(*record);
				}
			}
		}

		Record* FindByMod(const IMod* mod) {
			for (auto& record : records) {
				if (record.mod.get() == mod) {
					return &record;
				}
			}

			return nullptr;
		}

		// Single source of command-owner availability: the router asks this instead of caching its
		// own initialized/active bits. Reproduces the retired mirror exactly:
		//   Active   -> available for both Active and Initialized bindings
		//   Inactive -> Initialized-only (user-disabled or conflict-suppressed, still initialized)
		//   Registered / Faulted / unknown -> unavailable
		bool IsOwnerAvailable(const IMod* mod, Availability required) {
			const Record* record = FindByMod(mod);
			if (!record) return false;

			switch (record->state) {
			case ModState::Active:
				return true;
			case ModState::Inactive:
				return required == Availability::Initialized;
			default: // Registered, Faulted
				return false;
			}
		}

		void Activate(Record& record) {
			if (!Invoke(record, &IMod::OnEnabled, "OnEnabled")) {
				Fault(record, "threw in OnEnabled");
				return;
			}

			record.state = ModState::Active;
			LOG_INFO("[Framework] " << record.mod->Id() << " activated" << std::endl);
		}

		// Runs an active-only hook; on throw, tear down (best-effort revert) and fault immediately.
		bool InvokeActive(Record& record, Hook hook, const char* where) {
			if (Invoke(record, hook, where)) {
				return true;
			}

			BeginTeardown(record, DeactivationReason::Faulted);
			return false;
		}

		void ReconcileSongAndTick(Record& record, GamePhase phase) {
			const bool songPhase = phase == GamePhase::Song;

			if (songPhase && !record.inSong) {
				if (!InvokeActive(record, &IMod::OnSongEnter, "OnSongEnter")) return;
				record.inSong = true;
			}
			else if (!songPhase && record.inSong) {
				Invoke(record, &IMod::OnSongExit, "OnSongExit");
				record.inSong = false;
			}

			if (!InvokeActive(record, &IMod::OnTick, "OnTick")) return;
			if (phase == GamePhase::Menu && !InvokeActive(record, &IMod::OnMenuTick, "OnMenuTick")) return;
			if (phase == GamePhase::Song) InvokeActive(record, &IMod::OnSongTick, "OnSongTick");
		}

		void DrainSettings() {
			auto batch = Inbox().DrainSettings();
			if (batch.empty()) return;

			for (auto& apply : batch) {
				try {
					apply();
				}
				catch (...) {
					LOG_ERROR("[Framework] a queued settings update threw while applying" << std::endl);
				}
			}

			// Only settled Inactive/Active mods are notified (Registered isn't initialized yet;
			// Faulted is terminal). A mod suppressed this tick reverts synchronously below.
			for (auto& record : records) {
				if (IsResolutionEligible(record.state)) {
					Invoke(record, &IMod::OnSettingsChanged, "OnSettingsChanged");
				}
			}

			Commands().RefreshDiagnostics();
		}

		bool IsResolutionEligible(ModState state) const {
			return state == ModState::Inactive || state == ModState::Active;
		}

		void BuildResourceIndexIfNeeded() {
			if (!resourceIndexDirty) return;

			exclusiveResourcesByMod.assign(records.size(), {});

			for (size_t i = 0; i < records.size(); ++i) {
				for (std::string_view resource : records[i].mod->ClaimsExclusive()) {
					auto& resources = exclusiveResourcesByMod[i];
					const std::string owned(resource);

					if (std::ranges::find(resources, owned) == resources.end()) {
						resources.push_back(owned);
					}
				}
			}

			resourceIndexDirty = false;
		}

		ActivationMask ComputeRequestedActive() {
			ActivationMask requestedActive(records.size(), 0);

			for (size_t i = 0; i < records.size(); ++i) {
				if (IsResolutionEligible(records[i].state)) {
					requestedActive[i] = IsRequestedSafe(records[i]);
				}
			}

			return requestedActive;
		}

		ActivationMask SelectActiveMods(const ActivationMask& requestedActive, const std::unordered_set<std::string>& initiallyReserved = {}) {
			std::vector<Resolver::Candidate> candidates(records.size());
			
			for (size_t i = 0; i < records.size(); ++i) {
				Resolver::Candidate& candidate = candidates[i];
				candidate.id = records[i].mod->Id();
				candidate.priority = records[i].mod->Priority();
				candidate.exclusiveResources = &exclusiveResourcesByMod[i];
				candidate.requested = requestedActive[i] && IsResolutionEligible(records[i].state);
			}

			return Resolver::Resolve(candidates, initiallyReserved);
		}

		// Publish the selected winners' exclusive resources to the ledger so CC effects see what the
		// registry currently holds. Flattens each winner's resource list into one owner-wide claim.
		void PublishSelectedResources(const ActivationMask& selectedActive) {
			std::vector<std::string> winnerResources;

			for (size_t i = 0; i < records.size(); ++i) {
				if (!selectedActive[i]) continue;
				const auto& res = exclusiveResourcesByMod[i];
				winnerResources.insert(winnerResources.end(), res.begin(), res.end());
			}

			Ledger().Publish(&registryOwner, std::move(winnerResources));
		}

		// Tear down every Active mod the resolver did not select. Reverts run synchronously, so a
		// suppressed mod releases its exclusive resources before any replacement is activated.
		void BeginOutgoingTeardowns(const ActivationMask& requestedActive, const ActivationMask& selectedActive) {
			for (size_t i = 0; i < records.size(); ++i) {
				auto& record = records[i];

				if (record.state == ModState::Active && !selectedActive[i]) {
					const DeactivationReason reason = requestedActive[i]
						? DeactivationReason::Suppressed
						: DeactivationReason::Disabled;
					BeginTeardown(record, reason);
				}
			}
		}

		void ActivateAndTickSelected(const ActivationMask& selectedActive, GamePhase phase) {
			for (size_t i = 0; i < records.size(); ++i) {
				auto& record = records[i];

				if (!selectedActive[i]) continue;

				if (record.state != ModState::Active) Activate(record);
				if (record.state == ModState::Active) ReconcileSongAndTick(record, phase);
			}
		}

		std::vector<Record> records;
		ModContext ctx;
		bool resourceIndexDirty = false;
		std::vector<std::vector<std::string>> exclusiveResourcesByMod;
	};

	ModRegistry::ModRegistry() : impl(std::make_unique<Impl>()) {}
	ModRegistry::~ModRegistry() = default;

	void ModRegistry::InstantiatePending() {
		for (PendingRegistration* pending = g_modPendingHead; pending; pending = pending->next) {
			if (pending->factory) {
				Register(pending->factory());
			}
		}
	}

	void ModRegistry::Register(std::unique_ptr<IMod> mod) {
		const std::string_view id = mod->Id();

		if (std::ranges::any_of(impl->records, [id](const Impl::Record& r) { return r.mod->Id() == id; })) {
			LOG_ERROR("[Framework] Duplicate mod Id '" << id << "' - registration rejected" << std::endl);
			return;
		}

		LOG_INFO("[Framework] Registered mod: " << id << std::endl);
		impl->records.push_back(Impl::Record{ .mod = std::move(mod) });
		impl->resourceIndexDirty = true;
	}

	void ModRegistry::DispatchInitialize() {
		for (auto& record : impl->records) {
			if (record.state != ModState::Registered) continue;

			if (impl->Invoke(record, &IMod::OnInitialize, "OnInitialize")) {
				record.state = ModState::Inactive;
			}
			else {
				impl->Fault(record, "threw in OnInitialize");
			}
		}

		Commands().RefreshDiagnostics();
	}

	void ModRegistry::DispatchCommands(GamePhase phase, bool gameLoaded) {
		impl->ctx.phase = phase;

		// Always drain the inbox so startup input isn't replayed later; the router discards it
		// when the game isn't loaded yet. The router resolves owner availability through the
		// registry (single source of lifecycle truth) rather than a cached mirror.
		const auto events = Inbox().DrainKeyEvents();
		Commands().DispatchPending(impl->ctx, events, gameLoaded,
			[this](const IMod* mod, Availability required) { return impl->IsOwnerAvailable(mod, required); });
		impl->HandleCommandFaults();
	}

	void ModRegistry::EnqueueSettingsUpdate(std::function<void()> apply) {
		Inbox().PostSettingsUpdate(std::move(apply));
	}

	void ModRegistry::Tick(GamePhase phase) {
		impl->ctx.phase = phase;

		impl->HandleCommandFaults();
		impl->DrainSettings();
		impl->BuildResourceIndexIfNeeded();

		// Snapshot CC's live holdings before computing (read-seed → compute unlocked → publish).
		const auto ccHeld = Ledger().HeldExcluding(&registryOwner);
		const auto requestedActive = impl->ComputeRequestedActive();
		const auto selectedActive = impl->SelectActiveMods(requestedActive, ccHeld);
		impl->PublishSelectedResources(selectedActive);

		// Outgoing teardowns run before any activation so a replacement cannot acquire an exclusive
		// resource the outgoing mod hasn't released; reverts are synchronous, so one pass suffices.
		impl->BeginOutgoingTeardowns(requestedActive, selectedActive);
		impl->ActivateAndTickSelected(selectedActive, phase);
	}

	void ModRegistry::Shutdown() {
		for (auto& record : impl->records) {
			if (record.state == ModState::Registered)
				continue;

			if (record.state == ModState::Active) {
				impl->Revert(record);
			}

			impl->Invoke(record, &IMod::OnShutdown, "OnShutdown");
			Commands().RemoveMod(record.mod.get());
			Hud().RemoveMod(record.mod.get());
		}

		impl->records.clear();
		Ledger().Release(&registryOwner);
	}

	ModRegistry& Registry() {
		static ModRegistry instance;
		return instance;
	}
}
