#include "stdafx.h"
#include "SamplingProfiler.hpp"

namespace SamplingProfiler {
	namespace {
		struct Sample {
			uintptr_t eip;
			uintptr_t caller;
		};

		constexpr size_t maxSamples = 1 << 20; // ~17 minutes at 1ms

		std::vector<Sample> samples;
		std::atomic<bool> running = false;
		HANDLE target = nullptr;
		HANDLE sampler = nullptr;
		std::string label;
		ULONGLONG startTime = 0;

		std::string PathNextToGame(const std::string& fileName) {
			char executable[MAX_PATH]{};
			GetModuleFileNameA(NULL, executable, MAX_PATH);
			std::string path(executable);
			path.resize(path.find_last_of("\\/") + 1);
			return path + fileName;
		}

		constexpr ULONGLONG reportIntervalMs = 10000;

		void WriteReport(bool inProgress);

		/// <summary>
		/// The sample itself is taken while the target is suspended, so that part must not allocate or take any lock the target could hold.
		/// Reports are written between samples, while the target runs, so a load that never finishes can still be profiled.
		/// </summary>
		DWORD WINAPI SampleLoop(LPVOID) {
			HANDLE timer = CreateWaitableTimerExW(nullptr, nullptr, CREATE_WAITABLE_TIMER_HIGH_RESOLUTION, TIMER_ALL_ACCESS);
			ULONGLONG lastReport = GetTickCount64();

			while (running && samples.size() < maxSamples) {
				if (SuspendThread(target) != (DWORD)-1) {
					CONTEXT context{};
					context.ContextFlags = CONTEXT_CONTROL | CONTEXT_INTEGER;
					if (GetThreadContext(target, &context)) {
						uintptr_t caller = 0;
						ReadProcessMemory(GetCurrentProcess(), reinterpret_cast<LPCVOID>(context.Ebp + 4), &caller, sizeof(caller), nullptr);
						samples.push_back({ context.Eip, caller });
					}
					ResumeThread(target);
				}

				if (GetTickCount64() - lastReport >= reportIntervalMs) {
					WriteReport(true);
					lastReport = GetTickCount64();
				}

				if (timer) {
					LARGE_INTEGER due{};
					due.QuadPart = -10000; // 1ms
					SetWaitableTimer(timer, &due, 0, nullptr, nullptr, FALSE);
					WaitForSingleObject(timer, INFINITE);
				}
				else
					Sleep(1);
			}

			if (timer)
				CloseHandle(timer);
			return 0;
		}

		std::string Describe(uintptr_t address) {
			HMODULE module = nullptr;
			char name[MAX_PATH] = "?";
			if (GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, reinterpret_cast<LPCSTR>(address), &module)) {
				GetModuleFileNameA(module, name, MAX_PATH);
				const char* file = strrchr(name, '\\');
				std::ostringstream out;
				out << (file ? file + 1 : name) << "+0x" << std::hex << (address - reinterpret_cast<uintptr_t>(module));
				return out.str();
			}
			std::ostringstream out;
			out << "0x" << std::hex << address;
			return out.str();
		}

		void WriteTop(std::ofstream& file, const std::map<uintptr_t, size_t>& counts, size_t total, size_t limit) {
			std::vector<std::pair<size_t, uintptr_t>> sorted;
			for (const auto& [address, count] : counts)
				sorted.push_back({ count, address });
			std::sort(sorted.rbegin(), sorted.rend());

			for (size_t i = 0; i < sorted.size() && i < limit; i++)
				file << std::setw(8) << sorted[i].first << "  " << std::fixed << std::setprecision(1) << std::setw(5) << (100.0 * sorted[i].first / total) << "%  " << Describe(sorted[i].second) << "\n";
		}
	}

	namespace {
		/// <summary>
		/// Only called by the sampler thread (between samples) or after it has exited, so it never races the sample buffer.
		/// </summary>
		void WriteReport(bool inProgress) {
			const size_t count = samples.size();
			if (count == 0)
				return;

			std::map<uintptr_t, size_t> eips, callers;
			for (size_t i = 0; i < count; i++) {
				eips[samples[i].eip]++;
				callers[samples[i].caller]++;
			}

			std::ofstream file(PathNextToGame("RSMods_profile_" + label + ".txt"));
			file << label << ": " << count << " samples over " << (GetTickCount64() - startTime) / 1000.0 << "s" << (inProgress ? " (still running)" : "") << "\n\n";
			file << "Top instructions:\n";
			WriteTop(file, eips, count, 2000);
			file << "\nTop return addresses at [EBP+4] (callers, when the function keeps a frame):\n";
			WriteTop(file, callers, count, 500);
		}
	}

	bool Enabled() {
		static const bool enabled = std::ifstream(PathNextToGame("RSMods_profiling.txt")).good();
		return enabled;
	}

	void Start(DWORD threadId, const char* sessionLabel) {
		if (!Enabled() || running)
			return;

		target = OpenThread(THREAD_SUSPEND_RESUME | THREAD_GET_CONTEXT | THREAD_QUERY_INFORMATION, FALSE, threadId);
		if (!target)
			return;

		label = sessionLabel;
		samples.clear();
		samples.reserve(maxSamples); // No allocation while the target is suspended.
		startTime = GetTickCount64();
		running = true;
		sampler = CreateThread(nullptr, 0, SampleLoop, nullptr, 0, nullptr);
		if (!sampler) {
			running = false;
			CloseHandle(target);
			target = nullptr;
		}
	}

	void Stop() {
		if (!running)
			return;

		running = false;
		WaitForSingleObject(sampler, INFINITE);
		CloseHandle(sampler);
		CloseHandle(target);
		sampler = target = nullptr;

		WriteReport(false);

		LOG_INFO("(PROFILER) Wrote RSMods_profile_" << label << ".txt (" << samples.size() << " samples)" << std::endl);
		std::vector<Sample>().swap(samples);
	}
}
