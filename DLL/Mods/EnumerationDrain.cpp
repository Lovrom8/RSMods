#include "../stdafx.h"
#include "EnumerationDrain.hpp"
#include "AssetLoadDrain.hpp"
#include "EnumerationEnvironment.hpp"
#include "../SamplingProfiler.hpp"
#include <algorithm>
#include <atomic>
#include <intrin.h>
#include <memory>
#include <string_view>

namespace {
	// The DLC service object, as its tick and install step use it.
	//   +0x04 request     set to ask for a scan. The tick runs the scan setup while request and enabled are set, and clears it if the setup started a scan.
	//   +0x05 enabled
	//   +0x06 installing  The install step installs while this is set, and clears it once the queue is empty.
	//   +0xD0 load slot   The package the load step is loading, 0 when free.
	//   +0xDC first tick  Set by the first tick.
	//   +0xE0/+0xE4       Scan queue: begin/end of a vector of 24 byte strings. The install step installs from the back.
	constexpr unsigned int off_request = 0x04;
	constexpr unsigned int off_enabled = 0x05;
	constexpr unsigned int off_installing = 0x06;
	constexpr unsigned int off_loadSlot = 0xD0;
	constexpr unsigned int off_firstTick = 0xDC;
	constexpr unsigned int off_queueBegin = 0xE0;
	constexpr unsigned int off_queueEnd = 0xE4;
	constexpr unsigned int queueEntrySize = 0x18;

	// A package whose byte at +0x38 is zero is DLC (that's how the game counts its DLC).
	constexpr unsigned int off_packageDlcFlag = 0x38;

	// Resolved from Offsets in Install(). The naked thunks below jump through these.
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

	// Tuning, from RSMods.ini [Fast Enumeration] in Install() (ReadTuning). These are the defaults.
	unsigned int maxPerTick = 16;
	unsigned int maxLoadsPerTick = 32;
	double budgetMs = 24.0;
	// Once the main menu is up, the game streams the loft and album art on this thread, with whatever time is left after us.
	// A 24 ms drain in 33 ms frames left a 3796 package library's menu with placeholder cubes and no art for 30 s.
	double menuBudgetMs = 6.0;
	bool requestEarly = false;
	double bootBudgetMs = 30.0;
	unsigned int bootMaxPerTick = 32;
	unsigned int earlyMaxPackages = 1500;
	bool rescanSkip = true;
	bool skipShaderScan = true;

	// Menu state, published from the mod thread (NoteMenu).
	std::atomic<bool> mainMenuSeen = false;
	std::atomic<bool> onSignInScreen = false;

	double EffectiveBudgetMs() {
		if (mainMenuSeen.load(std::memory_order_relaxed))
			return menuBudgetMs;
		return requestEarly ? bootBudgetMs : budgetMs;
	}

	unsigned int EffectiveMaxPerTick() {
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

	// ---- Game data ----

	unsigned int QueueLength(const unsigned char* service) {
		const auto begin = *reinterpret_cast<const uintptr_t*>(service + off_queueBegin);
		const auto end = *reinterpret_cast<const uintptr_t*>(service + off_queueEnd);
		return end > begin ? static_cast<unsigned int>((end - begin) / queueEntrySize) : 0;
	}

	// The game's string layout (see ProfileSaveStreaming's EngineString): data at +0, finish at +0x10, end of storage at +0x14.
	// A short string keeps its characters inline from +0, and then its end of storage points at +0x10.
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

	// The install step installs the last entry first.
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

	// ---- Calls into the game ----

	// The install step: service in ESI, no stack args. Saves EBX/EDI itself and never writes ESI.
	void __declspec(naked) __cdecl CallInstallNext(void* /*service*/) {
		__asm {
			push esi
			mov esi, [esp + 8]
			call dword ptr [installNextFunc]
			pop esi
			ret
		}
	}

	// The scan queue's pop_back: vector in EAX, destroys the last string and moves the end back. Clobbers EAX/ECX/EDX only.
	// It doesn't check for an empty vector, so callers must.
	void __declspec(naked) __cdecl CallRemoveLast(void* /*vector*/) {
		__asm {
			mov eax, [esp + 4]
			call dword ptr [removeLastFunc]
			ret
		}
	}

	// The load step (thiscall), the part of the service's tick shared by every platform. Per call: sorts the waiting list, moves one package into the free load slot and
	// starts its load, frees the slot of a finished load, and tidies the priority set. The rest of it only runs once.
	void CallLoadTick(void* service) {
		reinterpret_cast<TickFn>(loadTickFunc)(service, nullptr);
	}

	// ---- Package install -> package registration ----
	// The CALL is redirected through a thunk that marks "a DLC scan install is registering" while it runs, so the shader scan
	// skip only applies to those (registration is also reached from the game's own package mounts and song preview loads),
	// and that records whether the package list grew (the install registered).
	// The package install is only called from the install step, on the game thread, so this never nests.

	volatile LONG inInstallRegister = 0;
	uintptr_t registerSavedReturn = 0;
	unsigned int packagesBeforeRegister = 0;
	bool lastInstallRegistered = false;
	bool registerHooked = false;
	int32_t registerOriginalRel = 0;

	// When the last install entered and left registration, for the per-install time split (0 = didn't get there, e.g. the
	// package was refused before registering). Everything before registration is the open and the header/TOC/appid reads.
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

	// Register args (EAX, CL) and the three stack args pass through untouched. The return address is swapped for
	// RegisterReturnThunk, which keeps the stack depth the same; the caller's add esp, 0xC cleans up as before.
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

	// ---- Shader cache scan skip ----
	// The per-package shader cache scan asks the renderer for every loaded shader cache name and probes
	// "<package>/Shaders/Generated/<name>" in the VFS for each one. It returns nothing the caller uses (the caller only pops its
	// one stack arg), so returning straight away is safe.

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

	// ---- Existence check timing ----
	// The install step first asks the VFS whether the psarc exists. The check does that through a pointer
	// the VFS fills in at startup, to a probe that opens the file read-only and closes it again. On a tester's HDD library under
	// Defender that open was 87% of the game thread's time, before any read. Its CALL [ptr] is pointed at a slot of ours, so the
	// check is timed on its own and still calls whatever the VFS installed, read at every call (so it doesn't matter whether the
	// VFS has started yet). Timed only during drained installs, on the game thread.

	constexpr unsigned char existsPrologue[] = { 0x55, 0x8B, 0xEC, 0x8B, 0x4D, 0x0C, 0x33, 0xC0, 0x85, 0xC9, 0x74, 0x02, 0x8B, 0x01, 0x50, 0x8B, 0x45, 0x08, 0x50, 0xFF, 0x15 };
	uintptr_t* existsGameSlot = nullptr;	// The VFS's pointer.
	uintptr_t existsHookSlot = 0;			// What the patched CALL reads.
	bool existsHooked = false;
	DWORD gameThreadId = 0;
	bool timingInstall = false;
	long long existsTicks = 0;				// Time in the check during the current drained install.
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

	// ---- Native banner ----
	// The game's notification banner function (cdecl, the caller pops 0x30 bytes). The callee constructs an out-string at
	// ESI (inline, empty) and returns it in EAX, which the caller copies. It also owns a by-value string arg at [esp+0x1C] and
	// frees it. The skip path does both, so the caller sees what it would after a real call.

	bool bannerHooked = false;
	int32_t bannerOriginalRel = 0;
	unsigned int bannerSkips = 0;

	// The tick's own install step call, held while its next file isn't warm (see "Holding the game's own install").
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

	// ---- Call site patching ----

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

	// ---- CRT stream cap ----
	// MSVCR100 caps open FILE streams at 512 (_setmaxstdio can raise it to 2048, and its low-level handle table __pioinfo is 64
	// blocks of 32). With 8 installs per tick, 28 of 846 packages were lost at 512 and none at 2048. A 3796 package library
	// can't be open at once even at 2048, so the drain also counts open handles and pauses near the cap, and the game's own
	// one-per-frame pace lets finished loads close their streams.

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

	// __pioinfo[64] -> blocks of 32 ioinfo entries: the OS handle at +0, osfile at +4 with FOPEN (0x01) set while open.
	// The entry size isn't exported, so it's found by matching fds 0..2 against _get_osfhandle.
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

	// ---- Rescan skip ----
	// A rescan of an unchanged library only opens every psarc again for the game to refuse it by name (10 s stock for 838).
	// Each entry the scan installs is snapshotted as path|size|write time. An entry is only remembered if its install registered
	// a package, so failed installs (not owned, unreadable) and duplicates are retried on the next scan like the game would.
	// On the next scan, remembered entries are popped through the game's own pop_back without being opened.

	std::vector<std::string> knownEntries;		// Sorted, from the last completed scan.
	std::vector<std::string> currentSnapshot;	// This scan's registered or already known entries.
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

	bool IsKnown(const std::string& line) {
		return lastSnapshotComplete && std::binary_search(knownEntries.begin(), knownEntries.end(), line);
	}

	// True when the next entry was registered by the last scan and is unchanged. It's popped without being opened.
	bool SkipIfKnown(unsigned char* service) {
		if (!rescanSkip || !lastSnapshotComplete)
			return false;
		std::string line = SnapshotLine(NextQueuedPath(service));
		if (!IsKnown(line))
			return false;
		currentSnapshot.push_back(std::move(line));
		CallRemoveLast(service + off_queueBegin);
		++unchangedSkipped;
		return true;
	}

	// ---- Scan state ----

	bool draining = false;
	long long drainStartTicks = 0;
	unsigned int queueAtStart = 0;
	unsigned int extraInstalls = 0;
	unsigned int extraLoads = 0;
	unsigned int drainTicks = 0;
	unsigned int unregisteredInstalls = 0;
	unsigned int droppedRequests = 0;		// Requests held back while a scan ran.
	unsigned int packagesAtStart = 0;
	unsigned int dlcAtStart = 0;
	long long queueEmptyTicks = 0;
	unsigned int waitingAtQueueEmpty = 0;
	long long lastProgressLogTicks = 0;
	unsigned int scanCount = 0;
	std::string samplerLabel;

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

	// Index of the next entry in install order (0 = the first one the scan installs). The prefetch job's paths use the same order.
	unsigned int NextInstallIndex(const unsigned char* service) {
		const unsigned int length = QueueLength(service);
		return queueAtStart >= length ? queueAtStart - length : 0;
	}

	std::string FileName(std::string_view path) {
		const size_t slash = path.find_last_of("\\/");
		return std::string(slash == std::string_view::npos ? path : path.substr(slash + 1));
	}

	// ---- Game thread CPU time ----
	// Wall time minus the thread's own CPU time is the time an install spent blocked: waiting on the disk, a real-time scanner,
	// or a lock. QueryThreadCycleTime counts TSC cycles, converted with a TSC rate measured between Install() and the scan start.

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

	// ---- Warm-up (read-ahead) ----
	// The package install first checks that the psarc exists by opening it (GENERIC_READ) and closing it again, then opens it for
	// real and reads its head twice (header and TOC, then the appid entry). Background threads open each queued file, read its
	// first FastEnumerationPrefetchKB and close it, in install order, a bounded distance ahead of the game thread, so the game's
	// own opens and reads find the file already warm. Forced on an SSD it cost nothing measurable (2756 vs 2713 ms for 844 packages).
	//
	// What "warm" has to cover. A 2840 package library, ~1070 official packages on an NVMe SSD and ~1770 CDLC on a symlinked HDD,
	// with Defender real-time protection on: ~1.5 ms per install while the scan was on the SSD, then 80-900 ms per install from
	// the first HDD file on, at the same file in two builds. 87% of the game thread's samples were in the existence check's
	// CreateFile, not in a read; the reads right after it were cheap, the time didn't follow the file size, and the file attribute
	// lookup just before it took 0.05 ms. A plain cold HDD install measured 12.5 ms. That's an on-access scanner reading the file
	// on its first open; the result stays with the file, so later opens are cheap. The cost is per first open, so it can only be
	// moved off the game thread:
	//   - Several threads open files at once. A single thread used to open a batch of 64 before reading any of them; at ~75 ms per
	//     scanned open it fell behind the game within one batch and never got ahead again.
	//   - The game thread waits for its next file to be warm instead of opening it cold (IsNextWarm): the drain's extra installs
	//     stop for the tick, and the game's own per-tick install is held (StockInstallThunk). Frames keep coming while files warm,
	//     instead of one 100-900 ms frame per package.
	//   - Normal I/O priority. The threads are the scan's critical path now, and the game only opens warm files. At background
	//     priority they'd also queue behind any other process using the same disk.
	//   - If warm files still install slowly, warming doesn't help on this machine: it stops for the rest of the scan and the game
	//     installs at its own pace again (BackOffPrefetchIfSlow).

	unsigned int prefetchKb = 128;
	bool prefetchForced = true;
	constexpr unsigned int prefetchLead = 256;		// Files ahead of the game thread, bounds the cache use.
	constexpr unsigned int prefetchThreads = 8;		// Files being opened at once.
	// Warm-up threads only open, read into a heap buffer and close. Small stacks keep them out of the way of the large blocks a
	// profile save needs in the game's 2 GB of address space.
	constexpr SIZE_T warmupStackSize = 64 * 1024;
	// The game thread waits at most this long for one file, in case a warm-up thread is stuck (e.g. a network drive). After that
	// the game opens the file itself, like it would without the warm-up.
	constexpr double maxWarmWaitMs = 5000.0;
	// A warm file installs in ~1-2 ms. Warm files averaging this many ms per install for this many progress windows in a row
	// (~1 s each) means warming isn't what makes installs slow here.
	constexpr double prefetchBackoffMsPerInstall = 40.0;
	constexpr unsigned int prefetchBackoffWindows = 2;
	constexpr unsigned int maxPrefetchErrorLogs = 20;
	constexpr double slowWarmOpenMs = 50.0;
	constexpr unsigned int maxSlowWarmOpenLogs = 20;

	// The job owns the path list. Each thread and StopPrefetch drop a reference, so a thread still stuck in CreateFile or ReadFile
	// after the stop timeout keeps a valid list, and the next scan starts a fresh job.
	struct PrefetchJob {
		std::vector<std::string> paths;		// Install order.
		// Per file, in install order: when it was warmed (QPC ticks), 0 = not yet, -1 = couldn't open or read it.
		std::unique_ptr<std::atomic<long long>[]> readTicks;
		std::atomic<bool> stop = false;
		std::atomic<bool> backedOff = false;
		std::atomic<unsigned int> files = 0;
		std::atomic<unsigned int> skipped = 0;	// Already installed by the time a thread got to them.
		std::atomic<unsigned int> failures = 0;	// Couldn't open or read.
		std::atomic<unsigned int> slowOpens = 0;	// Opens of slowWarmOpenMs or more.
		std::atomic<unsigned int> next = 0;		// Install-order index of the next file to claim.
		std::atomic<unsigned int> inFlight = 0;	// Files being opened or read right now.
		std::atomic<unsigned int> running = 0;	// Threads still running.
		std::atomic<long long> ioTicks = 0;		// Time spent opening and reading, summed over the threads.
		std::atomic<long long> openTicks = 0;	// Of that, opening.
		std::atomic<unsigned long long> bytes = 0;
		std::atomic<int> refs = 1;
		long long startTicks = 0;
		std::atomic<long long> endTicks = 0;
		void Release() {
			if (refs.fetch_sub(1, std::memory_order_acq_rel) == 1)
				delete this;
		}
	};
	PrefetchJob* prefetchJob = nullptr;
	std::vector<HANDLE> prefetchHandles;

	unsigned int ConsumedFiles(unsigned int total) {
		return total - (std::min)(total, progressRemaining.load(std::memory_order_acquire));
	}

	void NotePrefetchFailure(PrefetchJob* job, unsigned int index, const char* what, DWORD error) {
		job->readTicks[index].store(-1, std::memory_order_release);
		if (++job->failures <= maxPrefetchErrorLogs)
			LOG_WARNING("(ENUMERATION) Prefetch couldn't " << what << " #" << index << " " << job->paths[index] << ": error " << error
				<< (job->failures == maxPrefetchErrorLogs ? " (further prefetch errors are only counted)" : "") << std::endl);
	}

	// Claims the next file within the lead, skipping the ones the game has already installed. False once there's nothing left.
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
		while (ClaimNextFile(job, total, index)) {
			// Already warm: the boot warm-up got to it first (HandOffBootWarmup).
			if (job->readTicks[index].load(std::memory_order_acquire) != 0) {
				++job->skipped;
				continue;
			}
			++job->inFlight;
			wchar_t wide[MAX_PATH * 2]{};
			MultiByteToWideChar(CP_UTF8, 0, job->paths[index].c_str(), -1, wide, MAX_PATH * 2);
			const long long openStart = Now();
			// No read-ahead hint: the cache manager would read past the head, and on an HDD that's more time the game waits.
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
			LOG_INFO("(ENUMERATION) Prefetch threads finished" << (job->stop.load() ? " (stopped)" : "") << " after " << MsSince(job->startTicks) << " ms" << std::endl);
		}
		job->Release();
		return 0;
	}

	// True when any of up to 32 queued packages, spread over the queue, is on a volume with a seek penalty (or one that won't say).
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
		if (prefetchKb == 0) {
			LOG_INFO("(ENUMERATION) Prefetch off (FastEnumerationPrefetchKB=0)" << std::endl);
			return;
		}
		const unsigned int count = QueueLength(service);
		if (count == 0)
			return;
		if (!prefetchForced) {
			static int seekPenalty = -1;
			if (seekPenalty < 0)
				seekPenalty = LibraryHasSeekPenalty(service, count) ? 1 : 0;
			if (seekPenalty == 0) {
				LOG_INFO("(ENUMERATION) Prefetch off: FastEnumerationPrefetchHddOnly is on and the library is all SSD" << std::endl);
				return;
			}
		}
		PrefetchJob* job = new PrefetchJob;
		job->paths.reserve(count);
		for (unsigned int i = count; i > 0; --i)
			job->paths.emplace_back(QueuedPath(service, i - 1));
		job->readTicks.reset(new std::atomic<long long>[count]);
		for (unsigned int i = 0; i < count; ++i)
			job->readTicks[i].store(0, std::memory_order_relaxed);
		job->startTicks = Now();
		// Counted up front, so the first thread to finish can't see itself as the last one.
		job->running.store(prefetchThreads, std::memory_order_release);
		for (unsigned int i = 0; i < prefetchThreads; ++i) {
			job->refs.fetch_add(1, std::memory_order_acq_rel);
			HANDLE thread = CreateThread(nullptr, warmupStackSize, PrefetchMain, job, STACK_SIZE_PARAM_IS_A_RESERVATION, nullptr);
			if (!thread) {
				LOG_WARNING("(ENUMERATION) Prefetch thread couldn't start (error " << GetLastError() << ")" << std::endl);
				job->refs.fetch_sub(1, std::memory_order_acq_rel);
				// Threads that already started may all have finished; then this was the last one, and the job has to read as done.
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
		LOG_INFO("(ENUMERATION) Prefetch started: " << count << " files, " << prefetchHandles.size() << " threads, first " << prefetchKb << " KB of each, up to "
			<< prefetchLead << " files ahead of the game. The game waits for a file that isn't warm yet (up to " << maxWarmWaitMs << " ms) instead of opening it cold" << std::endl);
	}

	void StopPrefetch() {
		if (!prefetchJob)
			return;
		PrefetchJob* job = prefetchJob;
		job->stop.store(true, std::memory_order_release);
		const DWORD wait = WaitForMultipleObjects(static_cast<DWORD>(prefetchHandles.size()), prefetchHandles.data(), TRUE, 3000);
		for (HANDLE thread : prefetchHandles)
			CloseHandle(thread);
		prefetchHandles.clear();
		prefetchJob = nullptr;
		const long long endTicks = job->endTicks.load(std::memory_order_acquire);
		const long long end = endTicks ? endTicks : Now();
		const unsigned int files = job->files.load();
		LOG_INFO("(ENUMERATION) Prefetch: " << files << " of " << job->paths.size() << " files warmed, " << job->skipped.load() << " skipped as already installed, "
			<< job->failures.load() << " failed, " << job->slowOpens.load() << " opens >= " << slowWarmOpenMs << " ms, " << (job->bytes.load() >> 20) << " MB, "
			<< (end - job->startTicks) / ticksPerMs << " ms, disk " << job->ioTicks.load() / ticksPerMs << " ms summed over the threads (opening "
			<< job->openTicks.load() / ticksPerMs << " ms, " << (files ? job->ioTicks.load() / ticksPerMs / files : 0.0) << " ms/file)"
			<< (job->backedOff.load() ? ", backed off" : "") << (wait == WAIT_TIMEOUT ? " (a thread was still in an open or read, left to finish)" : "") << std::endl);
		job->Release();
	}

	// Called once per progress window on the game thread, with the drained installs of files that were warm. Only sets the stop
	// flag: a thread may be in a slow open, and waiting for it here would stall the frame. StopPrefetch at the end of the scan
	// still joins and logs them.
	unsigned int slowPrefetchWindows = 0;

	void BackOffPrefetchIfSlow(unsigned int warmInstalls, double warmInstallMs) {
		PrefetchJob* job = prefetchJob;
		if (!job || job->stop.load(std::memory_order_acquire) || job->endTicks.load(std::memory_order_acquire) || warmInstalls == 0)
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

	// What the read-ahead is doing right now, for charging an install to it.
	enum class PrefetchActivity { Reading, Waiting, NotRunning };
	constexpr const char* prefetchActivityNames[] = { "while the prefetch was reading", "while the prefetch was waiting for the game", "with no prefetch running" };

	PrefetchActivity CurrentPrefetchActivity() {
		PrefetchJob* job = prefetchJob;
		if (!job || job->stop.load(std::memory_order_acquire) || job->endTicks.load(std::memory_order_acquire))
			return PrefetchActivity::NotRunning;
		return job->inFlight.load(std::memory_order_acquire) ? PrefetchActivity::Reading : PrefetchActivity::Waiting;
	}

	// Whether the read-ahead had read this file before the game installed it, and how long before (ms), for the slow-install lines.
	// -1 = not read yet, -2 = the read-ahead failed on it, -3 = no read-ahead.
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
		out << "prefetched " << msBefore << " ms before";
		return out.str();
	}

	// For the progress line: how far the read-ahead is ahead of the game (negative = behind), or why it isn't running.
	std::string PrefetchStatus(unsigned int consumed) {
		PrefetchJob* job = prefetchJob;
		if (!job)
			return "prefetch off";
		if (job->backedOff.load(std::memory_order_acquire))
			return "prefetch stopped (backed off)";
		if (job->endTicks.load(std::memory_order_acquire))
			return "prefetch done";
		const long long ahead = static_cast<long long>(job->next.load(std::memory_order_acquire)) - consumed;
		return "prefetch " + std::to_string(ahead) + " ahead, " + std::to_string(job->inFlight.load(std::memory_order_acquire)) + " opening";
	}

	// ---- Boot warm-up ----
	// The first scan of a session waits for the profile screens, about a minute after the DLL loads (61.7 and 79.5 s in a
	// tester's logs), and a scanner's first open of each file on that tester's HDD cost 80-900 ms. At startup, threads walk the
	// dlc folder and open every psarc the way the warm-up does, so the scanner has been through them by the time the scan asks.
	// They run at background I/O priority, so the game's own loading comes first. When the scan starts it takes over
	// (HandOffBootWarmup): the files already warmed are marked warm in the scan's job, and the scan's threads do the rest.
	// The process has 2 GB of address space and profile saves need large free blocks, so the threads get small stacks and the
	// path list is freed when they finish. Only a hash and a flag per file stay until the scan starts.

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

	// FNV-1a over a normalized path.
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
		std::vector<std::wstring> paths;	// Install order, freed by the last thread.
		std::vector<unsigned long long> hashes;
		std::unique_ptr<std::atomic<bool>[]> warmed;
		std::atomic<bool> stop = false;
		std::atomic<bool> listed = false;	// paths, hashes and warmed are filled in.
		std::atomic<unsigned int> next = 0;
		std::atomic<unsigned int> files = 0;
		std::atomic<unsigned int> failures = 0;
		std::atomic<unsigned int> slowOpens = 0;
		std::atomic<unsigned int> running = 0;
		std::atomic<long long> openTicks = 0;	// Summed over the threads.
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

	// The order the scan installs in, as the logs show it: a folder's own psarcs from Z to A, then its subfolders, also from Z to A.
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
			const long long openStart = Now();
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
				<< job->slowOpens.load() << " opens >= " << slowWarmOpenMs << " ms, " << job->failures.load() << " failed" << std::endl);
			std::vector<std::wstring>().swap(job->paths);
		}
		job->Release();
		return 0;
	}

	// Lists the dlc folder, then becomes one of the warm-up threads and starts the others.
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
			job->running.fetch_sub(1, std::memory_order_acq_rel);	// This thread is still running, so it's never the last one here.
		}
		return BootWarmupMain(job);
	}

	void StartBootWarmup() {
		if (!bootWarmup || bootJob)
			return;
		BootJob* job = new BootJob;
		job->startTicks = Now();
		job->refs.store(2, std::memory_order_release);	// The list thread and bootJob.
		HANDLE thread = CreateThread(nullptr, warmupStackSize, BootListMain, job, STACK_SIZE_PARAM_IS_A_RESERVATION, nullptr);
		if (!thread) {
			LOG_WARNING("(ENUMERATION) Boot warm-up thread couldn't start (error " << GetLastError() << ")" << std::endl);
			delete job;
			return;
		}
		CloseHandle(thread);
		bootJob = job;
	}

	// At the scan start, on the game thread: marks what the boot warm-up warmed as warm in the scan's job (if there is one) and
	// stops it. Threads still in an open finish it and exit.
	void HandOffBootWarmup() {
		BootJob* job = bootJob;
		if (!job)
			return;
		bootJob = nullptr;
		job->stop.store(true, std::memory_order_release);
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
		if (prefetch) {
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

	// ---- Waiting for warm files ----
	// The game thread installs a file only once the warm-up has been through it (or failed on it), so a scanner's first-open cost
	// lands on a warm-up thread. Opening it cold here would cost the same scan anyway, just on the game thread.

	unsigned int warmWaitIndex = ~0u;		// The file the game thread is waiting for, ~0 = none.
	long long warmWaitStart = 0;
	bool warmWaitTimedOut = false;
	unsigned int warmWaitFiles = 0;			// Files the game thread had to wait for.
	unsigned int warmWaitTimeouts = 0;		// Of those, ones it gave up on after maxWarmWaitMs.
	double warmWaitMs = 0.0;				// Time spent waiting, summed over those files.
	unsigned int stockHolds = 0;			// Ticks where the game's own install was held (StockInstallThunk).
	bool warmOrderMismatchLogged = false;

	void EndWarmWait(long long now) {
		if (warmWaitIndex == ~0u)
			return;
		warmWaitMs += (now - warmWaitStart) / ticksPerMs;
		warmWaitIndex = ~0u;
	}

	// False while the next file to install hasn't been warmed yet and a warm-up thread is still on its way to it.
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
			// The queue should be in the order the job was built from. If it isn't, waiting on the job could be waiting on the wrong file.
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
			warmWaitTimedOut = true;
			++warmWaitTimeouts;
			LOG_INFO("(ENUMERATION) Waited " << maxWarmWaitMs << " ms for #" << index << " " << job->paths[index] << " to warm, the game opens it itself" << std::endl);
			return true;
		}
		return false;
	}

	// ---- Install timing ----
	// Every install the drain does is split into:
	//   stat      the rescan snapshot's file attribute lookup (GetFileAttributesEx on the psarc, before the install)
	//   open+read from the install step's start until package registration starts: opening the psarc, reading its header, TOC and
	//             appid entry. On a cold HDD or under a real-time scanner, this is where the time goes.
	//   register  package registration (CPU: building the package, VFS mounts; the shader scan is skipped)
	//   after     from registration to the end of the install step
	// plus the game thread's CPU time over the whole install, so "blocked" (wall - CPU) is time spent waiting on something.
	// Installs that never reach registration (refused: not owned, duplicate, unreadable) charge everything to open+read.

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
		double existsMs = 0.0;		// Of open+read, the existence check (opens and closes the file, reads nothing).
		double registerMs = 0.0;
		double afterMs = 0.0;
		double cpuMs = -1.0;		// -1 = unknown.
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
		double cpuKnownTotalMs = 0.0;	// Wall time of the installs with a known CPU time.
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

		// "avg 4.2 ms (stat 0.1, open+read 3.0, register 1.0, after 0.1; CPU 1.1, blocked 3.1 = 74%), max 12 ms, 0 >= 50 ms"
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

	// Per scan.
	TimingSums scanTiming;
	TimingSums timingByActivity[3];
	TimingSums timingPrefetched;		// The read-ahead had read the file before the install.
	TimingSums timingNotPrefetched;		// It hadn't (or there was no read-ahead).
	unsigned int installHistogram[histogramBuckets]{};
	std::vector<InstallTiming> slowest;	// Top topSlowInstalls, slowest first.
	unsigned int slowInstallLogs = 0;
	double stockTickMs = 0.0;			// The game's own tick (which does one install itself), while scanning.
	unsigned int stockTicks = 0;
	double loadStepMs = 0.0;			// The extra load-stage steps the drain runs.

	// Per progress window (~1 s).
	struct ProgressWindow {
		unsigned int ticks = 0;			// Game ticks (frames) seen.
		unsigned int installs = 0;		// Installs done by the drain.
		unsigned int budgetStops = 0;	// Ticks where the install loop stopped on the time budget.
		unsigned int ceilingStops = 0;	// Ticks where it stopped on the open file ceiling.
		unsigned int warmStops = 0;		// Ticks where it stopped to wait for the next file to be warmed.
		unsigned int stockHolds = 0;	// Ticks where the game's own install was held for the same reason.
		unsigned int warmInstalls = 0;	// Drained installs of files the warm-up had been through.
		double warmInstallMs = 0.0;
		unsigned int loads = 0;			// Extra load-stage steps.
		double drainMs = 0.0;			// Time spent in the drain.
		double installMs = 0.0;			// Of that, time spent in installs.
		double loadMs = 0.0;			// Of that, time spent in the extra load-stage steps.
		double stockTickMs = 0.0;		// The game's own tick, outside the drain.
		double maxFrameMs = 0.0;		// Longest gap between two ticks.
		long long lastTickStart = 0;
		TimingSums timing;
		unsigned int prefetchFiles = 0;	// Prefetch counters at the window's start, for the per-window deltas.
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

	// Installs the next queued entry through the install step, remembers it for the next rescan if it registered, and times it.
	void InstallNext(unsigned char* service, bool countExtra) {
		InstallTiming t;
		t.index = NextInstallIndex(service);
		t.path = std::string(NextQueuedPath(service));
		t.activity = CurrentPrefetchActivity();
		const long long start = Now();
		t.prefetchedMsBefore = PrefetchedMsBefore(t.index, start);
		const unsigned long long cyclesStart = ThreadCycles();

		std::string line;
		if (rescanSkip)
			line = SnapshotLine(t.path);
		const long long callStart = Now();
		lastInstallRegistered = false;
		inInstallRegister = 0;
		registerEnterTicks = 0;
		registerLeaveTicks = 0;
		existsTicks = 0;
		existsCalls = 0;
		timingInstall = existsHooked;
		CallInstallNext(service);
		timingInstall = false;
		const long long end = Now();
		const unsigned long long cycles = ThreadCycles() - cyclesStart;

		if (lastInstallRegistered) {
			if (rescanSkip)
				currentSnapshot.push_back(std::move(line));
		}
		else if (countExtra) {
			++unregisteredInstalls;
		}

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
		const double cpuMs = CyclesToMs(cycles);
		t.cpuMs = cpuMs < 0.0 ? -1.0 : (std::min)(cpuMs, t.totalMs);
		if (t.activity == PrefetchActivity::Waiting && CurrentPrefetchActivity() == PrefetchActivity::Reading)
			t.activity = PrefetchActivity::Reading;
		RecordInstall(std::move(t));
	}

	// ---- Scan start / finish ----

	// Packages spread over the queue, for the environment probe (which volumes the library is on).
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
			<< budgetMs << " ms (menus " << menuBudgetMs << ", boot " << bootBudgetMs << "), rescan skip " << (rescanSkip ? "on" : "off")
			<< ", shader scan skip " << (skipShaderScan ? "on" : "off") << " (hooked " << (shaderScanHooked ? "yes" : "no") << "), early scan "
			<< (requestEarly ? "on" : "off") << " (max " << earlyMaxPackages << "), prefetch " << prefetchKb << " KB " << (prefetchForced ? "always" : "HDD only")
			<< ", CRT stream limit " << crtStreamLimit << ", open file ceiling " << openFileCeiling << ", register hook " << (registerHooked ? "yes" : "no")
			<< ", banner hook " << (bannerHooked ? "yes" : "no") << ", stock install hold " << (stockInstallHooked ? "yes" : "no") << ", exists check timed " << (existsHooked ? "yes" : "no")
			<< ", boot warm-up " << (bootWarmup ? "on" : "off") << std::endl);
	}

	// ---- Progress display for rescans ----
	// The game re-requests a scan while the first one runs; it's held back and runs the moment the first finishes. That rescan
	// pops every unchanged entry without opening it and ends silently, but resetting the display at its start used to cut the
	// first scan's "UPDATED!" off after a few ms. So a rescan only takes over the display once it installs something (a new
	// or changed package); a rescan that installs nothing leaves the previous display as it was.
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

	// Called before each drained install. The first one of a deferred rescan takes over the display.
	void EndProgressDeferral() {
		if (!progressDeferred)
			return;
		progressDeferred = false;
		PublishProgressStart();
		LOG_INFO("(ENUMERATION) Rescan found a new or changed package after popping " << unchangedSkipped << " unchanged ones, showing its progress" << std::endl);
	}

	void Start(unsigned char* service, long long now) {
		draining = true;
		drainStartTicks = now;
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
		// Not cleared here: the game's own install step already installed (and recorded) the first entry this tick.
		currentSnapshot.reserve(queueAtStart);
		CountPackages(packagesAtStart, dlcAtStart);
		CalibrateCycles();
		gameThreadId = GetCurrentThreadId();

		// Also read by the prefetch thread, so it's kept current even while the display is deferred.
		progressRemaining.store(queueAtStart, std::memory_order_release);
		// A rescan leaves the display alone until it installs something (see "Progress display for rescans").
		progressDeferred = rescanSkip && lastSnapshotComplete;
		if (!progressDeferred)
			PublishProgressStart();

		LOG_INFO("(ENUMERATION) Scan " << scanCount + 1 << " start: queue=" << queueAtStart << " extra/tick=" << EffectiveMaxPerTick() << " loads/tick=" << maxLoadsPerTick
			<< " budget=" << EffectiveBudgetMs() << " ms, open files=" << OpenCrtFiles() << ", waiting loads=" << WaitingLoads() << ", packages " << packagesAtStart
			<< " (dlc " << dlcAtStart << "), " << (rescanSkip && lastSnapshotComplete ? "rescan: " + std::to_string(knownEntries.size()) + " entries known from the last scan" : std::string("first scan this session"))
			<< ", " << (mainMenuSeen.load(std::memory_order_relaxed) ? "in menus" : "before the main menu") << ", " << EnumerationEnvironment::MemoryLine() << std::endl);
		if (scanCount == 0) {
			LogSettings();
			EnumerationEnvironment::LogAsync(SamplePaths(service, queueAtStart, 32));
		}

		// A rescan against a complete snapshot pops unchanged entries unopened, so reading them ahead would be wasted.
		if (!(rescanSkip && lastSnapshotComplete))
			StartPrefetch(service);
		else
			LOG_INFO("(ENUMERATION) Prefetch skipped: rescan, unchanged entries are popped without being opened" << std::endl);
		HandOffBootWarmup();
		ResetWindow();
		AssetLoadDrain::SetActive(true);

		// Samples the game thread for the whole scan when RSMods_profiling.txt is next to the game.
		samplerLabel = "enumeration_" + std::to_string(++scanCount);
		SamplingProfiler::Start(GetCurrentThreadId(), samplerLabel.c_str());
	}

	// Hint lines: a first reading of the numbers, so a log can be triaged at a glance. Heuristics, not verdicts.
	void LogHints() {
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
		SamplingProfiler::Stop(samplerLabel.c_str());
		EndWarmWait(Now());
		StopPrefetch();
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

		// Nothing new: the game's own re-request at boot stays silent, and a later one shows "UP TO DATE" briefly.
		const bool nothingNew = registered == 0 && unchangedSkipped > 0;
		const bool silentScan = nothingNew && !mainMenuSeen.load(std::memory_order_relaxed);

		progressRemaining.store(0, std::memory_order_release);
		if (progressDeferred && silentScan) {
			// A rescan that installed nothing: whatever the display shows (the previous scan's "UPDATED!") stays as it was.
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
		// Every request queues the whole dlc folder, so files beyond the registered DLC count are the ones the game refused
		// (duplicate DLC keys, not owned, unreadable).
		progressNotRegistered.store(queueAtStart > dlc ? queueAtStart - dlc : 0, std::memory_order_release);
		progressElapsedSeconds.store(MsSince(drainStartTicks) / 1000.0, std::memory_order_release);
		progressUpToDate.store(nothingNew, std::memory_order_release);
		progressActive.store(!silentScan, std::memory_order_release);
		progressCompleted.store(true, std::memory_order_release);
		completionStartTicks.store(Now(), std::memory_order_release);
	}

	void LogScanDone(unsigned int total, unsigned int dlc, unsigned int newDlc) {
		const double elapsed = MsSince(drainStartTicks);
		const double installPhase = queueEmptyTicks ? (queueEmptyTicks - drainStartTicks) / ticksPerMs : elapsed;
		LOG_INFO("(ENUMERATION) Scan done: " << queueAtStart << " queued, " << extraInstalls << " installed by the drain over " << drainTicks << " ticks, "
			<< elapsed << " ms (installs " << installPhase << " ms, then " << waitingAtQueueEmpty << " loads still waiting, " << extraLoads << " extra load ticks), "
			<< "packages total=" << total << " dlc=" << dlc << " (new " << newDlc << "), drained installs that did not register=" << unregisteredInstalls
			<< ", open files now=" << OpenCrtFiles() << " peak=" << peakOpenFiles << " ceiling pauses=" << ceilingPauses
			<< ", unchanged entries popped unopened=" << unchangedSkipped << ", shader scans skipped=" << shaderScansSkipped
			<< ", requests held back=" << droppedRequests << ", " << EnumerationEnvironment::MemoryLine() << std::endl);
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

		const long long start = Now();
		if (!draining)
			Start(service, start);
		++drainTicks;
		++window.ticks;
		if (window.lastTickStart)
			window.maxFrameMs = (std::max)(window.maxFrameMs, (start - window.lastTickStart) / ticksPerMs);
		window.lastTickStart = start;
		const double budget = EffectiveBudgetMs();

		// Loads first, within half the budget. They're cheap (~0.5 ms) and keep the open streams and the waiting list short.
		unsigned int loads = 0;
		while (loads < maxLoadsPerTick && (WaitingLoads() > 0 || IsLoadSlotBusy(service))) {
			if (MsSince(start) >= budget * 0.5)
				break;
			CallLoadTick(service);
			++loads;
		}
		extraLoads += loads;
		window.loads += loads;
		const double loadMs = MsSince(start);
		window.loadMs += loadMs;
		loadStepMs += loadMs;

		unsigned int done = 0;
		const unsigned int perTick = EffectiveMaxPerTick();
		while (done < perTick && service[off_installing] && QueueLength(service) > 0) {
			if (MsSince(start) >= budget) {
				++window.budgetStops;
				break;
			}
			if (pioinfo && OpenCrtFiles() >= openFileCeiling) {
				++ceilingPauses;
				++window.ceilingStops;
				break;
			}
			if (SkipIfKnown(service))
				continue;
			if (!IsNextWarm(service)) {
				++window.warmStops;
				break;
			}
			EndProgressDeferral();
			const long long installStart = Now();
			InstallNext(service, true);
			window.installMs += MsSince(installStart);
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
		}

		window.drainMs += MsSince(start);
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
		maxPerTick = 0;
		maxLoadsPerTick = 0;
		LOG_ERROR("(ENUMERATION) Exception while draining, the drain is off for this session" << std::endl);
	}

	// ---- Scan requests ----

	// Early scan (FastEnumerationEarlyScan, off by default). Asks for the scan at the DLC service's first tick, while the title
	// screen waits, instead of when the game asks after the profile screens. The song list is then complete when the menu comes
	// up, but measured with profile autoload the menu came up 2 s later, so it's opt in. A library over FastEnumerationEarlyScanMax
	// packages is left to the stock timing, since a boot time scan holds the boot flow.
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

	// The scan setup refuses while a scan is installing and the tick then leaves the request set, so the game rescans the moment the
	// current scan empties its queue, before the load stage (and the drain's scan) has finished. A request that arrives mid
	// scan is held back instead and raised again once the scan is done. With the rescan skip, that rescan only opens new files.
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

	// The game asks for a scan at every sign-in. Once the early scan has completed, those are answered by clearing the request.
	// Requests from ForceEnumeration (keybind or automatic) always go through.
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

	// ---- Holding the game's own install ----
	// The tick calls the install step (service in ESI, no stack args, its result unused) once per frame, outside the drain. While
	// its next file isn't warm, that call is skipped, so it doesn't open the file cold. The scan state isn't touched: clearing the
	// installing flag instead would also run the tick's end-of-scan work.

	unsigned char holdStockNow = 0;

	extern "C" void __cdecl EnumDecideStockInstall(unsigned char* service) {
		holdStockNow = (service && !IsNextWarm(service)) ? 1 : 0;
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

	// ---- The hook ----

	// The install the game's own install step does inside the original tick. Remembered the same way as drained ones.
	// Not gated on `draining`: the tick runs the install step before it handles a request, so the first install of a scan happens
	// on the tick after the queue fills, before the drain has seen the scan start.
	void RecordStockInstall(const std::string& line) {
		if (lastInstallRegistered && rescanSkip)
			currentSnapshot.push_back(line);
	}

	std::string PeekStockInstall(unsigned char* service) {
		lastInstallRegistered = false;
		inInstallRegister = 0;
		if (!rescanSkip || !service[off_installing] || QueueLength(service) == 0)
			return {};
		std::string line = SnapshotLine(NextQueuedPath(service));
		// Already known: remember it now, the game's reinstall is refused as a duplicate and won't register.
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
		auto* service = static_cast<unsigned char*>(self);
		if (!service) {
			originalTick(self, edx);
			return;
		}

		BeforeOriginalTick(service);
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
		stringDestroyFunc = Offsets::func_engineStringDestroy;
		return tickSlot && tickFunc && loadTickFunc && installNextFunc && removeLastFunc && packagesPtr && waitingPtr
			&& registerCallSite && registerFunc && shaderScanCallSite && shaderScanFunc && bannerCallSite && bannerFunc && stringDestroyFunc;
	}

	uintptr_t ReadSlot() {
		if (MemUtil::IsBadReadPtr(reinterpret_cast<void*>(tickSlot)))
			return 0;
		return *reinterpret_cast<const uintptr_t*>(tickSlot);
	}

	// Hidden tuning, from RSMods.ini [Fast Enumeration] (not in the GUI). Out-of-range values are clamped.
	unsigned int SettingInRange(const char* name, int low, int high) {
		return static_cast<unsigned int>((std::clamp)(Settings::GetModSetting(name), low, high));
	}

	void ReadTuning() {
		namespace Setting = Settings::Setting;
		maxPerTick = SettingInRange(Setting::FastEnumerationInstallsPerTick, 0, 256);
		maxLoadsPerTick = SettingInRange(Setting::FastEnumerationLoadsPerTick, 0, 256);
		budgetMs = SettingInRange(Setting::FastEnumerationBudgetMs, 1, 100);
		menuBudgetMs = SettingInRange(Setting::FastEnumerationMenuBudgetMs, 1, 100);
		bootMaxPerTick = SettingInRange(Setting::FastEnumerationBootInstallsPerTick, 0, 256);
		bootBudgetMs = SettingInRange(Setting::FastEnumerationBootBudgetMs, 1, 100);
		rescanSkip = Settings::IsOn(Setting::FastEnumerationSkipUnchanged);
		skipShaderScan = Settings::IsOn(Setting::FastEnumerationSkipShaderScan);
		requestEarly = Settings::IsOn(Setting::FastEnumerationEarlyScan);
		earlyMaxPackages = SettingInRange(Setting::FastEnumerationEarlyScanMax, 0, 1000000);
		prefetchKb = SettingInRange(Setting::FastEnumerationPrefetchKB, 0, 4096);
		prefetchForced = !Settings::IsOn(Setting::FastEnumerationPrefetchHddOnly);
		bootWarmup = Settings::IsOn(Setting::FastEnumerationBootWarmup);
		crtStreamLimit = static_cast<int>(SettingInRange(Setting::FastEnumerationStreamLimit, 512, crtStreamLimitMax));
		openFileCeiling = SettingInRange(Setting::FastEnumerationFileCeiling, 64, crtStreamLimit - 64);
	}
}

void EnumerationDrain::Install() {
	if (installed)
		return;

	if (!ResolveAddresses()) {
		LOG_WARNING("(ENUMERATION) Fast enumeration isn't supported on this game version" << std::endl);
		return;
	}

	ReadTuning();

	LARGE_INTEGER frequency{};
	QueryPerformanceFrequency(&frequency);
	ticksPerMs = frequency.QuadPart / 1000.0;
	// Anchors for the TSC rate, measured at the first scan start (CalibrateCycles), for the game thread's CPU time per install.
	calibrationQpc = Now();
	calibrationTsc = __rdtsc();

	// Without the higher stream cap, any pace above one install per frame loses packages.
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

	// The register scope hook comes first: the shader scan skip and the rescan snapshot both depend on it.
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
	bannerHooked = RedirectCall(bannerCallSite, bannerFunc, &BannerThunk, bannerOriginalRel, "Enumeration banner");
	// The install step is the tick's second call: mov esi, ecx; call <load step>; call <install step>.
	stockInstallHooked = RedirectCall(tickFunc + stockInstallCallOffset, installNextFunc, &StockInstallThunk, stockInstallOriginalRel, "Tick install step");
	existsHooked = HookExistsCheck();
	StartBootWarmup();

	LOG_INFO("(ENUMERATION) Fast enumeration installed: extra installs/tick=" << maxPerTick << ", budget=" << budgetMs << " ms (" << menuBudgetMs
		<< " ms in menus), shader scan skip " << (shaderScanHooked ? "on" : "off") << ", rescan skip " << (rescanSkip ? "on" : "off")
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
	if (menu == "MainMenu" && !mainMenuSeen.exchange(true, std::memory_order_relaxed))
		LOG_INFO("(ENUMERATION) Main menu reached, scan budget is now " << menuBudgetMs << " ms per frame" << std::endl);
	onSignInScreen.store(menu == "pre_enter_prompt" || menu == "TitleScreen" || menu == "SelectionListDialog" ||
		menu == "ProfileSelect" || menu == "SimpleDialog" || menu == "UplayLoginDialog", std::memory_order_relaxed);
}
