#define NOMINMAX
#include <Windows.h>

#include "../../../Proxy.hpp"

#include <atomic>
#include <iostream>
#include <thread>
#include <vector>

namespace {
	int g_failures = 0;

	void Check(const char* name, bool passed) {
		if (passed) {
			std::cout << "[PASS] " << name << "\n";
		}
		else {
			std::cout << "[FAIL] " << name << "\n";
			++g_failures;
		}
	}

	void ChecksParallelInitialization(bool loadResult, const char* name) {
		std::once_flag once;
		bool initialized = false;
		std::atomic<unsigned> attempts = 0;
		std::atomic<bool> inconsistent = false;
		std::vector<std::thread> callers;
		for (int i = 0; i < 8; ++i) {
			callers.emplace_back([&] {
				const bool result = Proxy::detail::InitializeOnce(once, initialized, [&] {
					attempts.fetch_add(1, std::memory_order_relaxed);
					std::this_thread::yield();
					return loadResult;
				});
				if (result != loadResult)
					inconsistent.store(true, std::memory_order_relaxed);
			});
		}
		for (auto& caller : callers)
			caller.join();

		Check(name, attempts.load(std::memory_order_relaxed) == 1
			&& !inconsistent.load(std::memory_order_relaxed) && initialized == loadResult);
		Check("later call reuses result", Proxy::detail::InitializeOnce(once, initialized, [&] {
			attempts.fetch_add(1, std::memory_order_relaxed);
			return !loadResult;
		}) == loadResult && attempts.load(std::memory_order_relaxed) == 1);
	}
}

int main() {
	ChecksParallelInitialization(true, "parallel first calls initialize once");
	ChecksParallelInitialization(false, "load failure is remembered");
	if (g_failures != 0) {
		std::cout << g_failures << " proxy lazy-load test(s) failed\n";
		return 1;
	}
	std::cout << "All proxy lazy-load tests passed\n";
	return 0;
}
