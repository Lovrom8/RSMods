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

	private:
		struct Impl;
		std::unique_ptr<Impl> impl;
	};

	HudRegistry& Hud();
}
