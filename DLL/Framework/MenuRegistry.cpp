#include "MenuRegistry.hpp"

#include <algorithm>
#include <mutex>
#include <utility>

namespace Framework {
	struct MenuRegistry::Impl {
		mutable std::mutex mutex;
		std::vector<MenuEntry> entries;
		std::unordered_map<const IMod*, Availability> ownerAvailability;
		bool hasAvailabilitySnapshot = false;

		std::vector<MenuEntry>::iterator Find(const IMod* owner, const std::string& id) {
			return std::find_if(entries.begin(), entries.end(),
				[&](const MenuEntry& e) { return e.owner == owner && e.id == id; });
		}
	};

	MenuRegistry::MenuRegistry() : impl(std::make_unique<Impl>()) {}
	MenuRegistry::~MenuRegistry() = default;

	void MenuRegistry::Register(const IMod* owner, std::string id, std::string title, int order,
		MenuDrawFn drawFn, Availability availability, bool standaloneWindow) {
		std::lock_guard<std::mutex> lock(impl->mutex);

		auto it = impl->Find(owner, id);
		if (it == impl->entries.end()) {
			impl->entries.push_back(MenuEntry{
				owner,
				std::move(id),
				std::move(title),
				order,
				availability,
				std::move(drawFn),
				standaloneWindow
			});
		}
		else {
			it->title = std::move(title);
			it->order = order;
			it->availability = availability;
			it->drawFn = std::move(drawFn);
			it->standaloneWindow = standaloneWindow;
		}
	}

	void MenuRegistry::RemoveMod(const IMod* owner) {
		std::lock_guard<std::mutex> lock(impl->mutex);
		std::erase_if(impl->entries, [owner](const MenuEntry& e) { return e.owner == owner; });
		impl->ownerAvailability.erase(owner);
	}

	void MenuRegistry::PublishAvailability(std::unordered_map<const IMod*, Availability> availability) {
		std::lock_guard<std::mutex> lock(impl->mutex);
		impl->ownerAvailability = std::move(availability);
		impl->hasAvailabilitySnapshot = true;
	}

	std::vector<MenuEntry> MenuRegistry::GetEntries(const OwnerAvailabilityFn& isOwnerAvailable) const {
		std::vector<MenuEntry> out;
		{
			std::lock_guard<std::mutex> lock(impl->mutex);
			out.reserve(impl->entries.size());
			for (const auto& entry : impl->entries) {
				if (!entry.owner) {
					out.push_back(entry);
					continue;
				}

				if (isOwnerAvailable) {
					if (isOwnerAvailable(entry.owner, entry.availability)) {
						out.push_back(entry);
					}
					continue;
				}

				if (impl->hasAvailabilitySnapshot) {
					auto it = impl->ownerAvailability.find(entry.owner);
					if (it != impl->ownerAvailability.end()) {
						if (entry.availability == Availability::Initialized) {
							out.push_back(entry);
						}
						else if (entry.availability == Availability::Active && it->second == Availability::Active) {
							out.push_back(entry);
						}
					}
				}
				else {
					out.push_back(entry);
				}
			}
		}

		std::stable_sort(out.begin(), out.end(), [](const MenuEntry& a, const MenuEntry& b) {
			if (a.order != b.order) return a.order < b.order;
			return a.title < b.title;
		});

		return out;
	}

	std::vector<MenuEntry> MenuRegistry::SnapshotAll() const {
		std::lock_guard<std::mutex> lock(impl->mutex);
		return impl->entries;
	}

	MenuRegistry& Menus() {
		static MenuRegistry instance;
		return instance;
	}
}
