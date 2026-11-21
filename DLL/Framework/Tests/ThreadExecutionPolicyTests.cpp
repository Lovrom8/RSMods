#include "../ThreadExecutionPolicy.hpp"
#include <array>
#include <cassert>
#include <cstdio>

using Framework::ThreadExecution::CheckResult;

struct FakeOperations {
	std::array<CheckResult, 4> checks{ CheckResult::Clear, CheckResult::Clear,
		CheckResult::Clear, CheckResult::Clear };
	size_t threadCount = 1;
	size_t checkCount = 0;
	unsigned int snapshots = 0;
	unsigned int preparations = 0;
	unsigned int opens = 0;
	unsigned int suspends = 0;
	unsigned int resumes = 0;
	unsigned int delays = 0;
	unsigned int passLimit = 0;
	bool snapshotOk = true;
	bool prepareOk = true;
	bool openOk = true;
	bool suspendOk = true;

	bool Snapshot(size_t) { ++snapshots; return snapshotOk; }
	size_t ThreadCount() const { return threadCount; }
	bool Prepare() { ++preparations; return prepareOk; }
	bool Open(size_t) { ++opens; return openOk; }
	bool Suspend(size_t) { ++suspends; return suspendOk; }
	CheckResult Recheck(size_t) { return checks[checkCount++]; }
	void Resume() { ++resumes; }
	void RetryDelay(size_t) { ++delays; }
	void PassLimit(size_t pass) { passLimit = static_cast<unsigned int>(pass); }
};

int main() {
	{
		FakeOperations operations;
		operations.threadCount = 0;
		bool committed = false;
		assert(Framework::ThreadExecution::Run(operations, 4, [&] {
			committed = true;
			return true;
		}));
		assert(committed && operations.snapshots == 1 && operations.preparations == 0);
		assert(operations.opens == 0 && operations.suspends == 0 && operations.resumes == 0);
	}
	{
		FakeOperations operations;
		operations.checks[0] = CheckResult::Retry;
		bool committed = false;
		assert(Framework::ThreadExecution::Run(operations, 4, [&] {
			committed = true;
			return true;
		}));
		assert(committed && operations.snapshots == 2 && operations.opens == 2);
		assert(operations.suspends == 2 && operations.resumes == 2 && operations.delays == 1);
	}
	{
		FakeOperations operations;
		operations.checks.fill(CheckResult::Retry);
		bool committed = false;
		assert(!Framework::ThreadExecution::Run(operations, 3, [&] {
			committed = true;
			return true;
		}));
		assert(!committed && operations.suspends == 3 && operations.resumes == 3);
		assert(operations.delays == 2 && operations.passLimit == 3);
	}
	{
		FakeOperations operations;
		operations.openOk = false;
		bool committed = false;
		assert(!Framework::ThreadExecution::Run(operations, 4, [&] {
			committed = true;
			return true;
		}));
		assert(!committed && operations.resumes == 1 && operations.suspends == 0);
	}
	{
		FakeOperations operations;
		operations.suspendOk = false;
		bool committed = false;
		assert(!Framework::ThreadExecution::Run(operations, 4, [&] {
			committed = true;
			return true;
		}));
		assert(!committed && operations.resumes == 1 && operations.suspends == 1);
	}
	{
		FakeOperations operations;
		operations.checks[0] = CheckResult::Failed;
		bool committed = false;
		assert(!Framework::ThreadExecution::Run(operations, 4, [&] {
			committed = true;
			return true;
		}));
		assert(!committed && operations.resumes == 1);
	}
	{
		// The re-check cannot run, so nothing is suspended and nothing is written.
		FakeOperations operations;
		operations.prepareOk = false;
		bool committed = false;
		assert(!Framework::ThreadExecution::Run(operations, 4, [&] {
			committed = true;
			return true;
		}));
		assert(!committed && operations.opens == 0 && operations.suspends == 0 && operations.resumes == 1);
	}
	{
		FakeOperations operations;
		operations.snapshotOk = false;
		bool committed = false;
		assert(!Framework::ThreadExecution::Run(operations, 4, [&] {
			committed = true;
			return true;
		}));
		assert(!committed && operations.preparations == 0 && operations.suspends == 0);
	}
	{
		// A thread born after the snapshot, then one inside a span, then a clear pass that commits
		// while suspended.
		FakeOperations operations;
		operations.checks[0] = CheckResult::Retry;
		operations.checks[1] = CheckResult::Retry;
		bool committedSuspended = false;
		assert(Framework::ThreadExecution::Run(operations, Framework::ThreadExecution::kMaxPasses, [&] {
			committedSuspended = operations.suspends == operations.resumes + 1;
			return true;
		}));
		assert(committedSuspended && operations.snapshots == 3 && operations.resumes == 3);
		assert(operations.delays == 2);
	}
	puts("Thread execution policy ALL PASS");
}
