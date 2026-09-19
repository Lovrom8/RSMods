#pragma once

#include <chrono>
#include <cstdint>
#include <map>
#include <optional>
#include <string_view>
#include <utility>

namespace Framework {
	class HookWatchdog {
	public:
		using clock = std::chrono::steady_clock;

		struct Breach {
			long long elapsedMs = 0;
			long long budgetMs = 0;
			unsigned suppressed = 0;   // Over-budget runs at this site silenced since the last report.
		};

		HookWatchdog(clock::duration budget, clock::duration cooldown)
			: budget(budget), cooldown(cooldown) {}

		[[nodiscard]] std::optional<Breach> Observe(const void* owner, std::string_view where,
			clock::duration elapsed, clock::time_point now);

		// Drop a mod's sites so a recycled address can't inherit stale suppression state.
		void Forget(const void* owner);

	private:
		struct SiteState {
			bool reported = false;   // Not lastReport==epoch: the epoch is a valid timestamp.
			clock::time_point lastReport{};
			unsigned suppressed = 0;
		};

		clock::duration budget;
		clock::duration cooldown;
		std::map<std::pair<const void*, std::string_view>, SiteState> sites;
	};
}
