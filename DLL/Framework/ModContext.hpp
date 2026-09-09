#pragma once

#include <string>
#include <string_view>
#include <utility>

#include "GamePhase.hpp"
#include "CommandRouter.hpp"
#include "HudRegistry.hpp"

namespace Settings {
	// Opaque declarations keep the framework core free of the whole of Settings.hpp; only ModContext.cpp pulls it in. 
	// Scoped enums default to an int underlying type, so these stay ABI-compatible with the definitions in Settings.hpp.
	enum class When;
	enum class StringColorMode;
	enum class NoteColorMode;
}

namespace Framework {
	class IMod;

	struct CommandBinder {
		CommandRouter& router;
		const IMod* mod;

		void BindSetting(std::string keySetting, KeyEdge edge, Availability availability,
			KeyAction action, KeyPredicate predicate = {}, std::string logMessage = {}) const {
			router.BindSetting(mod, std::move(keySetting), edge, availability, std::move(action),
				std::move(predicate), std::move(logMessage));
		}

		void BindKey(std::string name, std::uint32_t virtualKey, KeyEdge edge,
			Availability availability, KeyAction action,
			KeyPredicate predicate = {}, std::string logMessage = {}) const {
			router.BindKey(mod, std::move(name), virtualKey, edge, availability,
				std::move(action), std::move(predicate), std::move(logMessage));
		}
	};

	struct HudBinder {
		HudRegistry& hud;
		const IMod* mod;

		// Publish (or update) an on-screen text element keyed by `id` within this mod. Call it every
		// tick with the current snapshot; publish `{ .visible = false }` to hide it. The element is torn
		// down automatically when the mod deactivates or faults. `placement` brace-inits from a bare
		// anchor (order 0) for single-occupant elements: Set("id", { HudAnchor::TopLeft }, snapshot).
		void Set(std::string id, HudPlacement placement, HudText snapshot) const {
			hud.Set(mod, std::move(id), placement, std::move(snapshot));
		}
	};

	// Internal per-hook context
	struct ModContext {
		GamePhase phase = GamePhase::Loading;
		const IMod* currentMod = nullptr; // Set by the registry before each hook call.

		CommandBinder Commands() const { return { Framework::Commands(), currentMod }; }
		HudBinder Hud() const { return { Framework::Hud(), currentMod }; }

		// Defined in ModContext.cpp so this header stays free of Settings.hpp.
		bool IsOn(std::string_view key) const;
		bool IsOff(std::string_view key) const;
		std::string Value(std::string_view key) const;
		int  Int(std::string_view key) const;

		Settings::When When(std::string_view key) const;
		Settings::StringColorMode ColorMode() const;
		Settings::NoteColorMode NoteColorMode() const;
	};
}
