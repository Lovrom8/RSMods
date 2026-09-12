#include "HudRegistry.hpp"

#include <algorithm>
#include <mutex>
#include <utility>

namespace Framework {
	struct HudRegistry::Impl {
		mutable std::mutex mutex;
		std::vector<HudElement> elements;

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

	HudRegistry& Hud() {
		static HudRegistry instance;
		return instance;
	}
}
