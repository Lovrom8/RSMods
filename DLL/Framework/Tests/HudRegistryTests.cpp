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
using Framework::HudArea;
using Framework::HudBand;
using Framework::HudAlign;
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

	// Full-display bands match the old overlay's LONG-truncated formulas exactly.
	{
		const float w = 1920.0f, h = 1080.0f;
		const HudArea full{ 0.0f, 0.0f, w, h };
		const auto legacy = [](float v) { return static_cast<float>(static_cast<long>(v)); };

		const HudBand tr = Framework::AnchorBand(HudAnchor::TopRight, full);
		Check("TopRight matches legacy band", tr.left == legacy(w - w / 16.0f) && tr.right == legacy(w - w / 96.0f)
			&& tr.top == legacy(h / 54.0f) && tr.align == HudAlign::Right);

		const HudBand tc = Framework::AnchorBand(HudAnchor::TopCenter, full);
		Check("TopCenter matches legacy band", tc.left == legacy(w / 2.0f - w / 38.4f)
			&& tc.right == legacy(w / 2.0f + w / 38.4f) && tc.align == HudAlign::Center);

		const HudBand hl = Framework::AnchorBand(HudAnchor::HighwayLeft, full);
		Check("HighwayLeft matches legacy band", hl.left == legacy(w / 5.5f) && hl.top == legacy(h / 1.75f)
			&& hl.align == HudAlign::Left);

		const HudBand tl = Framework::AnchorBand(HudAnchor::TopLeft, full);
		Check("TopLeft matches legacy band", tl.left == legacy(w / 96.0f) && tl.top == legacy(h / 54.0f));
	}

	// A confined area shifts every band by its left edge and scales by its width.
	{
		const HudArea band{ 1280.0f, 0.0f, 2560.0f, 1440.0f };
		const HudArea plain{ 0.0f, 0.0f, 2560.0f, 1440.0f };
		const HudBand confined = Framework::AnchorBand(HudAnchor::TopRight, band);
		const HudBand reference = Framework::AnchorBand(HudAnchor::TopRight, plain);
		Check("confined band is the 16:9 band shifted", confined.left == reference.left + 1280.0f
			&& confined.right == reference.right + 1280.0f && confined.top == reference.top);
	}

	// CenteredArea: 32:9 confined to a centred 16:9 band; displays not wider than the aspect stay full.
	{
		const HudArea wide = Framework::CenteredArea(5120.0f, 1440.0f, 16.0f / 9.0f);
		Check("32:9 centres a 16:9 band", wide.left == 1280.0f && wide.width == 2560.0f && wide.height == 1440.0f);

		const HudArea exact = Framework::CenteredArea(1920.0f, 1080.0f, 16.0f / 9.0f);
		Check("16:9 display stays full", exact.left == 0.0f && exact.width == 1920.0f);

		const HudArea narrow = Framework::CenteredArea(1280.0f, 1024.0f, 16.0f / 9.0f);
		Check("narrower display stays full", narrow.left == 0.0f && narrow.width == 1280.0f);

		const HudArea none = Framework::CenteredArea(5120.0f, 1440.0f, 0.0f);
		Check("aspect 0 means whole display", none.left == 0.0f && none.width == 5120.0f);
	}

	// SetLayoutAspect drives LayoutArea; 0 and junk values fall back to the whole display.
	{
		HudRegistry hud;
		Check("default layout is the whole display", hud.LayoutArea(5120.0f, 1440.0f).width == 5120.0f);

		hud.SetLayoutAspect(16.0f / 9.0f);
		const HudArea confined = hud.LayoutArea(5120.0f, 1440.0f);
		Check("SetLayoutAspect confines the layout", confined.left == 1280.0f && confined.width == 2560.0f);

		hud.SetLayoutAspect(0.0f);
		Check("SetLayoutAspect(0) restores the whole display", hud.LayoutArea(5120.0f, 1440.0f).width == 5120.0f);

		hud.SetLayoutAspect(-2.0f);
		Check("negative aspect is ignored", hud.LayoutArea(5120.0f, 1440.0f).width == 5120.0f);
	}

	std::cout << (g_failures == 0 ? "ALL HUDREGISTRY TESTS PASSED\n" : "HUDREGISTRY TESTS FAILED\n");
	return g_failures == 0 ? 0 : 1;
}
