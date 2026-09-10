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

	struct MenuEntry {
		const IMod* owner = nullptr; // Opaque key; nullptr means system/shared entry.
		std::string id;
		std::string title;
		int order = 0;
		Availability availability = Availability::Active;
		MenuDrawFn drawFn;
		bool standaloneWindow = false; // true = drawFn manages its own window; false = rendered as collapsible header in RS Mods window
	};

	using OwnerAvailabilityFn = std::function<bool(const IMod*, Availability)>;

	class MenuRegistry {
	public:
		MenuRegistry();
		~MenuRegistry();

		MenuRegistry(const MenuRegistry&) = delete;
		MenuRegistry& operator=(const MenuRegistry&) = delete;

		// MainThread / initialization: register or update a menu entry.
		void Register(const IMod* owner, std::string id, std::string title, int order,
			MenuDrawFn drawFn, Availability availability = Availability::Active,
			bool standaloneWindow = false);

		// Drop all menu entries owned by this mod (e.g. on mod teardown or fault).
		void RemoveMod(const IMod* owner);

		// MainThread: publish snapshot of owner availability.
		void PublishAvailability(std::unordered_map<const IMod*, Availability> availability);

		// Returns a snapshot of entries whose owners are available, sorted by order asc, then title asc.
		[[nodiscard]] std::vector<MenuEntry> GetEntries(const OwnerAvailabilityFn& isOwnerAvailable = {}) const;

		// Snapshot of all registered entries regardless of owner availability.
		[[nodiscard]] std::vector<MenuEntry> SnapshotAll() const;

	private:
		struct Impl;
		std::unique_ptr<Impl> impl;
	};

	MenuRegistry& Menus();
}
