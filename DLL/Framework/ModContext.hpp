#pragma once

#include <string>
#include <string_view>
#include <utility>

#include "GamePhase.hpp"
#include "CommandRouter.hpp"
#include "HudRegistry.hpp"
#include "MenuRegistry.hpp"
#include "DrawRegistry.hpp"

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

	struct MenuBinder {
		MenuRegistry& registry;
		const IMod* mod;

		void Register(std::string id, std::string title, int order, MenuDrawFn drawFn,
			Availability availability = Availability::Active, bool standaloneWindow = false) const {
			registry.Register(mod, std::move(id), std::move(title), order,
				std::move(drawFn), availability, standaloneWindow);
		}
	};

	struct DrawBinder {
		DrawRegistry& draw;
		const IMod* mod;

		void Register(std::string id, int priority, DrawPath path, DrawInterceptor fn) const {
			draw.Register(mod, std::move(id), priority, path, std::move(fn));
		}
	};

	// Internal per-hook context
	struct ModContext {
		GamePhase phase = GamePhase::Loading;
		const IMod* currentMod = nullptr; // Set by the registry before each hook call.
		bool fastTickRequested = false; // Raised by a mod that needs a tighter tick this pass; the registry resets and consumes it.

		// Ask the MainThread loop to shorten its next maintenance interval. Call it every pass the need
		// persists (e.g. while watching a loop end for a seek-back); it self-clears when no mod requests it.
		void RequestFastTick() { fastTickRequested = true; }

		CommandBinder Commands() const { return { Framework::Commands(), currentMod }; }
		HudBinder Hud() const { return { Framework::Hud(), currentMod }; }
		MenuBinder Menu() const { return { Framework::Menus(), currentMod }; }
		DrawBinder Draw() const { return { Framework::Draw(), currentMod }; }

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
