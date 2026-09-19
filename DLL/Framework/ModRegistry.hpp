#pragma once

#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "CommandTypes.hpp"
#include "IMod.hpp"

namespace Framework {
	using ModFactory = std::unique_ptr<IMod>(*)();

	// Splits the internal Inactive back into disabled vs suppressed, the distinction the status view needs.
	enum class ModStatusKind {
		Registered,  // Not yet initialized (transient).
		Active,      // Enabled and winning every claimed resource; only these get tick hooks.
		Disabled,    // IsEnabled() == false.
		Suppressed,  // Enabled but lost a resource conflict.
		Faulted,     // A hook threw; terminal.
	};

	inline const char* ToString(ModStatusKind kind) {
		switch (kind) {
		case ModStatusKind::Registered: return "Registered";
		case ModStatusKind::Active:     return "Active";
		case ModStatusKind::Disabled:   return "Disabled";
		case ModStatusKind::Suppressed: return "Suppressed";
		case ModStatusKind::Faulted:    return "Faulted";
		}
		return "Unknown";
	}

	// Owns its strings so the render thread can read it after the registry has moved on.
	struct ModStatus {
		std::string id;
		ModStatusKind kind = ModStatusKind::Registered;
		bool inSong = false;
		int priority = 0;
		std::vector<std::string> claimsExclusive; // Explains a Suppressed line.
	};

	struct PendingRegistration;
	extern PendingRegistration* g_modPendingHead;

	struct PendingRegistration {
		ModFactory factory;
		PendingRegistration* next;

		explicit PendingRegistration(ModFactory factory) noexcept
			: factory(factory), next(g_modPendingHead) {
			g_modPendingHead = this;
		}
	};

	class ModRegistry {
	public:
		ModRegistry();
		~ModRegistry();

		ModRegistry(const ModRegistry&) = delete;
		ModRegistry& operator=(const ModRegistry&) = delete;

		void InstantiatePending();
		void Register(std::unique_ptr<IMod> mod);
		void DispatchInitialize();
		void DispatchCommands(GamePhase phase, bool gameLoaded);
		void EnqueueSettingsUpdate(std::function<void()> apply);
		bool Tick(GamePhase phase); // Returns whether any ticked mod requested a tighter next interval.
		bool IsOwnerAvailable(const IMod* mod, Availability required = Availability::Active) const;

		// Thread-safe; call it from the render thread. Ordered by registration.
		[[nodiscard]] std::vector<ModStatus> StatusSnapshot() const;

		void Shutdown();

	private:
		struct Impl;
		std::unique_ptr<Impl> impl;
	};

	ModRegistry& Registry();

	template <typename T>
	struct ModRegistrar {
		static std::unique_ptr<IMod> Create() { return std::make_unique<T>(); }
		PendingRegistration node{ &Create };
	};
}
