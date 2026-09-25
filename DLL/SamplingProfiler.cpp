#include "stdafx.h"
#include "SamplingProfiler.hpp"

namespace SamplingProfiler {
	namespace {
		// Return addresses from the EBP chain, innermost first. Enough to get from a system call back up into game code.
		constexpr size_t maxFrames = 12;

		struct Sample {
			uintptr_t eip;
			uintptr_t frames[maxFrames];	// frames[0] is the return address at [EBP+4]. Unused entries are 0.
		};

		constexpr size_t maxSamples = 1 << 18; // ~4.5 minutes at 1ms (samples are 52 bytes)

		// A plain array, not a std::vector: Debug builds' iterator checks take a process-wide STL lock on push_back, and if the
		// target was suspended while holding it, the sampler would wait on it forever with the target never resumed.
		Sample* samples = nullptr;
		size_t sampleCount = 0;
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
		/// Follows the saved-EBP chain. ReadProcessMemory takes no user-mode lock and fails cleanly on a bad pointer, so this is safe
		/// while the target is suspended. Stops at the first frame that doesn't move up the stack (frame-pointer-omitted code).
		/// </summary>
		void WalkFrames(uintptr_t ebp, uintptr_t (&frames)[maxFrames]) {
			size_t depth = 0;
			while (depth < maxFrames && ebp) {
				uintptr_t frame[2]{}; // Saved EBP, return address.
				SIZE_T read = 0;
				if (!ReadProcessMemory(GetCurrentProcess(), reinterpret_cast<LPCVOID>(ebp), frame, sizeof(frame), &read) || read != sizeof(frame))
					break;
				frames[depth++] = frame[1];
				if (frame[0] <= ebp || frame[0] - ebp > 0x100000)
					break;
				ebp = frame[0];
			}
			for (; depth < maxFrames; depth++)
				frames[depth] = 0;
		}

		/// <summary>
		/// The sample itself is taken while the target is suspended, so that part must not allocate or take any lock the target could hold.
		/// Reports are written between samples, while the target runs, so a load that never finishes can still be profiled.
		/// </summary>
		DWORD WINAPI SampleLoop(LPVOID) {
			HANDLE timer = CreateWaitableTimerExW(nullptr, nullptr, CREATE_WAITABLE_TIMER_HIGH_RESOLUTION, TIMER_ALL_ACCESS);
			ULONGLONG lastReport = GetTickCount64();

			while (running && sampleCount < maxSamples) {
				if (SuspendThread(target) != (DWORD)-1) {
					CONTEXT context{};
					context.ContextFlags = CONTEXT_CONTROL | CONTEXT_INTEGER;
					if (GetThreadContext(target, &context)) {
						Sample& sample = samples[sampleCount++];
						sample.eip = context.Eip;
						WalkFrames(context.Ebp, sample.frames);
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
			const size_t count = sampleCount;
			if (count == 0)
				return;

			// Code we can act on: the game and RSMods. Everything else (ntdll, kernel32, drivers, the CRT) is charged to the nearest
			// game or RSMods frame below it, so time spent blocked in a system call shows up under whoever made the call.
			MODULEINFO game{}, mods{};
			GetModuleInformation(GetCurrentProcess(), GetModuleHandleA(nullptr), &game, sizeof(game));
			HMODULE ourModule = nullptr;
			GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, reinterpret_cast<LPCSTR>(&WriteReport), &ourModule);
			GetModuleInformation(GetCurrentProcess(), ourModule, &mods, sizeof(mods));
			const auto inRange = [](const MODULEINFO& module, uintptr_t address) {
				const auto base = reinterpret_cast<uintptr_t>(module.lpBaseOfDll);
				return address >= base && address < base + module.SizeOfImage;
			};
			const auto actionable = [&](uintptr_t address) { return inRange(game, address) || inRange(mods, address); };

			std::map<uintptr_t, size_t> eips, callers, attributed;
			std::map<std::vector<uintptr_t>, size_t> chains;
			size_t outside = 0;
			for (size_t i = 0; i < count; i++) {
				const Sample& sample = samples[i];
				eips[sample.eip]++;
				callers[sample.frames[0]]++;

				std::vector<uintptr_t> chain;
				if (actionable(sample.eip))
					chain.push_back(sample.eip);
				else
					outside++;
				for (uintptr_t frame : sample.frames)
					if (frame && actionable(frame) && chain.size() < 5)
						chain.push_back(frame);
				attributed[chain.empty() ? 0 : chain.front()]++;
				chains[chain]++;
			}

			std::ofstream file(PathNextToGame("RSMods_profile_" + label + ".txt"));
			file << label << ": " << count << " samples over " << (GetTickCount64() - startTime) / 1000.0 << "s" << (inProgress ? " (still running)" : "") << "\n\n";
			file << "Top instructions:\n";
			WriteTop(file, eips, count, 2000);
			file << "\nTop return addresses at [EBP+4] (callers, when the function keeps a frame):\n";
			WriteTop(file, callers, count, 500);

			file << "\nCharged to the nearest game / RSMods frame (" << outside << " samples, " << std::fixed << std::setprecision(1)
				<< (100.0 * outside / count) << "%, were outside game and RSMods code; 0x0 = no game frame found):\n";
			WriteTop(file, attributed, count, 300);

			file << "\nTop game / RSMods call chains (innermost first, up to 5 frames):\n";
			std::vector<std::pair<size_t, const std::vector<uintptr_t>*>> sortedChains;
			for (const auto& [chain, chainCount] : chains)
				sortedChains.push_back({ chainCount, &chain });
			std::sort(sortedChains.begin(), sortedChains.end(), [](const auto& a, const auto& b) { return a.first > b.first; });
			for (size_t i = 0; i < sortedChains.size() && i < 60; i++) {
				file << std::setw(8) << sortedChains[i].first << "  " << std::fixed << std::setprecision(1) << std::setw(5) << (100.0 * sortedChains[i].first / count) << "% ";
				if (sortedChains[i].second->empty())
					file << " (no game frame)";
				for (uintptr_t frame : *sortedChains[i].second)
					file << " < " << Describe(frame);
				file << "\n";
			}
		}
	}

	bool IsEnabled() {
		static const bool enabled = std::ifstream(PathNextToGame("RSMods_profiling.txt")).good();
		return enabled;
	}

	void Start(DWORD threadId, const char* sessionLabel) {
		if (!IsEnabled() || running)
			return;

		target = OpenThread(THREAD_SUSPEND_RESUME | THREAD_GET_CONTEXT | THREAD_QUERY_INFORMATION, FALSE, threadId);
		if (!target)
			return;

		label = sessionLabel;
		delete[] samples;
		samples = new (std::nothrow) Sample[maxSamples]; // No allocation while the target is suspended.
		sampleCount = 0;
		if (!samples) {
			CloseHandle(target);
			target = nullptr;
			return;
		}
		startTime = GetTickCount64();
		running = true;
		sampler = CreateThread(nullptr, 0, SampleLoop, nullptr, 0, nullptr);
		if (!sampler) {
			running = false;
			CloseHandle(target);
			target = nullptr;
		}
	}

	void Stop(const char* sessionLabel) {
		if (!running || label != sessionLabel)
			return;

		running = false;
		WaitForSingleObject(sampler, INFINITE);
		CloseHandle(sampler);
		CloseHandle(target);
		sampler = target = nullptr;

		WriteReport(false);

		LOG_INFO("(PROFILER) Wrote RSMods_profile_" << label << ".txt (" << sampleCount << " samples)" << std::endl);
		delete[] samples;
		samples = nullptr;
		sampleCount = 0;
	}
}
