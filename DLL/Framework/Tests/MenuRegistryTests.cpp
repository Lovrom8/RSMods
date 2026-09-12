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
		menus.Register(Owner(1), "settings", "New Title", 15, [] {}, Availability::Initialized, true);

		const auto all = menus.SnapshotAll();
		Check("upsert replaces in place",
			all.size() == 1 &&
			all[0].title == "New Title" &&
			all[0].order == 15 &&
			all[0].availability == Availability::Initialized &&
			all[0].standaloneWindow == true);
	}

	// Id is scoped per owner.
	{
		MenuRegistry menus;
		menus.Register(Owner(1), "menu", "Menu 1", 10, [] {});
		menus.Register(Owner(2), "menu", "Menu 2", 10, [] {});

		Check("id is scoped per owner", menus.SnapshotAll().size() == 2);
	}

	// Availability gating.
	{
		MenuRegistry menus;
		menus.Register(Owner(1), "active_only", "Active Item", 10, [] {}, Availability::Active);
		menus.Register(Owner(2), "initialized", "Initialized Item", 20, [] {}, Availability::Initialized);

		// Owner 1 is inactive, Owner 2 is inactive (but initialized).
		auto isAvailable = [](const IMod* owner, Availability required) {
			if (owner == Owner(1)) return false; // Inactive
			if (owner == Owner(2)) return required == Availability::Initialized; // Initialized
			return false;
		};

		const auto entries = menus.GetEntries(isAvailable);
		Check("gated on owner availability", entries.size() == 1 && entries[0].id == "initialized");
	}

	// RemoveMod drops only that owner's entries.
	{
		MenuRegistry menus;
		menus.Register(Owner(1), "m1", "Menu 1", 10, [] {});
		menus.Register(Owner(1), "m2", "Menu 2", 20, [] {});
		menus.Register(Owner(2), "m3", "Menu 3", 30, [] {});

		menus.RemoveMod(Owner(1));
		const auto all = menus.SnapshotAll();
		Check("RemoveMod is owner-scoped", all.size() == 1 && all[0].owner == Owner(2));
	}

	// PublishAvailability snapshot gating.
	{
		MenuRegistry menus;
		menus.Register(Owner(1), "active_only", "Active Item", 10, [] {}, Availability::Active);
		menus.Register(Owner(2), "initialized", "Initialized Item", 20, [] {}, Availability::Initialized);
		menus.Register(Owner(3), "unsettled_active", "Unsettled Item", 30, [] {}, Availability::Active);

		menus.PublishAvailability({
			{ Owner(1), Availability::Active },
			{ Owner(2), Availability::Initialized }
		});

		const auto entries = menus.GetEntries();
		Check("snapshotted availability gates entries",
			entries.size() == 2 &&
			entries[0].id == "active_only" &&
			entries[1].id == "initialized");
	}

	std::cout << (g_failures == 0 ? "ALL MENUREGISTRY TESTS PASSED\n" : "MENUREGISTRY TESTS FAILED\n");
	return g_failures == 0 ? 0 : 1;
}
