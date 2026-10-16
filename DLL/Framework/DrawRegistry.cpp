#include "DrawRegistry.hpp"
#include "IMod.hpp"

#include <algorithm>
#include <atomic>
#include <exception>
#include <iterator>
#include <mutex>
#include <utility>

namespace Framework {
	static DrawContext::CrcSource s_defaultCrcProvider = nullptr;

	void DrawContext::SetDefaultCrcProvider(CrcSource provider) {
		s_defaultCrcProvider = std::move(provider);
	}

	DrawContext::DrawContext(
		IDirect3DDevice9* dev,
		const Mesh& m,
		const ThiccMesh& t,
		DrawPath p,
		bool song,
		CrcSource customCrc
	) : device(dev), mesh(m), thicc(t), path(p), inSong(song), crcSource(std::move(customCrc)) {
		crcCache.reserve(4);
	}

	std::optional<DWORD> DrawContext::StageCRC(DWORD stage) {
		for (const auto& [cachedStage, crcVal] : crcCache) {
			if (cachedStage == stage) return crcVal;
		}

		std::optional<DWORD> computed;
		if (crcSource) {
			computed = crcSource(device, stage);
		}
		else if (s_defaultCrcProvider) {
			computed = s_defaultCrcProvider(device, stage);
		}

		crcCache.emplace_back(stage, computed);
		return computed;
	}

	void DrawContext::AfterDraw(std::function<void()> restore) {
		if (restore) restores.push_back(std::move(restore));
	}

	DrawContext::~DrawContext() {
		for (auto it = restores.rbegin(); it != restores.rend(); ++it) {
			// A throw out of a destructor terminates the game, and a restore has no owner to fault.
			try {
				(*it)();
			}
			catch (...) {
			}
		}
	}

	struct RegisteredInterceptor {
		const IMod* owner = nullptr;
		std::string id;
		int priority = 0;
		DrawPath path = DrawPath::Indexed;
		DrawInterceptor fn;
	};

	namespace {
		// One active list per single draw path, in DrawPath bit order.
		constexpr DrawPath singlePaths[] = { DrawPath::Indexed, DrawPath::Primitive, DrawPath::IndexedUP, DrawPath::PrimitiveUP };
		constexpr size_t pathCount = std::size(singlePaths);

		int PathSlot(DrawPath path) {
			for (size_t i = 0; i < pathCount; ++i) {
				if (singlePaths[i] == path) return static_cast<int>(i);
			}
			return -1;
		}

		template <typename Fn>
		struct OwnedCallback {
			const IMod* owner = nullptr;
			Fn fn;
		};

		template <typename Fn>
		void Upsert(std::vector<OwnedCallback<Fn>>& list, const IMod* owner, Fn fn) {
			auto it = std::find_if(list.begin(), list.end(), [owner](const OwnedCallback<Fn>& r) { return r.owner == owner; });
			if (it == list.end())
				list.push_back({ owner, std::move(fn) });
			else
				it->fn = std::move(fn);
		}
	}

	struct DrawRegistry::Impl {
		mutable std::mutex mutex;
		std::vector<RegisteredInterceptor> interceptors;
		std::vector<OwnedCallback<FrameCallback>> frameCallbacks;
		std::vector<OwnedCallback<DeviceResetCallback>> resetCallbacks;
		std::vector<OwnedCallback<DeviceResetCallback>> beforeResetCallbacks; // Not dropped by RemoveMod.
		std::vector<OwnedCallback<TextureRegenCallback>> regenCallbacks;
		std::vector<OwnedCallback<TextureReleaseCallback>> releaseCallbacks;
		std::vector<const IMod*> pendingReleases;  // owners waiting for deferred release
		std::vector<OwnedCallback<TextureReleaseCallback>> orphanedReleases; // queued by an owner removed before EndScene ran them
		std::vector<const IMod*> faultedOwners; // threw on the render thread; kept out of the snapshots until RemoveMod
		std::vector<RenderFault> pendingFaults; // drained by TakeFaults

		std::atomic<std::shared_ptr<const std::vector<ActiveEntry>>> active[pathCount];
		std::atomic<std::shared_ptr<const std::vector<OwnedCallback<FrameCallback>>>> activeFrame;

		DeviceChannels device; // Locks on its own; never touched under `mutex`.

		Impl() {
			auto empty = std::make_shared<const std::vector<ActiveEntry>>();
			for (auto& list : active) list.store(empty, std::memory_order_relaxed);
			activeFrame.store(std::make_shared<const std::vector<OwnedCallback<FrameCallback>>>(), std::memory_order_relaxed);
			device.SetFaultSink([this](const IMod* owner, const char* where, const char* detail) { NoteFault(owner, where, detail); });
		}

		bool IsFaultedLocked(const IMod* owner) const {
			return std::ranges::find(faultedOwners, owner) != faultedOwners.end();
		}

		// Republish the snapshots without `owner`, so the render thread stops calling it before the next rebuild.
		void UnpublishLocked(const IMod* owner) {
			for (auto& list : active) {
				const auto current = list.load(std::memory_order_acquire);
				auto kept = std::make_shared<std::vector<ActiveEntry>>();
				std::copy_if(current->begin(), current->end(), std::back_inserter(*kept), [owner](const ActiveEntry& e) { return e.owner != owner; });
				list.store(std::move(kept), std::memory_order_release);
			}

			const auto frames = activeFrame.load(std::memory_order_acquire);
			auto keptFrames = std::make_shared<std::vector<OwnedCallback<FrameCallback>>>();
			std::copy_if(frames->begin(), frames->end(), std::back_inserter(*keptFrames),
				[owner](const OwnedCallback<FrameCallback>& r) { return r.owner != owner; });
			activeFrame.store(std::move(keptFrames), std::memory_order_release);
		}

		// Render thread. Reports each owner once; MainThread faults it, and RemoveMod clears the mark.
		void NoteFault(const IMod* owner, const char* where, const char* detail) {
			{
				std::lock_guard<std::mutex> lock(mutex);
				if (owner && IsFaultedLocked(owner)) return;

				pendingFaults.push_back({ owner, where, detail });
				if (!owner) return;

				faultedOwners.push_back(owner);
				UnpublishLocked(owner);
			}

			device.Suspend(owner);
		}

		template <typename Fn>
		void Guarded(const IMod* owner, const char* where, Fn&& fn) {
			try {
				fn();
			}
			catch (const std::exception& ex) {
				NoteFault(owner, where, ex.what());
			}
			catch (...) {
				NoteFault(owner, where, "threw an unknown exception");
			}
		}

		// Copies the live callbacks under the lock, so they run without it.
		template <typename Fn>
		std::vector<OwnedCallback<Fn>> CopyCallbacks(const std::vector<OwnedCallback<Fn>>& list, bool skipFaulted) const {
			std::lock_guard<std::mutex> lock(mutex);
			std::vector<OwnedCallback<Fn>> callbacks;
			callbacks.reserve(list.size());

			for (const auto& r : list) {
				if (r.fn && !(skipFaulted && IsFaultedLocked(r.owner))) callbacks.push_back(r);
			}
			return callbacks;
		}
	};


	DrawRegistry::DrawRegistry() : impl(std::make_unique<Impl>()) {}
	DrawRegistry::~DrawRegistry() = default;

	void DrawRegistry::Register(const IMod* owner, std::string id, int priority, DrawPath path, DrawInterceptor fn) {
		std::lock_guard<std::mutex> lock(impl->mutex);

		auto it = std::find_if(impl->interceptors.begin(), impl->interceptors.end(),
			[&](const RegisteredInterceptor& r) { return r.owner == owner && r.id == id; });

		if (it == impl->interceptors.end()) {
			impl->interceptors.push_back(RegisteredInterceptor{
				owner, std::move(id), priority, path, std::move(fn)
			});
		}
		else {
			it->priority = priority;
			it->path = path;
			it->fn = std::move(fn);
		}
	}

	void DrawRegistry::RegisterTextureLifecycle(const IMod* owner, TextureRegenCallback regenFn, TextureReleaseCallback releaseFn) {
		std::lock_guard<std::mutex> lock(impl->mutex);
		Upsert(impl->regenCallbacks, owner, std::move(regenFn));
		Upsert(impl->releaseCallbacks, owner, std::move(releaseFn));
	}

	void DrawRegistry::RegisterTextureRegen(const IMod* owner, TextureRegenCallback fn) {
		std::lock_guard<std::mutex> lock(impl->mutex);
		Upsert(impl->regenCallbacks, owner, std::move(fn));
	}

	void DrawRegistry::RequestTextureRelease(const IMod* owner) {
		std::lock_guard<std::mutex> lock(impl->mutex);
		// Only enqueue if not already pending (idempotent)
		auto it = std::find(impl->pendingReleases.begin(), impl->pendingReleases.end(), owner);
		if (it == impl->pendingReleases.end())
			impl->pendingReleases.push_back(owner);
	}

	void DrawRegistry::CancelTextureRelease(const IMod* owner) {
		std::lock_guard<std::mutex> lock(impl->mutex);
		std::erase_if(impl->pendingReleases, [owner](const IMod* p) { return p == owner; });
		// A release orphaned by a fault would otherwise free what the retried mod just regenerated.
		std::erase_if(impl->orphanedReleases, [owner](const OwnedCallback<TextureReleaseCallback>& r) { return r.owner == owner; });
	}

	void DrawRegistry::RegisterFrame(const IMod* owner, FrameCallback fn) {
		std::lock_guard<std::mutex> lock(impl->mutex);
		Upsert(impl->frameCallbacks, owner, std::move(fn));
	}

	void DrawRegistry::RegisterDeviceReset(const IMod* owner, DeviceResetCallback fn) {
		std::lock_guard<std::mutex> lock(impl->mutex);
		Upsert(impl->resetCallbacks, owner, std::move(fn));
	}

	void DrawRegistry::RegisterBeforeReset(const IMod* owner, DeviceResetCallback fn) {
		std::lock_guard<std::mutex> lock(impl->mutex);
		Upsert(impl->beforeResetCallbacks, owner, std::move(fn));
	}

	void DrawRegistry::RemoveMod(const IMod* owner) {
		impl->device.RemoveMod(owner);

		std::lock_guard<std::mutex> lock(impl->mutex);

		// A mod faulting while Active queues its release in OnDisabled and is removed in the same call;
		// keep that release so its textures still get freed at the next EndScene.
		if (std::ranges::find(impl->pendingReleases, owner) != impl->pendingReleases.end()) {
			auto it = std::find_if(impl->releaseCallbacks.begin(), impl->releaseCallbacks.end(),
				[owner](const OwnedCallback<TextureReleaseCallback>& r) { return r.owner == owner; });
			if (it != impl->releaseCallbacks.end() && it->fn)
				impl->orphanedReleases.push_back(*it);
		}

		std::erase_if(impl->frameCallbacks, [owner](const OwnedCallback<FrameCallback>& r) { return r.owner == owner; });
		std::erase_if(impl->resetCallbacks, [owner](const OwnedCallback<DeviceResetCallback>& r) { return r.owner == owner; });
		std::erase_if(impl->interceptors, [owner](const RegisteredInterceptor& r) { return r.owner == owner; });
		std::erase_if(impl->regenCallbacks, [owner](const OwnedCallback<TextureRegenCallback>& r) { return r.owner == owner; });
		std::erase_if(impl->releaseCallbacks, [owner](const OwnedCallback<TextureReleaseCallback>& r) { return r.owner == owner; });
		std::erase_if(impl->pendingReleases, [owner](const IMod* p) { return p == owner; });
		std::erase(impl->faultedOwners, owner);
		std::erase_if(impl->pendingFaults, [owner](const RenderFault& f) { return f.owner == owner; });

		// Like the device channels: a faulted mod must stop drawing now, not at the end-of-tick rebuild.
		impl->UnpublishLocked(owner);
	}

	DeviceChannels& DrawRegistry::Device() {
		return impl->device;
	}

	void DrawRegistry::RebuildActive(std::function<bool(const IMod*)> isOwnerEnabled) {
		impl->device.Rebuild(isOwnerEnabled);

		std::lock_guard<std::mutex> lock(impl->mutex);

		std::shared_ptr<std::vector<ActiveEntry>> lists[pathCount];
		for (auto& list : lists) list = std::make_shared<std::vector<ActiveEntry>>();

		for (const auto& reg : impl->interceptors) {
			if (!reg.owner || !reg.fn || impl->IsFaultedLocked(reg.owner) || !isOwnerEnabled(reg.owner)) continue;

			ActiveEntry entry{
				reg.priority,
				reg.owner->Id(),
				reg.fn,
				reg.owner
			};

			for (size_t i = 0; i < pathCount; ++i) {
				if (Includes(reg.path, singlePaths[i])) lists[i]->push_back(entry);
			}
		}

		auto entryComparator = [](const ActiveEntry& a, const ActiveEntry& b) {
			if (a.priority != b.priority) return a.priority < b.priority;
			return a.ownerId < b.ownerId;
		};

		for (size_t i = 0; i < pathCount; ++i) {
			std::stable_sort(lists[i]->begin(), lists[i]->end(), entryComparator);
			impl->active[i].store(lists[i], std::memory_order_release);
		}

		// Frame callbacks in owner-Id order, so the run order doesn't depend on registration order.
		std::vector<const OwnedCallback<FrameCallback>*> enabledFrames;
		for (const auto& reg : impl->frameCallbacks) {
			if (reg.owner && reg.fn && !impl->IsFaultedLocked(reg.owner) && isOwnerEnabled(reg.owner)) enabledFrames.push_back(&reg);
		}
		std::stable_sort(enabledFrames.begin(), enabledFrames.end(),
			[](const auto* a, const auto* b) { return a->owner->Id() < b->owner->Id(); });

		auto frames = std::make_shared<std::vector<OwnedCallback<FrameCallback>>>();
		frames->reserve(enabledFrames.size());
		for (const auto* reg : enabledFrames) frames->push_back(*reg);
		impl->activeFrame.store(frames, std::memory_order_release);
	}

	std::vector<RenderFault> DrawRegistry::TakeFaults() {
		std::lock_guard<std::mutex> lock(impl->mutex);
		return std::exchange(impl->pendingFaults, {});
	}

	std::shared_ptr<const std::vector<ActiveEntry>> DrawRegistry::ActiveSnapshot(DrawPath path) const {
		const int slot = PathSlot(path);
		if (slot < 0) {
			static const auto empty = std::make_shared<const std::vector<ActiveEntry>>();
			return empty;
		}
		return impl->active[slot].load(std::memory_order_acquire);
	}

	DrawResult DrawRegistry::Intercept(const ActiveEntry& entry, DrawContext& ctx) {
		DrawResult result{ DrawOutcome::Pass };
		impl->Guarded(entry.owner, "draw interceptor", [&] { result = entry.fn(ctx); });
		return result;
	}

	void DrawRegistry::RunFrame(IDirect3DDevice9* pDevice) {
		if (!pDevice) return;

		const auto frames = impl->activeFrame.load(std::memory_order_acquire);
		for (const auto& r : *frames) {
			impl->Guarded(r.owner, "frame callback", [&] { r.fn(pDevice); });
		}
	}

	void DrawRegistry::RunBeforeReset(IDirect3DDevice9* pDevice) {
		if (!pDevice) return;

		for (const auto& r : impl->CopyCallbacks(impl->beforeResetCallbacks, false)) {
			impl->Guarded(r.owner, "before-reset callback", [&] { r.fn(pDevice); });
		}
	}

	void DrawRegistry::RunDeviceReset(IDirect3DDevice9* pDevice) {
		if (!pDevice) return;

		for (const auto& r : impl->CopyCallbacks(impl->resetCallbacks, true)) {
			impl->Guarded(r.owner, "device-reset callback", [&] { r.fn(pDevice); });
		}
	}

	void DrawRegistry::RegenerateAllTextures(IDirect3DDevice9* pDevice) {
		if (!pDevice) return;

		for (const auto& r : impl->CopyCallbacks(impl->regenCallbacks, true)) {
			impl->Guarded(r.owner, "texture regen", [&] { r.fn(pDevice); });
		}
	}

	void DrawRegistry::RunPendingReleases() {
		std::vector<OwnedCallback<TextureReleaseCallback>> callbacks;
		{
			std::lock_guard<std::mutex> lock(impl->mutex);
			if (impl->pendingReleases.empty() && impl->orphanedReleases.empty()) return;

			callbacks = std::exchange(impl->orphanedReleases, {});
			for (const IMod* owner : impl->pendingReleases) {
				auto it = std::find_if(impl->releaseCallbacks.begin(), impl->releaseCallbacks.end(),
					[owner](const OwnedCallback<TextureReleaseCallback>& r) { return r.owner == owner; });
				if (it != impl->releaseCallbacks.end() && it->fn)
					callbacks.push_back(*it);
			}
			impl->pendingReleases.clear();
		}

		for (const auto& r : callbacks) {
			impl->Guarded(r.owner, "texture release", [&] { r.fn(); });
		}
	}

	DrawRegistry& Draw() {
		static DrawRegistry instance;
		return instance;
	}
}
