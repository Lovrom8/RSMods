#include "../stdafx.h"
#include "EnumerationDrain.hpp"
#include "UltrafastLoadMod.hpp"
#include "EnumerationFrameBudget.hpp"
#include "LazyVfs.hpp"
#include "AssetLoadDrain.hpp"
#include "EnumerationEnvironment.hpp"
#include "EnumerationPreparation.hpp"
#include "EnumerationCache.hpp"
#include "EnumerationOwnership.hpp"
#include <algorithm>
#include <atomic>
#include <intrin.h>
#include <memory>
#include <string_view>

namespace {
	// DLC service: +0x04 request, +0x05 enabled, +0x06 installing, +0xD0 load slot.
	// +0xDC first tick; +0xE0/+0xE4 queue begin/end, consumed from the back.
	constexpr unsigned int off_request = 0x04;
	constexpr unsigned int off_enabled = 0x05;
	constexpr unsigned int off_installing = 0x06;
	constexpr unsigned int off_loadSlot = 0xD0;
	constexpr unsigned int off_firstTick = 0xDC;
	constexpr unsigned int off_queueBegin = 0xE0;
	constexpr unsigned int off_queueEnd = 0xE4;
	constexpr unsigned int queueEntrySize = 0x18;

	// Package +0x38 == 0 identifies DLC.
	constexpr unsigned int off_packageDlcFlag = 0x38;

	// Install resolves these targets before naked thunks can run.
	uintptr_t tickSlot = 0;
	uintptr_t tickFunc = 0;
	uintptr_t loadTickFunc = 0;
	uintptr_t installNextFunc = 0;
	uintptr_t removeLastFunc = 0;
	uintptr_t packagesPtr = 0;
	uintptr_t waitingPtr = 0;
	uintptr_t registerCallSite = 0;
	uintptr_t registerFunc = 0;
	uintptr_t shaderScanCallSite = 0;
	uintptr_t shaderScanFunc = 0;
	uintptr_t bannerCallSite = 0;
	uintptr_t bannerFunc = 0;
	uintptr_t stringDestroyFunc = 0;

	using TickFn = void(__fastcall*)(void* self, void* edx);
	TickFn originalTick = nullptr;
	bool installed = false;
	double ticksPerMs = 0.0;

	unsigned int maxPerTick = 16;
	unsigned int maxLoadsPerTick = 32;
	bool draining = false;
	unsigned int scanCount = 0;
	unsigned int targetFps = 60;
	double frameWithoutEnumerationMs = 0.0;
	long long previousFrameTicks = 0;
	double previousEnumerationMs = 0.0;
	double scanBudgetSumMs = 0.0;
	unsigned int scanBudgetFrames = 0;
	uintptr_t menuReadyBody = 0, menuReadyReturn = 0;
	std::atomic<bool> mainMenuReady = false;
	std::atomic<double> menuReadySeconds{ 0.0 };
	double ProcessSeconds() {
		static const ULONGLONG created = [] {
			FILETIME start{}, exit{}, kernel{}, user{};
			if (!GetProcessTimes(GetCurrentProcess(), &start, &exit, &kernel, &user)) return ULONGLONG{ 0 };
			return (ULONGLONG{ start.dwHighDateTime } << 32) | start.dwLowDateTime;
		}();
		FILETIME now{};
		GetSystemTimePreciseAsFileTime(&now);
		const ULONGLONG ticks = (ULONGLONG{ now.dwHighDateTime } << 32) | now.dwLowDateTime;
		return created && ticks >= created ? (ticks - created) / 1e7 : 0.0;
	}
	void __declspec(naked) MenuReadyReturn() {
		__asm { mov byte ptr [mainMenuReady], 1 }
		__asm { jmp dword ptr [menuReadyReturn] }
	}
	void __declspec(naked) MenuReadyHook() {
		__asm { pop dword ptr [menuReadyReturn] }
		__asm { push offset MenuReadyReturn }
		__asm { jmp dword ptr [menuReadyBody] }
	}
	double budgetMs = 2.0;
	// Menu streaming retains a small budget outside active scans.
	double menuBudgetMs = 2.0;
	long long frameStartTicks = 0;
	double nextInstallEstimateMs = 0.0;
	bool requestEarly = false;
	double bootBudgetMs = 30.0;
	constexpr double bootScanBudgetMs = 12.0;
	unsigned int bootMaxPerTick = 32;
	unsigned int earlyMaxPackages = 1500;
	bool rescanSkip = true;
	bool skipShaderScan = true;

	// Menu state is published by the mod thread.
	std::atomic<bool> mainMenuSeen = false;
	std::atomic<bool> onSignInScreen = false;

	double EffectiveBudgetMs() {
		if (draining) {
			// Boot screens allow a larger scan budget until the main menu.
			if (!mainMenuReady.load(std::memory_order_relaxed))
				return previousFrameTicks ? bootScanBudgetMs : 2.0;
			if (!previousFrameTicks)
				return 2.0;
			// The first scan retains its larger budget after menu readiness; rescans use the menu budget.
			if (scanCount == 1)
				return (std::min)(bootScanBudgetMs, 12.0);
			return EnumerationDrain::FrameBudgetMs(true, targetFps, frameWithoutEnumerationMs, 0.0);
		}
		if (mainMenuSeen.load(std::memory_order_relaxed))
			return (std::min)(menuBudgetMs, 2.0);
		return (std::min)(requestEarly ? bootBudgetMs : budgetMs, 2.0);
	}

	unsigned int EffectiveMaxPerTick() {
		if (draining && scanCount == 1 && maxPerTick)
			return (std::max)(bootMaxPerTick, maxPerTick);
		return (!mainMenuSeen.load(std::memory_order_relaxed) && requestEarly && maxPerTick) ? bootMaxPerTick : maxPerTick;
	}

	long long Now() {
		LARGE_INTEGER now{};
		QueryPerformanceCounter(&now);
		return now.QuadPart;
	}

	double MsSince(long long start) {
		return (Now() - start) / ticksPerMs;
	}


	unsigned int QueueLength(const unsigned char* service) {
		const auto begin = *reinterpret_cast<const uintptr_t*>(service + off_queueBegin);
		const auto end = *reinterpret_cast<const uintptr_t*>(service + off_queueEnd);
		return end > begin ? static_cast<unsigned int>((end - begin) / queueEntrySize) : 0;
	}

	// Engine string: data +0, finish +0x10, storage end +0x14.
	// Inline storage points its end at +0x10.
	std::string_view QueuedPath(const unsigned char* service, unsigned int index) {
		const auto begin = *reinterpret_cast<const uintptr_t*>(service + off_queueBegin);
		const uintptr_t entry = begin + index * queueEntrySize;
		const auto finish = *reinterpret_cast<const char* const*>(entry + 0x10);
		const bool inlineChars = *reinterpret_cast<const uintptr_t*>(entry + 0x14) == entry + 0x10;
		const char* data = inlineChars ? reinterpret_cast<const char*>(entry) : *reinterpret_cast<const char* const*>(entry);
		if (!data || finish < data)
			return {};
		return std::string_view(data, static_cast<size_t>(finish - data));
	}

	std::string_view NextQueuedPath(const unsigned char* service) {
		return QueuedPath(service, QueueLength(service) - 1);
	}

	unsigned int VectorCount(uintptr_t globalPtr, unsigned int elementSize) {
		const auto vector = *reinterpret_cast<const uintptr_t*>(globalPtr);
		if (!vector)
			return 0;
		const auto begin = *reinterpret_cast<const uintptr_t*>(vector);
		const auto end = *reinterpret_cast<const uintptr_t*>(vector + 4);
		return end > begin ? static_cast<unsigned int>((end - begin) / elementSize) : 0;
	}

	unsigned int WaitingLoads() {
		return VectorCount(waitingPtr, 4);
	}

	bool IsLoadSlotBusy(const unsigned char* service) {
		return *reinterpret_cast<const uintptr_t*>(service + off_loadSlot) != 0;
	}

	void CountPackages(unsigned int& total, unsigned int& dlc) {
		total = 0;
		dlc = 0;
		const auto vector = *reinterpret_cast<const uintptr_t*>(packagesPtr);
		if (!vector)
			return;
		const auto begin = *reinterpret_cast<const uintptr_t*>(vector);
		const auto end = *reinterpret_cast<const uintptr_t*>(vector + 4);
		for (uintptr_t entry = begin; entry < end; entry += 4) {
			const auto package = *reinterpret_cast<const uintptr_t*>(entry);
			++total;
			if (package && *reinterpret_cast<const unsigned char*>(package + off_packageDlcFlag) == 0)
				++dlc;
		}
	}


	// Install step: service in ESI, no stack arguments; preserves EBX/EDI/ESI.
	void __declspec(naked) __cdecl CallInstallNext(void*) {
		__asm {
			push esi
			mov esi, [esp + 8]
			call dword ptr [installNextFunc]
			pop esi
			ret
		}
	}

	// pop_back takes the vector in EAX and clobbers EAX/ECX/EDX; requires a nonempty queue.
	void __declspec(naked) __cdecl CallRemoveLast(void*) {
		__asm {
			mov eax, [esp + 4]
			call dword ptr [removeLastFunc]
			ret
		}
	}

	// thiscall load step advances queued packages and frees completed load slots.
	void CallLoadTick(void* service) {
		reinterpret_cast<TickFn>(loadTickFunc)(service, nullptr);
	}

	// Scope registration to game-thread DLC installs; shader skipping must exclude other mounts.
	// This call path does not nest.

	volatile LONG inInstallRegister = 0;
	uintptr_t registerSavedReturn = 0;
	unsigned int packagesBeforeRegister = 0;
	bool lastInstallRegistered = false;
	bool registerHooked = false;
	int32_t registerOriginalRel = 0;

	// Zero timestamps mean the install never reached registration.
	long long registerEnterTicks = 0;
	long long registerLeaveTicks = 0;

	extern "C" void __cdecl EnumRegisterEnter() {
		registerEnterTicks = Now();
		inInstallRegister = 1;
		packagesBeforeRegister = VectorCount(packagesPtr, 4);
	}

	extern "C" void __cdecl EnumRegisterLeave() {
		inInstallRegister = 0;
		lastInstallRegistered = VectorCount(packagesPtr, 4) > packagesBeforeRegister;
		registerLeaveTicks = Now();
	}

	void __declspec(naked) RegisterReturnThunk() {
		__asm {
			pushfd
			pushad
			call EnumRegisterLeave
			popad
			popfd
			jmp dword ptr [registerSavedReturn]
		}
	}

	// Preserve EAX, CL and all three stack arguments when substituting the return thunk.
	// The caller still removes 0xC bytes.
	void __declspec(naked) RegisterEnterThunk() {
		__asm {
			pushfd
			pushad
			call EnumRegisterEnter
			popad
			popfd
			pop dword ptr [registerSavedReturn]
			push offset RegisterReturnThunk
			jmp dword ptr [registerFunc]
		}
	}

	// Skip generated-shader probing for DLC installs; the caller ignores its return value.

	bool shaderScanHooked = false;
	int32_t shaderScanOriginalRel = 0;
	unsigned char skipShaderScanNow = 0;
	unsigned int shaderScansSkipped = 0;

	extern "C" void __cdecl EnumDecideShaderScan() {
		skipShaderScanNow = (skipShaderScan && inInstallRegister) ? 1 : 0;
		if (skipShaderScanNow)
			++shaderScansSkipped;
	}

	void __declspec(naked) ShaderScanThunk() {
		__asm {
			pushfd
			pushad
			call EnumDecideShaderScan
			popad
			popfd
			cmp byte ptr [skipShaderScanNow], 0
			jne skip
			jmp dword ptr [shaderScanFunc]
		skip:
			ret
		}
	}

	// Time the native VFS existence probe only during drained game-thread installs.
	// Read its target on every call because VFS initialization can replace it.

	constexpr unsigned char existsPrologue[] = { 0x55, 0x8B, 0xEC, 0x8B, 0x4D, 0x0C, 0x33, 0xC0, 0x85, 0xC9, 0x74, 0x02, 0x8B, 0x01, 0x50, 0x8B, 0x45, 0x08, 0x50, 0xFF, 0x15 };
	uintptr_t* existsGameSlot = nullptr;
	uintptr_t existsHookSlot = 0;
	bool existsHooked = false;
	DWORD gameThreadId = 0;
	bool timingInstall = false;
	long long existsTicks = 0;
	unsigned int existsCalls = 0;

	using ExistsProbeFn = int(__cdecl*)(const char* path, int mode);

	int __cdecl ExistsProbeTimed(const char* path, int mode) {
		const auto original = reinterpret_cast<ExistsProbeFn>(*existsGameSlot);
		if (!timingInstall || GetCurrentThreadId() != gameThreadId)
			return original(path, mode);
		const long long start = Now();
		const int result = original(path, mode);
		existsTicks += Now() - start;
		++existsCalls;
		return result;
	}

	bool HookExistsCheck() {
		const uintptr_t func = Offsets::func_fileExistsCheck;
		if (!func || MemUtil::IsBadReadPtr(reinterpret_cast<void*>(func)) || memcmp(reinterpret_cast<const void*>(func), existsPrologue, sizeof(existsPrologue)) != 0) {
			LOG_WARNING("(ENUMERATION) Existence check doesn't look as expected, it isn't timed" << std::endl);
			return false;
		}
		const uintptr_t operand = func + sizeof(existsPrologue);
		existsGameSlot = *reinterpret_cast<uintptr_t* const*>(operand);
		existsHookSlot = reinterpret_cast<uintptr_t>(&ExistsProbeTimed);
		const uintptr_t slot = reinterpret_cast<uintptr_t>(&existsHookSlot);
		return MemUtil::PatchAdr(reinterpret_cast<LPVOID>(operand), &slot, sizeof(slot));
	}

	// Banner ABI: cdecl, caller pops 0x30; construct the out-string at ESI and return it in EAX.
	// The skip must also destroy the by-value string at [esp+0x1C].

	bool bannerHooked = false;
	int32_t bannerOriginalRel = 0;
	unsigned int bannerSkips = 0;

	// Hold the native install while its next file is not warm.
	constexpr uintptr_t stockInstallCallOffset = 0x11;
	bool stockInstallHooked = false;
	int32_t stockInstallOriginalRel = 0;

	extern "C" void __cdecl EnumNoteBannerSkip() {
		if (++bannerSkips == 1)
			LOG_INFO("(ENUMERATION) Native enumeration banner replaced with the progress bar" << std::endl);
	}

	void __declspec(naked) BannerThunk() {
		__asm {
			pushfd
			pushad
			call EnumNoteBannerSkip
			popad
			popfd
			push esi
			lea ecx, [esp + 0x1C + 4]
			call dword ptr [stringDestroyFunc]
			pop esi
			lea eax, [esi + 0x10]
			mov dword ptr [eax], esi
			mov dword ptr [esi + 0x14], eax
			mov byte ptr [esi], 0
			mov eax, esi
			ret
		}
	}


	bool RedirectCall(uintptr_t site, uintptr_t expectedTarget, void* thunk, int32_t& originalRel, const char* what) {
		if (MemUtil::IsBadReadPtr(reinterpret_cast<void*>(site)) || *reinterpret_cast<const unsigned char*>(site) != 0xE8) {
			LOG_WARNING("(ENUMERATION) " << what << " call site is not a CALL, leaving it alone" << std::endl);
			return false;
		}
		originalRel = *reinterpret_cast<const int32_t*>(site + 1);
		if (site + 5 + originalRel != expectedTarget) {
			LOG_WARNING("(ENUMERATION) " << what << " call site targets 0x" << std::hex << site + 5 + originalRel << std::dec
				<< " instead of the expected function, leaving it alone" << std::endl);
			return false;
		}
		const int32_t rel = static_cast<int32_t>(reinterpret_cast<uintptr_t>(thunk) - (site + 5));
		return MemUtil::PatchAdr(reinterpret_cast<LPVOID>(site + 1), &rel, sizeof(rel));
	}

	// MSVCR100 permits at most 2048 streams; pause near the cap until native loads close handles.

	constexpr int crtStreamLimitMax = 2048;
	int crtStreamLimit = crtStreamLimitMax;
	unsigned int openFileCeiling = 1792;
	unsigned int peakOpenFiles = 0;
	unsigned int ceilingPauses = 0;
	size_t ioinfoStride = 0;
	void*** pioinfo = nullptr;

	bool RaiseCrtStreamLimit() {
		using SetMaxStdio = int(__cdecl*)(int);
		using GetMaxStdio = int(__cdecl*)();
		const HMODULE crt = GetModuleHandleA("msvcr100.dll");
		const auto set = crt ? reinterpret_cast<SetMaxStdio>(GetProcAddress(crt, "_setmaxstdio")) : nullptr;
		const auto get = crt ? reinterpret_cast<GetMaxStdio>(GetProcAddress(crt, "_getmaxstdio")) : nullptr;
		if (!set || !get) {
			LOG_WARNING("(ENUMERATION) msvcr100 _setmaxstdio not found, the scan stays at one package per frame" << std::endl);
			return false;
		}
		const int before = get();
		if (before >= crtStreamLimit)
			return true;
		const int result = set(crtStreamLimit);
		LOG_INFO("(ENUMERATION) msvcr100 stream limit " << before << " -> " << result << std::endl);
		return result == crtStreamLimit;
	}

	// __pioinfo has 64 blocks of 32 entries: handle +0, osfile +4, FOPEN bit 0.
	// Infer entry stride by matching fds 0..2 against _get_osfhandle.
	bool ResolveIoinfo() {
		using GetOsfHandle = intptr_t(__cdecl*)(int);
		const HMODULE crt = GetModuleHandleA("msvcr100.dll");
		const auto table = crt ? reinterpret_cast<void***>(GetProcAddress(crt, "__pioinfo")) : nullptr;
		const auto getHandle = crt ? reinterpret_cast<GetOsfHandle>(GetProcAddress(crt, "_get_osfhandle")) : nullptr;
		if (!table || !getHandle || !table[0])
			return false;
		const auto block = reinterpret_cast<const unsigned char*>(table[0]);
		const size_t candidates[]{ 0x40, 0x38, 0x48, 0x30 };
		for (size_t stride : candidates) {
			bool match = true;
			for (int fd = 0; fd < 3 && match; ++fd)
				match = *reinterpret_cast<const intptr_t*>(block + fd * stride) == getHandle(fd);
			if (match) {
				pioinfo = table;
				ioinfoStride = stride;
				return true;
			}
		}
		LOG_WARNING("(ENUMERATION) __pioinfo layout not recognised, the open file guard is off" << std::endl);
		return false;
	}

	unsigned int OpenCrtFiles() {
		if (!pioinfo)
			return 0;
		unsigned int open = 0;
		for (int blockIndex = 0; blockIndex < 64; ++blockIndex) {
			const auto block = reinterpret_cast<const unsigned char*>(pioinfo[blockIndex]);
			if (!block)
				continue;
			for (int i = 0; i < 32; ++i)
				if (block[i * ioinfoStride + sizeof(intptr_t)] & 0x01)
					++open;
		}
		if (open > peakOpenFiles)
			peakOpenFiles = open;
		return open;
	}

	// Skip unchanged registered paths using path, size and write time; retry failed installs.

	std::vector<std::string> knownEntries;
	std::vector<std::string> currentSnapshot;
	bool lastSnapshotComplete = false;
	unsigned int unchangedSkipped = 0;

	std::string SnapshotLine(std::string_view path) {
		wchar_t wide[MAX_PATH * 2]{};
		MultiByteToWideChar(CP_UTF8, 0, path.data(), static_cast<int>(path.size()), wide, MAX_PATH * 2 - 1);
		WIN32_FILE_ATTRIBUTE_DATA data{};
		std::string line(path);
		if (GetFileAttributesExW(wide, GetFileExInfoStandard, &data)) {
			line += '|';
			line += std::to_string((static_cast<unsigned long long>(data.nFileSizeHigh) << 32) | data.nFileSizeLow);
			line += '|';
			line += std::to_string((static_cast<unsigned long long>(data.ftLastWriteTime.dwHighDateTime) << 32) | data.ftLastWriteTime.dwLowDateTime);
		}
		else {
			line += "|missing";
		}
		return line;
	}

	std::string PreparedSnapshot(const unsigned char* service);

	bool IsKnown(const std::string& line) {
		return lastSnapshotComplete && std::binary_search(knownEntries.begin(), knownEntries.end(), line);
	}

	bool SkipIfKnown(unsigned char* service) {
		if (!rescanSkip || !lastSnapshotComplete)
			return false;
		std::string line = PreparedSnapshot(service);
		if (!IsKnown(line))
			return false;
		currentSnapshot.push_back(std::move(line));
		CallRemoveLast(service + off_queueBegin);
		++unchangedSkipped;
		return true;
	}


	long long drainStartTicks = 0;
	unsigned int queueAtStart = 0;
	unsigned int extraInstalls = 0;
	unsigned int extraLoads = 0;
	unsigned int drainTicks = 0;
	unsigned int unregisteredInstalls = 0;
	unsigned int droppedRequests = 0;
	unsigned int packagesAtStart = 0;
	unsigned int dlcAtStart = 0;
	long long queueEmptyTicks = 0;
	unsigned int waitingAtQueueEmpty = 0;
	long long lastProgressLogTicks = 0;

	std::atomic<bool> progressActive = false;
	std::atomic<bool> progressCompleted = false;
	std::atomic<bool> progressUpToDate = false;
	std::atomic<unsigned int> progressDetected = 0;
	std::atomic<unsigned int> progressProcessed = 0;
	std::atomic<unsigned int> progressRemaining = 0;
	std::atomic<unsigned int> progressRegistered = 0;
	std::atomic<unsigned int> progressNotRegistered = 0;
	std::atomic<unsigned int> progressNewDlc = 0;
	std::atomic<unsigned int> progressTotalDlc = 0;
	std::atomic<double> progressElapsedSeconds = 0.0;
	std::atomic<long long> completionStartTicks = 0;

	// Prefetch paths follow the native install order.
	unsigned int NextInstallIndex(const unsigned char* service) {
		const unsigned int length = QueueLength(service);
		return queueAtStart >= length ? queueAtStart - length : 0;
	}

	std::string FileName(std::string_view path) {
		const size_t slash = path.find_last_of("\\/");
		return std::string(slash == std::string_view::npos ? path : path.substr(slash + 1));
	}

	// Blocked time is wall time minus thread CPU time; convert TSC cycles using the calibrated rate.

	long long calibrationQpc = 0;
	unsigned long long calibrationTsc = 0;
	double cyclesPerMs = 0.0;

	unsigned long long ThreadCycles() {
		ULONG64 cycles = 0;
		QueryThreadCycleTime(GetCurrentThread(), &cycles);
		return cycles;
	}

	void CalibrateCycles() {
		if (cyclesPerMs > 0.0 || !calibrationQpc)
			return;
		const double ms = MsSince(calibrationQpc);
		if (ms < 50.0)
			return;
		cyclesPerMs = static_cast<double>(__rdtsc() - calibrationTsc) / ms;
		LOG_INFO("(ENUMERATION) CPU time measurement: " << cyclesPerMs / 1000.0 << " MHz TSC, measured over " << ms / 1000.0 << " s" << std::endl);
	}

	double CyclesToMs(unsigned long long cycles) {
		return cyclesPerMs > 0.0 ? cycles / cyclesPerMs : -1.0;
	}

	// Workers warm queued file headers ahead of installation; the game thread waits without joining I/O.
	// Stop warming when completed warm installs remain slow.

	unsigned int prefetchKb = 128;
	bool prefetchForced = true;
	// Bound prepared buffers within the 32-bit address space.
	constexpr unsigned int prefetchLead = 1000;
	constexpr unsigned int prefetchThreads = 8;
	// Small worker stacks preserve contiguous address space for profile saves.
	constexpr SIZE_T warmupStackSize = 64 * 1024;
	// Bound warm-up waits so stuck I/O falls back to native opening.
	constexpr double maxWarmWaitMs = 5000.0;
	// Stop ineffective warming after consecutive slow progress windows.
	constexpr double prefetchBackoffMsPerInstall = 40.0;
	constexpr unsigned int prefetchBackoffWindows = 2;
	constexpr unsigned int maxPrefetchErrorLogs = 20;
	constexpr double slowWarmOpenMs = 50.0;
	constexpr unsigned int maxSlowWarmOpenLogs = 20;

	// Workers and StopPrefetch hold job references so timed-out I/O cannot outlive its path list.
	struct PrefetchJob {
		std::vector<std::string> paths;
		std::vector<std::unique_ptr<EnumerationPreparation::Prepared>> prepared;
		// Per-file QPC readiness: 0 pending, -1 open/read failed.
		std::unique_ptr<std::atomic<long long>[]> readTicks;
		std::atomic<bool> stop = false;
		std::atomic<bool> backedOff = false;
		std::atomic<unsigned int> files = 0;
		std::atomic<unsigned int> skipped = 0;
		std::atomic<unsigned int> failures = 0;
		std::atomic<unsigned int> slowOpens = 0;
		std::atomic<unsigned int> next = 0;
		std::atomic<unsigned int> inFlight = 0;
		std::atomic<unsigned int> running = 0;
		std::atomic<long long> ioTicks = 0;
		std::atomic<long long> openTicks = 0;
		std::atomic<long long> parseTicks = 0;
		std::atomic<long long> hashTicks = 0;
		std::atomic<unsigned int> preparedFiles = 0;
		std::atomic<unsigned long long> bytes = 0;
		std::atomic<int> refs = 1;
		uint64_t cacheGeneration = 0;
		long long startTicks = 0;
		std::atomic<long long> endTicks = 0;
		void Release() {
			if (refs.fetch_sub(1, std::memory_order_acq_rel) == 1)
				delete this;
		}
	};
	PrefetchJob* prefetchJob = nullptr;
	uint64_t scanCacheGeneration = 0;
	std::vector<HANDLE> prefetchHandles;

	EnumerationPreparation::Prepared* NextPrepared(const unsigned char* service) {
		if (!EnumerationPreparation::IsInstalled() || !prefetchJob) return nullptr;
		const unsigned int index = NextInstallIndex(service);
		if (index >= prefetchJob->prepared.size()
			|| prefetchJob->readTicks[index].load(std::memory_order_acquire) == 0)
			return nullptr;
		return prefetchJob->prepared[index].get();
	}

	std::string PreparedSnapshot(const unsigned char* service) {
		if (const auto* prepared = NextPrepared(service)) return prepared->snapshot;
		return SnapshotLine(NextQueuedPath(service));
	}

	unsigned int ConsumedFiles(unsigned int total) {
		return total - (std::min)(total, progressRemaining.load(std::memory_order_acquire));
	}

	void NotePrefetchFailure(PrefetchJob* job, unsigned int index, const char* what, DWORD error) {
		job->readTicks[index].store(-1, std::memory_order_release);
		if (++job->failures <= maxPrefetchErrorLogs)
			LOG_WARNING("(ENUMERATION) Prefetch couldn't " << what << " #" << index << " " << job->paths[index] << ": error " << error
				<< (job->failures == maxPrefetchErrorLogs ? " (further prefetch errors are only counted)" : "") << std::endl);
	}

	bool ClaimNextFile(PrefetchJob* job, unsigned int total, unsigned int& index) {
		while (!job->stop.load(std::memory_order_acquire)) {
			unsigned int next = job->next.load(std::memory_order_acquire);
			if (next >= total)
				return false;
			const unsigned int consumed = ConsumedFiles(total);
			if (next < consumed) {
				if (job->next.compare_exchange_weak(next, consumed, std::memory_order_acq_rel))
					job->skipped += consumed - next;
				continue;
			}
			if (next >= consumed + prefetchLead) {
				Sleep(1);
				continue;
			}
			if (job->next.compare_exchange_weak(next, next + 1, std::memory_order_acq_rel)) {
				index = next;
				return true;
			}
		}
		return false;
	}

	DWORD WINAPI PrefetchMain(LPVOID param) {
		PrefetchJob* job = static_cast<PrefetchJob*>(param);
		SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_BELOW_NORMAL);

		const unsigned int total = static_cast<unsigned int>(job->paths.size());
		std::vector<char> buffer(static_cast<size_t>(prefetchKb) * 1024);
		unsigned int index = 0;
		// The first worker loads the cache; other workers wait here.
		if (EnumerationPreparation::IsInstalled())
			EnumerationCache::Load();
		while (ClaimNextFile(job, total, index)) {
			if (EnumerationPreparation::IsInstalled()) {
				++job->inFlight;
				bool ready = false;
				try {
					wchar_t wide[MAX_PATH * 2]{};
					MultiByteToWideChar(CP_UTF8, 0, job->paths[index].c_str(), -1, wide, MAX_PATH * 2);
					const long long statStart = EnumerationPackage::Clock();
					std::string snapshot = SnapshotLine(job->paths[index]);
					const long long statTicks = EnumerationPackage::Clock() - statStart;
					auto prepared = EnumerationPreparation::Prepare(wide, std::move(snapshot));
					prepared->package.ioTicks += statTicks;
					if (prepared->valid) {
						ready = true;
						++job->preparedFiles;
						job->ioTicks += prepared->package.ioTicks;
						job->parseTicks += prepared->package.parseTicks;
						job->hashTicks += prepared->package.hashTicks;
					}
					else {
						if (++job->failures <= maxPrefetchErrorLogs)
							LOG_WARNING("(ENUMERATION) Preparation fallback #" << index << " "
								<< job->paths[index] << ": unsupported, changed, unreadable or invalid archive" << std::endl);
					}
					job->prepared[index] = std::move(prepared);
					++job->files;
				}
				catch (const std::exception& error) {
					if (++job->failures <= maxPrefetchErrorLogs)
						LOG_WARNING("(ENUMERATION) Preparation fallback #" << index << ": " << error.what() << std::endl);
				}
				catch (...) {
					if (++job->failures <= maxPrefetchErrorLogs)
						LOG_WARNING("(ENUMERATION) Preparation fallback #" << index << ": worker exception" << std::endl);
				}
				--job->inFlight;
				// Publish readiness last; the consumer may immediately free the entry.
				job->readTicks[index].store(ready ? Now() : -1, std::memory_order_release);
				continue;
			}
			if (job->readTicks[index].load(std::memory_order_acquire) != 0) {
				++job->skipped;
				continue;
			}
			++job->inFlight;
			wchar_t wide[MAX_PATH * 2]{};
			MultiByteToWideChar(CP_UTF8, 0, job->paths[index].c_str(), -1, wide, MAX_PATH * 2);
			const long long openStart = Now();
			// Avoid read-ahead hints that fetch beyond the required header.
			HANDLE file = CreateFileW(wide, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr, OPEN_EXISTING, FILE_FLAG_RANDOM_ACCESS, nullptr);
			const long long opened = Now();
			job->openTicks += opened - openStart;
			if (file == INVALID_HANDLE_VALUE) {
				NotePrefetchFailure(job, index, "open", GetLastError());
				job->ioTicks += opened - openStart;
				--job->inFlight;
				continue;
			}
			DWORD read = 0;
			const bool ok = ReadFile(file, buffer.data(), static_cast<DWORD>(buffer.size()), &read, nullptr) != FALSE;
			const DWORD readError = ok ? 0 : GetLastError();
			CloseHandle(file);
			const long long done = Now();
			job->ioTicks += done - openStart;
			if (ok) {
				++job->files;
				job->bytes += read;
				job->readTicks[index].store(done, std::memory_order_release);
			}
			else {
				NotePrefetchFailure(job, index, "read", readError);
			}
			--job->inFlight;

			const double openMs = (opened - openStart) / ticksPerMs;
			if (openMs >= slowWarmOpenMs) {
				const unsigned int slow = ++job->slowOpens;
				if (slow <= maxSlowWarmOpenLogs)
					LOG_INFO("(ENUMERATION) Slow warm-up #" << index << " " << job->paths[index] << ": open " << openMs << " ms, read+close "
						<< (done - opened) / ticksPerMs << " ms, game at " << ConsumedFiles(total)
						<< (slow == maxSlowWarmOpenLogs ? " (further slow warm-ups are only counted)" : "") << std::endl);
			}
		}
		if (job->running.fetch_sub(1, std::memory_order_acq_rel) == 1) {
			job->endTicks.store(Now(), std::memory_order_release);
			LOG_INFO("(ENUMERATION) " << (EnumerationPreparation::IsInstalled() ? "Preparation workers" : "Prefetch threads")
				<< " finished" << (job->stop.load() ? " (stopped)" : "") << " after " << MsSince(job->startTicks) << " ms" << std::endl);
			// Save only after the full queue was claimed; partial jobs cannot replace the cache.
			if (EnumerationPreparation::IsInstalled() && job->next.load(std::memory_order_acquire) >= total)
				EnumerationCache::Complete(job->paths, job->cacheGeneration);
		}
		job->Release();
		return 0;
	}

	// Unknown seek-penalty information also enables warming.
	bool LibraryHasSeekPenalty(const unsigned char* service, unsigned int count) {
		std::vector<std::pair<std::wstring, EnumerationEnvironment::SeekPenalty>> volumes;
		const unsigned int samples = (std::min)(count, 32u);
		bool penalty = false;
		for (unsigned int s = 0; s < samples; ++s) {
			const unsigned int index = samples == 1 ? 0 : static_cast<unsigned int>((static_cast<unsigned long long>(s) * (count - 1)) / (samples - 1));
			std::wstring finalPath;
			std::wstring mountPoint;
			if (!EnumerationEnvironment::ResolveMountPoint(QueuedPath(service, index), finalPath, mountPoint)) {
				penalty = true;
				continue;
			}
			const auto known = std::find_if(volumes.begin(), volumes.end(), [&](const auto& v) { return _wcsicmp(v.first.c_str(), mountPoint.c_str()) == 0; });
			if (known == volumes.end())
				volumes.emplace_back(mountPoint, EnumerationEnvironment::QueryVolume(mountPoint).seekPenalty);
		}
		for (const auto& volume : volumes)
			if (volume.second != EnumerationEnvironment::SeekPenalty::None)
				penalty = true;
		LOG_INFO("(ENUMERATION) DLC library is on " << volumes.size() << " volume(s), " << (penalty ? "at least one spinning or unknown" : "all SSD") << std::endl);
		return penalty;
	}


	void StartPrefetch(const unsigned char* service) {
		if (prefetchJob)
			return;
		if (prefetchKb == 0 && !EnumerationPreparation::IsInstalled()) {
			LOG_INFO("(ENUMERATION) Prefetch off (FastEnumerationPrefetchKB=0)" << std::endl);
			return;
		}
		const unsigned int count = QueueLength(service);
		if (count == 0)
			return;
		if (!prefetchForced && !EnumerationPreparation::IsInstalled()) {
			static int seekPenalty = -1;
			if (seekPenalty < 0)
				seekPenalty = LibraryHasSeekPenalty(service, count) ? 1 : 0;
			if (seekPenalty == 0) {
				LOG_INFO("(ENUMERATION) Prefetch off: FastEnumerationPrefetchHddOnly is on and the library is all SSD" << std::endl);
				return;
			}
		}
		PrefetchJob* job = new PrefetchJob;
		job->cacheGeneration = scanCacheGeneration;
		job->prepared.resize(count);
		job->paths.reserve(count);
		for (unsigned int i = count; i > 0; --i)
			job->paths.emplace_back(QueuedPath(service, i - 1));
		job->readTicks.reset(new std::atomic<long long>[count]);
		for (unsigned int i = 0; i < count; ++i)
			job->readTicks[i].store(0, std::memory_order_relaxed);
		job->startTicks = Now();
		// Count workers before starting them to prevent premature last-worker cleanup.
		job->running.store(prefetchThreads, std::memory_order_release);
		for (unsigned int i = 0; i < prefetchThreads; ++i) {
			job->refs.fetch_add(1, std::memory_order_acq_rel);
			HANDLE thread = CreateThread(nullptr, EnumerationPreparation::IsInstalled() ? 1024 * 1024 : warmupStackSize, PrefetchMain, job, STACK_SIZE_PARAM_IS_A_RESERVATION, nullptr);
			if (!thread) {
				LOG_WARNING("(ENUMERATION) Prefetch thread couldn't start (error " << GetLastError() << ")" << std::endl);
				job->refs.fetch_sub(1, std::memory_order_acq_rel);
				// Started workers may already have exited; publish completion if none remain.
				if (job->running.fetch_sub(1, std::memory_order_acq_rel) == 1)
					job->endTicks.store(Now(), std::memory_order_release);
				continue;
			}
			prefetchHandles.push_back(thread);
		}
		if (prefetchHandles.empty()) {
			job->Release();
			return;
		}
		prefetchJob = job;
		if (EnumerationPreparation::IsInstalled()) {
			LOG_INFO("(ENUMERATION) Preparation started: " << count << " files, "
				<< prefetchHandles.size() << " workers, at most " << prefetchLead
				<< " packages ahead; no game-thread I/O wait or warm-up timeout fallback" << std::endl);
			return;
		}
		LOG_INFO("(ENUMERATION) Prefetch started: " << count << " files, " << prefetchHandles.size() << " threads, first " << prefetchKb << " KB of each, up to "
			<< prefetchLead << " files ahead of the game. The game waits for a file that isn't warm yet (up to " << maxWarmWaitMs << " ms) instead of opening it cold" << std::endl);
	}

	void StopPrefetch() {
		if (!prefetchJob)
			return;
		PrefetchJob* job = prefetchJob;
		job->stop.store(true, std::memory_order_release);
		// Game ticks must not join I/O; workers retain the job until exit.
		const DWORD wait = WaitForMultipleObjects(static_cast<DWORD>(prefetchHandles.size()), prefetchHandles.data(), TRUE, 0);
		for (HANDLE thread : prefetchHandles)
			CloseHandle(thread);
		prefetchHandles.clear();
		prefetchJob = nullptr;
		const long long endTicks = job->endTicks.load(std::memory_order_acquire);
		const long long end = endTicks ? endTicks : Now();
		const unsigned int files = job->files.load();
		if (EnumerationPreparation::IsInstalled()) {
			LOG_INFO("(ENUMERATION) Preparation done: " << job->preparedFiles.load() << " prepared of "
				<< job->paths.size() << " queued, " << EnumerationPreparation::CacheOnlyHits() << " without opening the archive, "
				<< job->failures.load() << " fallbacks, "
				<< (end - job->startTicks) / ticksPerMs << " ms"
				<< (wait == WAIT_TIMEOUT ? " (workers retain the job until they finish)" : "") << std::endl);
			job->Release();
			return;
		}
		LOG_INFO("(ENUMERATION) Prefetch: " << files << " of " << job->paths.size() << " files warmed, " << job->skipped.load() << " skipped as already installed, "
			<< job->failures.load() << " failed, " << job->slowOpens.load() << " opens >= " << slowWarmOpenMs << " ms, " << (job->bytes.load() >> 20) << " MB, "
			<< (end - job->startTicks) / ticksPerMs << " ms, disk " << job->ioTicks.load() / ticksPerMs << " ms summed over the threads (opening "
			<< job->openTicks.load() / ticksPerMs << " ms, " << (files ? job->ioTicks.load() / ticksPerMs / files : 0.0) << " ms/file)"
			<< (job->backedOff.load() ? ", backed off" : "") << (wait == WAIT_TIMEOUT ? " (a thread was still in an open or read, left to finish)" : "") << std::endl);
		job->Release();
	}

	// Only set the stop flag on the game thread; joining a blocked worker would stall the frame.
	unsigned int slowPrefetchWindows = 0;

	void BackOffPrefetchIfSlow(unsigned int warmInstalls, double warmInstallMs) {
		PrefetchJob* job = prefetchJob;
		if (EnumerationPreparation::IsInstalled() || !job || job->stop.load(std::memory_order_acquire) || job->endTicks.load(std::memory_order_acquire) || warmInstalls == 0)
			return;
		const double msPerInstall = warmInstallMs / warmInstalls;
		slowPrefetchWindows = msPerInstall >= prefetchBackoffMsPerInstall ? slowPrefetchWindows + 1 : 0;
		if (slowPrefetchWindows < prefetchBackoffWindows)
			return;
		job->backedOff.store(true, std::memory_order_release);
		job->stop.store(true, std::memory_order_release);
		LOG_INFO("(ENUMERATION) Prefetch stopped: files it had warmed still took " << msPerInstall << " ms/install (limit " << prefetchBackoffMsPerInstall << ") for "
			<< slowPrefetchWindows << " windows in a row, so warming isn't what makes installs slow here. The game installs at its own pace for the rest of the scan." << std::endl);
	}

	enum class PrefetchActivity { Reading, Waiting, NotRunning };
	constexpr const char* prefetchActivityNames[] = { "while the prefetch was reading", "while the prefetch was waiting for the game", "with no prefetch running" };

	PrefetchActivity CurrentPrefetchActivity() {
		PrefetchJob* job = prefetchJob;
		if (!job || job->stop.load(std::memory_order_acquire) || job->endTicks.load(std::memory_order_acquire))
			return PrefetchActivity::NotRunning;
		return job->inFlight.load(std::memory_order_acquire) ? PrefetchActivity::Reading : PrefetchActivity::Waiting;
	}

	// Warm age in milliseconds: -1 pending, -2 failed, -3 no prefetch.
	double PrefetchedMsBefore(unsigned int index, long long installStart) {
		PrefetchJob* job = prefetchJob;
		if (!job || index >= job->paths.size())
			return -3.0;
		const long long ticks = job->readTicks[index].load(std::memory_order_acquire);
		if (ticks == 0 || ticks > installStart)
			return -1.0;
		if (ticks < 0)
			return -2.0;
		return (installStart - ticks) / ticksPerMs;
	}

	std::string DescribePrefetched(double msBefore) {
		if (msBefore == -3.0)
			return "no prefetch";
		if (msBefore == -2.0)
			return "prefetch failed on it";
		if (msBefore < 0.0)
			return "NOT prefetched yet";
		std::ostringstream out;
		out << (EnumerationPreparation::IsInstalled() ? "prepared " : "prefetched ") << msBefore << " ms before";
		return out.str();
	}

	std::string PrefetchStatus(unsigned int consumed) {
		PrefetchJob* job = prefetchJob;
		if (!job)
			return "prefetch off";
		if (job->backedOff.load(std::memory_order_acquire))
			return "prefetch stopped (backed off)";
		if (job->endTicks.load(std::memory_order_acquire))
			return EnumerationPreparation::IsInstalled() ? "preparation done" : "prefetch done";
		const long long ahead = static_cast<long long>(job->next.load(std::memory_order_acquire)) - consumed;
		return std::string(EnumerationPreparation::IsInstalled() ? "preparation " : "prefetch ")
			+ std::to_string(ahead) + " ahead, " + std::to_string(job->inFlight.load(std::memory_order_acquire)) + " working";
	}

	// Warm DLC files in background threads, then transfer readiness to the scan job.
	// Small stacks and early path-list release preserve the 32-bit address space.

	constexpr unsigned int bootWarmupThreads = 8;
	constexpr unsigned int maxDlcFolderDepth = 16;
	bool bootWarmup = true;

	void NormalizePath(std::wstring& path) {
		for (wchar_t& c : path)
			if (c == L'/')
				c = L'\\';
		if (!path.empty())
			CharLowerBuffW(path.data(), static_cast<DWORD>(path.size()));
	}

	unsigned long long PathHash(const std::wstring& normalized) {
		unsigned long long hash = 14695981039346656037ull;
		for (wchar_t c : normalized) {
			hash ^= static_cast<unsigned short>(c);
			hash *= 1099511628211ull;
		}
		return hash;
	}

	unsigned long long QueuedPathHash(std::string_view path) {
		std::wstring wide(path.size() + 1, L'\0');
		const int length = MultiByteToWideChar(CP_UTF8, 0, path.data(), static_cast<int>(path.size()), wide.data(), static_cast<int>(wide.size()));
		wide.resize(length > 0 ? static_cast<size_t>(length) : 0);
		NormalizePath(wide);
		return PathHash(wide);
	}

	struct BootJob {
		// The last worker frees the install-order path list.
		std::vector<std::wstring> paths;
		std::vector<unsigned long long> hashes;
		std::unique_ptr<std::atomic<bool>[]> warmed;
		std::atomic<bool> stop = false;
		// Publish after paths, hashes and warmed flags are initialized.
		std::atomic<bool> listed = false;
		// Set before listed: open the files without reading them.
		bool metadataOnly = false;
		std::atomic<unsigned int> next = 0;
		std::atomic<unsigned int> files = 0;
		std::atomic<unsigned int> failures = 0;
		std::atomic<unsigned int> slowOpens = 0;
		std::atomic<unsigned int> covered = 0;
		std::atomic<unsigned int> running = 0;
		std::atomic<long long> openTicks = 0;
		std::atomic<int> refs = 1;
		long long startTicks = 0;
		long long listedTicks = 0;
		std::atomic<long long> endTicks = 0;
		void Release() {
			if (refs.fetch_sub(1, std::memory_order_acq_rel) == 1)
				delete this;
		}
	};
	BootJob* bootJob = nullptr;

	// Match native order: files Z to A, then subfolders Z to A.
	void ListDlcFolder(const std::wstring& folder, unsigned int depth, std::vector<std::wstring>& out) {
		if (depth > maxDlcFolderDepth)
			return;
		std::vector<std::wstring> files;
		std::vector<std::wstring> folders;
		WIN32_FIND_DATAW data{};
		HANDLE find = FindFirstFileExW((folder + L"\\*").c_str(), FindExInfoBasic, &data, FindExSearchNameMatch, nullptr, FIND_FIRST_EX_LARGE_FETCH);
		if (find == INVALID_HANDLE_VALUE)
			return;
		do {
			if (data.cFileName[0] == L'.' && (data.cFileName[1] == 0 || (data.cFileName[1] == L'.' && data.cFileName[2] == 0)))
				continue;
			if (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
				folders.emplace_back(folder + L"\\" + data.cFileName);
			}
			else {
				const size_t length = wcslen(data.cFileName);
				if (length > 6 && _wcsicmp(data.cFileName + length - 6, L".psarc") == 0)
					files.emplace_back(folder + L"\\" + data.cFileName);
			}
		} while (FindNextFileW(find, &data));
		FindClose(find);
		const auto byName = [](const std::wstring& a, const std::wstring& b) { return _wcsicmp(a.c_str(), b.c_str()) > 0; };
		std::sort(files.begin(), files.end(), byName);
		std::sort(folders.begin(), folders.end(), byName);
		for (std::wstring& file : files)
			out.push_back(std::move(file));
		for (const std::wstring& sub : folders)
			ListDlcFolder(sub, depth + 1, out);
	}

	DWORD WINAPI BootWarmupMain(LPVOID param) {
		BootJob* job = static_cast<BootJob*>(param);
		if (!SetThreadPriority(GetCurrentThread(), THREAD_MODE_BACKGROUND_BEGIN))
			SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_LOWEST);
		const unsigned int total = static_cast<unsigned int>(job->paths.size());
		std::vector<char> buffer(static_cast<size_t>(prefetchKb) * 1024);
		while (!job->stop.load(std::memory_order_acquire)) {
			const unsigned int index = job->next.fetch_add(1, std::memory_order_acq_rel);
			if (index >= total)
				break;
			// Skip warming when the boot cache already supplies the reads.
			if (LazyVfs::Covers(job->paths[index].c_str())) {
				job->warmed[index].store(true, std::memory_order_release);
				++job->covered;
				continue;
			}
			const long long openStart = Now();
			if (job->metadataOnly) {
				// The game's existence check opens the file, and the first open is the slow one.
				HANDLE probe = CreateFileW(job->paths[index].c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
				const bool found = probe != INVALID_HANDLE_VALUE;
				if (found)
					CloseHandle(probe);
				const long long looked = Now();
				job->openTicks += looked - openStart;
				if (!found) {
					++job->failures;
					continue;
				}
				job->warmed[index].store(true, std::memory_order_release);
				++job->files;
				if ((looked - openStart) / ticksPerMs >= slowWarmOpenMs)
					++job->slowOpens;
				continue;
			}
			HANDLE file = CreateFileW(job->paths[index].c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr, OPEN_EXISTING, FILE_FLAG_RANDOM_ACCESS, nullptr);
			const long long opened = Now();
			job->openTicks += opened - openStart;
			if (file == INVALID_HANDLE_VALUE) {
				++job->failures;
				continue;
			}
			DWORD read = 0;
			const bool ok = buffer.empty() || ReadFile(file, buffer.data(), static_cast<DWORD>(buffer.size()), &read, nullptr) != FALSE;
			CloseHandle(file);
			if (!ok) {
				++job->failures;
				continue;
			}
			job->warmed[index].store(true, std::memory_order_release);
			++job->files;
			if ((opened - openStart) / ticksPerMs >= slowWarmOpenMs)
				++job->slowOpens;
		}
		if (job->running.fetch_sub(1, std::memory_order_acq_rel) == 1) {
			const long long end = Now();
			job->endTicks.store(end, std::memory_order_release);
			const unsigned int files = job->files.load();
			const double ms = (end - job->listedTicks) / ticksPerMs;
			LOG_INFO("(ENUMERATION) Boot warm-up " << (job->stop.load() ? "stopped" : "done") << ": " << files << " of " << total << " files warmed in " << ms
				<< " ms (" << (ms > 0.0 ? files * 1000.0 / ms : 0.0) << " files/s), opening " << job->openTicks.load() / ticksPerMs << " ms summed over the threads, "
				<< job->slowOpens.load() << " opens >= " << slowWarmOpenMs << " ms, " << job->failures.load() << " failed, "
				<< job->covered.load() << " skipped as covered by the boot read cache" << std::endl);
			std::vector<std::wstring>().swap(job->paths);
		}
		job->Release();
		return 0;
	}

	DWORD WINAPI BootListMain(LPVOID param) {
		BootJob* job = static_cast<BootJob*>(param);
		SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_BELOW_NORMAL);
		wchar_t exe[MAX_PATH]{};
		GetModuleFileNameW(nullptr, exe, MAX_PATH);
		std::wstring folder(exe);
		const size_t slash = folder.find_last_of(L"\\/");
		folder = (slash == std::wstring::npos ? L"" : folder.substr(0, slash + 1)) + L"dlc";
		ListDlcFolder(folder, 0, job->paths);

		const size_t count = job->paths.size();
		job->hashes.reserve(count);
		for (const std::wstring& path : job->paths) {
			std::wstring normalized(path);
			NormalizePath(normalized);
			job->hashes.push_back(PathHash(normalized));
		}
		job->warmed.reset(new std::atomic<bool>[count]);
		for (size_t i = 0; i < count; ++i)
			job->warmed[i].store(false, std::memory_order_relaxed);
		// A complete cache needs metadata warming only; avoid competing with boot reads.
		EnumerationCache::Load();
		const size_t cached = EnumerationCache::LoadedCount();
		if (count && cached * 100 >= count * 80) {
			job->metadataOnly = true;
			LOG_INFO("(ENUMERATION) Boot warm-up opens the files without reading them: the cache covers " << cached << " of " << count << " psarcs" << std::endl);
		}
		job->listedTicks = Now();
		job->listed.store(true, std::memory_order_release);
		LOG_INFO("(ENUMERATION) Boot warm-up: " << count << " psarcs listed in " << (job->listedTicks - job->startTicks) / ticksPerMs << " ms, opening them on "
			<< bootWarmupThreads << " threads at background I/O priority" << std::endl);

		job->running.store(bootWarmupThreads, std::memory_order_release);
		for (unsigned int i = 1; i < bootWarmupThreads; ++i) {
			job->refs.fetch_add(1, std::memory_order_acq_rel);
			HANDLE thread = CreateThread(nullptr, warmupStackSize, BootWarmupMain, job, STACK_SIZE_PARAM_IS_A_RESERVATION, nullptr);
			if (thread) {
				CloseHandle(thread);
				continue;
			}
			job->refs.fetch_sub(1, std::memory_order_acq_rel);
			// This running thread keeps the worker count above zero.
			job->running.fetch_sub(1, std::memory_order_acq_rel);
		}
		return BootWarmupMain(job);
	}

	void StartBootWarmup() {
		if (!bootWarmup || bootJob)
			return;
		BootJob* job = new BootJob;
		job->startTicks = Now();
		job->refs.store(2, std::memory_order_release);
		HANDLE thread = CreateThread(nullptr, warmupStackSize, BootListMain, job, STACK_SIZE_PARAM_IS_A_RESERVATION, nullptr);
		if (!thread) {
			LOG_WARNING("(ENUMERATION) Boot warm-up thread couldn't start (error " << GetLastError() << ")" << std::endl);
			delete job;
			return;
		}
		CloseHandle(thread);
		bootJob = job;
	}

	// Transfer warmed flags on the game thread, then stop boot workers after their current I/O.
	void HandOffBootWarmup() {
		BootJob* job = bootJob;
		if (!job)
			return;
		bootJob = nullptr;
		job->stop.store(true, std::memory_order_release);
		if (EnumerationPreparation::IsInstalled()) {
			LOG_INFO("(ENUMERATION) Boot warm-up stopped for worker preparation: "
				<< job->files.load() << " files warmed" << std::endl);
			job->Release();
			return;
		}
		if (!job->listed.load(std::memory_order_acquire)) {
			LOG_INFO("(ENUMERATION) Boot warm-up was still listing the dlc folder when the scan started, the scan's warm-up does it all" << std::endl);
			job->Release();
			return;
		}
		const size_t count = job->hashes.size();
		std::vector<unsigned long long> warmedHashes;
		warmedHashes.reserve(job->files.load(std::memory_order_acquire));
		for (size_t i = 0; i < count; ++i)
			if (job->warmed[i].load(std::memory_order_acquire))
				warmedHashes.push_back(job->hashes[i]);
		std::sort(warmedHashes.begin(), warmedHashes.end());
		const long long end = job->endTicks.load(std::memory_order_acquire);
		const double ranMs = ((end ? end : Now()) - job->listedTicks) / ticksPerMs;

		unsigned int marked = 0;
		PrefetchJob* prefetch = prefetchJob;
		if (prefetch && !EnumerationPreparation::IsInstalled()) {
			const long long now = Now();
			for (size_t i = 0; i < prefetch->paths.size(); ++i) {
				if (!std::binary_search(warmedHashes.begin(), warmedHashes.end(), QueuedPathHash(prefetch->paths[i])))
					continue;
				long long expected = 0;
				if (prefetch->readTicks[i].compare_exchange_strong(expected, now, std::memory_order_acq_rel))
					++marked;
			}
		}
		LOG_INFO("(ENUMERATION) Boot warm-up handed over at the scan start: it had warmed " << warmedHashes.size() << " of " << count << " files in " << ranMs
			<< " ms" << (end ? "" : " and was still running") << "; " << marked << " of the " << (prefetch ? prefetch->paths.size() : 0)
			<< " queued files matched and are marked warm" << (prefetch ? "" : " (no scan warm-up to mark them in)") << std::endl);
		job->Release();
	}

	// Wait for worker readiness or failure so cold opens stay off the game thread.

	// ~0 means no pending file.
	unsigned int warmWaitIndex = ~0u;
	long long warmWaitStart = 0;
	bool warmWaitTimedOut = false;
	unsigned int warmWaitFiles = 0;
	unsigned int warmWaitTimeouts = 0;
	double warmWaitMs = 0.0;
	unsigned int stockHolds = 0;
	bool warmOrderMismatchLogged = false;

	void EndWarmWait(long long now) {
		if (warmWaitIndex == ~0u)
			return;
		warmWaitMs += (now - warmWaitStart) / ticksPerMs;
		warmWaitIndex = ~0u;
	}

	bool IsNextWarm(const unsigned char* service) {
		PrefetchJob* job = prefetchJob;
		const long long now = Now();
		if (!draining || !job || job->stop.load(std::memory_order_acquire) || job->endTicks.load(std::memory_order_acquire)
			|| !service[off_installing] || QueueLength(service) == 0) {
			EndWarmWait(now);
			return true;
		}
		const unsigned int index = NextInstallIndex(service);
		if (index >= job->paths.size()) {
			EndWarmWait(now);
			return true;
		}
		if (job->readTicks[index].load(std::memory_order_acquire) != 0) {
			EndWarmWait(now);
			return true;
		}
		if (warmWaitIndex != index) {
			EndWarmWait(now);
			// Do not wait if the native queue differs from the prefetch job order.
			if (NextQueuedPath(service) != job->paths[index]) {
				if (!warmOrderMismatchLogged) {
					warmOrderMismatchLogged = true;
					LOG_WARNING("(ENUMERATION) Scan queue no longer matches the prefetch order at #" << index << ", the game won't wait for warm files" << std::endl);
				}
				return true;
			}
			warmWaitIndex = index;
			warmWaitStart = now;
			warmWaitTimedOut = false;
			++warmWaitFiles;
		}
		if (warmWaitTimedOut)
			return true;
		if ((now - warmWaitStart) / ticksPerMs >= maxWarmWaitMs) {
			if (EnumerationPreparation::IsInstalled()) return false;
			warmWaitTimedOut = true;
			++warmWaitTimeouts;
			LOG_INFO("(ENUMERATION) Waited " << maxWarmWaitMs << " ms for #" << index << " " << job->paths[index] << " to warm, the game opens it itself" << std::endl);
			return true;
		}
		return false;
	}

	// Split install time into stat, open/read, registration and post-registration work.
	// Installs that never register charge all remaining time to open/read.

	constexpr double slowInstallMs = 50.0;
	constexpr unsigned int maxSlowInstallLogs = 100;
	constexpr unsigned int topSlowInstalls = 10;
	constexpr double histogramEdgesMs[] = { 1, 2, 5, 10, 25, 50, 100, 250, 500 };
	constexpr size_t histogramBuckets = sizeof(histogramEdgesMs) / sizeof(histogramEdgesMs[0]) + 1;

	struct InstallTiming {
		unsigned int index = 0;
		std::string path;
		double totalMs = 0.0;
		double statMs = 0.0;
		double openReadMs = 0.0;
		double existsMs = 0.0;
		double registerMs = 0.0;
		double afterMs = 0.0;
		// -1 means unknown.
		double cpuMs = -1.0;
		bool reachedRegister = false;
		bool registered = false;
		double prefetchedMsBefore = -3.0;
		PrefetchActivity activity = PrefetchActivity::NotRunning;
	};

	struct TimingSums {
		unsigned int installs = 0;
		double totalMs = 0.0;
		double statMs = 0.0;
		double openReadMs = 0.0;
		double existsMs = 0.0;
		double registerMs = 0.0;
		double afterMs = 0.0;
		double cpuMs = 0.0;
		unsigned int cpuKnown = 0;
		double cpuKnownTotalMs = 0.0;
		double maxMs = 0.0;
		unsigned int slow = 0;

		void Add(const InstallTiming& t) {
			++installs;
			totalMs += t.totalMs;
			statMs += t.statMs;
			openReadMs += t.openReadMs;
			existsMs += t.existsMs;
			registerMs += t.registerMs;
			afterMs += t.afterMs;
			if (t.cpuMs >= 0.0) {
				++cpuKnown;
				cpuMs += t.cpuMs;
				cpuKnownTotalMs += t.totalMs;
			}
			maxMs = (std::max)(maxMs, t.totalMs);
			if (t.totalMs >= slowInstallMs)
				++slow;
		}
		double Avg(double sum) const { return installs ? sum / installs : 0.0; }
		double BlockedShare() const { return cpuKnownTotalMs > 0.0 ? 1.0 - (std::min)(1.0, cpuMs / cpuKnownTotalMs) : -1.0; }

		std::string Describe() const {
			std::ostringstream out;
			out << "avg " << Avg(totalMs) << " ms (stat " << Avg(statMs) << ", open+read " << Avg(openReadMs) << " (exists check " << Avg(existsMs) << ")" << ", register " << Avg(registerMs)
				<< ", after " << Avg(afterMs);
			if (cpuKnown) {
				const double cpuAvg = cpuMs / cpuKnown;
				const double wallAvg = cpuKnownTotalMs / cpuKnown;
				out << "; CPU " << cpuAvg << ", blocked " << (std::max)(0.0, wallAvg - cpuAvg) << " = " << static_cast<int>(BlockedShare() * 100.0 + 0.5) << "%";
			}
			else {
				out << "; CPU unknown";
			}
			out << "), max " << maxMs << " ms, " << slow << " >= " << slowInstallMs << " ms";
			return out.str();
		}
	};

	TimingSums scanTiming;
	TimingSums timingByActivity[3];
	TimingSums timingPrefetched;
	TimingSums timingNotPrefetched;
	unsigned int installHistogram[histogramBuckets]{};
	std::vector<InstallTiming> slowest;
	unsigned int slowInstallLogs = 0;
	double stockTickMs = 0.0;
	unsigned int stockTicks = 0;
	double loadStepMs = 0.0;

	struct ProgressWindow {
		unsigned int ticks = 0;
		unsigned int installs = 0;
		unsigned int budgetStops = 0;
		unsigned int ceilingStops = 0;
		unsigned int warmStops = 0;
		unsigned int stockHolds = 0;
		unsigned int warmInstalls = 0;
		double warmInstallMs = 0.0;
		unsigned int loads = 0;
		double drainMs = 0.0;
		double installMs = 0.0;
		double loadMs = 0.0;
		double stockTickMs = 0.0;
		double maxFrameMs = 0.0;
		long long lastTickStart = 0;
		TimingSums timing;
		// Window-start snapshots allow per-window prefetch deltas.
		unsigned int prefetchFiles = 0;
		long long prefetchIoTicks = 0;
		long long prefetchOpenTicks = 0;
	};
	ProgressWindow window;

	void ResetWindow() {
		const long long lastTickStart = window.lastTickStart;
		window = ProgressWindow{};
		window.lastTickStart = lastTickStart;
		if (PrefetchJob* job = prefetchJob) {
			window.prefetchFiles = job->files.load(std::memory_order_acquire);
			window.prefetchIoTicks = job->ioTicks.load(std::memory_order_acquire);
			window.prefetchOpenTicks = job->openTicks.load(std::memory_order_acquire);
		}
	}

	size_t HistogramBucket(double ms) {
		for (size_t i = 0; i < histogramBuckets - 1; ++i)
			if (ms < histogramEdgesMs[i])
				return i;
		return histogramBuckets - 1;
	}

	std::string DescribeInstall(const InstallTiming& t) {
		std::ostringstream out;
		out << "#" << t.index << " " << t.path << ": " << t.totalMs << " ms (stat " << t.statMs << ", open+read " << t.openReadMs << " (exists check " << t.existsMs << ")" << ", register " << t.registerMs
			<< ", after " << t.afterMs << "; CPU ";
		if (t.cpuMs >= 0.0)
			out << t.cpuMs << ", blocked " << (std::max)(0.0, t.totalMs - t.cpuMs);
		else
			out << "unknown";
		out << "), " << (t.reachedRegister ? (t.registered ? "registered" : "reached registration but didn't register") : "refused before registration")
			<< ", " << DescribePrefetched(t.prefetchedMsBefore) << ", installed " << prefetchActivityNames[static_cast<int>(t.activity)];
		WIN32_FILE_ATTRIBUTE_DATA data{};
		wchar_t wide[MAX_PATH * 2]{};
		MultiByteToWideChar(CP_UTF8, 0, t.path.c_str(), -1, wide, MAX_PATH * 2 - 1);
		if (GetFileAttributesExW(wide, GetFileExInfoStandard, &data))
			out << ", file " << (((static_cast<unsigned long long>(data.nFileSizeHigh) << 32) | data.nFileSizeLow) >> 10) << " KB";
		return out.str();
	}

	void RecordInstall(InstallTiming&& t) {
		scanTiming.Add(t);
		window.timing.Add(t);
		timingByActivity[static_cast<int>(t.activity)].Add(t);
		(t.prefetchedMsBefore >= 0.0 ? timingPrefetched : timingNotPrefetched).Add(t);
		if (t.prefetchedMsBefore >= 0.0) {
			++window.warmInstalls;
			window.warmInstallMs += t.totalMs;
		}
		++installHistogram[HistogramBucket(t.totalMs)];

		if (t.totalMs >= slowInstallMs && slowInstallLogs < maxSlowInstallLogs) {
			++slowInstallLogs;
			LOG_INFO("(ENUMERATION) Slow install " << DescribeInstall(t)
				<< (slowInstallLogs == maxSlowInstallLogs ? " (further slow installs are only counted, see the scan summary)" : "") << std::endl);
		}
		if (slowest.size() < topSlowInstalls || t.totalMs > slowest.back().totalMs) {
			const auto at = std::find_if(slowest.begin(), slowest.end(), [&](const InstallTiming& s) { return t.totalMs > s.totalMs; });
			slowest.insert(at, std::move(t));
			if (slowest.size() > topSlowInstalls)
				slowest.pop_back();
		}
	}

	// Balance hook scope even when native code reaches TryDrain through SEH.
	long long InstallPrepared(unsigned char* service, EnumerationPreparation::Prepared* prepared) {
		long long ioTicks = 0;
		EnumerationPreparation::Begin(prepared);
		__try {
			CallInstallNext(service);
		}
		__finally {
			if (AbnormalTermination()) EnumerationPreparation::NoteInstallFault();
			ioTicks = EnumerationPreparation::End();
		}
		return ioTicks;
	}


	void InstallNext(unsigned char* service, bool countExtra) {
		InstallTiming t;
		t.index = NextInstallIndex(service);
		const unsigned int preparedIndex = t.index;
		t.path = std::string(NextQueuedPath(service));
		t.activity = CurrentPrefetchActivity();
		const long long start = Now();
		t.prefetchedMsBefore = PrefetchedMsBefore(t.index, start);
		const unsigned long long cyclesStart = ThreadCycles();

		std::string line;
		if (rescanSkip)
			line = PreparedSnapshot(service);
		const long long callStart = Now();
		lastInstallRegistered = false;
		inInstallRegister = 0;
		registerEnterTicks = 0;
		registerLeaveTicks = 0;
		existsTicks = 0;
		existsCalls = 0;
		timingInstall = existsHooked;
		auto* prepared = NextPrepared(service);
		const long long mainIoTicks = InstallPrepared(service, prepared);
		timingInstall = false;
		const long long end = Now();
		const unsigned long long cycles = ThreadCycles() - cyclesStart;

		// Skip unchanged duplicates on rescans; a fresh boot retries them if the original was removed.
		if (rescanSkip)
			currentSnapshot.push_back(std::move(line));
		if (!lastInstallRegistered && countExtra)
			++unregisteredInstalls;

		t.totalMs = (end - start) / ticksPerMs;
		t.statMs = (callStart - start) / ticksPerMs;
		t.reachedRegister = registerEnterTicks >= callStart && registerLeaveTicks >= registerEnterTicks && registerLeaveTicks <= end;
		if (t.reachedRegister) {
			t.openReadMs = (registerEnterTicks - callStart) / ticksPerMs;
			t.registerMs = (registerLeaveTicks - registerEnterTicks) / ticksPerMs;
			t.afterMs = (end - registerLeaveTicks) / ticksPerMs;
		}
		else {
			t.openReadMs = (end - callStart) / ticksPerMs;
		}
		t.registered = lastInstallRegistered;
		t.existsMs = existsTicks / ticksPerMs;
		(void)mainIoTicks;
		const double cpuMs = CyclesToMs(cycles);
		t.cpuMs = cpuMs < 0.0 ? -1.0 : (std::min)(cpuMs, t.totalMs);
		if (t.activity == PrefetchActivity::Waiting && CurrentPrefetchActivity() == PrefetchActivity::Reading)
			t.activity = PrefetchActivity::Reading;
		RecordInstall(std::move(t));
		if (prefetchJob && prepared)
			prefetchJob->prepared[preparedIndex].reset();
	}


	std::vector<std::string> SamplePaths(const unsigned char* service, unsigned int count, unsigned int wanted) {
		std::vector<std::string> paths;
		const unsigned int samples = (std::min)(count, wanted);
		for (unsigned int s = 0; s < samples; ++s) {
			const unsigned int index = samples == 1 ? 0 : static_cast<unsigned int>((static_cast<unsigned long long>(s) * (count - 1)) / (samples - 1));
			paths.emplace_back(QueuedPath(service, index));
		}
		return paths;
	}

	void LogSettings() {
		LOG_INFO("(ENUMERATION) Settings: installs/tick " << maxPerTick << " (boot " << bootMaxPerTick << "), loads/tick " << maxLoadsPerTick << ", budget "
			<< budgetMs << " ms (menus " << menuBudgetMs << ", boot " << bootBudgetMs
			<< "), target " << targetFps << " fps, rescan skip " << (rescanSkip ? "on" : "off")
			<< ", shader scan skip " << (skipShaderScan ? "on" : "off") << " (hooked " << (shaderScanHooked ? "yes" : "no") << "), early scan "
			<< (requestEarly ? "on" : "off") << " (max " << earlyMaxPackages << "), prefetch " << prefetchKb << " KB " << (prefetchForced ? "always" : "HDD only")
			<< ", CRT stream limit " << crtStreamLimit << ", open file ceiling " << openFileCeiling << ", register hook " << (registerHooked ? "yes" : "no")
			<< ", banner hook " << (bannerHooked ? "yes" : "no") << ", stock install hold " << (stockInstallHooked ? "yes" : "no") << ", exists check timed " << (existsHooked ? "yes" : "no")
			<< ", boot warm-up " << (bootWarmup ? "on" : "off") << std::endl);
	}

	// A rescan takes over progress only when it installs a changed or new package.
	bool progressDeferred = false;

	void PublishProgressStart() {
		progressDetected.store(queueAtStart, std::memory_order_release);
		progressProcessed.store(0, std::memory_order_release);
		progressRegistered.store(0, std::memory_order_release);
		progressNotRegistered.store(0, std::memory_order_release);
		progressTotalDlc.store(dlcAtStart, std::memory_order_release);
		progressNewDlc.store(0, std::memory_order_release);
		progressElapsedSeconds.store(0.0, std::memory_order_release);
		progressUpToDate.store(false, std::memory_order_release);
		progressCompleted.store(false, std::memory_order_release);
		progressActive.store(true, std::memory_order_release);
	}

	void EndProgressDeferral() {
		if (!progressDeferred)
			return;
		progressDeferred = false;
		PublishProgressStart();
		LOG_INFO("(ENUMERATION) Rescan found a new or changed package after popping " << unchangedSkipped << " unchanged ones, showing its progress" << std::endl);
	}

	void Start(unsigned char* service, long long now) {
		EnumerationPreparation::ResetIndexStats();
		EnumerationPreparation::ResetMapperRestore();
		scanCacheGeneration = EnumerationCache::BeginMapperRefsScan();
		draining = true;
		drainStartTicks = now;
		scanBudgetSumMs = 0.0;
		scanBudgetFrames = 0;
		lastProgressLogTicks = now;
		window = ProgressWindow{};
		queueAtStart = QueueLength(service);
		extraInstalls = 0;
		extraLoads = 0;
		drainTicks = 0;
		unregisteredInstalls = 0;
		droppedRequests = 0;
		shaderScansSkipped = 0;
		peakOpenFiles = 0;
		ceilingPauses = 0;
		queueEmptyTicks = 0;
		waitingAtQueueEmpty = 0;
		unchangedSkipped = 0;
		slowPrefetchWindows = 0;
		warmWaitIndex = ~0u;
		warmWaitFiles = 0;
		warmWaitTimeouts = 0;
		warmWaitMs = 0.0;
		stockHolds = 0;
		warmOrderMismatchLogged = false;
		scanTiming = TimingSums{};
		nextInstallEstimateMs = 0.0;
		for (TimingSums& sums : timingByActivity)
			sums = TimingSums{};
		timingPrefetched = TimingSums{};
		timingNotPrefetched = TimingSums{};
		std::fill(std::begin(installHistogram), std::end(installHistogram), 0u);
		slowest.clear();
		slowInstallLogs = 0;
		stockTickMs = 0.0;
		stockTicks = 0;
		loadStepMs = 0.0;
		// Hold native installation while worker preparation controls the queue.
		currentSnapshot.reserve(queueAtStart);
		CountPackages(packagesAtStart, dlcAtStart);
		CalibrateCycles();
		gameThreadId = GetCurrentThreadId();

		// Keep this current for the prefetch thread even while display updates are deferred.
		progressRemaining.store(queueAtStart, std::memory_order_release);
		progressDeferred = rescanSkip && lastSnapshotComplete;
		if (!progressDeferred)
			PublishProgressStart();

		LOG_INFO("(ENUMERATION) Scan " << scanCount + 1 << " start: queue=" << queueAtStart << " extra/tick=" << EffectiveMaxPerTick() << " loads/tick=" << maxLoadsPerTick
			<< " budget=" << EffectiveBudgetMs() << " ms, avg budget=n/a, reached fps=n/a, target fps=" << targetFps << ", open files=" << OpenCrtFiles() << ", waiting loads=" << WaitingLoads() << ", packages " << packagesAtStart
			<< " (dlc " << dlcAtStart << "), " << (rescanSkip && lastSnapshotComplete ? "rescan: " + std::to_string(knownEntries.size()) + " entries known from the last scan" : std::string("first scan this session"))
			<< ", " << (mainMenuSeen.load(std::memory_order_relaxed) ? "in menus" : "before the main menu") << ", " << EnumerationEnvironment::MemoryLine() << std::endl);
		if (scanCount == 0) {
			LogSettings();
			EnumerationEnvironment::LogAsync(SamplePaths(service, queueAtStart, 32));
		}

		// Skip worker preparation on snapshot rescans; unchanged entries need only a stat check.
		if (!(rescanSkip && lastSnapshotComplete))
			StartPrefetch(service);
		else
			LOG_INFO("(ENUMERATION) Prefetch skipped: rescan, unchanged entries are popped without being opened" << std::endl);
		HandOffBootWarmup();
		ResetWindow();
		AssetLoadDrain::SetActive(true);

		++scanCount;
	}

	// Log hints are heuristic diagnoses.
	void LogHints() {
		if (EnumerationPreparation::IsInstalled()) {
			LOG_INFO("(ENUMERATION) Hint: worker preparation replaces TOC/appid reads and MD5; scan-time ownership is answered locally without platform queries; VFS opens, tree allocation, census writes and registration remain serialized. Individual native calls can exceed the scheduling budget." << std::endl);
			return;
		}
		if (scanTiming.installs < 20) {
			LOG_INFO("(ENUMERATION) Hint: too few drained installs (" << scanTiming.installs << ") to judge the install pace" << std::endl);
			return;
		}
		const double avg = scanTiming.Avg(scanTiming.totalMs);
		const double blocked = scanTiming.BlockedShare();
		const double openShare = scanTiming.totalMs > 0.0 ? scanTiming.openReadMs / scanTiming.totalMs : 0.0;
		const double registerShare = scanTiming.totalMs > 0.0 ? scanTiming.registerMs / scanTiming.totalMs : 0.0;
		const double statShare = scanTiming.totalMs > 0.0 ? scanTiming.statMs / scanTiming.totalMs : 0.0;

		if (avg < 10.0)
			LOG_INFO("(ENUMERATION) Hint: installs were fast (avg " << avg << " ms). If the scan still felt slow, look at fps, budget stops and the in-menu budget in the progress lines." << std::endl);
		else if (openShare >= 0.6 && blocked >= 0.6 && scanTiming.openReadMs > 0.0 && scanTiming.existsMs / scanTiming.openReadMs >= 0.6)
			LOG_INFO("(ENUMERATION) Hint: installs were slow (avg " << avg << " ms) and " << static_cast<int>(scanTiming.existsMs / scanTiming.openReadMs * 100)
				<< "% of open+read was the existence check, which opens the file and reads nothing: that's a real-time scanner checking the file on its first open,"
				<< " not the disk reading it. The warm-up and boot warm-up move that open off the game thread; a scanner exclusion for the DLC folder removes it." << std::endl);
		else if (openShare >= 0.6 && blocked >= 0.6)
			LOG_INFO("(ENUMERATION) Hint: installs were slow (avg " << avg << " ms) and mostly WAITING in open+read (" << static_cast<int>(openShare * 100) << "% of install time, "
				<< static_cast<int>(blocked * 100) << "% blocked, not CPU): the disk or a real-time scanner is the bottleneck. See the Environment lines for the drive type and antivirus." << std::endl);
		else if (registerShare >= 0.5 && blocked >= 0 && blocked < 0.4)
			LOG_INFO("(ENUMERATION) Hint: installs were slow (avg " << avg << " ms) and mostly CPU in package registration (" << static_cast<int>(registerShare * 100)
				<< "%). Check the shader scan skip is hooked, and for unusually large packages in the slowest list." << std::endl);
		else if (statShare >= 0.3)
			LOG_INFO("(ENUMERATION) Hint: " << static_cast<int>(statShare * 100) << "% of install time was the rescan snapshot's file attribute lookup: slow file metadata (cold HDD or a scanner)." << std::endl);
		else
			LOG_INFO("(ENUMERATION) Hint: installs averaged " << avg << " ms with no single dominant cause; see the split above." << std::endl);

		const TimingSums& reading = timingByActivity[static_cast<int>(PrefetchActivity::Reading)];
		const TimingSums& waiting = timingByActivity[static_cast<int>(PrefetchActivity::Waiting)];
		const TimingSums& none = timingByActivity[static_cast<int>(PrefetchActivity::NotRunning)];
		const auto avgOf = [](const TimingSums& s) { return s.Avg(s.totalMs); };
		if (reading.installs >= 20 && (waiting.installs >= 20 || none.installs >= 20)) {
			const TimingSums& other = waiting.installs >= none.installs ? waiting : none;
			if (avgOf(reading) > 2.0 * avgOf(other) && avgOf(reading) > 10.0)
				LOG_INFO("(ENUMERATION) Hint: installs were " << avgOf(reading) / (std::max)(0.001, avgOf(other)) << "x slower while the prefetch was reading ("
					<< avgOf(reading) << " vs " << avgOf(other) << " ms): the prefetch is competing with the game for the disk. Try FastEnumerationPrefetchKB=0." << std::endl);
			else
				LOG_INFO("(ENUMERATION) Hint: installs weren't slower while the prefetch was reading (" << avgOf(reading) << " vs " << avgOf(other) << " ms): it isn't getting in the game's way." << std::endl);
		}
		if (timingPrefetched.installs >= 20 && timingNotPrefetched.installs >= 20) {
			const double pre = timingPrefetched.Avg(timingPrefetched.openReadMs);
			const double cold = timingNotPrefetched.Avg(timingNotPrefetched.openReadMs);
			if (pre < 0.5 * cold)
				LOG_INFO("(ENUMERATION) Hint: prefetched files opened+read in " << pre << " ms against " << cold << " ms for the rest: the prefetch warms the files." << std::endl);
			else
				LOG_INFO("(ENUMERATION) Hint: prefetched files were no faster to open+read (" << pre << " vs " << cold << " ms). The time isn't in reading the file's head:"
					<< " likely a real-time scanner on open, file metadata, or the cache being evicted." << std::endl);
		}
		else if (timingNotPrefetched.installs >= 20 && prefetchKb > 0 && timingPrefetched.installs < 20) {
			LOG_INFO("(ENUMERATION) Hint: the prefetch got ahead of the game for only " << timingPrefetched.installs << " installs; it was too slow, stopped, or skipped." << std::endl);
		}
	}

	void LogScanSummary(double elapsedMs) {
		EnumerationPreparation::LogMapperRestore();
		if (scanTiming.installs == 0)
			return;
		LOG_INFO("(ENUMERATION) Install time over " << scanTiming.installs << " drained installs: " << scanTiming.Describe()
			<< "; " << scanTiming.totalMs / 1000.0 << " s of the scan's " << elapsedMs / 1000.0 << " s" << std::endl);

		std::ostringstream histogram;
		for (size_t i = 0; i < histogramBuckets; ++i) {
			if (i)
				histogram << ", ";
			if (i == 0)
				histogram << "<" << histogramEdgesMs[0];
			else if (i == histogramBuckets - 1)
				histogram << ">=" << histogramEdgesMs[i - 1];
			else
				histogram << histogramEdgesMs[i - 1] << "-" << histogramEdgesMs[i];
			histogram << " ms: " << installHistogram[i];
		}
		LOG_INFO("(ENUMERATION) Install time histogram: " << histogram.str() << std::endl);

		for (int i = 0; i < 3; ++i)
			if (timingByActivity[i].installs)
				LOG_INFO("(ENUMERATION) Install pace " << prefetchActivityNames[i] << ": " << timingByActivity[i].installs << " installs, " << timingByActivity[i].Describe() << std::endl);
		if (timingPrefetched.installs)
			LOG_INFO("(ENUMERATION) Install pace for files the prefetch had already read: " << timingPrefetched.installs << " installs, " << timingPrefetched.Describe() << std::endl);
		if (timingNotPrefetched.installs)
			LOG_INFO("(ENUMERATION) Install pace for files it hadn't: " << timingNotPrefetched.installs << " installs, " << timingNotPrefetched.Describe() << std::endl);
		LOG_INFO("(ENUMERATION) Game thread during the scan: " << stockTicks << " stock ticks, " << stockTickMs << " ms in them (" << (stockTicks ? stockTickMs / stockTicks : 0.0)
			<< " ms/tick, each installs one package itself), " << extraLoads << " extra load steps, " << loadStepMs << " ms in them" << std::endl);
		if (warmWaitFiles)
			LOG_INFO("(ENUMERATION) Waited for the warm-up on " << warmWaitFiles << " files, " << warmWaitMs << " ms in all (" << warmWaitMs / warmWaitFiles
				<< " ms/file), " << warmWaitTimeouts << " given up on after " << maxWarmWaitMs << " ms, " << stockHolds << " stock installs held" << std::endl);
		for (size_t i = 0; i < slowest.size(); ++i)
			LOG_INFO("(ENUMERATION) Slowest install " << i + 1 << ": " << DescribeInstall(slowest[i]) << std::endl);
		if (scanTiming.slow > slowInstallLogs)
			LOG_INFO("(ENUMERATION) " << scanTiming.slow << " installs took >= " << slowInstallMs << " ms, " << slowInstallLogs << " of them logged individually" << std::endl);
		LogHints();
	}

	void PublishProgressEnd(unsigned int registered, unsigned int dlc, unsigned int newDlc, bool nothingNew, bool silentScan);
	void LogScanDone(unsigned int total, unsigned int dlc, unsigned int newDlc);

	void Finish() {
		draining = false;
		EnumerationPreparation::LogIndexStats();
		LazyVfs::Report("scan done");
		EndWarmWait(Now());
		StopPrefetch();
		EnumerationCache::MapperRefsScanEnded(scanCacheGeneration);
		AssetLoadDrain::SetActive(false);
		if (rescanSkip) {
			std::sort(currentSnapshot.begin(), currentSnapshot.end());
			knownEntries = std::move(currentSnapshot);
			currentSnapshot.clear();
			lastSnapshotComplete = true;
		}

		unsigned int total = 0;
		unsigned int dlc = 0;
		CountPackages(total, dlc);
		const unsigned int registered = total >= packagesAtStart ? total - packagesAtStart : 0;
		const unsigned int newDlc = dlc >= dlcAtStart ? dlc - dlcAtStart : 0;

		// Keep empty boot rescans silent; show UP TO DATE for later requests.
		const bool nothingNew = registered == 0 && unchangedSkipped > 0;
		const bool silentScan = nothingNew && !mainMenuSeen.load(std::memory_order_relaxed);

		progressRemaining.store(0, std::memory_order_release);
		if (progressDeferred && silentScan) {
			progressDeferred = false;
			LOG_INFO("(ENUMERATION) Silent rescan, nothing new: the progress display was left as it was" << std::endl);
		}
		else {
			EndProgressDeferral();
			PublishProgressEnd(registered, dlc, newDlc, nothingNew, silentScan);
		}
		LogScanDone(total, dlc, newDlc);
	}

	void PublishProgressEnd(unsigned int registered, unsigned int dlc, unsigned int newDlc, bool nothingNew, bool silentScan) {
		progressProcessed.store(queueAtStart, std::memory_order_release);
		progressRegistered.store(registered, std::memory_order_release);
		progressTotalDlc.store(dlc, std::memory_order_release);
		progressNewDlc.store(newDlc, std::memory_order_release);
		// Queued paths beyond the registered count include duplicates, unowned and unreadable packages.
		const unsigned int accepted = registered + unchangedSkipped;
		progressNotRegistered.store(queueAtStart > accepted ? queueAtStart - accepted : 0, std::memory_order_release);
		progressElapsedSeconds.store(MsSince(drainStartTicks) / 1000.0, std::memory_order_release);
		progressUpToDate.store(nothingNew, std::memory_order_release);
		progressActive.store(!silentScan, std::memory_order_release);
		progressCompleted.store(true, std::memory_order_release);
		completionStartTicks.store(Now(), std::memory_order_release);
	}

	void LogScanDone(unsigned int total, unsigned int dlc, unsigned int newDlc) {
		LOG_INFO("(ENUMERATION) Scan process clock: " << ProcessSeconds() << " s after launch" << std::endl);
		const double elapsed = MsSince(drainStartTicks);
		const double installPhase = queueEmptyTicks ? (queueEmptyTicks - drainStartTicks) / ticksPerMs : elapsed;
		LOG_INFO("(ENUMERATION) Scan done: " << queueAtStart << " queued, " << extraInstalls << " installed by the drain over " << drainTicks << " ticks, "
			<< elapsed << " ms (installs " << installPhase << " ms, then " << waitingAtQueueEmpty << " loads still waiting, " << extraLoads << " extra load ticks), "
			<< "packages total=" << total << " dlc=" << dlc << " (new " << newDlc << "), drained installs that did not register=" << unregisteredInstalls
			<< ", open files now=" << OpenCrtFiles() << " peak=" << peakOpenFiles << " ceiling pauses=" << ceilingPauses
			<< ", unchanged entries popped unopened=" << unchangedSkipped << ", shader scans skipped=" << shaderScansSkipped
			<< ", avg budget=" << (scanBudgetFrames ? scanBudgetSumMs / scanBudgetFrames : 0.0)
			<< " ms, reached fps=" << (elapsed > 0.0 ? 1000.0 * drainTicks / elapsed : 0.0)
			<< ", target fps=" << targetFps << ", budget restored=" << EffectiveBudgetMs() << " ms, requests held back=" << droppedRequests << ", " << EnumerationEnvironment::MemoryLine() << std::endl);
		EnumerationOwnership::Log();
		LogScanSummary(elapsed);
	}

	void LogProgress(const unsigned char* service, unsigned int remaining, double budget, double windowMs) {
		const double seconds = windowMs / 1000.0;
		const unsigned int consumed = queueAtStart >= remaining ? queueAtStart - remaining : 0;
		LOG_INFO("(ENUMERATION) Progress: installed " << consumed << "/" << queueAtStart
			<< ", waiting loads " << WaitingLoads() + (IsLoadSlotBusy(service) ? 1 : 0) << ", open files " << OpenCrtFiles()
			<< ", " << MsSince(drainStartTicks) / 1000.0 << " s | last " << seconds << " s: " << window.ticks / seconds << " fps (longest frame "
			<< window.maxFrameMs << " ms), drain " << window.installs / seconds << " installs/s, " << (window.ticks ? window.drainMs / window.ticks : 0.0)
			<< " ms/tick of " << budget << " ms budget, " << (window.installs ? window.installMs / window.installs : 0.0) << " ms/install, "
			<< window.budgetStops << " budget stops, " << window.ceilingStops << " ceiling stops, " << window.warmStops << " warm-up waits, "
			<< window.stockHolds << " stock installs held, " << PrefetchStatus(consumed)
			<< (mainMenuSeen.load(std::memory_order_relaxed) ? ", in menus" : "") << std::endl);

		std::ostringstream detail;
		if (window.timing.installs)
			detail << "installs " << window.timing.Describe();
		else
			detail << "no drained installs";
		detail << "; loads " << window.loads << " in " << window.loadMs << " ms; stock tick " << (window.ticks ? window.stockTickMs / window.ticks : 0.0) << " ms/tick";
		if (PrefetchJob* job = prefetchJob) {
			const unsigned int files = job->files.load(std::memory_order_acquire) - window.prefetchFiles;
			const double ioMs = (job->ioTicks.load(std::memory_order_acquire) - window.prefetchIoTicks) / ticksPerMs;
			const double openMs = (job->openTicks.load(std::memory_order_acquire) - window.prefetchOpenTicks) / ticksPerMs;
			detail << "; prefetch warmed " << files << " files, " << ioMs << " ms disk summed over the threads, of it " << openMs << " ms opening"
				<< (files ? " (" + std::to_string(ioMs / files) + " ms/file)" : std::string());
		}
		detail << "; " << EnumerationEnvironment::MemoryLine();
		LOG_INFO("(ENUMERATION) Detail: " << detail.str() << std::endl);
	}

	void Drain(unsigned char* service) {
		const bool installing = service[off_installing] && QueueLength(service) > 0;
		const bool loading = WaitingLoads() > 0 || IsLoadSlotBusy(service);
		if (draining && !installing && !loading) {
			Finish();
			return;
		}
		if (!draining && !installing)
			return;

		const long long start = frameStartTicks ? frameStartTicks : Now();
		const long long workStart = Now();
		if (!draining)
			Start(service, Now());
		++drainTicks;
		++window.ticks;
		if (window.lastTickStart)
			window.maxFrameMs = (std::max)(window.maxFrameMs, (start - window.lastTickStart) / ticksPerMs);
		window.lastTickStart = start;
		const double budget = EffectiveBudgetMs();
		scanBudgetSumMs += budget;
		++scanBudgetFrames;

		// Advance loads before installs to release streams within the shared budget.
		unsigned int loads = 0;
		const long long loadStart = Now();
		while (loads < maxLoadsPerTick && (WaitingLoads() > 0 || IsLoadSlotBusy(service))) {
			if (MsSince(start) >= budget * 0.5)
				break;
			CallLoadTick(service);
			++loads;
		}
		extraLoads += loads;
		window.loads += loads;
		const double loadMs = MsSince(loadStart);
		window.loadMs += loadMs;
		loadStepMs += loadMs;

		unsigned int done = 0;
		// Before menu readiness, drain before preloader commands are dispatched between frames.
		const bool passAll = !mainMenuSeen.load(std::memory_order_relaxed);
		const unsigned int perTick = passAll ? 0xFFFFFFFFu : EffectiveMaxPerTick();
		while (done < perTick && service[off_installing] && QueueLength(service) > 0) {
			if (!passAll && (MsSince(start) >= budget
				|| (done && MsSince(start) + nextInstallEstimateMs > budget))) {
				++window.budgetStops;
				break;
			}
			if (pioinfo && OpenCrtFiles() >= openFileCeiling) {
				++ceilingPauses;
				++window.ceilingStops;
				break;
			}
			if (!IsNextWarm(service)) {
				++window.warmStops;
				break;
			}
			const unsigned int index = NextInstallIndex(service);
			if (SkipIfKnown(service)) {
				if (prefetchJob && index < prefetchJob->prepared.size())
					prefetchJob->prepared[index].reset();
				continue;
			}
			EndProgressDeferral();
			const long long installStart = Now();
			InstallNext(service, true);
			const double installMs = MsSince(installStart);
			nextInstallEstimateMs = (std::max)(installMs, nextInstallEstimateMs * 0.9);
			window.installMs += installMs;
			++done;
		}
		extraInstalls += done;
		window.installs += done;
		if (queueEmptyTicks == 0 && QueueLength(service) == 0) {
			queueEmptyTicks = Now();
			waitingAtQueueEmpty = WaitingLoads();
			LOG_INFO("(ENUMERATION) Queue empty after " << (queueEmptyTicks - drainStartTicks) / ticksPerMs << " ms, " << waitingAtQueueEmpty << " loads still waiting" << std::endl);
		}

		const unsigned int remaining = QueueLength(service);
		progressRemaining.store(remaining, std::memory_order_release);
		if (!progressDeferred) {
			progressProcessed.store(queueAtStart >= remaining ? queueAtStart - remaining : 0, std::memory_order_release);
			progressElapsedSeconds.store(MsSince(drainStartTicks) / 1000.0, std::memory_order_release);
			unsigned int liveTotal = 0;
			unsigned int liveDlc = 0;
			CountPackages(liveTotal, liveDlc);
			progressTotalDlc.store(liveDlc, std::memory_order_release);
			const unsigned int registered = liveTotal >= packagesAtStart ? liveTotal - packagesAtStart : 0;
			const unsigned int consumed = queueAtStart >= remaining ? queueAtStart - remaining : 0;
			progressRegistered.store(registered, std::memory_order_release);
			progressNewDlc.store(liveDlc >= dlcAtStart ? liveDlc - dlcAtStart : 0, std::memory_order_release);
			progressNotRegistered.store(consumed > registered + unchangedSkipped
				? consumed - registered - unchangedSkipped : 0, std::memory_order_release);
		}

		window.drainMs += MsSince(workStart);
		const double windowMs = MsSince(lastProgressLogTicks);
		if (windowMs >= 1000.0) {
			lastProgressLogTicks = Now();
			LogProgress(service, remaining, budget, windowMs);
			BackOffPrefetchIfSlow(window.warmInstalls, window.warmInstallMs);
			ResetWindow();
		}

		if (remaining == 0 && WaitingLoads() == 0 && !IsLoadSlotBusy(service))
			Finish();
	}

	void DrainFailed() {
		AssetLoadDrain::SetBootActive(false);
		maxPerTick = 0;
		maxLoadsPerTick = 0;
		EnumerationPreparation::End();
		timingInstall = false;
		StopPrefetch();
		AssetLoadDrain::SetActive(false);
		progressActive.store(false, std::memory_order_release);
		LOG_ERROR("(ENUMERATION) Exception while draining, the drain is off for this session" << std::endl);
	}


	// Size-limit early scan requests to avoid holding the boot flow.
	bool earlyRequested = false;
	bool earlyCompleted = false;

	unsigned int CountDlcPsarcs(const std::wstring& folder, unsigned int limit) {
		unsigned int count = 0;
		WIN32_FIND_DATAW data{};
		HANDLE find = FindFirstFileW((folder + L"\\*").c_str(), &data);
		if (find == INVALID_HANDLE_VALUE)
			return 0;
		do {
			if (count > limit)
				break;
			if (data.cFileName[0] == L'.' && (data.cFileName[1] == 0 || (data.cFileName[1] == L'.' && data.cFileName[2] == 0)))
				continue;
			if (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
				count += CountDlcPsarcs(folder + L"\\" + data.cFileName, limit - count);
			}
			else {
				const size_t length = wcslen(data.cFileName);
				if (length > 6 && _wcsicmp(data.cFileName + length - 6, L".psarc") == 0)
					++count;
			}
		} while (FindNextFileW(find, &data));
		FindClose(find);
		return count;
	}

	bool IsEarlyScanAffordable() {
		wchar_t exe[MAX_PATH]{};
		GetModuleFileNameW(nullptr, exe, MAX_PATH);
		std::wstring folder(exe);
		const size_t slash = folder.find_last_of(L"\\/");
		folder = (slash == std::wstring::npos ? L"" : folder.substr(0, slash + 1)) + L"dlc";
		const unsigned int count = CountDlcPsarcs(folder, earlyMaxPackages + 1);
		const bool affordable = count <= earlyMaxPackages;
		LOG_INFO("(ENUMERATION) dlc folder holds " << (affordable ? "" : "more than ") << (affordable ? count : earlyMaxPackages)
			<< " psarcs, early scan " << (affordable ? "on" : "off") << std::endl);
		return affordable;
	}

	void RequestEarly(unsigned char* service) {
		if (!requestEarly || earlyRequested)
			return;
		if (!service[off_firstTick] || !service[off_enabled] || service[off_request])
			return;
		earlyRequested = true;
		if (!IsEarlyScanAffordable())
			return;
		service[off_request] = 1;
		service[off_enabled] = 1;
		LOG_INFO("(ENUMERATION) Scan requested early, at the DLC service's first tick" << std::endl);
	}

	// Defer mid-scan requests until both installation and loading finish.
	bool deferredRequest = false;

	void DeferRequestDuringScan(unsigned char* service) {
		if (!draining || !service[off_request])
			return;
		service[off_request] = 0;
		deferredRequest = true;
		if (++droppedRequests == 1)
			LOG_INFO("(ENUMERATION) Scan request while a scan runs, held until it's done" << std::endl);
	}

	void RaiseDeferredRequest(unsigned char* service) {
		if (draining || !deferredRequest)
			return;
		deferredRequest = false;
		service[off_request] = 1;
		service[off_enabled] = 1;
	}

	// Suppress redundant sign-in requests after an early scan; explicit rescan requests still run.
	std::atomic<bool> manualRequest = false;
	unsigned int signInRequestsDropped = 0;

	void DropSignInRequest(unsigned char* service) {
		if (!service[off_request])
			return;
		if (manualRequest.exchange(false))
			return;
		if (!requestEarly || !earlyCompleted || draining || !onSignInScreen.load(std::memory_order_relaxed))
			return;
		service[off_request] = 0;
		service[off_enabled] = 1;
		if (++signInRequestsDropped <= 3)
			LOG_INFO("(ENUMERATION) Game's sign-in scan request dropped, the early scan already covered it" << std::endl);
	}

	// Skip the native install call while waiting for warmth; clearing installing would trigger scan completion.
	// ABI: service in ESI, no stack arguments, unused result.

	unsigned char holdStockNow = 0;

	extern "C" void __cdecl EnumDecideStockInstall(unsigned char* service) {
		// Charge the stock prepared install to the same frame budget as drained installs.
		holdStockNow = (service && ((EnumerationPreparation::IsInstalled() && maxPerTick
			&& service[off_installing] && QueueLength(service)) || !IsNextWarm(service))) ? 1 : 0;
		if (holdStockNow) {
			++stockHolds;
			++window.stockHolds;
		}
	}

	void __declspec(naked) StockInstallThunk() {
		__asm {
			pushfd
			pushad
			push esi
			call EnumDecideStockInstall
			add esp, 4
			popad
			popfd
			cmp byte ptr [holdStockNow], 0
			jne skip
			jmp dword ptr [installNextFunc]
		skip:
			ret
		}
	}


	// Track stock installs before draining begins; the tick can install before handling a scan request.
	void RecordStockInstall(const std::string& line) {
		if (lastInstallRegistered && rescanSkip)
			currentSnapshot.push_back(line);
	}

	std::string PeekStockInstall(unsigned char* service) {
		lastInstallRegistered = false;
		inInstallRegister = 0;
		if ((EnumerationPreparation::IsInstalled() && maxPerTick)
			|| !rescanSkip || !service[off_installing] || QueueLength(service) == 0)
			return {};
		std::string line = SnapshotLine(NextQueuedPath(service));
		// Remember known entries even when native registration rejects them as duplicates.
		if (IsKnown(line)) {
			currentSnapshot.push_back(std::move(line));
			return {};
		}
		return line;
	}

	void BeforeOriginalTick(unsigned char* service) {
		if (requestEarly)
			RequestEarly(service);
		RaiseDeferredRequest(service);
		DeferRequestDuringScan(service);
		DropSignInRequest(service);
	}

	void AfterDrain() {
		if (requestEarly && !draining && progressCompleted.load(std::memory_order_relaxed))
			earlyCompleted = true;
	}

	bool TryDrain(unsigned char* service) {
		__try {
			Drain(service);
			return true;
		}
		__except (EXCEPTION_EXECUTE_HANDLER) {
			return false;
		}
	}

	void __fastcall Hook_DlcServiceTick(void* self, void* edx) {
		static bool menuStampLogged = false;
		if (!menuStampLogged && mainMenuReady.load(std::memory_order_relaxed)) {
			menuStampLogged = true;
			menuReadySeconds.store(ProcessSeconds(), std::memory_order_release);
			LOG_INFO("(BOOT) summary: main menu ready " << menuReadySeconds.load()
				<< " s after launch; source=first DLC tick after transition completion" << std::endl);
			// Transition completion catches title readiness even if menu polls miss it.
			GameState::UnlockRealMenu();
			// Stop cached folder answers here so later DLC additions are visible.
			LazyVfs::Report("main menu ready");
		}
		AssetLoadDrain::SetBootActive(maxPerTick != 0 && scanCount != 0 && !mainMenuReady.load(std::memory_order_relaxed));
		auto* service = static_cast<unsigned char*>(self);
		if (!service) {
			originalTick(self, edx);
			return;
		}

		BeforeOriginalTick(service);
		frameStartTicks = Now();
		// Subtract drain time from the prior frame before calculating headroom.
		if (previousFrameTicks)
			frameWithoutEnumerationMs = (std::max)(0.0,
				(frameStartTicks - previousFrameTicks) / ticksPerMs - previousEnumerationMs);
		previousFrameTicks = frameStartTicks;
		const std::string stockLine = PeekStockInstall(service);
		const long long tickStart = Now();
		originalTick(self, edx);
		if (draining) {
			const double tickMs = MsSince(tickStart);
			window.stockTickMs += tickMs;
			stockTickMs += tickMs;
			++stockTicks;
		}
		if (!stockLine.empty())
			RecordStockInstall(stockLine);

		if (!TryDrain(service))
			DrainFailed();
		AfterDrain();
		previousEnumerationMs = MsSince(frameStartTicks);
	}

	bool ResolveAddresses() {
		tickSlot = Offsets::ptr_dlcServiceTickSlot;
		tickFunc = Offsets::func_dlcServiceTick;
		loadTickFunc = Offsets::func_dlcServiceLoadTick;
		installNextFunc = Offsets::func_dlcScanInstallNext;
		removeLastFunc = Offsets::func_dlcScanQueueRemoveLast;
		packagesPtr = Offsets::ptr_packageList;
		waitingPtr = Offsets::ptr_packageLoadQueue;
		registerCallSite = Offsets::ptr_dlcInstallRegisterCall;
		registerFunc = Offsets::func_registerPackage;
		shaderScanCallSite = Offsets::ptr_packageShaderScanCall;
		shaderScanFunc = Offsets::func_packageShaderScan;
		bannerCallSite = Offsets::ptr_dlcBannerCall;
		bannerFunc = Offsets::func_showBanner;
		stringDestroyFunc = Offsets::enumBannerStringDestroy;
		return tickSlot && tickFunc && loadTickFunc && installNextFunc && removeLastFunc && packagesPtr && waitingPtr
			&& registerCallSite && registerFunc && shaderScanCallSite && shaderScanFunc && bannerCallSite && bannerFunc;
	}

	uintptr_t ReadSlot() {
		if (MemUtil::IsBadReadPtr(reinterpret_cast<void*>(tickSlot)))
			return 0;
		return *reinterpret_cast<const uintptr_t*>(tickSlot);
	}

	void ReadTuning() {
		maxPerTick = 16;
		maxLoadsPerTick = 32;
		targetFps = 60;
		budgetMs = 2;
		menuBudgetMs = 2;
		bootMaxPerTick = 32;
		bootBudgetMs = 30;
		rescanSkip = true;
		skipShaderScan = true;
		// Sign-in scans must not block the title screen.
		requestEarly = false;
		earlyMaxPackages = 1500;
		prefetchKb = 128;
		prefetchForced = true;
		bootWarmup = true;
		crtStreamLimit = crtStreamLimitMax;
		openFileCeiling = 1792;
	}
}

void EnumerationDrain::Install() {
	if (!UltrafastLoad::Enabled())
		return;
	if (installed)
		return;

	if (!ResolveAddresses()) {
		LOG_WARNING("(ENUMERATION) Fast enumeration isn't supported on this game version" << std::endl);
		return;
	}

	ReadTuning();

	// A menu name alone does not confirm readiness; failures retain the native budget.
	const unsigned char readyBytes[] = { 0x55, 0x8B, 0xEC, 0x83, 0xE4, 0xC0 };
	const uintptr_t ready = Offsets::enumMenuReady;
	if (!ready || !MemUtil::MatchesBytes(ready, readyBytes) || ReadSlot() != tickFunc) {
		LOG_WARNING("(ENUMERATION) Menu-ready/tick bytes mismatch; no enumeration patches applied" << std::endl);
		return;
	}
	{
		LOG_INFO("(ENUMERATION) Menu-ready original: 55 8b ec 83 e4 c0" << std::endl);
		menuReadyBody = reinterpret_cast<uintptr_t>(MemUtil::TrampHook(
			reinterpret_cast<PBYTE>(ready), reinterpret_cast<PBYTE>(&MenuReadyHook), sizeof(readyBytes)));
	}
	if (!menuReadyBody) {
		LOG_WARNING("(ENUMERATION) Menu-ready hook failed; no further enumeration patches applied" << std::endl);
		return;
	}

	LARGE_INTEGER frequency{};
	QueryPerformanceFrequency(&frequency);
	ticksPerMs = frequency.QuadPart / 1000.0;
	// Calibrate TSC rate at scan start to convert thread cycles into CPU time.
	calibrationQpc = Now();
	calibrationTsc = __rdtsc();

	// Raise the stream cap before batching installs to prevent dropped packages.
	if (!RaiseCrtStreamLimit())
		maxPerTick = 0;
	if (ResolveIoinfo())
		LOG_INFO("(ENUMERATION) Open file guard: ceiling " << openFileCeiling << ", ioinfo stride 0x" << std::hex << ioinfoStride << std::dec
			<< ", open now " << OpenCrtFiles() << std::endl);

	const uintptr_t current = ReadSlot();
	if (current != tickFunc) {
		LOG_WARNING("(ENUMERATION) Fast enumeration not installed: the DLC service's tick slot holds 0x" << std::hex << current
			<< ", expected 0x" << tickFunc << std::dec << std::endl);
		return;
	}

	// Install registration scope first; shader skipping and rescan snapshots depend on it.
	registerHooked = RedirectCall(registerCallSite, registerFunc, &RegisterEnterThunk, registerOriginalRel, "Package install register");
	if (!registerHooked) {
		skipShaderScan = false;
		rescanSkip = false;
	}

	originalTick = reinterpret_cast<TickFn>(current);
	const uintptr_t hook = reinterpret_cast<uintptr_t>(&Hook_DlcServiceTick);
	if (!MemUtil::PatchAdr(reinterpret_cast<LPVOID>(tickSlot), &hook, sizeof(hook))) {
		LOG_ERROR("(ENUMERATION) Fast enumeration not installed: couldn't write the tick slot" << std::endl);
		return;
	}
	installed = true;

	if (skipShaderScan)
		shaderScanHooked = RedirectCall(shaderScanCallSite, shaderScanFunc, &ShaderScanThunk, shaderScanOriginalRel, "Shader cache scan");
	// Banner suppression requires the engine destructor for its string argument.
	if (stringDestroyFunc)
		bannerHooked = RedirectCall(bannerCallSite, bannerFunc, &BannerThunk, bannerOriginalRel, "Enumeration banner");
	else
		LOG_INFO("(ENUMERATION) Native enumeration banner kept: its string destructor is unmapped on this game version" << std::endl);
	// Tick call order: load step, then install step with the service in ESI.
	stockInstallHooked = RedirectCall(tickFunc + stockInstallCallOffset, installNextFunc, &StockInstallThunk, stockInstallOriginalRel, "Tick install step");
	if (stockInstallHooked && registerHooked && maxPerTick)
		EnumerationPreparation::Install();
	existsHooked = HookExistsCheck();
	StartBootWarmup();

	LOG_INFO("(ENUMERATION) Fast enumeration installed: extra installs/tick=" << maxPerTick << ", budget=" << budgetMs << " ms (" << menuBudgetMs
		<< " ms in menus), target=" << targetFps << " fps, shader scan skip " << (shaderScanHooked ? "on" : "off") << ", rescan skip " << (rescanSkip ? "on" : "off")
		<< ", prefetch " << prefetchKb << " KB" << (requestEarly ? ", early scan on" : "") << std::endl);
}

bool EnumerationDrain::IsInstalled() {
	return installed;
}

EnumerationDrain::Progress EnumerationDrain::GetProgress() {
	Progress progress;
	progress.active = progressActive.load(std::memory_order_acquire);
	progress.completed = progressCompleted.load(std::memory_order_acquire);
	const long long finished = completionStartTicks.load(std::memory_order_acquire);
	if (progress.completed && finished != 0) {
		progress.completionElapsedSeconds = MsSince(finished) / 1000.0;
		if (progress.completionElapsedSeconds >= 2.0) {
			progressActive.store(false, std::memory_order_release);
			progress.active = false;
		}
	}
	progress.upToDate = progressUpToDate.load(std::memory_order_acquire);
	progress.detected = progressDetected.load(std::memory_order_acquire);
	progress.processed = progressProcessed.load(std::memory_order_acquire);
	progress.remaining = progressRemaining.load(std::memory_order_acquire);
	progress.registered = progressRegistered.load(std::memory_order_acquire);
	progress.notRegistered = progressNotRegistered.load(std::memory_order_acquire);
	progress.newDlc = progressNewDlc.load(std::memory_order_acquire);
	progress.totalDlc = progressTotalDlc.load(std::memory_order_acquire);
	progress.elapsedSeconds = progressElapsedSeconds.load(std::memory_order_acquire);
	return progress;
}

void EnumerationDrain::NoteManualRequest() {
	manualRequest.store(true);
}

void EnumerationDrain::NoteMenu(const std::string& menu) {
	if (menu == "MainMenu") mainMenuSeen.store(true, std::memory_order_relaxed);
	onSignInScreen.store(menu == "pre_enter_prompt" || menu == "TitleScreen" || menu == "SelectionListDialog" ||
		menu == "ProfileSelect" || menu == "SimpleDialog" || menu == "UplayLoginDialog", std::memory_order_relaxed);
}

double EnumerationDrain::MainMenuReadySeconds() { return menuReadySeconds.load(std::memory_order_acquire); }
