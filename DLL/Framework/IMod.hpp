#pragma once

#include <string_view>
#include <type_traits>
#include <vector>

#include "GamePhase.hpp"
#include "SettingsSchema.hpp"

// Keeps a mod's internal framework identity equal to its concrete class name.
// The assertion catches copying the macro from another class without updating its argument.
#define MOD_ID(Type)                                                   \
    std::string_view Id() const final {                                \
        using Self = std::remove_cvref_t<decltype(*this)>;             \
        static_assert(std::is_same_v<Type, Self>,                      \
            "MOD_ID must name the class that contains it");            \
        return #Type;                                                  \
    }

namespace Framework {
	struct ModContext;

	// Internal interface for built-in mods.
	class IMod {
	public:
		virtual ~IMod() = default;

		virtual std::string_view Id() const = 0;               // Unique internal name used for logs, duplicate detection, and conflict tie-breaking.

		// Must be cheap and side-effect free.
		virtual bool IsEnabled(const ModContext&) const { return true; }

		// Declarative configuration schema owned by this mod.
		virtual SettingDefs Settings() const { return {}; }

		// Losing any claimed resource suppresses the mod.
		virtual std::vector<std::string_view> ClaimsExclusive() const { return {}; }
		// Higher wins; ties favor the lexicographically smaller Id().
		virtual int Priority() const { return 0; }

		virtual void OnInitialize(ModContext&)      {} // Host services are ready; bind commands (ctx.Commands()) and do one-time setup here.
		virtual void OnShutdown(ModContext&)        {}

		virtual void OnSettingsChanged(ModContext&) {} // Applied AND delivered on MainThread.

		// OnEnabled MUST be strongly exception-safe: if it throws it becomes Faulted and
		// OnDisabled is NOT called, so it must leave no partially-applied game state behind.
		virtual void OnEnabled(ModContext&)  {} // Acquire game state here.
		virtual void OnDisabled(ModContext&) {} // REVERT game state here.

		virtual void OnTick(ModContext&)      {} // Every active tick, any phase (incl. Loading -> guard unsafe memory).
		virtual void OnMenuTick(ModContext&)  {} // In menus.
		virtual void OnSongEnter(ModContext&) {} // Edge: this mod became active-in-a-song (incl. enabled mid-song).
		virtual void OnSongTick(ModContext&)  {} // In a song.
		virtual void OnSongExit(ModContext&)  {} // Edge: this mod stopped being active-in-a-song (incl. disabled mid-song).
	};
}
