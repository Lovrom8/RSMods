#include "../MenuRegistry.hpp"

#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

using Framework::MenuRegistry;
using Framework::MenuEntry;
using Framework::Availability;
using Framework::IMod;

namespace {
	int g_failures = 0;

	void Check(const std::string& name, bool ok) {
		if (ok) {
			std::cout << "  PASS  " << name << "\n";
			return;
		}
		++g_failures;
		std::cout << "  FAIL  " << name << "\n";
	}

	const IMod* Owner(int n) { return reinterpret_cast<const IMod*>(static_cast<std::uintptr_t>(0x2000 + n)); }
}

int main() {
	std::cout << "MenuRegistry tests\n";

	// Basic registration and sorting: ordered by order ascending, then title.
	{
		MenuRegistry menus;
		int calledA = 0, calledB = 0, calledC = 0;

		menus.Register(Owner(1), "b", "Beta", 20, [&] { ++calledB; });
		menus.Register(Owner(1), "a", "Alpha", 10, [&] { ++calledA; });
		menus.Register(Owner(2), "c", "Charlie", 10, [&] { ++calledC; });

		const auto entries = menus.GetEntries();
		Check("returns all 3 entries", entries.size() == 3);
		Check("sorted by order asc, then title asc",
			entries[0].title == "Alpha" &&
			entries[1].title == "Charlie" &&
			entries[2].title == "Beta");

		entries[0].drawFn();
		entries[1].drawFn();
		entries[2].drawFn();
		Check("drawFn called properly", calledA == 1 && calledB == 1 && calledC == 1);
	}

	// Upsert replaces in place.
	{
		MenuRegistry menus;
		menus.Register(Owner(1), "settings", "Old Title", 5, [] {});
		menus.Register(Owner(1), "settings", "New Title", 15, [] {}, Availability::Initialized, true, true);

		const auto all = menus.GetEntries();
		Check("upsert replaces in place",
			all.size() == 1 &&
			all[0].title == "New Title" &&
			all[0].order == 15 &&
			all[0].availability == Availability::Initialized &&
			all[0].standaloneWindow == true &&
			all[0].playerFacing == true);
	}

	// Id is scoped per owner.
	{
		MenuRegistry menus;
		menus.Register(Owner(1), "menu", "Menu 1", 10, [] {});
		menus.Register(Owner(2), "menu", "Menu 2", 10, [] {});

		Check("id is scoped per owner", menus.GetEntries().size() == 2);
	}

	// RemoveMod drops only that owner's entries.
	{
		MenuRegistry menus;
		menus.Register(Owner(1), "m1", "Menu 1", 10, [] {});
		menus.Register(Owner(1), "m2", "Menu 2", 20, [] {});
		menus.Register(Owner(2), "m3", "Menu 3", 30, [] {});

		menus.RemoveMod(Owner(1));
		const auto all = menus.GetEntries();
		Check("RemoveMod is owner-scoped", all.size() == 1 && all[0].owner == Owner(2));
	}

	// PublishAvailability snapshot gating.
	{
		MenuRegistry menus;
		menus.Register(Owner(1), "active_only", "Active Item", 10, [] {}, Availability::Active);
		menus.Register(Owner(2), "initialized", "Initialized Item", 20, [] {}, Availability::Initialized);
		menus.Register(Owner(3), "unsettled_active", "Unsettled Item", 30, [] {}, Availability::Active);
		menus.Register(Owner(4), "inactive_active", "Inactive Item", 40, [] {}, Availability::Active);

		// Owner 2 and 4 are initialized but not active; owner 3 isn't settled at all.
		menus.PublishAvailability({
			{ Owner(1), Availability::Active },
			{ Owner(2), Availability::Initialized },
			{ Owner(4), Availability::Initialized }
		});

		const auto entries = menus.GetEntries();
		Check("snapshotted availability gates entries",
			entries.size() == 2 &&
			entries[0].id == "active_only" &&
			entries[1].id == "initialized");
	}

	// Overlays are Active-only, player-facing, and a later Register with the same id turns one back into a menu.
	{
		MenuRegistry menus;
		menus.RegisterOverlay(Owner(1), "hud", 5, [] {});
		menus.RegisterOverlay(Owner(2), "hud", 5, [] {});
		menus.PublishAvailability({
			{ Owner(1), Availability::Active },
			{ Owner(2), Availability::Initialized }
		});

		auto entries = menus.GetEntries();
		Check("overlay shows only while its owner is Active",
			entries.size() == 1 && entries[0].owner == Owner(1) && entries[0].overlay && entries[0].playerFacing);

		menus.Register(Owner(1), "hud", "Menu", 5, [] {});
		entries = menus.GetEntries();
		Check("re-registering as a menu clears overlay", entries.size() == 1 && !entries[0].overlay);
	}

	// Font loads run once each, in order; RemoveMod drops a pending one.
	{
		MenuRegistry menus;
		int ran = 0;
		menus.RegisterFonts(Owner(1), [&] { ran += 1; });
		menus.RegisterFonts(Owner(1), [&] { ran += 10; });
		menus.RegisterFonts(Owner(2), [&] { ran += 100; });
		menus.RegisterFonts(Owner(3), [&] { ran += 1000; });
		menus.RemoveMod(Owner(3));

		auto loads = menus.TakePendingFonts();
		for (const auto& load : loads) load.fn();
		Check("pending font loads: all kept, removed owner dropped, oldest first",
			ran == 111 && loads.size() == 3 && loads[0].owner == Owner(1) && loads[2].owner == Owner(2));
		Check("font loads are taken once", menus.TakePendingFonts().empty());

		menus.RegisterFonts(Owner(1), [&] { ran += 1; });
		Check("a retried mod's fonts load again", menus.TakePendingFonts().size() == 1);
	}

	std::cout << (g_failures == 0 ? "ALL MENUREGISTRY TESTS PASSED\n" : "MENUREGISTRY TESTS FAILED\n");
	return g_failures == 0 ? 0 : 1;
}
