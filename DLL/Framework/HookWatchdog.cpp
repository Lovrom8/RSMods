#include "HookWatchdog.hpp"

namespace Framework {
	std::optional<HookWatchdog::Breach> HookWatchdog::Observe(const void* owner, std::string_view where,
		clock::duration elapsed, clock::time_point now) {
		if (elapsed < budget) {
			return std::nullopt;
		}

		SiteState& site = sites[{ owner, where }];

		if (site.reported && now - site.lastReport < cooldown) {
			++site.suppressed;
			return std::nullopt;
		}

		const Breach breach{
			.elapsedMs = std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count(),
			.budgetMs = std::chrono::duration_cast<std::chrono::milliseconds>(budget).count(),
			.suppressed = site.suppressed,
		};

		site.reported = true;
		site.lastReport = now;
		site.suppressed = 0;
		return breach;
	}

	void HookWatchdog::Forget(const void* owner) {
		// Sites order by owner first, so one owner's entries are contiguous from here.
		auto it = sites.lower_bound({ owner, std::string_view{} });
		while (it != sites.end() && it->first.first == owner) {
			it = sites.erase(it);
		}
	}
}
