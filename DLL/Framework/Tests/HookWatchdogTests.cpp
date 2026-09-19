#include "../HookWatchdog.hpp"

#include <chrono>
#include <iostream>
#include <string>

using Framework::HookWatchdog;
using namespace std::chrono_literals;

namespace {
	int g_failures = 0;
	using clock = HookWatchdog::clock;

	// A fixed base time so every case is deterministic and independent of the real clock.
	constexpr clock::time_point kBase{};
	clock::time_point At(std::chrono::milliseconds offset) { return kBase + offset; }

	void Check(const std::string& name, bool ok) {
		if (ok) {
			std::cout << "[PASS] " << name << "\n";
		}
		else {
			std::cout << "[FAIL] " << name << "\n";
			++g_failures;
		}
	}

	// Distinct, stable site identities for the tests.
	const char modA = 0;
	const char modB = 0;

	void UnderBudgetNeverReports() {
		HookWatchdog wd(50ms, 5s);
		Check("under budget -> no breach", !wd.Observe(&modA, "OnTick", 49ms, At(0ms)).has_value());
		Check("well under budget -> no breach", !wd.Observe(&modA, "OnTick", 1ms, At(100ms)).has_value());
	}

	void AtOrOverBudgetReports() {
		HookWatchdog wd(50ms, 5s);

		// Budget is inclusive: exactly at budget is a breach.
		const auto atBudget = wd.Observe(&modA, "OnTick", 50ms, At(0ms));
		Check("exactly at budget -> breach", atBudget.has_value());
		Check("first breach reports elapsed", atBudget && atBudget->elapsedMs == 50);
		Check("first breach reports budget", atBudget && atBudget->budgetMs == 50);
		Check("first breach suppressed == 0", atBudget && atBudget->suppressed == 0);
	}

	void RepeatWithinCooldownIsSuppressedThenReported() {
		HookWatchdog wd(50ms, 5s);

		const auto first = wd.Observe(&modA, "OnTick", 200ms, At(0ms));
		Check("first over-budget reports", first && first->suppressed == 0);

		// Three more breaches inside the 5s cooldown: all silenced, but counted.
		Check("2nd within cooldown suppressed", !wd.Observe(&modA, "OnTick", 200ms, At(1000ms)).has_value());
		Check("3rd within cooldown suppressed", !wd.Observe(&modA, "OnTick", 200ms, At(2000ms)).has_value());
		Check("4th within cooldown suppressed", !wd.Observe(&modA, "OnTick", 200ms, At(4999ms)).has_value());

		// Cooldown boundary is inclusive: at exactly cooldown it reports again, carrying the count.
		const auto next = wd.Observe(&modA, "OnTick", 300ms, At(5000ms));
		Check("after cooldown reports again", next.has_value());
		Check("report carries suppressed count", next && next->suppressed == 3);
		Check("report shows latest elapsed", next && next->elapsedMs == 300);

		// The suppressed counter resets after a report.
		Check("next-again within cooldown suppressed", !wd.Observe(&modA, "OnTick", 300ms, At(6000ms)).has_value());
		const auto third = wd.Observe(&modA, "OnTick", 300ms, At(10000ms));
		Check("counter reset after report", third && third->suppressed == 1);
	}

	void SitesAreIndependent() {
		HookWatchdog wd(50ms, 5s);

		// Same mod, different hook names are distinct sites.
		Check("modA OnTick first breach", wd.Observe(&modA, "OnTick", 100ms, At(0ms)).has_value());
		Check("modA OnEnabled is its own site", wd.Observe(&modA, "OnEnabled", 100ms, At(10ms)).has_value());

		// Same hook name, different mod is a distinct site.
		Check("modB OnTick is its own site", wd.Observe(&modB, "OnTick", 100ms, At(20ms)).has_value());

		// modA OnTick again within cooldown is still suppressed (proves the earlier hits didn't
		// bleed across sites).
		Check("modA OnTick still on cooldown", !wd.Observe(&modA, "OnTick", 100ms, At(30ms)).has_value());
	}

	void ForgetClearsState() {
		HookWatchdog wd(50ms, 5s);

		const auto first = wd.Observe(&modA, "OnTick", 100ms, At(0ms));
		Check("forget: first breach", first && first->suppressed == 0);
		Check("forget: 2nd suppressed", !wd.Observe(&modA, "OnTick", 100ms, At(1000ms)).has_value());

		wd.Forget(&modA);

		// After Forget, the site behaves as brand new: reports immediately even inside the old
		// cooldown window, with a zeroed suppressed count.
		const auto afterForget = wd.Observe(&modA, "OnTick", 100ms, At(1500ms));
		Check("forget: reports again", afterForget.has_value());
		Check("forget: suppressed reset to 0", afterForget && afterForget->suppressed == 0);
	}

	void ForgetIsScopedToOneOwner() {
		HookWatchdog wd(50ms, 5s);

		(void)wd.Observe(&modA, "OnTick", 100ms, At(0ms)); // report, now on cooldown
		(void)wd.Observe(&modB, "OnTick", 100ms, At(0ms)); // report, now on cooldown

		wd.Forget(&modA);

		// modB's state is untouched: still on cooldown.
		Check("forget one owner leaves the other on cooldown",
			!wd.Observe(&modB, "OnTick", 100ms, At(1000ms)).has_value());
		// modA is fresh again.
		Check("forgotten owner reports again",
			wd.Observe(&modA, "OnTick", 100ms, At(1000ms)).has_value());
	}
}

int main() {
	UnderBudgetNeverReports();
	AtOrOverBudgetReports();
	RepeatWithinCooldownIsSuppressedThenReported();
	SitesAreIndependent();
	ForgetClearsState();
	ForgetIsScopedToOneOwner();

	if (g_failures == 0) {
		std::cout << "\nAll HookWatchdog tests passed.\n";
		return 0;
	}

	std::cout << "\n" << g_failures << " HookWatchdog test(s) failed.\n";
	return 1;
}
