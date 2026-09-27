#include "../StallMonitor.hpp"

#include <chrono>
#include <iostream>
#include <string>

using Framework::StallMonitor;
using namespace std::chrono_literals;

namespace {
	int g_failures = 0;
	using clock = StallMonitor::clock;
	using Kind = StallMonitor::ReportKind;

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

	void IdleNeverReports() {
		StallMonitor m(5s);
		Check("idle -> nothing", !m.Poll(At(60s)).has_value());

		m.Enter("AMod", "OnTick", At(0ms));
		m.Leave(At(10ms));
		Check("short call, polled late -> nothing", !m.Poll(At(60s)).has_value());
	}

	void UnderThresholdNotReported() {
		StallMonitor m(5s);
		m.Enter("AMod", "OnTick", At(0ms));
		Check("running under threshold -> nothing", !m.Poll(At(4999ms)).has_value());
	}

	void StuckReportedOnceThenRecovered() {
		StallMonitor m(5s);
		m.Enter("AMod", "OnTick", At(0ms));

		// The threshold is inclusive, like the watchdog budget.
		const auto stuck = m.Poll(At(5s));
		Check("at threshold -> stuck", stuck && stuck->kind == Kind::Stuck);
		Check("stuck names owner", stuck && stuck->owner == "AMod");
		Check("stuck names hook", stuck && stuck->where == "OnTick");
		Check("stuck reports elapsed", stuck && stuck->elapsedMs == 5000);

		Check("same call not reported twice", !m.Poll(At(6s)).has_value());
		Check("still silent much later", !m.Poll(At(60s)).has_value());

		m.Leave(At(12s));
		const auto recovered = m.Poll(At(13s));
		Check("return after stuck -> recovered", recovered && recovered->kind == Kind::Recovered);
		Check("recovered names owner", recovered && recovered->owner == "AMod");
		Check("recovered names hook", recovered && recovered->where == "OnTick");
		Check("recovered reports total run time", recovered && recovered->elapsedMs == 12000);

		Check("recovery reported once", !m.Poll(At(14s)).has_value());
	}

	void NextCallToSameHookIsNewCall() {
		StallMonitor m(5s);
		m.Enter("AMod", "OnTick", At(0ms));
		(void)m.Poll(At(5s));
		m.Leave(At(6s));
		(void)m.Poll(At(7s)); // Recovered

		// The same site stalling again is a separate incident and gets its own report.
		m.Enter("AMod", "OnTick", At(8s));
		const auto again = m.Poll(At(13s));
		Check("same site stuck again -> reported", again && again->kind == Kind::Stuck && again->elapsedMs == 5000);
	}

	void RecoveryComesBeforeNextStall() {
		StallMonitor m(5s);
		m.Enter("AMod", "OnTick", At(0ms));
		(void)m.Poll(At(5s));
		m.Leave(At(6s));

		// A second call stalls before the monitor wakes: recovery first, then the new stall.
		m.Enter("BMod", "Toggle", At(6s));
		const auto first = m.Poll(At(20s));
		Check("recovery reported first", first && first->kind == Kind::Recovered && first->owner == "AMod");
		const auto second = m.Poll(At(21s));
		Check("then the new stall", second && second->kind == Kind::Stuck && second->owner == "BMod"
			&& second->where == "Toggle" && second->elapsedMs == 15000);
	}

	void StringsOutliveTheCall() {
		StallMonitor m(5s);
		{
			std::string owner = "TempMod";
			std::string where = "OnSongTick";
			m.Enter(owner, where, At(0ms));
			(void)m.Poll(At(5s));
			m.Leave(At(6s));
			owner.assign(owner.size(), 'x');
			where.assign(where.size(), 'x');
		}

		const auto recovered = m.Poll(At(7s));
		Check("recovery keeps names copied at report time",
			recovered && recovered->owner == "TempMod" && recovered->where == "OnSongTick");
	}

	void ScopeLeavesOnUnwind() {
		StallMonitor m(0s);
		try {
			const StallMonitor::Scope scope(m, "AMod", "OnTick");
			throw 1;
		}
		catch (int) {}

		Check("scope left on throw -> nothing running", !m.Poll(clock::now() + 1h).has_value());
	}
}

int main() {
	IdleNeverReports();
	UnderThresholdNotReported();
	StuckReportedOnceThenRecovered();
	NextCallToSameHookIsNewCall();
	RecoveryComesBeforeNextStall();
	StringsOutliveTheCall();
	ScopeLeavesOnUnwind();

	if (g_failures == 0) {
		std::cout << "\nAll StallMonitor tests passed.\n";
		return 0;
	}

	std::cout << "\n" << g_failures << " StallMonitor test(s) failed.\n";
	return 1;
}
