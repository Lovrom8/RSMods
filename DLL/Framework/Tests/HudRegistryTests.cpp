#include "../HudRegistry.hpp"

#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

using Framework::HudRegistry;
using Framework::HudElement;
using Framework::HudText;
using Framework::HudPlacement;
using Framework::HudAnchor;
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

	// Owners are opaque keys the registry never dereferences, so fake distinct addresses stand in for mods.
	const IMod* Owner(int n) { return reinterpret_cast<const IMod*>(static_cast<std::uintptr_t>(0x1000 + n)); }

	HudText Text(bool visible, std::string text) {
		HudText t;
		t.visible = visible;
		t.text = std::move(text);
		return t;
	}

	const HudElement* Find(const std::vector<HudElement>& v, const IMod* owner, const std::string& id) {
		for (const auto& e : v) {
			if (e.owner == owner && e.id == id) return &e;
		}
		return nullptr;
	}
}

int main() {
	std::cout << "HudRegistry tests\n";

	// Only visible elements come back.
	{
		HudRegistry hud;
		hud.Set(Owner(1), "a", { HudAnchor::TopLeft, 0 }, Text(true, "shown"));
		hud.Set(Owner(1), "b", { HudAnchor::TopLeft, 1 }, Text(false, "hidden"));
		const auto snap = hud.SnapshotVisible();
		Check("only visible elements returned", snap.size() == 1 && snap[0].id == "a");
	}

	// Placement round-trips onto the element.
	{
		HudRegistry hud;
		hud.Set(Owner(1), "a", { HudAnchor::TopRight, 7 }, Text(true, "x"));
		const HudElement* e = Find(hud.SnapshotVisible(), Owner(1), "a");
		Check("anchor + order round-trip", e && e->anchor == HudAnchor::TopRight && e->order == 7);
	}

	// Same (owner, id) upserts in place - replaces snapshot and placement, never duplicates.
	{
		HudRegistry hud;
		hud.Set(Owner(1), "a", { HudAnchor::TopLeft, 0 }, Text(true, "first"));
		hud.Set(Owner(1), "a", { HudAnchor::TopCenter, 3 }, Text(true, "second"));
		const auto snap = hud.SnapshotVisible();
		const HudElement* e = Find(snap, Owner(1), "a");
		Check("upsert replaces in place", snap.size() == 1 && e && e->snapshot.text == "second"
			&& e->anchor == HudAnchor::TopCenter && e->order == 3);
	}

	// A hidden upsert retracts a previously visible element.
	{
		HudRegistry hud;
		hud.Set(Owner(1), "a", { HudAnchor::TopLeft, 0 }, Text(true, "on"));
		hud.Set(Owner(1), "a", { HudAnchor::TopLeft, 0 }, Text(false, "off"));
		Check("visible:false hides an element", hud.SnapshotVisible().empty());
	}

	// The same id under different owners are distinct elements.
	{
		HudRegistry hud;
		hud.Set(Owner(1), "a", { HudAnchor::TopLeft, 0 }, Text(true, "one"));
		hud.Set(Owner(2), "a", { HudAnchor::TopLeft, 0 }, Text(true, "two"));
		Check("id is scoped per owner", hud.SnapshotVisible().size() == 2);
	}

	// RemoveMod drops only that owner's elements.
	{
		HudRegistry hud;
		hud.Set(Owner(1), "a", { HudAnchor::TopLeft, 0 }, Text(true, "one"));
		hud.Set(Owner(1), "b", { HudAnchor::TopLeft, 1 }, Text(true, "two"));
		hud.Set(Owner(2), "a", { HudAnchor::TopLeft, 0 }, Text(true, "keep"));
		hud.RemoveMod(Owner(1));
		const auto snap = hud.SnapshotVisible();
		Check("RemoveMod is owner-scoped", snap.size() == 1 && snap[0].owner == Owner(2));
	}

	std::cout << (g_failures == 0 ? "ALL HUDREGISTRY TESTS PASSED\n" : "HUDREGISTRY TESTS FAILED\n");
	return g_failures == 0 ? 0 : 1;
}
