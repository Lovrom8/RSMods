#pragma once

#include <cstddef>

namespace Framework::ThreadExecution {
	enum class CheckResult { Clear, Retry, Failed };

	inline constexpr size_t kMaxPasses = 4;

	// Runs commit while every other thread of the process is suspended. Exclusion is skipped only when
	// the snapshot taken at this call lists no other thread: no other code of the process runs to start one.
	// Snapshot, Prepare and Open run while peers run and may allocate. Between the first suspend and
	// Resume only Suspend, Recheck and commit run, and none of them may allocate, log or lock. Recheck
	// returns Retry for a thread created after the snapshot or one stopped inside a patched span;
	// every pass resumes before the next snapshot, and the last failed pass fails closed.
	template <typename Operations, typename Commit>
	bool Run(Operations& operations, size_t maxPasses, Commit commit) noexcept {
		for (size_t pass = 1; pass <= maxPasses; ++pass) {
			if (!operations.Snapshot(pass))
				return false;
			if (operations.ThreadCount() == 0)
				return commit();
			if (!operations.Prepare() || !operations.Open(pass)) {
				operations.Resume();
				return false;
			}
			if (!operations.Suspend(pass)) {
				operations.Resume();
				return false;
			}

			const CheckResult check = operations.Recheck(pass);
			if (check == CheckResult::Failed) {
				operations.Resume();
				return false;
			}
			if (check == CheckResult::Retry) {
				operations.Resume();
				if (pass == maxPasses) {
					operations.PassLimit(pass);
					return false;
				}
				operations.RetryDelay(pass);
				continue;
			}

			const bool committed = commit();
			operations.Resume();
			return committed;
		}
		return false;
	}
}
