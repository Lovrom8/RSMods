#pragma once

#include <memory>
#include <string>
#include <vector>

namespace Framework {
	class IMod;

	enum class HudAnchor {
		TopLeft,
		TopCenter,
		TopRight,
		TopTuning,
		HighwayLeft,
		MenuBanner,
	};

	// Copyable snapshot published from the MainThread; the render thread reads copies only,
	// so mod code is never re-entered and there is no lifetime coupling to the mod object.
	struct HudText {
		bool visible = false;
		std::string text;
		int colorHex = static_cast<int>(0xFFFFFFFF); // ARGB
		int fontHeight = 0; // 0 = shared default font; >0 = custom size (feeds DX9DrawText::setFontSize)
	};

	// Lower order sits nearer the anchor edge within the same anchor.
	struct HudPlacement {
		HudAnchor anchor = HudAnchor::TopLeft;
		int order = 0;
	};

	enum class HudAlign { Left, Center, Right };

	// Pixel rectangle the HUD lays out in. Normally the whole display; narrower when the game
	// confines its own interface (e.g. ultrawide keeps it in a centred 16:9 band).
	struct HudArea {
		float left = 0.0f;
		float top = 0.0f;
		float width = 0.0f;
		float height = 0.0f;
	};

	// Where an anchor's stack starts: text is aligned between left and right, first line at top.
	struct HudBand {
		float left = 0.0f;
		float right = 0.0f;
		float top = 0.0f;
		HudAlign align = HudAlign::Left;
	};

	// Pure layout math, shared by the overlay and the tests.
	HudBand AnchorBand(HudAnchor anchor, const HudArea& area);
	// The centred band of the given aspect (width / height), or the whole display when the display
	// is not wider than that or aspect <= 0.
	HudArea CenteredArea(float displayWidth, float displayHeight, float aspect);

	struct HudElement {
		const IMod* owner = nullptr; // Opaque key; never dereferenced on the render thread.
		std::string id;
		HudAnchor anchor = HudAnchor::TopLeft;
		int order = 0;
		HudText snapshot;
	};

	// Decoupling-only: mods push snapshots here instead of writing GameOverlay globals directly.
	class HudRegistry {
	public:
		HudRegistry();
		~HudRegistry();

		HudRegistry(const HudRegistry&) = delete;
		HudRegistry& operator=(const HudRegistry&) = delete;

		// MainThread: upsert this owner's element by id.
		void Set(const IMod* owner, std::string id, HudPlacement placement, HudText snapshot);
		// MainThread: drop every element owned by this mod.
		void RemoveMod(const IMod* owner);

		// Render thread: copy of visible elements (copied under lock to keep D3D calls off the critical section).
		[[nodiscard]] std::vector<HudElement> SnapshotVisible() const;

		// Any thread: confine the HUD to the centred band of this aspect. 0 (the default) means the whole display.
		void SetLayoutAspect(float aspect);
		// Render thread: the area to lay out in for this display size.
		[[nodiscard]] HudArea LayoutArea(float displayWidth, float displayHeight) const;

	private:
		struct Impl;
		std::unique_ptr<Impl> impl;
	};

	HudRegistry& Hud();
}
