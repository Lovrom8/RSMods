#include "StallMonitor.hpp"

namespace Framework {
	namespace {
		// No honest hook or command comes near this; the hook watchdog already warns at 50 ms.
		constexpr auto kStallThreshold = std::chrono::seconds(5);

		long long ToMs(StallMonitor::clock::duration d) {
			return std::chrono::duration_cast<std::chrono::milliseconds>(d).count();
		}
	}

	void StallMonitor::Enter(std::string_view newOwner, std::string_view newWhere, clock::time_point now) {
		std::lock_guard<std::mutex> lock(mutex);
		inCall = true;
		++callId;
		owner = newOwner;
		where = newWhere;
		enteredAt = now;
	}

	void StallMonitor::Leave(clock::time_point now) {
		std::lock_guard<std::mutex> lock(mutex);
		if (inCall && stuckReported && stuckCallId == callId) {
			recoveredAfter = now - enteredAt;
		}

		inCall = false;
		owner = {};
		where = {};
	}

	std::optional<StallMonitor::Report> StallMonitor::Poll(clock::time_point now) {
		std::lock_guard<std::mutex> lock(mutex);

		if (recoveredAfter) {
			Report report{ ReportKind::Recovered, std::move(stuckOwner), std::move(stuckWhere), ToMs(*recoveredAfter) };
			stuckReported = false;
			recoveredAfter.reset();
			return report;
		}

		if (!inCall || now - enteredAt < threshold) return std::nullopt;
		if (stuckReported && stuckCallId == callId) return std::nullopt;

		stuckReported = true;
		stuckCallId = callId;
		stuckOwner = std::string(owner);
		stuckWhere = std::string(where);
		return Report{ ReportKind::Stuck, stuckOwner, stuckWhere, ToMs(now - enteredAt) };
	}

	StallMonitor& Stalls() {
		static StallMonitor instance{ kStallThreshold };

		return instance;
	}
}
