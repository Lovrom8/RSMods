#pragma once

#include <functional>
#include <memory>

#include "CommandTypes.hpp"
#include "IMod.hpp"

namespace Framework {
	using ModFactory = std::unique_ptr<IMod>(*)();

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
