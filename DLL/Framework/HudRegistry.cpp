#include "HudRegistry.hpp"

#include <algorithm>
#include <atomic>
#include <cmath>
#include <mutex>
#include <utility>

namespace Framework {
	struct HudRegistry::Impl {
		mutable std::mutex mutex;
		std::vector<HudElement> elements;
		std::atomic<float> layoutAspect = 0.0f;

		std::vector<HudElement>::iterator Find(const IMod* owner, const std::string& id) {
			return std::find_if(elements.begin(), elements.end(),
				[&](const HudElement& e) { return e.owner == owner && e.id == id; });
		}
	};

	HudRegistry::HudRegistry() : impl(std::make_unique<Impl>()) {}
	HudRegistry::~HudRegistry() = default;

	void HudRegistry::Set(const IMod* owner, std::string id, HudPlacement placement, HudText snapshot) {
		std::lock_guard<std::mutex> lock(impl->mutex);

		auto it = impl->Find(owner, id);
		if (it == impl->elements.end()) {
			impl->elements.push_back(HudElement{ owner, std::move(id), placement.anchor, placement.order, std::move(snapshot) });
		}
		else {
			it->anchor = placement.anchor;
			it->order = placement.order;
			it->snapshot = std::move(snapshot);
		}
	}

	void HudRegistry::RemoveMod(const IMod* owner) {
		std::lock_guard<std::mutex> lock(impl->mutex);
		std::erase_if(impl->elements, [owner](const HudElement& e) { return e.owner == owner; });
	}

	std::vector<HudElement> HudRegistry::SnapshotVisible() const {
		std::lock_guard<std::mutex> lock(impl->mutex);

		std::vector<HudElement> out;
		out.reserve(impl->elements.size());
		for (const auto& element : impl->elements) {
			if (element.snapshot.visible) out.push_back(element);
		}

		return out;
	}

	void HudRegistry::SetLayoutAspect(float aspect) {
		impl->layoutAspect.store(std::isfinite(aspect) && aspect > 0.0f ? aspect : 0.0f, std::memory_order_relaxed);
	}

	HudArea HudRegistry::LayoutArea(float displayWidth, float displayHeight) const {
		return CenteredArea(displayWidth, displayHeight, impl->layoutAspect.load(std::memory_order_relaxed));
	}

	HudArea CenteredArea(float displayWidth, float displayHeight, float aspect) {
		HudArea area{ 0.0f, 0.0f, displayWidth, displayHeight };
		if (!(aspect > 0.0f) || displayHeight <= 0.0f)
			return area;

		const float bandWidth = displayHeight * aspect;
		if (bandWidth >= displayWidth)
			return area;

		area.left = std::floor((displayWidth - bandWidth) * 0.5f);
		area.width = bandWidth;
		return area;
	}

	HudBand AnchorBand(HudAnchor anchor, const HudArea& area) {
		const float w = area.width;
		const float h = area.height;
		// Offsets truncate like the old hand-written overlays' LONG casts, so a full-display area lands
		// pixel-for-pixel where they always did.
		const auto x = [&](float offset) { return area.left + std::trunc(offset); };
		const auto y = [&](float offset) { return area.top + std::trunc(offset); };

		switch (anchor) {
		case HudAnchor::TopRight:
			return { x(w - w / 16.0f), x(w - w / 96.0f), y(h / 54.0f), HudAlign::Right };
		case HudAnchor::TopCenter:
			return { x(w / 2.0f - w / 38.4f), x(w / 2.0f + w / 38.4f), y(h / 54.0f), HudAlign::Center };
		case HudAnchor::TopTuning:
			return { x(w / 5.5f), x(w / 5.65f), y(h / 30.85f), HudAlign::Left };
		case HudAnchor::HighwayLeft:
			return { x(w / 5.5f), x(w / 5.75f), y(h / 1.75f), HudAlign::Left };
		case HudAnchor::MenuBanner:
			return { x(w / 3.87f), x(w / 4.0f), y(h / 30.85f), HudAlign::Left };
		case HudAnchor::TopLeft:
		default:
			return { x(w / 96.0f), x(w / 19.2f), y(h / 54.0f), HudAlign::Left };
		}
	}

	HudRegistry& Hud() {
		static HudRegistry instance;
		return instance;
	}
}
