#include "stdafx.h"
#include "FrameProfiler.hpp"
#include <Psapi.h>
#include <deque>

namespace FrameProfiler {
	namespace {
		constexpr size_t stackWords = 96;			// Stack scanned per sample for return addresses
		constexpr size_t maxReturns = 8;			// Return addresses kept per sample
		constexpr size_t maxFrameSamples = 8192;	// Samples kept for one frame (8s at 1ms)
		constexpr size_t frameRing = 1 << 16;
		constexpr size_t dipsKept = 300;
		constexpr double dipMinimumMs = 12.0;		// Never call a frame faster than this a dip
		constexpr double dipFactor = 2.0;			// A dip takes this many times the recent average frame
		constexpr ULONGLONG reportIntervalMs = 10000;

		struct RawSample {
			uint32_t frame;
			uintptr_t eip;
			uintptr_t esp;
			uint32_t stack[stackWords];
		};

		struct Dip {
			double secondsIn;
			double ms;
			size_t samples;
			std::vector<std::pair<size_t, uintptr_t>> top;
		};

		std::atomic<bool> started = false;
		std::atomic<uint32_t> frameNumber = 0;
		std::unique_ptr<std::atomic<int64_t>[]> frameStart;	// QPC at each EndScene
		HANDLE target = nullptr;
		LARGE_INTEGER frequency{};
		int64_t startTime = 0;

		std::vector<RawSample> pending; // Samples of the frame in progress; allocated once
		size_t pendingCount = 0;
		uint32_t pendingFrame = 0;

		uintptr_t gameBase = 0, gameEnd = 0;
		std::map<uintptr_t, size_t> allEips, dipEips, dipInclusive, dipCallers;
		std::deque<Dip> dips;
		size_t totalSamples = 0, dipSamples = 0, framesSeen = 0, dipCount = 0;
		double averageMs = 0;
		double totalFrameMs = 0;

		std::string PathNextToGame(const std::string& fileName) {
			char executable[MAX_PATH]{};
			GetModuleFileNameA(NULL, executable, MAX_PATH);
			std::string path(executable);
			path.resize(path.find_last_of("\\/") + 1);
			return path + fileName;
		}

		std::string Describe(uintptr_t address) {
			HMODULE module = nullptr;
			char name[MAX_PATH] = "?";
			std::ostringstream out;
			if (GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, reinterpret_cast<LPCSTR>(address), &module)) {
				GetModuleFileNameA(module, name, MAX_PATH);
				const char* file = strrchr(name, '\\');
				out << (file ? file + 1 : name) << "+0x" << std::hex << (address - reinterpret_cast<uintptr_t>(module));
				if (reinterpret_cast<uintptr_t>(module) == gameBase)
					out << " (0x" << std::hex << address << ")";
				return out.str();
			}
			out << "0x" << std::hex << address;
			return out.str();
		}

		/// Is address right after a CALL in the game's code? Weeds stale values out of the stack scan.
		bool IsGameReturnAddress(uintptr_t address) {
			if (address < gameBase + 8 || address >= gameEnd)
				return false;
			uint8_t code[7]{};
			if (!ReadProcessMemory(GetCurrentProcess(), reinterpret_cast<LPCVOID>(address - 7), code, sizeof(code), nullptr))
				return false;
			if (code[2] == 0xE8) // call rel32
				return true;
			const auto indirect = [&](int at) { return code[at] == 0xFF && (code[at + 1] & 0x38) == 0x10; };
			return indirect(5) || indirect(4) || indirect(1) || indirect(0); // call r/m32 with 0, 1, 4 or 5 bytes after ModRM
		}

		double FrameMs(uint32_t frame) {
			const int64_t begin = frameStart[frame % frameRing].load();
			const int64_t end = frameStart[(frame + 1) % frameRing].load();
			return end > begin ? (end - begin) * 1000.0 / frequency.QuadPart : 0;
		}

		void WriteTop(std::ofstream& file, const std::map<uintptr_t, size_t>& counts, size_t total, size_t limit) {
			std::vector<std::pair<size_t, uintptr_t>> sorted;
			for (const auto& [address, count] : counts)
				sorted.push_back({ count, address });
			std::sort(sorted.rbegin(), sorted.rend());
			for (size_t i = 0; i < sorted.size() && i < limit; i++)
				file << std::setw(8) << sorted[i].first << "  " << std::fixed << std::setprecision(1) << std::setw(5) << (total ? 100.0 * sorted[i].first / total : 0) << "%  " << Describe(sorted[i].second) << "\n";
		}

		/// Called by the sampler thread once the frame the pending samples belong to has ended.
		void FinishFrame() {
			const double ms = FrameMs(pendingFrame);
			const bool dip = framesSeen > 60 && ms > dipMinimumMs && ms > dipFactor * averageMs;

			std::map<uintptr_t, size_t> frameEips;
			for (size_t i = 0; i < pendingCount; i++) {
				const RawSample& sample = pending[i];
				allEips[sample.eip]++;
				totalSamples++;
				if (!dip)
					continue;

				dipEips[sample.eip]++;
				frameEips[sample.eip]++;
				dipSamples++;
				std::vector<uintptr_t> returns;
				for (size_t word = 0; word < stackWords && returns.size() < maxReturns; word++) {
					const uintptr_t value = sample.stack[word];
					if (IsGameReturnAddress(value) && std::find(returns.begin(), returns.end(), value) == returns.end())
						returns.push_back(value);
				}
				if (!returns.empty())
					dipCallers[returns[0]]++;
				for (uintptr_t address : returns)
					dipInclusive[address]++;
			}

			if (dip) {
				Dip entry{ (frameStart[pendingFrame % frameRing].load() - startTime) / static_cast<double>(frequency.QuadPart), ms, pendingCount, {} };
				for (const auto& [address, count] : frameEips)
					entry.top.push_back({ count, address });
				std::sort(entry.top.rbegin(), entry.top.rend());
				if (entry.top.size() > 4)
					entry.top.resize(4);
				dips.push_back(std::move(entry));
				if (dips.size() > dipsKept)
					dips.pop_front();
				dipCount++;
			}
			else if (ms > 0) {
				averageMs = framesSeen == 0 ? ms : averageMs * 0.97 + ms * 0.03;
			}
			if (ms > 0) {
				framesSeen++;
				totalFrameMs += ms;
			}
			pendingCount = 0;
		}

		void WriteReport() {
			std::ofstream file(PathNextToGame("RSMods_profile_frames.txt"));
			const double seconds = (frameStart[frameNumber.load() % frameRing].load() - startTime) / static_cast<double>(frequency.QuadPart);
			file << "Frame profile: " << framesSeen << " frames over " << std::fixed << std::setprecision(1) << seconds << "s, average "
				<< (framesSeen ? totalFrameMs / framesSeen : 0) << "ms (" << (totalFrameMs > 0 ? 1000.0 * framesSeen / totalFrameMs : 0) << " fps)\n";
			file << "Dips (frame > " << dipMinimumMs << "ms and > " << dipFactor << "x the recent average, now " << averageMs << "ms): "
				<< dipCount << " frames, " << dipSamples << " samples\n";
			file << "Samples are ~1ms each, so a sample count is about the milliseconds spent there.\n\n";

			file << "=== Dips: top instructions ===\n";
			WriteTop(file, dipEips, dipSamples, 150);
			file << "\n=== Dips: nearest game caller (first return address on the stack) ===\n";
			WriteTop(file, dipCallers, dipSamples, 100);
			file << "\n=== Dips: on the stack anywhere (inclusive, return addresses) ===\n";
			WriteTop(file, dipInclusive, dipSamples, 150);

			file << "\n=== Recent dips (seconds since start, frame ms, samples, top instructions) ===\n";
			for (auto it = dips.rbegin(); it != dips.rend(); ++it) {
				file << std::fixed << std::setprecision(2) << std::setw(9) << it->secondsIn << "s " << std::setw(8) << it->ms << "ms " << std::setw(5) << it->samples << "  ";
				for (const auto& [count, address] : it->top)
					file << count << "x " << Describe(address) << "   ";
				file << "\n";
			}

			file << "\n=== All frames: top instructions ===\n";
			WriteTop(file, allEips, totalSamples, 150);
		}

		/// <summary>
		/// The sample itself is taken while the render thread is suspended, so that part must not allocate or take any lock.
		/// Everything else happens after it resumes.
		/// </summary>
		DWORD WINAPI SampleLoop(LPVOID) {
			HANDLE timer = CreateWaitableTimerExW(nullptr, nullptr, CREATE_WAITABLE_TIMER_HIGH_RESOLUTION, TIMER_ALL_ACCESS);
			ULONGLONG lastReport = GetTickCount64();
			pendingFrame = frameNumber.load();

			while (!GameState::GameClosing) {
				const uint32_t frame = frameNumber.load();
				if (frame != pendingFrame) {
					FinishFrame();
					pendingFrame = frame;
				}

				if (SuspendThread(target) != (DWORD)-1) {
					CONTEXT context{};
					context.ContextFlags = CONTEXT_CONTROL | CONTEXT_INTEGER;
					if (GetThreadContext(target, &context) && pendingCount < maxFrameSamples) {
						RawSample& sample = pending[pendingCount++];
						sample.frame = frame;
						sample.eip = context.Eip;
						sample.esp = context.Esp;
						SIZE_T read = 0;
						if (!ReadProcessMemory(GetCurrentProcess(), reinterpret_cast<LPCVOID>(context.Esp), sample.stack, sizeof(sample.stack), &read))
							memset(sample.stack, 0, sizeof(sample.stack));
					}
					ResumeThread(target);
				}

				if (GetTickCount64() - lastReport >= reportIntervalMs) {
					WriteReport();
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

		bool Enabled() {
			static const bool enabled = std::ifstream(PathNextToGame("RSMods_frame_profiling.txt")).good();
			return enabled;
		}

		void Start() {
			MODULEINFO info{};
			GetModuleInformation(GetCurrentProcess(), GetModuleHandleA(NULL), &info, sizeof(info));
			gameBase = reinterpret_cast<uintptr_t>(info.lpBaseOfDll);
			gameEnd = gameBase + info.SizeOfImage;

			QueryPerformanceFrequency(&frequency);
			frameStart = std::make_unique<std::atomic<int64_t>[]>(frameRing);
			LARGE_INTEGER now{};
			QueryPerformanceCounter(&now);
			startTime = now.QuadPart;
			frameStart[0].store(startTime);
			pending.resize(maxFrameSamples);

			target = OpenThread(THREAD_SUSPEND_RESUME | THREAD_GET_CONTEXT | THREAD_QUERY_INFORMATION, FALSE, GetCurrentThreadId());
			if (!target)
				return;
			HANDLE sampler = CreateThread(nullptr, 0, SampleLoop, nullptr, 0, nullptr);
			if (sampler) {
				CloseHandle(sampler);
				LOG_INFO("(PROFILER) Frame profiler running, writing RSMods_profile_frames.txt every 10 seconds" << std::endl);
			}
		}
	}

	void OnFrame() {
		if (!Enabled())
			return;
		if (!started.exchange(true)) {
			Start();
			return;
		}
		LARGE_INTEGER now{};
		QueryPerformanceCounter(&now);
		const uint32_t next = frameNumber.load() + 1;
		frameStart[next % frameRing].store(now.QuadPart);
		frameNumber.store(next);
	}
}
