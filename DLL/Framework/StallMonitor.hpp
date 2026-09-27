#pragma once

#include <chrono>
#include <cstdint>
#include <mutex>
#include <optional>
#include <string>
#include <string_view>

namespace Framework {
	// Catches the MainThread call that never returns, which HookWatchdog can't: it times a hook only
	// after it returns. MainThread brackets each mod call with Enter/Leave; a host monitor thread
	// calls Poll about once a second and logs what it returns. No clock or thread of its own, so it
	// unit-tests deterministically.
	class StallMonitor {
	public:
		using clock = std::chrono::steady_clock;

		enum class ReportKind {
			Stuck,       // The call has run past the threshold and is still running.
			Recovered,   // A call reported Stuck has since returned.
		};

		struct Report {
			ReportKind kind = ReportKind::Stuck;
			std::string owner;
			std::string where;
			long long elapsedMs = 0;
		};

		explicit StallMonitor(clock::duration threshold) : threshold(threshold) {}

		StallMonitor(const StallMonitor&) = delete;
		StallMonitor& operator=(const StallMonitor&) = delete;

		// MainThread. The views must stay valid until the matching Leave; Poll copies them.
		void Enter(std::string_view owner, std::string_view where, clock::time_point now);
		void Leave(clock::time_point now);

		// Monitor thread. Reports each stuck call once, then once more when it returns.
		[[nodiscard]] std::optional<Report> Poll(clock::time_point now);

		// Brackets one call on MainThread, leaving on unwind too.
		class Scope {
		public:
			Scope(StallMonitor& monitor, std::string_view owner, std::string_view where)
				: monitor(monitor) { monitor.Enter(owner, where, clock::now()); }
			~Scope() { monitor.Leave(clock::now()); }

			Scope(const Scope&) = delete;
			Scope& operator=(const Scope&) = delete;

		private:
			StallMonitor& monitor;
		};

	private:
		clock::duration threshold;

		std::mutex mutex;
		bool inCall = false;
		std::uint64_t callId = 0;   // Bumped on every Enter, so a new call to the same hook is a new call.
		std::string_view owner;
		std::string_view where;
		clock::time_point enteredAt{};

		bool stuckReported = false;   // The current or last reported call was reported Stuck.
		std::uint64_t stuckCallId = 0;
		std::string stuckOwner;       // Copied at report time: the views die with the call.
		std::string stuckWhere;
		std::optional<clock::duration> recoveredAfter;
	};

	StallMonitor& Stalls();
}
