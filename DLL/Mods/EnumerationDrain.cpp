#include "../stdafx.h"
#include "EnumerationDrain.hpp"
#include "AssetLoadDrain.hpp"
#include "../SamplingProfiler.hpp"
#include <algorithm>
#include <atomic>
#include <string_view>
#include <winioctl.h>

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

	extern "C" void __cdecl EnumRegisterEnter() {
		inInstallRegister = 1;
		packagesBeforeRegister = VectorCount(packagesPtr, 4);
	}

	extern "C" void __cdecl EnumRegisterLeave() {
		inInstallRegister = 0;
		lastInstallRegistered = VectorCount(packagesPtr, 4) > packagesBeforeRegister;
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

	// ---- Native banner ----
	// The game's notification banner function (cdecl, the caller pops 0x30 bytes). The callee constructs an out-string at
	// ESI (inline, empty) and returns it in EAX, which the caller copies. It also owns a by-value string arg at [esp+0x1C] and
	// frees it. The skip path does both, so the caller sees what it would after a real call.

	bool bannerHooked = false;
	int32_t bannerOriginalRel = 0;
	unsigned int bannerSkips = 0;

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

	// Per-second numbers for the progress line, to tell a slow frame rate apart from a drain that runs out of budget.
	struct ProgressWindow {
		unsigned int ticks = 0;			// Game ticks (frames) seen.
		unsigned int installs = 0;		// Installs done by the drain.
		unsigned int budgetStops = 0;	// Ticks where the install loop stopped on the time budget.
		double drainMs = 0.0;			// Time spent in the drain.
		double installMs = 0.0;			// Of that, time spent in installs.
		double maxFrameMs = 0.0;		// Longest gap between two ticks.
		long long lastTickStart = 0;
	};
	ProgressWindow window;

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

	// Installs the next queued entry through the install step and remembers it for the next rescan if it registered.
	void InstallNext(unsigned char* service, bool countExtra) {
		std::string line;
		if (rescanSkip)
			line = SnapshotLine(NextQueuedPath(service));
		lastInstallRegistered = false;
		inInstallRegister = 0;
		CallInstallNext(service);
		if (lastInstallRegistered) {
			if (rescanSkip)
				currentSnapshot.push_back(std::move(line));
		}
		else if (countExtra) {
			++unregisteredInstalls;
		}
	}

	// ---- Read-ahead for slow disks ----
	// The package install reads the head of every psarc twice (header and TOC, then the appid entry), a seek each on an HDD:
	// a tester's 2800 package HDD library spent 12.5 ms per install there against 0.12 ms on an SSD. A background thread reads
	// the first FastEnumerationPrefetchKB of each queued file, in install order, a bounded distance ahead of the game thread, so
	// both reads hit the OS cache. Forced on an SSD it cost nothing measurable (2756 vs 2713 ms for 844 packages).

	unsigned int prefetchKb = 128;
	bool prefetchForced = true;
	constexpr unsigned int prefetchLead = 256;	// Files ahead of the game thread, bounds the cache use.

	// The job owns the path list. The thread and StopPrefetch each drop a reference, so a thread still stuck in ReadFile after
	// the stop timeout keeps a valid list, and the next scan starts a fresh job.
	struct PrefetchJob {
		std::vector<std::string> paths;		// Install order.
		std::atomic<bool> stop = false;
		std::atomic<unsigned int> files = 0;
		std::atomic<unsigned long long> bytes = 0;
		std::atomic<int> refs = 2;
		long long startTicks = 0;
		std::atomic<long long> endTicks = 0;
		void Release() {
			if (refs.fetch_sub(1, std::memory_order_acq_rel) == 1)
				delete this;
		}
	};
	PrefetchJob* prefetchJob = nullptr;
	HANDLE prefetchThread = nullptr;

	DWORD WINAPI PrefetchMain(LPVOID param) {
		PrefetchJob* job = static_cast<PrefetchJob*>(param);
		std::vector<char> buffer(static_cast<size_t>(prefetchKb) * 1024);
		const unsigned int total = static_cast<unsigned int>(job->paths.size());
		for (unsigned int i = 0; i < total && !job->stop.load(std::memory_order_acquire); ++i) {
			while (!job->stop.load(std::memory_order_acquire)) {
				const unsigned int consumed = total - (std::min)(total, progressRemaining.load(std::memory_order_acquire));
				if (i < consumed + prefetchLead)
					break;
				Sleep(1);
			}
			wchar_t wide[MAX_PATH * 2]{};
			MultiByteToWideChar(CP_UTF8, 0, job->paths[i].c_str(), -1, wide, MAX_PATH * 2);
			HANDLE file = CreateFileW(wide, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, nullptr);
			if (file == INVALID_HANDLE_VALUE)
				continue;
			DWORD read = 0;
			if (ReadFile(file, buffer.data(), static_cast<DWORD>(buffer.size()), &read, nullptr)) {
				++job->files;
				job->bytes += read;
			}
			CloseHandle(file);
		}
		job->endTicks.store(Now(), std::memory_order_release);
		job->Release();
		return 0;
	}

	// 1 = no seek penalty (SSD), 0 = seek penalty (HDD), -1 = unknown (fails under Wine and on some USB bridges).
	int QuerySeekPenalty(const wchar_t* mountPoint) {
		wchar_t guid[64]{};
		std::wstring device;
		if (GetVolumeNameForVolumeMountPointW(mountPoint, guid, 64))
			device = guid;
		else
			device = std::wstring(L"\\\\.\\") + mountPoint;
		while (!device.empty() && device.back() == L'\\')
			device.pop_back();
		HANDLE h = CreateFileW(device.c_str(), 0, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING, 0, nullptr);
		if (h == INVALID_HANDLE_VALUE)
			return -1;
		STORAGE_PROPERTY_QUERY query{ StorageDeviceSeekPenaltyProperty, PropertyStandardQuery };
		DEVICE_SEEK_PENALTY_DESCRIPTOR descriptor{};
		DWORD bytes = 0;
		const BOOL ok = DeviceIoControl(h, IOCTL_STORAGE_QUERY_PROPERTY, &query, sizeof(query), &descriptor, sizeof(descriptor), &bytes, nullptr);
		CloseHandle(h);
		if (!ok)
			return -1;
		return descriptor.IncursSeekPenalty ? 0 : 1;
	}

	// The dlc folder, or a folder in it, is often a link to another drive, so the file's final path is resolved first.
	bool ResolveMountPoint(std::string_view path, std::wstring& mountPoint) {
		wchar_t wide[MAX_PATH * 2]{};
		MultiByteToWideChar(CP_UTF8, 0, path.data(), static_cast<int>(path.size()), wide, MAX_PATH * 2 - 1);
		wchar_t resolved[MAX_PATH * 2]{};
		const wchar_t* target = wide;
		HANDLE probe = CreateFileW(wide, 0, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, nullptr);
		if (probe != INVALID_HANDLE_VALUE) {
			const DWORD length = GetFinalPathNameByHandleW(probe, resolved, MAX_PATH * 2, VOLUME_NAME_DOS);
			CloseHandle(probe);
			if (length > 0 && length < MAX_PATH * 2)
				target = wcsncmp(resolved, L"\\\\?\\", 4) == 0 ? resolved + 4 : resolved;
		}
		wchar_t volume[MAX_PATH]{};
		if (!GetVolumePathNameW(target, volume, MAX_PATH))
			return false;
		mountPoint = volume;
		return true;
	}

	// True when any of up to 32 queued packages, spread over the queue, is on a volume with a seek penalty (or one that won't say).
	bool LibraryHasSeekPenalty(const unsigned char* service, unsigned int count) {
		std::vector<std::pair<std::wstring, int>> volumes;
		const unsigned int samples = (std::min)(count, 32u);
		bool penalty = false;
		for (unsigned int s = 0; s < samples; ++s) {
			const unsigned int index = samples == 1 ? 0 : static_cast<unsigned int>((static_cast<unsigned long long>(s) * (count - 1)) / (samples - 1));
			std::wstring mountPoint;
			if (!ResolveMountPoint(QueuedPath(service, index), mountPoint)) {
				penalty = true;
				continue;
			}
			const auto known = std::find_if(volumes.begin(), volumes.end(), [&](const auto& v) { return _wcsicmp(v.first.c_str(), mountPoint.c_str()) == 0; });
			if (known == volumes.end())
				volumes.emplace_back(mountPoint, QuerySeekPenalty(mountPoint.c_str()));
		}
		for (const auto& volume : volumes)
			if (volume.second != 1)
				penalty = true;
		LOG_INFO("(ENUMERATION) DLC library is on " << volumes.size() << " volume(s), " << (penalty ? "at least one spinning or unknown" : "all SSD") << std::endl);
		return penalty;
	}

	void StartPrefetch(const unsigned char* service) {
		if (prefetchKb == 0 || prefetchThread)
			return;
		const unsigned int count = QueueLength(service);
		if (count == 0)
			return;
		if (!prefetchForced) {
			static int seekPenalty = -1;
			if (seekPenalty < 0)
				seekPenalty = LibraryHasSeekPenalty(service, count) ? 1 : 0;
			if (seekPenalty == 0)
				return;
		}
		PrefetchJob* job = new PrefetchJob;
		job->paths.reserve(count);
		for (unsigned int i = count; i > 0; --i)
			job->paths.emplace_back(QueuedPath(service, i - 1));
		job->startTicks = Now();
		prefetchThread = CreateThread(nullptr, 0, PrefetchMain, job, 0, nullptr);
		if (!prefetchThread) {
			delete job;
			return;
		}
		prefetchJob = job;
		SetThreadPriority(prefetchThread, THREAD_PRIORITY_BELOW_NORMAL);
	}

	void StopPrefetch() {
		if (!prefetchThread)
			return;
		PrefetchJob* job = prefetchJob;
		job->stop.store(true, std::memory_order_release);
		const DWORD wait = WaitForSingleObject(prefetchThread, 3000);
		CloseHandle(prefetchThread);
		prefetchThread = nullptr;
		prefetchJob = nullptr;
		const long long endTicks = job->endTicks.load(std::memory_order_acquire);
		const long long end = endTicks ? endTicks : Now();
		LOG_INFO("(ENUMERATION) Prefetch: " << job->files.load() << " of " << job->paths.size() << " files, " << (job->bytes.load() >> 20) << " MB, "
			<< (end - job->startTicks) / ticksPerMs << " ms" << (wait == WAIT_OBJECT_0 ? "" : " (thread still in a read, left to finish)") << std::endl);
		job->Release();
	}

	// ---- Scan start / finish ----

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
		// Not cleared here: the game's own install step already installed (and recorded) the first entry this tick.
		currentSnapshot.reserve(queueAtStart);
		CountPackages(packagesAtStart, dlcAtStart);

		progressDetected.store(queueAtStart, std::memory_order_release);
		progressProcessed.store(0, std::memory_order_release);
		progressRemaining.store(queueAtStart, std::memory_order_release);
		progressRegistered.store(0, std::memory_order_release);
		progressNotRegistered.store(0, std::memory_order_release);
		progressTotalDlc.store(dlcAtStart, std::memory_order_release);
		progressNewDlc.store(0, std::memory_order_release);
		progressElapsedSeconds.store(0.0, std::memory_order_release);
		progressUpToDate.store(false, std::memory_order_release);
		progressCompleted.store(false, std::memory_order_release);
		progressActive.store(true, std::memory_order_release);

		LOG_INFO("(ENUMERATION) Scan start: queue=" << queueAtStart << " extra/tick=" << EffectiveMaxPerTick() << " loads/tick=" << maxLoadsPerTick
			<< " budget=" << EffectiveBudgetMs() << " ms, open files=" << OpenCrtFiles() << ", waiting loads=" << WaitingLoads() << std::endl);

		// A rescan against a complete snapshot pops unchanged entries unopened, so reading them ahead would be wasted.
		if (!(rescanSkip && lastSnapshotComplete))
			StartPrefetch(service);
		AssetLoadDrain::SetActive(true);

		// Samples the game thread for the whole scan when RSMods_profiling.txt is next to the game.
		samplerLabel = "enumeration_" + std::to_string(++scanCount);
		SamplingProfiler::Start(GetCurrentThreadId(), samplerLabel.c_str());
	}

	void Finish() {
		draining = false;
		SamplingProfiler::Stop(samplerLabel.c_str());
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

		progressProcessed.store(queueAtStart, std::memory_order_release);
		progressRemaining.store(0, std::memory_order_release);
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

		const double elapsed = MsSince(drainStartTicks);
		const double installPhase = queueEmptyTicks ? (queueEmptyTicks - drainStartTicks) / ticksPerMs : elapsed;
		LOG_INFO("(ENUMERATION) Scan done: " << queueAtStart << " queued, " << extraInstalls << " installed by the drain over " << drainTicks << " ticks, "
			<< elapsed << " ms (installs " << installPhase << " ms, then " << waitingAtQueueEmpty << " loads still waiting, " << extraLoads << " extra load ticks), "
			<< "packages total=" << total << " dlc=" << dlc << ", drained installs that did not register=" << unregisteredInstalls
			<< ", open files now=" << OpenCrtFiles() << " peak=" << peakOpenFiles << " ceiling pauses=" << ceilingPauses
			<< ", unchanged entries popped unopened=" << unchangedSkipped << ", shader scans skipped=" << shaderScansSkipped
			<< ", requests held back=" << droppedRequests << std::endl);
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

		unsigned int done = 0;
		const unsigned int perTick = EffectiveMaxPerTick();
		while (done < perTick && service[off_installing] && QueueLength(service) > 0) {
			if (MsSince(start) >= budget) {
				++window.budgetStops;
				break;
			}
			if (pioinfo && OpenCrtFiles() >= openFileCeiling) {
				++ceilingPauses;
				break;
			}
			if (SkipIfKnown(service))
				continue;
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
		}

		const unsigned int remaining = QueueLength(service);
		progressRemaining.store(remaining, std::memory_order_release);
		progressProcessed.store(queueAtStart >= remaining ? queueAtStart - remaining : 0, std::memory_order_release);
		progressElapsedSeconds.store(MsSince(drainStartTicks) / 1000.0, std::memory_order_release);
		unsigned int liveTotal = 0;
		unsigned int liveDlc = 0;
		CountPackages(liveTotal, liveDlc);
		progressTotalDlc.store(liveDlc, std::memory_order_release);

		window.drainMs += MsSince(start);
		const double windowMs = MsSince(lastProgressLogTicks);
		if (windowMs >= 1000.0) {
			lastProgressLogTicks = Now();
			const double seconds = windowMs / 1000.0;
			LOG_INFO("(ENUMERATION) Progress: installed " << (queueAtStart >= remaining ? queueAtStart - remaining : 0) << "/" << queueAtStart
				<< ", waiting loads " << WaitingLoads() + (IsLoadSlotBusy(service) ? 1 : 0) << ", open files " << OpenCrtFiles()
				<< ", " << MsSince(drainStartTicks) / 1000.0 << " s | last " << seconds << " s: " << window.ticks / seconds << " fps (longest frame "
				<< window.maxFrameMs << " ms), drain " << window.installs / seconds << " installs/s, " << (window.ticks ? window.drainMs / window.ticks : 0.0)
				<< " ms/tick of " << budget << " ms budget, " << (window.installs ? window.installMs / window.installs : 0.0) << " ms/install, "
				<< window.budgetStops << " budget stops" << (mainMenuSeen.load(std::memory_order_relaxed) ? ", in menus" : "") << std::endl);
			const long long lastTickStart = window.lastTickStart;
			window = ProgressWindow{};
			window.lastTickStart = lastTickStart;
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
		originalTick(self, edx);
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
