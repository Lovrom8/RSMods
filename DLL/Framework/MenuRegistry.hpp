#pragma once

#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "CommandTypes.hpp"

namespace Framework {
	class IMod;

	using MenuDrawFn = std::function<void()>;
	using FontLoadFn = std::function<void()>;

	struct FontLoad {
		const IMod* owner = nullptr;
		FontLoadFn fn;
	};

	struct MenuEntry {
		const IMod* owner = nullptr; // Opaque key; nullptr means system/shared entry.
		std::string id;
		std::string title;
		int order = 0;
		Availability availability = Availability::Active;
		MenuDrawFn drawFn;
		bool standaloneWindow = false; // true = drawFn manages its own window; false = rendered as collapsible header in RS Mods window
		bool playerFacing = false;     // Standalone windows only: shown in release builds too; everything else is a debug tool
		bool overlay = false;          // Drawn every frame whether the menu is open or not; see RegisterOverlay
	};

	class MenuRegistry {
	public:
		MenuRegistry();
		~MenuRegistry();

		MenuRegistry(const MenuRegistry&) = delete;
		MenuRegistry& operator=(const MenuRegistry&) = delete;

		// MainThread / initialization: register or update a menu entry.
		void Register(const IMod* owner, std::string id, std::string title, int order,
			MenuDrawFn drawFn, Availability availability = Availability::Active,
			bool standaloneWindow = false, bool playerFacing = false);

		// Something the mod draws in game with ImGui every frame, menu open or not, while it's Active. drawFn opens
		// its own window (with ImGuiWindowFlags_NoInputs unless it wants the mouse) or uses a background draw list.
		void RegisterOverlay(const IMod* owner, std::string id, int order, MenuDrawFn drawFn);

		// Fonts for the mod's ImGui drawing: `fn` calls ImGui::GetIO().Fonts->AddFont* and keeps the ImFont*s. Runs
		// once, on the render thread before the next ImGui frame. The atlas lives as long as the game (a device reset
		// only re-uploads its texture), so fonts never need adding again. One load per mod: calling again before it
		// runs replaces it, after it ran (a retried mod re-running OnInitialize) does nothing, so keep the ImFont*s.
		void RegisterFonts(const IMod* owner, FontLoadFn fn);

		// Render thread, outside an ImGui frame: the loads that haven't run yet, oldest first. The caller runs them
		// and rebuilds the atlas.
		[[nodiscard]] std::vector<FontLoad> TakePendingFonts();

		// Drop all menu entries and pending font loads owned by this mod (e.g. on mod teardown or fault).
		void RemoveMod(const IMod* owner);

		// MainThread: publish snapshot of owner availability.
		void PublishAvailability(std::unordered_map<const IMod*, Availability> availability);

		// Render thread: entries whose owners are available per the last published snapshot (all of them
		// before the first one), sorted by order asc, then title asc.
		[[nodiscard]] std::vector<MenuEntry> GetEntries() const;

	private:
		void Upsert(MenuEntry entry);

		struct Impl;
		std::unique_ptr<Impl> impl;
	};

	MenuRegistry& Menus();
}
