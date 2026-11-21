#include "stdafx.h"
#include "MemUtil.hpp"
#include "SettingEdits.hpp"
#include "Framework/ThreadExecutionPolicy.hpp"
#include "Framework/TrampolinePolicy.hpp"
#include "Lib/Detours/detours.h"
#include <algorithm>
#include <optional>
#include <TlHelp32.h>

namespace {
	struct BootPatch {
		uintptr_t address = 0;
		size_t length = 0;
		const char* member;
	};
	struct BootBundle {
		bool staging = false;
		Framework::BootPatch::Bundle bundle = Framework::BootPatch::Bundle::UltrafastCore;
		const char* member = "startup";
		const char* failed = nullptr;
		std::vector<BootPatch> patches;
		MemUtil::PreparedPatchSet transaction;
	};
	struct BootBundleGate {
		std::atomic<bool> enabled{ true };
	};
	thread_local BootBundle g_bootBundle;
	std::array<BootBundleGate, Framework::BootPatch::BundleCount> g_bootBundleGates;

	size_t BootBundleIndex(Framework::BootPatch::Bundle bundle) {
		return static_cast<size_t>(bundle);
	}

	bool IsPatchOverlap(uintptr_t address, size_t length, uintptr_t other, size_t otherLength) {
		if (length > (std::numeric_limits<uintptr_t>::max)() - address
			|| otherLength > (std::numeric_limits<uintptr_t>::max)() - other)
			return true;
		return address < other + otherLength && other < address + length;
	}

	bool StageBootPatch(const MemUtil::PreparedPatchRequest& request, size_t length) {
		if (g_bootBundle.failed)
			return false;
		const uintptr_t start = reinterpret_cast<uintptr_t>(request.address);
		for (const BootPatch& previous : g_bootBundle.patches) {
			if (IsPatchOverlap(start, length, previous.address, previous.length)) {
				MemUtil::FailBootPatchBundle();
				return false;
			}
		}
		if (g_bootBundle.transaction.Size() >= MemUtil::PreparedPatchSet::MaxSites) {
			LOG_ERROR("(BOOT BUNDLE) " << Framework::BootPatch::BundleName(g_bootBundle.bundle)
				<< " exceeded the " << MemUtil::PreparedPatchSet::MaxSites
				<< "-site limit while staging " << g_bootBundle.member << std::endl);
			MemUtil::FailBootPatchBundle();
			return false;
		}
		if (!g_bootBundle.transaction.Append(request)) {
			MemUtil::FailBootPatchBundle();
			return false;
		}
		g_bootBundle.patches.push_back({
			g_bootBundle.transaction.SiteAddress(g_bootBundle.transaction.Size() - 1),
			g_bootBundle.transaction.SiteLength(g_bootBundle.transaction.Size() - 1),
			g_bootBundle.member });
		return true;
	}

	bool QueryRegions(uintptr_t address, size_t length,
		std::vector<MemUtil::Detail::PatchTransactionRegion>& regions) {
		uintptr_t cursor = address;
		regions.clear();
		if (!address || !length || length > (std::numeric_limits<uintptr_t>::max)() - cursor)
			return false;
		const uintptr_t end = cursor + length;
		while (cursor < end) {
			MEMORY_BASIC_INFORMATION info{};
			if (!NT_SUCCESS(MemUtil::HookedQueryVirtualMemory(reinterpret_cast<LPVOID>(cursor), &info, sizeof(info)))
				|| info.State != MEM_COMMIT || info.RegionSize == 0)
				return false;
			const uintptr_t regionEnd = reinterpret_cast<uintptr_t>(info.BaseAddress) + info.RegionSize;
			if (regionEnd <= cursor)
				return false;
			const uintptr_t stop = regionEnd < end ? regionEnd : end;
			regions.push_back({ cursor, stop - cursor, info.Protect,
				reinterpret_cast<uintptr_t>(info.BaseAddress), info.RegionSize, false });
			cursor = stop;
		}
		return !regions.empty();
	}

	void AddTransactionRegion(std::vector<MemUtil::Detail::PatchTransactionRegion>& regions,
		const MemUtil::Detail::PatchTransactionRegion& candidate) {
		const uintptr_t candidateEnd = candidate.base + candidate.size;
		for (auto& region : regions) {
			if (region.queryBase != candidate.queryBase || region.querySize != candidate.querySize
				|| region.protection != candidate.protection)
				continue;
			const uintptr_t end = region.base + region.size;
			const uintptr_t mergedStart = region.base < candidate.base ? region.base : candidate.base;
			const uintptr_t mergedEnd = end > candidateEnd ? end : candidateEnd;
			region.base = mergedStart;
			region.size = mergedEnd - mergedStart;
			return;
		}
		regions.push_back(candidate);
	}

	void ReportStandalonePatch(const char* operation, uintptr_t address, MemUtil::PatchResult result,
		const MemUtil::PatchOutcome& outcome) {
		if (!outcome.protectionRestored)
			LOG_WARNING("(MEMORY) Page protection at 0x" << std::hex << address << std::dec
				<< " stayed writable after a patch" << std::endl);
		if (result == MemUtil::PatchResult::RollbackIncomplete)
			LOG_ERROR("(MEMORY) " << operation << " at 0x" << std::hex << address << std::dec
				<< " could not be confirmed; rollback may be incomplete" << std::endl);
	}
}

void MemUtil::BeginBootPatchBundle(Framework::BootPatch::Bundle bundle) {
	g_bootBundle.transaction.Reset();
	g_bootBundle.patches.clear();
	g_bootBundle.failed = nullptr;
	g_bootBundle.member = "startup";
	g_bootBundle.bundle = bundle;
	g_bootBundle.staging = true;
	g_bootBundleGates[BootBundleIndex(bundle)].enabled.store(false);
}

bool MemUtil::BootPatchMember(Framework::BootPatch::Member member) {
	g_bootBundle.member = Framework::BootPatch::MemberName(member);
	if (Framework::BootPatch::BundleFor(member) != g_bootBundle.bundle) {
		FailBootPatchBundle();
		return false;
	}
	return !g_bootBundle.failed;
}

void MemUtil::FailBootPatchBundle() {
	if (g_bootBundle.staging && !g_bootBundle.failed)
		g_bootBundle.failed = g_bootBundle.member;
}

bool MemUtil::BootPatchBundleEnabled(Framework::BootPatch::Bundle bundle) {
	return g_bootBundleGates[BootBundleIndex(bundle)].enabled.load();
}

bool MemUtil::BootPatchBundleStaging(Framework::BootPatch::Bundle bundle) {
	return g_bootBundle.staging && g_bootBundle.bundle == bundle;
}

bool MemUtil::BootPatchStaged(uintptr_t address) {
	for (const BootPatch& patch : g_bootBundle.patches) {
		if (address >= patch.address && address - patch.address < patch.length)
			return true;
	}
	return false;
}

bool MemUtil::CommitBootPatchBundle() {
	const Framework::BootPatch::Bundle bundle = g_bootBundle.bundle;
	g_bootBundle.staging = false;
	if (!g_bootBundle.failed && g_bootBundle.patches.empty())
		g_bootBundle.transaction.Prepare({});
	if (!g_bootBundle.failed && g_bootBundle.transaction.Commit() != PatchResult::Installed) {
		g_bootBundle.failed = g_bootBundle.member;
		if (g_bootBundle.transaction.ExclusionFailed())
			g_bootBundle.transaction.Report(Framework::BootPatch::BundleName(bundle));
	}
	const PatchOutcome outcome = g_bootBundle.transaction.Outcome();
	if (!g_bootBundle.failed && (outcome.bytes != PatchByteState::Replacement
		|| !outcome.instructionCacheFlushed || !outcome.protectionRestored))
		g_bootBundle.failed = g_bootBundle.member;
	if (g_bootBundle.failed && g_bootBundle.transaction.FailureAddress()) {
		for (const BootPatch& patch : g_bootBundle.patches) {
			if (g_bootBundle.transaction.FailureAddress() >= patch.address
				&& g_bootBundle.transaction.FailureAddress() - patch.address < patch.length) {
				g_bootBundle.failed = patch.member;
				break;
			}
		}
	}
	const bool restored = outcome.protectionRestored
		&& outcome.bytes != PatchByteState::Unknown && outcome.instructionCacheFlushed;
	if (g_bootBundle.failed && outcome.bytes == PatchByteState::Replacement
		&& !outcome.protectionRestored) {
		LOG_WARNING("(BOOT BUNDLE) " << Framework::BootPatch::BundleName(bundle)
			<< " installed, but page protection was not restored at 0x" << std::hex
			<< g_bootBundle.transaction.FailureAddress() << std::dec << "; gate off; features off: "
			<< Framework::BootPatch::DisabledFeatures(bundle) << std::endl);
	} else if (g_bootBundle.failed) {
		LOG_WARNING("(BOOT BUNDLE) " << Framework::BootPatch::BundleName(bundle) << " failed member: "
			<< g_bootBundle.failed << "; features off: " << Framework::BootPatch::DisabledFeatures(bundle)
			<< "; rollback=" << (restored ? "complete" : "incomplete") << std::endl);
	} else {
		g_bootBundleGates[BootBundleIndex(bundle)].enabled.store(true);
		LOG_INFO("(BOOT BUNDLE) " << Framework::BootPatch::BundleName(bundle) << " committed" << std::endl);
	}
	g_bootBundle.patches.clear();
	g_bootBundle.transaction.Reset();
	return g_bootBundleGates[BootBundleIndex(bundle)].enabled.load();
}

/// <summary>
/// Compares memory chunk to pattern. 
/// </summary>
/// <param name="pData"> - Data in memory.</param>
/// <param name="bMask"> - Pattern to look for.</param>
/// <param name="szMask"> - Mask of what bytes we know (notated with an "x") and what bytes we don't (notated with a "?").</param>
/// <returns></returns>
bool MemUtil::bCompare(const BYTE* pData, const byte* bMask, const char* szMask) {
	for (; *szMask; ++szMask, ++pData, ++bMask) {
		if (*szMask == 'x' && *pData != *bMask)
			return 0;
	}

	return (*szMask) == NULL;
}

bool MemUtil::PatchAdr(uintptr_t address, LPCVOID changeToMake, size_t len, bool addBaseHandle) {
	uintptr_t addr = address + (addBaseHandle ? Offsets::baseHandle : 0);
	return PatchAdr((LPVOID)(addr), changeToMake, len);
}

bool MemUtil::PatchAdr(VersioningStruct<uintptr_t>& address, LPCVOID changeToMake, size_t len, bool addBaseHandle) {
	uintptr_t addr = address.Get() + (addBaseHandle ? Offsets::baseHandle : 0);
	return PatchAdr((LPVOID)(addr), changeToMake, len);

}

bool MemUtil::PatchAdr(uintptr_t address, std::string_view data, bool addBaseHandle) {
	return PatchAdr(address, static_cast<LPCVOID>(data.data()), data.size(), addBaseHandle);
}

/// <summary>
/// Write x86 ASM (HEX) to address.
/// </summary>
/// <param name="address"> - Pointer you want to edit</param>
/// <param name="changeToMake"> - Edit you want to make</param>
/// <param name="len"> - How long is the edit</param>
/// <returns></returns>
bool MemUtil::PatchAdr(LPVOID address, LPCVOID changeToMake, size_t len) {
	return WritePatch(address, changeToMake, len) == PatchResult::Installed;
}

MemUtil::PatchResult MemUtil::WritePatch(LPVOID address, LPCVOID changeToMake, size_t len) {
	PreparedPatchRequest request{};
	request.address = address;
	request.kind = PreparedPatchKind::Bytes;
	request.replacementBytes = std::span<const unsigned char>(
		static_cast<const unsigned char*>(changeToMake), len);
	request.length = len;
	if (g_bootBundle.staging)
		return StageBootPatch(request, len) ? PatchResult::Installed : PatchResult::Original;
	PreparedPatchSet transaction;
	if (!transaction.Prepare(std::span<const PreparedPatchRequest>(&request, 1)))
		return ToPatchResult(transaction.Outcome());
	const PatchResult result = transaction.Commit();
	ReportStandalonePatch("Patch", reinterpret_cast<uintptr_t>(address), result, transaction.Outcome());
	return result;
}

MemUtil::PatchResult MemUtil::WritePointerPatch(LPVOID address, LPCVOID value) {
	PreparedPatchRequest request{};
	request.address = address;
	request.kind = PreparedPatchKind::PointerStore;
	request.replacementBytes = std::span<const unsigned char>(
		static_cast<const unsigned char*>(value), sizeof(void*));
	request.length = sizeof(void*);
	if (g_bootBundle.staging)
		return StageBootPatch(request, sizeof(void*)) ? PatchResult::Installed : PatchResult::Original;
	PreparedPatchSet transaction;
	if (!transaction.Prepare(std::span<const PreparedPatchRequest>(&request, 1)))
		return ToPatchResult(transaction.Outcome());
	const PatchResult result = transaction.Commit();
	ReportStandalonePatch("Pointer patch", reinterpret_cast<uintptr_t>(address), result,
		transaction.Outcome());
	return result;
}

bool MemUtil::PlaceHook(VersioningStruct<uintptr_t>& hookSpot, void* ourFunct, int len, bool addBaseHandle) {
	uintptr_t addr = hookSpot.Get() + (addBaseHandle ? Offsets::baseHandle : 0);
	return PlaceHook((void*)(addr), ourFunct, len);
}

/// <summary>
/// Leave an x86 ASM hook by jumping to a versioned address. Every register and flag is kept as it was.
/// Usage, at the end of a naked hook:
///		push offset Offsets::ptr_SomethingJmpBck
///		jmp MemUtil::JumpToVersioned
/// The address only lives on this thread's stack, so hooks running on different threads can't send each other to the wrong place.
/// </summary>
void __declspec(naked) MemUtil::JumpToVersioned() {
	__asm {
		pushfd
		pushad
		mov ecx, [esp + 36]							// The VersioningStruct the hook pushed (above 8 registers and the flags)
		call VersioningStruct<uintptr_t>::GetValue
		mov [esp + 36], eax							// Replace it with the address it resolves to
		popad
		popfd
		ret											// Jump there
	}
}

/// <summary>
/// Place x86 ASM (__asm) hook
/// </summary>
/// <param name="hookSpot"> - Where should we hook?</param>
/// <param name="ourFunct"> - What should we run (x86 ASM)</param>
/// <param name="len"> - How long is the first command (x86 ASM length)</param>
/// <returns>Can we place the hook?</returns>
bool MemUtil::PlaceHook(void* hookSpot, void* ourFunct, int len) {
	if (len < 5) {
		FailBootPatchBundle();
		return false;
	}
	PreparedPatchRequest request{};
	request.address = hookSpot;
	request.kind = PreparedPatchKind::Jump;
	request.replacementTarget = reinterpret_cast<uintptr_t>(ourFunct);
	request.length = static_cast<size_t>(len);
	if (g_bootBundle.staging)
		return StageBootPatch(request, request.length);
	PreparedPatchSet transaction;
	if (!transaction.Prepare(std::span<const PreparedPatchRequest>(&request, 1)))
		return ToPatchResult(transaction.Outcome()) == PatchResult::Installed;
	const PatchResult result = transaction.Commit();
	ReportStandalonePatch("Hook", reinterpret_cast<uintptr_t>(hookSpot), result, transaction.Outcome());
	return result == PatchResult::Installed;
}

/// <summary>
/// Get the vtable of a C++ / COM object.
/// </summary>
void** MemUtil::GetVTable(void* object)
{
	return *reinterpret_cast<void***>(object);
}

/// <summary>
/// Redirect one vtable slot. Every instance of the class shares the vtable, including ones created before the patch.
/// Read the original out of the slot first if you need to call it.
/// </summary>
/// <param name="vtable"> - Vtable to patch (see GetVTable)</param>
/// <param name="slot"> - Method index</param>
/// <param name="replacement"> - Function to call instead</param>
/// <returns>Was the slot patched?</returns>
bool MemUtil::PatchVTableSlot(void** vtable, size_t slot, void* replacement)
{
	return SetStaticValue(reinterpret_cast<uintptr_t>(&vtable[slot]), replacement, sizeof(void*));
}

bool MemUtil::VTablePatcher::Patch(void** vtable, size_t slot, void* replacement)
{
	const size_t n = count.load(std::memory_order_relaxed);

	bool known = false;
	for (size_t i = 0; i < n; ++i)
	{
		if (entries[i].vtable == vtable && entries[i].slot == slot)
			known = true;
	}

	if (!known)
	{
		if (n == entries.size())
			return false;

		// Publish the original before writing the slot, so a thread entering the hook mid-patch finds it.
		entries[n] = { vtable, slot, vtable[slot] };
		count.store(n + 1, std::memory_order_release);
	}

	return PatchVTableSlot(vtable, slot, replacement);
}

void* MemUtil::VTablePatcher::Original(void* object, size_t slot) const
{
	void** vtable = GetVTable(object);
	const size_t n = count.load(std::memory_order_acquire);

	for (size_t i = 0; i < n; ++i)
	{
		if (entries[i].vtable == vtable && entries[i].slot == slot)
			return entries[i].original;
	}

	return nullptr;
}

void MemUtil::VTablePatcher::RestoreAll()
{
	const size_t n = count.load(std::memory_order_relaxed);

	for (size_t i = 0; i < n; ++i)
	{
		PatchVTableSlot(entries[i].vtable, entries[i].slot, entries[i].original);
	}
}

/// <summary>
/// Hook DirectX Functions
/// </summary>
/// <param name="src"> - Where should we hook?</param>
/// <param name="dst"> - What should we run (x86 ASM)</param>
/// <param name="len"> - How long is the first command (x86 ASM length)</param>
/// <returns></returns>
PBYTE MemUtil::TrampHook(PBYTE src, PBYTE dst, unsigned int len, PatchResult* result,
	uintptr_t* original)
{
	if (result)
		*result = PatchResult::Original;
	if (!src || !dst || len < 5)
	{
		FailBootPatchBundle();
		return nullptr;
	}
	PreparedPatchRequest request{};
	request.address = src;
	request.kind = PreparedPatchKind::Jump;
	request.replacementTarget = reinterpret_cast<uintptr_t>(dst);
	request.captureNativeEntry = true;
	// Callers pass checked prologue spans, which end on a boundary; vendor entries (D3D) are not
	// known in advance, so the span extends to the first boundary at or after len.
	request.minimumLength = len;
	if (g_bootBundle.staging) {
		if (!StageBootPatch(request, len))
			return nullptr;
		const size_t site = g_bootBundle.transaction.Size() - 1;
		PBYTE gateway = static_cast<PBYTE>(g_bootBundle.transaction.sites_[site].nativeEntry);
		if (!gateway) {
			FailBootPatchBundle();
			return nullptr;
		}
		g_bootBundle.transaction.sites_[site].nativeEntryExposed = true;
		if (original)
			*original = reinterpret_cast<uintptr_t>(gateway);
		if (result)
			*result = PatchResult::Installed;
		return gateway;
	}

	PreparedPatchSet transaction;
	if (!transaction.Prepare(std::span<const PreparedPatchRequest>(&request, 1))) {
		FailBootPatchBundle();
		return nullptr;
	}
	PBYTE gateway = static_cast<PBYTE>(transaction.sites_[0].nativeEntry);
	if (original) {
		*original = reinterpret_cast<uintptr_t>(gateway);
		transaction.sites_[0].nativeEntryExposed = true;
	}
	const PatchResult outcome = transaction.Commit();
	if (!transaction.Outcome().protectionRestored)
		LOG_WARNING("(MEMORY) Page protection at 0x" << std::hex << reinterpret_cast<uintptr_t>(src)
			<< std::dec << " stayed writable after a patch" << std::endl);
	if (result)
		*result = outcome;
	if (outcome == PatchResult::Installed) {
		transaction.sites_[0].nativeEntryExposed = true;
		return gateway;
	}
	FailBootPatchBundle();
	if (outcome == PatchResult::RollbackIncomplete) {
		transaction.sites_[0].nativeEntryExposed = true;
		LOG_ERROR("(MEMORY) Hook at 0x" << std::hex << reinterpret_cast<uintptr_t>(src) << std::dec
			<< " may still be live; its trampoline is kept" << std::endl);
		return gateway;
	}
	return nullptr;
}

/// <summary>
/// Will reading this pointer kill the game?
/// </summary>
/// <param name="p"> - Pointer</param>
/// <returns>True - Bad Pointer, do not read. False - Safe to read.</returns>
bool MemUtil::IsBadReadPtr(const void* pointer)
{
	//NOTE: We are very aware this is not exactly the optimal (neither completely thread safe nor very fast) way to handle pointers to a non-initialized variable.
	//      but for now it will have to do the job until we figure out a better current menu check.

	MEMORY_BASIC_INFORMATION mbi = { 0 };
	if (NT_SUCCESS(HookedQueryVirtualMemory(const_cast<LPVOID>(pointer), &mbi, sizeof(mbi))))
	{
		uint32_t mask = (PAGE_READONLY | PAGE_READWRITE | PAGE_WRITECOPY | PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE | PAGE_EXECUTE_WRITECOPY);
		bool badPtr = !(mbi.Protect & mask);

		// Check the page is not a guard page
		if (mbi.Protect & (PAGE_GUARD | PAGE_NOACCESS)) badPtr = true;

		return badPtr;
	}
	return true;
}

bool MemUtil::MatchesBytes(uintptr_t address, std::span<const unsigned char> expected) {
	for (size_t i = 0; i < expected.size(); ++i) {
		unsigned char actual = 0;
		if (!TryRead(address + i, actual) || actual != expected[i])
			return false;
	}

	return true;
}

/// <summary>
/// Get memory address from base pointer and offsets (often used with values found in Cheat Engine).
/// </summary>
/// <param name="ptr"> - Memory Pointer</param>
/// <param name="offsets"> - Cheat Engine Offsets</param>
/// <param name="safe"> - Should we trust this to not crash our game?</param>
/// <returns>Memory Address</returns>
uintptr_t MemUtil::FindDMAAddy(uintptr_t ptr, std::span<const unsigned int> offsets, bool safe)
{
	// Set addr to the base pointer.
	uintptr_t addr = ptr;

	for (const auto& offset : offsets)
	{
		if (safe && IsBadReadPtr((void*)addr))
			return 0;

		if (!addr)
			return 0;

		addr = *(uintptr_t*)addr;

		if (!addr)
			return 0;

		addr += offset;
	}

	// The loop validates each address before stepping through it, which leaves the
	// final address, the one the caller actually dereferences, unchecked.
	if (safe && IsBadReadPtr((void*)addr))
		return 0;

	return addr;
}

/// <summary>
/// Read Pointer
/// </summary>
/// <param name="adr"> - Pointer</param>
/// <returns>Memory Address</returns>
uintptr_t MemUtil::ReadPtr(uintptr_t adr) {
	if (adr == NULL)
		return NULL;

	return *(uintptr_t*)adr;
}

/// <summary>
/// Are we running under Wine / Proton rather than on Windows?
/// </summary>
/// <returns>True if the host is Wine.</returns>
namespace {
	const char* DetectHost() {
		const HMODULE ntdll = GetModuleHandleW(L"ntdll.dll");
		if (ntdll) {
			using GetHostVersion = void (__cdecl*)(const char**, const char**);
			const auto getHostVersion = reinterpret_cast<GetHostVersion>(
				GetProcAddress(ntdll, "wine_get_host_version"));
			if (getHostVersion) {
				const char* sysname = nullptr;
				const char* release = nullptr;
				getHostVersion(&sysname, &release);
				if (sysname && strcmp(sysname, "Linux") == 0) return "Wine-Linux";
				if (sysname && strcmp(sysname, "Darwin") == 0) return "Wine-macOS";
				return "Wine";
			}
			for (const char* name : { "wine_get_version", "wine_get_build_id" })
				if (GetProcAddress(ntdll, name)) return "Wine";
		}
		// No registry probe: this runs under the loader lock on every boot. A Wine build that hides
		// its ntdll exports is covered by [Platform] HostOverride = wine.
		return "Windows";
	}

	struct HostPolicy {
		const char* detected = DetectHost();
		std::string overrideValue;
		std::string savedHost;
		bool useWineApis = false;

		HostPolicy() {
			// Memory patches start before the main settings load.
			CSimpleIniA reader;
			reader.LoadFile("RSMods.ini");
			overrideValue = reader.GetValue("Platform", "HostOverride", "auto");
			if (overrideValue != "windows" && overrideValue != "wine") overrideValue = "auto";
			savedHost = reader.GetValue("Platform", "DetectedHost", "");
			useWineApis = overrideValue == "wine"
				|| strcmp(detected, "Windows") != 0;
		}
	};

	HostPolicy& MemoryHostPolicy() {
		static HostPolicy policy;
		return policy;
	}
}

const char* MemUtil::DetectedHost() {
	return MemoryHostPolicy().detected;
}

bool MemUtil::IsRunningOnWine() {
	return strcmp(DetectedHost(), "Windows") != 0;
}

void MemUtil::LogMemoryProtectionPolicy() {
	auto& policy = MemoryHostPolicy();
	if (policy.savedHost != policy.detected) {
		SettingEdits::Apply({ Settings::Setting::DetectedHost, "Platform", "DetectedHost",
			policy.detected, "Detected host", false });
		policy.savedHost = policy.detected;
	}
	LOG_INFO("(MEMORY) Detected host=" << policy.detected << "; HostOverride="
		<< policy.overrideValue << "; path="
		<< (policy.useWineApis ? "documented only" : "syscall first, documented on failure") << std::endl);
}

namespace {
	std::atomic_flag g_loggedUnresolvedSyscall = ATOMIC_FLAG_INIT;
	std::atomic_flag g_loggedSyscallFallback = ATOMIC_FLAG_INIT;

	void LogMemoryCall(const char* operation, void* address, SIZE_T length,
		bool hasProtection, ULONG protection, const char* path, NTSTATUS status,
		DWORD error, ULONGLONG start) {
		const ULONGLONG ms = GetTickCount64() - start;
		if (ms >= 50 || !NT_SUCCESS(status)) {
			if (hasProtection)
				LOG_WARNING("(MEMORY) " << operation << " address=0x" << std::hex
					<< reinterpret_cast<uintptr_t>(address) << " length=0x" << length
					<< " protection=0x" << protection << " path=" << path
					<< " status=0x" << status << " win32=" << error << std::dec
					<< " ms=" << ms << std::endl);
			else
				LOG_WARNING("(MEMORY) " << operation << " address=0x" << std::hex
					<< reinterpret_cast<uintptr_t>(address) << " length=0x" << length
					<< " protection=n/a path=" << path
					<< " status=0x" << status << " win32=" << error << std::dec
					<< " ms=" << ms << std::endl);
		}
	}

	void LogUnresolvedSyscallOnce(const char* operation, void* address, SIZE_T length,
		bool hasProtection, ULONG protection, ULONGLONG start) {
		if (g_loggedUnresolvedSyscall.test_and_set(std::memory_order_relaxed))
			return;
		const ULONGLONG ms = GetTickCount64() - start;
		if (hasProtection)
			LOG_WARNING("(MEMORY) " << operation << " address=0x" << std::hex
				<< reinterpret_cast<uintptr_t>(address) << " length=0x" << length
				<< " protection=0x" << protection << std::dec
				<< " path=documented reason=syscall number unresolved status=unresolved win32=0 ms="
				<< ms << std::endl);
		else
			LOG_WARNING("(MEMORY) " << operation << " address=0x" << std::hex
				<< reinterpret_cast<uintptr_t>(address) << " length=0x" << length
				<< " protection=n/a path=documented reason=syscall number unresolved status=unresolved win32=0 ms="
				<< ms << std::endl);
	}

	void LogSyscallFailureOnce(const char* operation, void* address, SIZE_T length,
		bool hasProtection, ULONG protection, NTSTATUS status, ULONGLONG start) {
		if (g_loggedSyscallFallback.test_and_set(std::memory_order_relaxed))
			return;
		LogMemoryCall(operation, address, length, hasProtection, protection, "syscall", status, 0, start);
	}
}

namespace MemUtil {
	namespace Detail {
		static uint32_t ssnProtect = 0;
		static uint32_t ssnQuery = 0;

		static std::vector<BYTE> ReadNtdllFromDisk() {
			char sysDir[MAX_PATH];
			if (!GetSystemDirectoryA(sysDir, MAX_PATH)) return {};

			std::string ntdllPath = std::string(sysDir) + "\\ntdll.dll";
			HANDLE hFile = CreateFileA(ntdllPath.c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL, OPEN_EXISTING, 0, NULL);
			if (hFile == INVALID_HANDLE_VALUE) return {};

			DWORD fileSize = GetFileSize(hFile, NULL);
			if (fileSize == 0 || fileSize > 50 * 1024 * 1024) {
				CloseHandle(hFile);
				return {};
			}

			std::vector<BYTE> buf(fileSize);
			DWORD bytesRead = 0;
			if (!ReadFile(hFile, buf.data(), fileSize, &bytesRead, NULL) || bytesRead != fileSize) {
				CloseHandle(hFile);
				return {};
			}
			CloseHandle(hFile);
			return buf;
		}

		static std::optional<uint32_t> ExtractSyscallFromPE(const std::vector<BYTE>& buf,
			const char* funcName) {
			auto HasRange = [&](size_t offset, size_t length) {
				return offset <= buf.size() && length <= buf.size() - offset;
			};
			if (!HasRange(0, sizeof(IMAGE_DOS_HEADER)))
				return std::nullopt;

			IMAGE_DOS_HEADER dos{};
			memcpy(&dos, buf.data(), sizeof(dos));
			if (dos.e_magic != IMAGE_DOS_SIGNATURE || dos.e_lfanew < 0)
				return std::nullopt;
			const size_t ntOffset = static_cast<size_t>(dos.e_lfanew);
			if (!HasRange(ntOffset, sizeof(DWORD) + sizeof(IMAGE_FILE_HEADER)))
				return std::nullopt;

			DWORD signature = 0;
			memcpy(&signature, buf.data() + ntOffset, sizeof(signature));
			if (signature != IMAGE_NT_SIGNATURE)
				return std::nullopt;
			IMAGE_FILE_HEADER fileHeader{};
			memcpy(&fileHeader, buf.data() + ntOffset + sizeof(signature), sizeof(fileHeader));
			if (fileHeader.Machine != IMAGE_FILE_MACHINE_I386 || fileHeader.NumberOfSections == 0)
				return std::nullopt;

			const size_t optionalOffset = ntOffset + sizeof(signature) + sizeof(fileHeader);
			constexpr size_t exportHeaderSize = offsetof(IMAGE_OPTIONAL_HEADER32, DataDirectory)
				+ (IMAGE_DIRECTORY_ENTRY_EXPORT + 1) * sizeof(IMAGE_DATA_DIRECTORY);
			if (fileHeader.SizeOfOptionalHeader < exportHeaderSize
				|| !HasRange(optionalOffset, fileHeader.SizeOfOptionalHeader))
				return std::nullopt;
			IMAGE_OPTIONAL_HEADER32 optional{};
			memcpy(&optional, buf.data() + optionalOffset,
				fileHeader.SizeOfOptionalHeader < sizeof(optional)
					? fileHeader.SizeOfOptionalHeader : sizeof(optional));
			if (optional.Magic != IMAGE_NT_OPTIONAL_HDR32_MAGIC
				|| optional.NumberOfRvaAndSizes <= IMAGE_DIRECTORY_ENTRY_EXPORT
				|| optional.SizeOfHeaders > buf.size())
				return std::nullopt;

			const size_t sectionsOffset = optionalOffset + fileHeader.SizeOfOptionalHeader;
			if (fileHeader.NumberOfSections > (std::numeric_limits<size_t>::max)()
				/ sizeof(IMAGE_SECTION_HEADER))
				return std::nullopt;
			const size_t sectionsSize = static_cast<size_t>(fileHeader.NumberOfSections)
				* sizeof(IMAGE_SECTION_HEADER);
			if (!HasRange(sectionsOffset, sectionsSize))
				return std::nullopt;

			auto RvaToRaw = [&](DWORD rva, size_t length, size_t& offset, size_t& available) {
				if (rva < optional.SizeOfHeaders) {
					offset = rva;
					available = optional.SizeOfHeaders - rva;
					if (offset > buf.size())
						return false;
					const size_t fileAvailable = buf.size() - offset;
					if (available > fileAvailable)
						available = fileAvailable;
					return length <= available;
				}
				for (WORD i = 0; i < fileHeader.NumberOfSections; ++i) {
					IMAGE_SECTION_HEADER section{};
					memcpy(&section, buf.data() + sectionsOffset + i * sizeof(section), sizeof(section));
					if (rva < section.VirtualAddress)
						continue;
					const DWORD delta = rva - section.VirtualAddress;
					if (delta >= section.SizeOfRawData)
						continue;
					if (length > section.SizeOfRawData - delta
						|| section.PointerToRawData > buf.size()
						|| delta > buf.size() - section.PointerToRawData)
						return false;
					offset = section.PointerToRawData + delta;
					available = section.SizeOfRawData - delta;
					const size_t fileAvailable = buf.size() - offset;
					if (available > fileAvailable)
						available = fileAvailable;
					return length <= available;
				}
				return false;
			};

			const IMAGE_DATA_DIRECTORY& exportDirectory =
				optional.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT];
			size_t exportOffset = 0, available = 0;
			if (!exportDirectory.VirtualAddress || exportDirectory.Size < sizeof(IMAGE_EXPORT_DIRECTORY)
				|| !RvaToRaw(exportDirectory.VirtualAddress, sizeof(IMAGE_EXPORT_DIRECTORY),
					exportOffset, available))
				return std::nullopt;
			IMAGE_EXPORT_DIRECTORY exports{};
			memcpy(&exports, buf.data() + exportOffset, sizeof(exports));
			if (!exports.NumberOfNames || !exports.NumberOfFunctions)
				return std::nullopt;

			auto ArrayOffset = [&](DWORD rva, DWORD count, size_t elementSize, size_t& offset) {
				if (count > (std::numeric_limits<size_t>::max)() / elementSize)
					return false;
				return RvaToRaw(rva, static_cast<size_t>(count) * elementSize, offset, available);
			};
			size_t namesOffset = 0, ordinalsOffset = 0, functionsOffset = 0;
			if (!ArrayOffset(exports.AddressOfNames, exports.NumberOfNames, sizeof(DWORD), namesOffset)
				|| !ArrayOffset(exports.AddressOfNameOrdinals, exports.NumberOfNames, sizeof(WORD), ordinalsOffset)
				|| !ArrayOffset(exports.AddressOfFunctions, exports.NumberOfFunctions, sizeof(DWORD), functionsOffset))
				return std::nullopt;

			const size_t wantedNameLength = strlen(funcName);
			const uint64_t exportEnd = static_cast<uint64_t>(exportDirectory.VirtualAddress)
				+ exportDirectory.Size;
			for (DWORD i = 0; i < exports.NumberOfNames; ++i) {
				DWORD nameRva = 0;
				WORD ordinal = 0;
				memcpy(&nameRva, buf.data() + namesOffset + static_cast<size_t>(i) * sizeof(nameRva),
					sizeof(nameRva));
				memcpy(&ordinal, buf.data() + ordinalsOffset + static_cast<size_t>(i) * sizeof(ordinal),
					sizeof(ordinal));
				if (ordinal >= exports.NumberOfFunctions)
					return std::nullopt;
				size_t nameOffset = 0, nameAvailable = 0;
				if (!RvaToRaw(nameRva, 1, nameOffset, nameAvailable))
					return std::nullopt;
				const BYTE* name = buf.data() + nameOffset;
				const BYTE* end = static_cast<const BYTE*>(memchr(name, 0, nameAvailable));
				if (!end)
					return std::nullopt;
				if (static_cast<size_t>(end - name) != wantedNameLength
					|| memcmp(name, funcName, wantedNameLength) != 0)
					continue;

				DWORD functionRva = 0;
				memcpy(&functionRva, buf.data() + functionsOffset
					+ static_cast<size_t>(ordinal) * sizeof(functionRva), sizeof(functionRva));
				if (!functionRva || (functionRva >= exportDirectory.VirtualAddress
					&& static_cast<uint64_t>(functionRva) < exportEnd))
					return std::nullopt;
				size_t codeOffset = 0, codeAvailable = 0;
				if (!RvaToRaw(functionRva, sizeof(BYTE) + sizeof(uint32_t), codeOffset, codeAvailable))
					return std::nullopt;
				if (buf[codeOffset] != 0xB8)
					return std::nullopt;
				uint32_t number = 0;
				memcpy(&number, buf.data() + codeOffset + 1, sizeof(number));
				return number;
			}
			return std::nullopt;
		}

		static std::optional<uint32_t> GetSyscallNumber(const char* funcName) {
			HMODULE hNtdll = GetModuleHandleA("ntdll.dll");
			if (hNtdll) {
				BYTE* pFunc = reinterpret_cast<BYTE*>(GetProcAddress(hNtdll, funcName));
				BYTE opcode = 0;
				uint32_t number = 0;
				if (pFunc && MemUtil::TryRead(reinterpret_cast<uintptr_t>(pFunc), opcode)
					&& opcode == 0xB8
					&& MemUtil::TryRead(reinterpret_cast<uintptr_t>(pFunc + 1), number)) {
					return number;
				}
			}

			// If memory was modified/hooked by VMProtect, parse fresh unhooked ntdll from disk
			static const std::vector<BYTE> ntdllDiskBuf = ReadNtdllFromDisk();
			return ExtractSyscallFromPE(ntdllDiskBuf, funcName);
		}

		using NtGetNextThreadFunction = NTSTATUS(NTAPI*)(HANDLE, HANDLE, ACCESS_MASK,
			ULONG, ULONG, PHANDLE);
		static uint32_t ssnNextThread = 0;
		static NtGetNextThreadFunction ntGetNextThread = nullptr;
		static bool useNextThreadSyscall = false;

		static __declspec(naked) NTSTATUS NTAPI Syscall_NtProtectVirtualMemory(HANDLE, PVOID*, PSIZE_T, ULONG, PULONG)
		{
			__asm {
				mov eax, ssnProtect
				mov edx, fs: [0C0h]
				test edx, edx
				jnz is_wow64
				mov edx, 7FFE0300h
				call dword ptr[edx]
				ret 14h
				is_wow64 :
				call edx
					ret 14h
			}
		}

		static __declspec(naked) NTSTATUS NTAPI Syscall_NtQueryVirtualMemory(HANDLE, PVOID, ULONG, PVOID, SIZE_T, PSIZE_T)
		{
			__asm {
				mov eax, ssnQuery
				mov edx, fs: [0C0h]
				test edx, edx
				jnz is_wow64
				mov edx, 7FFE0300h
				call dword ptr[edx]
				ret 18h
				is_wow64 :
				call edx
					ret 18h
			}
		}

		static __declspec(naked) NTSTATUS NTAPI Syscall_NtGetNextThread(HANDLE, HANDLE, ACCESS_MASK,
			ULONG, ULONG, PHANDLE) {
			__asm {
				mov eax, ssnNextThread
				mov edx, fs: [0C0h]
				test edx, edx
				jnz is_wow64
				mov edx, 7FFE0300h
				call dword ptr[edx]
				ret 18h
			is_wow64:
				call edx
				ret 18h
			}
		}

		static bool ResolveNextThreadDispatch() {
			static const bool resolved = [] {
				HMODULE ntdll = GetModuleHandleA("ntdll.dll");
				if (!ntdll)
					return false;
				auto exported = reinterpret_cast<NtGetNextThreadFunction>(
					GetProcAddress(ntdll, "NtGetNextThread"));
				if (!exported)
					return false;
				if (MemoryHostPolicy().useWineApis) {
					ntGetNextThread = exported;
					return true;
				}
				const std::optional<uint32_t> number = GetSyscallNumber("NtGetNextThread");
				if (number) {
					ssnNextThread = *number;
					useNextThreadSyscall = true;
				}
				else {
					ntGetNextThread = exported;
				}
				return true;
			}();
			return resolved;
		}

		static NTSTATUS GetNextThread(HANDLE process, HANDLE previous, PHANDLE next) noexcept {
			if (useNextThreadSyscall)
				return Syscall_NtGetNextThread(process, previous, THREAD_QUERY_LIMITED_INFORMATION | SYNCHRONIZE,
					0, 0, next);
			return ntGetNextThread(process, previous, THREAD_QUERY_LIMITED_INFORMATION | SYNCHRONIZE,
				0, 0, next);
		}

#ifdef MEMUTIL_TESTING
		static void(__cdecl* g_executionSyncTestBeforeSuspend)() = nullptr;
		static void(__cdecl* g_executionSyncTestWhileSuspended)() = nullptr;
		static unsigned int g_executionSyncTestUntrackedThreads = 0;
#endif

		enum class ExclusionFailure { None, Snapshot, TooManyThreads, OpenLiveThread,
			SuspendLiveThread, Recheck, NewThread, Context, InPatchSpan };

		const char* ExclusionFailureName(ExclusionFailure failure) noexcept {
			switch (failure) {
			case ExclusionFailure::Snapshot: return "thread snapshot failed";
			case ExclusionFailure::TooManyThreads: return "thread count exceeded the fixed capacity";
			case ExclusionFailure::OpenLiveThread: return "could not open a live thread";
			case ExclusionFailure::SuspendLiveThread: return "could not suspend a live thread";
			case ExclusionFailure::Recheck: return "could not re-check threads while suspended";
			case ExclusionFailure::NewThread: return "a thread started after the snapshot";
			case ExclusionFailure::Context: return "could not read a suspended thread context";
			case ExclusionFailure::InPatchSpan: return "a thread was stopped inside a patched span";
			case ExclusionFailure::None: break;
			}
			return "unknown";
		}

		struct ExclusionThread {
			DWORD id = 0;
			HANDLE handle = nullptr;
			bool suspended = false;
		};
		constexpr size_t maxExclusionThreads = 4096;

		// Exclusions are serialized, so one fixed store serves all of them instead of each caller's stack.
		// Two writers suspending each other would stop both.
		struct ExclusionStorage {
			std::array<ExclusionThread, maxExclusionThreads> threads{};
			std::array<DWORD, maxExclusionThreads> ids{};
			std::array<DWORD, maxExclusionThreads> listed{};
			std::array<HANDLE, maxExclusionThreads + 1> recheckHandles{};
		};
		static ExclusionStorage g_exclusionStorage;
		static SRWLOCK g_exclusionLock = SRWLOCK_INIT;

		// Thread operations for Framework::ThreadExecution::Run. While peers are stopped the only calls
		// are SuspendThread, NtGetNextThread, GetThreadId, GetThreadContext, WaitForSingleObject(h, 0)
		// and the prepared commit. Handles are closed after the resume.
		class ThreadExclusionOperations {
		public:
			struct FailureInfo {
				ExclusionFailure reason = ExclusionFailure::None;
				DWORD threadId = 0;
				DWORD error = ERROR_SUCCESS;
				unsigned pass = 0;
			};

		private:
			static constexpr NTSTATUS statusNoMoreEntries = static_cast<NTSTATUS>(0x8000001A);
			ExclusionStorage& storage_;
			std::span<const std::pair<uintptr_t, size_t>> spans_;
			size_t threadCount_ = 0;
			size_t idCount_ = 0;
			size_t recheckHandleCount_ = 0;
			DWORD currentThreadId_ = 0;
			FailureInfo failure_{};

			bool SnapshotIds(std::array<DWORD, maxExclusionThreads>& ids, size_t& count, unsigned pass) {
				count = 0;
				HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
				if (snapshot == INVALID_HANDLE_VALUE) {
					failure_ = { ExclusionFailure::Snapshot, 0, GetLastError(), pass };
					return false;
				}
				THREADENTRY32 entry{};
				entry.dwSize = sizeof(entry);
				BOOL more = Thread32First(snapshot, &entry);
				if (!more) {
					failure_ = { ExclusionFailure::Snapshot, 0, GetLastError(), pass };
					CloseHandle(snapshot);
					return false;
				}
				const DWORD processId = GetCurrentProcessId();
				currentThreadId_ = GetCurrentThreadId();
				while (more) {
					if (entry.th32OwnerProcessID == processId && entry.th32ThreadID != currentThreadId_) {
						if (count == ids.size()) {
							failure_ = { ExclusionFailure::TooManyThreads, entry.th32ThreadID, ERROR_MORE_DATA, pass };
							CloseHandle(snapshot);
							return false;
						}
						ids[count++] = entry.th32ThreadID;
					}
					entry.dwSize = sizeof(entry);
					SetLastError(ERROR_SUCCESS);
					more = Thread32Next(snapshot, &entry);
					if (!more && GetLastError() != ERROR_NO_MORE_FILES) {
						failure_ = { ExclusionFailure::Snapshot, 0, GetLastError(), pass };
						CloseHandle(snapshot);
						return false;
					}
				}
				CloseHandle(snapshot);
				std::sort(ids.begin(), ids.begin() + count);
				return true;
			}

			bool IsStillListed(DWORD id, bool& listed, unsigned pass) {
				size_t count = 0;
				if (!SnapshotIds(storage_.listed, count, pass))
					return false;
				listed = std::binary_search(storage_.listed.begin(), storage_.listed.begin() + count, id);
				return true;
			}

			void CloseHandles() noexcept {
				for (size_t i = threadCount_; i > 0; --i) {
					CloseHandle(storage_.threads[i - 1].handle);
					storage_.threads[i - 1] = {};
				}
				threadCount_ = 0;
				for (size_t i = recheckHandleCount_; i > 0; --i) {
					CloseHandle(storage_.recheckHandles[i - 1]);
					storage_.recheckHandles[i - 1] = nullptr;
				}
				recheckHandleCount_ = 0;
			}

			// Opened threads are in snapshot order, which is sorted by id.
			bool IsOpened(DWORD id) const noexcept {
				size_t low = 0;
				size_t high = threadCount_;
				while (low < high) {
					const size_t middle = low + (high - low) / 2;
					if (storage_.threads[middle].id < id)
						low = middle + 1;
					else
						high = middle;
				}
				return low < threadCount_ && storage_.threads[low].id == id;
			}

			bool AnyInside(unsigned pass) noexcept {
				for (size_t i = 0; i < threadCount_; ++i) {
					const ExclusionThread& thread = storage_.threads[i];
					if (!thread.suspended)
						continue;
					CONTEXT context{};
					context.ContextFlags = CONTEXT_CONTROL;
					if (!GetThreadContext(thread.handle, &context)) {
						const DWORD error = GetLastError();
						if (WaitForSingleObject(thread.handle, 0) == WAIT_OBJECT_0)
							continue;
						failure_ = { ExclusionFailure::Context, thread.id, error, pass };
						return true;
					}
					for (const auto& [start, length] : spans_)
						if (context.Eip >= start && context.Eip - start < length) {
							failure_ = { ExclusionFailure::InPatchSpan, thread.id, ERROR_SUCCESS, pass };
							return true;
						}
				}
				return false;
			}

		public:
			ThreadExclusionOperations(ExclusionStorage& storage,
				std::span<const std::pair<uintptr_t, size_t>> spans) noexcept
				: storage_(storage), spans_(spans) {}

			~ThreadExclusionOperations() { Resume(); }

			bool Snapshot(size_t pass) {
				failure_ = {};
				return SnapshotIds(storage_.ids, idCount_, static_cast<unsigned>(pass));
			}

			size_t ThreadCount() const noexcept { return idCount_; }

			bool Prepare() {
				if (ResolveNextThreadDispatch())
					return true;
				failure_ = { ExclusionFailure::Recheck, 0, ERROR_PROC_NOT_FOUND, 0 };
				return false;
			}

			bool Open(size_t pass) {
				for (size_t i = 0; i < idCount_; ++i) {
					const DWORD id = storage_.ids[i];
					HANDLE thread = OpenThread(THREAD_SUSPEND_RESUME | THREAD_GET_CONTEXT
						| THREAD_QUERY_LIMITED_INFORMATION | SYNCHRONIZE, FALSE, id);
					if (!thread) {
						const DWORD error = GetLastError();
						bool listed = true;
						if (IsStillListed(id, listed, static_cast<unsigned>(pass)) && !listed)
							continue;
						failure_ = { ExclusionFailure::OpenLiveThread, id, error, static_cast<unsigned>(pass) };
						return false;
					}
					storage_.threads[threadCount_++] = { id, thread, false };
				}
				return true;
			}

			bool Suspend(size_t pass) noexcept {
#ifdef MEMUTIL_TESTING
				if (g_executionSyncTestBeforeSuspend)
					g_executionSyncTestBeforeSuspend();
#endif
				for (size_t i = 0; i < threadCount_; ++i) {
					ExclusionThread& thread = storage_.threads[i];
					if (SuspendThread(thread.handle) == static_cast<DWORD>(-1)) {
						const DWORD error = GetLastError();
						if (WaitForSingleObject(thread.handle, 0) == WAIT_OBJECT_0)
							continue;
						failure_ = { ExclusionFailure::SuspendLiveThread, thread.id, error, static_cast<unsigned>(pass) };
						return false;
					}
					thread.suspended = true;
				}
				return true;
			}

			// Every live thread of the process must be the caller or one opened from the snapshot; a thread
			// born between the snapshot and the suspension is found here and the pass is retried. An exited
			// thread whose object a handle keeps alive is still enumerated here but not by Toolhelp; it
			// cannot run, so it is skipped.
			Framework::ThreadExecution::CheckResult Recheck(size_t pass) noexcept {
				const unsigned passNumber = static_cast<unsigned>(pass);
				HANDLE previous = nullptr;
				for (;;) {
					if (recheckHandleCount_ == storage_.recheckHandles.size()) {
						failure_ = { ExclusionFailure::TooManyThreads, 0, ERROR_MORE_DATA, passNumber };
						return Framework::ThreadExecution::CheckResult::Failed;
					}
					HANDLE next = nullptr;
					const NTSTATUS status = GetNextThread(GetCurrentProcess(), previous, &next);
					if (status == statusNoMoreEntries)
						break;
					if (!NT_SUCCESS(status) || !next) {
						failure_ = { ExclusionFailure::Recheck, 0, static_cast<DWORD>(status), passNumber };
						return Framework::ThreadExecution::CheckResult::Failed;
					}
					storage_.recheckHandles[recheckHandleCount_++] = next;
					previous = next;
					const DWORD id = GetThreadId(next);
					if (!id) {
						failure_ = { ExclusionFailure::Recheck, 0, GetLastError(), passNumber };
						return Framework::ThreadExecution::CheckResult::Failed;
					}
					if (id != currentThreadId_ && !IsOpened(id)) {
						if (WaitForSingleObject(next, 0) == WAIT_OBJECT_0)
							continue;
						failure_ = { ExclusionFailure::NewThread, id, ERROR_RETRY, passNumber };
#ifdef MEMUTIL_TESTING
						++g_executionSyncTestUntrackedThreads;
#endif
						return Framework::ThreadExecution::CheckResult::Retry;
					}
				}
				if (AnyInside(passNumber))
					return failure_.reason == ExclusionFailure::InPatchSpan
						? Framework::ThreadExecution::CheckResult::Retry
						: Framework::ThreadExecution::CheckResult::Failed;
#ifdef MEMUTIL_TESTING
				if (g_executionSyncTestWhileSuspended)
					g_executionSyncTestWhileSuspended();
#endif
				return Framework::ThreadExecution::CheckResult::Clear;
			}

			void Resume() noexcept {
				for (size_t i = threadCount_; i > 0; --i) {
					ExclusionThread& thread = storage_.threads[i - 1];
					if (thread.suspended) {
						ResumeThread(thread.handle);
						thread.suspended = false;
					}
				}
				CloseHandles();
			}

			void RetryDelay(size_t) noexcept { Sleep(10); }

			void PassLimit(size_t pass) noexcept {
				failure_.pass = static_cast<unsigned>(pass);
			}

			const FailureInfo& FailureDetails() const noexcept { return failure_; }
		};

#ifdef MEMUTIL_TESTING
		void SetExecutionSyncBeforeSuspendForTest(void(__cdecl* hook)()) noexcept {
			g_executionSyncTestBeforeSuspend = hook;
		}
		void SetExecutionSyncWhileSuspendedForTest(void(__cdecl* hook)()) noexcept {
			g_executionSyncTestWhileSuspended = hook;
		}
		unsigned int ExecutionSyncUntrackedThreadsForTest() noexcept {
			return g_executionSyncTestUntrackedThreads;
		}
#endif
	}

	NTSTATUS HookedQueryVirtualMemory(LPVOID address,
		PMEMORY_BASIC_INFORMATION memoryBuffer, SIZE_T dwLength) {
		ULONGLONG start = GetTickCount64();
		if (!MemoryHostPolicy().useWineApis) {
			static const bool resolved = [] {
				const std::optional<uint32_t> number = Detail::GetSyscallNumber("NtQueryVirtualMemory");
				if (!number)
					return false;
				Detail::ssnQuery = *number;
				return true;
			}();
			if (resolved) {
				SIZE_T returnLength = 0;
				const NTSTATUS status = Detail::Syscall_NtQueryVirtualMemory(GetCurrentProcess(),
					address, 0, memoryBuffer, dwLength, &returnLength);
				if (NT_SUCCESS(status)) {
					LogMemoryCall("VirtualQuery", address, dwLength, false, 0,
						"syscall", status, 0, start);
					return status;
				}
				LogSyscallFailureOnce("VirtualQuery", address, dwLength, false, 0, status, start);
			} else {
				LogUnresolvedSyscallOnce("VirtualQuery", address, dwLength, false, 0, start);
			}
			start = GetTickCount64();
		}
		const SIZE_T result = VirtualQuery(address, memoryBuffer, dwLength);
		const DWORD error = result ? ERROR_SUCCESS : GetLastError();
		LogMemoryCall("VirtualQuery", address, dwLength, false, 0, "documented",
			result ? STATUS_SUCCESS : STATUS_UNSUCCESSFUL, error, start);
		return result ? STATUS_SUCCESS : STATUS_UNSUCCESSFUL;
	}

	namespace {
		bool ReadPatchMemory(void*, uintptr_t address, unsigned char* output, size_t length) noexcept {
			if (!address || !output || !length)
				return false;
			__try {
				memcpy(output, reinterpret_cast<const void*>(address), length);
				return true;
			}
			__except (EXCEPTION_EXECUTE_HANDLER) {
				return false;
			}
		}

		bool WritePatchMemory(void*, uintptr_t address, const unsigned char* bytes, size_t length,
			bool pointerStore) noexcept {
			if (!address || !bytes || !length)
				return false;
			if (pointerStore) {
				if (length != sizeof(void*) || address % alignof(void*))
					return false;
				void* value = nullptr;
				memcpy(&value, bytes, sizeof(value));
				InterlockedExchangePointer(reinterpret_cast<PVOID volatile*>(address), value);
				return true;
			}
			__try {
				memcpy(reinterpret_cast<void*>(address), bytes, length);
				return true;
			}
			__except (EXCEPTION_EXECUTE_HANDLER) {
				return false;
			}
		}

		bool FlushPatchMemory(void*, uintptr_t address, size_t length) noexcept {
			return FlushInstructionCache(GetCurrentProcess(), reinterpret_cast<LPCVOID>(address), length) != FALSE;
		}

		bool ProtectPatchMemory(void* context, uintptr_t address, size_t length,
			uint32_t protection) noexcept {
			const bool useSyscall = *static_cast<const bool*>(context);
			if (useSyscall) {
				PVOID baseAddress = reinterpret_cast<PVOID>(address);
				SIZE_T regionSize = length;
				ULONG previous = 0;
				return NT_SUCCESS(Detail::Syscall_NtProtectVirtualMemory(GetCurrentProcess(),
					&baseAddress, &regionSize, protection, &previous));
			}
			DWORD previous = 0;
			return VirtualProtect(reinterpret_cast<LPVOID>(address), length, protection, &previous) != FALSE;
		}

		Detail::PatchTransactionOperations TransactionOperations(void* context) noexcept {
			return { context, ReadPatchMemory, WritePatchMemory, ProtectPatchMemory, FlushPatchMemory };
		}

		bool ResolveProtectionDispatch(bool& useSyscall) {
			useSyscall = false;
			if (MemoryHostPolicy().useWineApis)
				return true;
			static const bool resolved = [] {
				const std::optional<uint32_t> number = Detail::GetSyscallNumber("NtProtectVirtualMemory");
				if (!number)
					return false;
				Detail::ssnProtect = *number;
				return true;
			}();
			useSyscall = resolved;
			return true;
		}

		// Builds the native entry for a JMP over [source, source + span). With exactLength the span is
		// requestedLength and must end on an instruction boundary; otherwise it ends at the first
		// boundary at or after requestedLength (entries whose bytes are not known in advance).
		// Decoding runs on one snapshot of the live bytes, so the gateway holds exactly the bytes
		// returned in original, which the caller compares and the commit verifies again.
		// Refused: spans outside 5..kMaxSpan, undecodable bytes, any relative operand other than
		// a leading E8/E9 rel32, and a leading target inside the span (or a call to its end).
		bool BuildGateway(uintptr_t source, size_t requestedLength, bool exactLength,
			std::vector<unsigned char>& original, void*& gateway, size_t& gatewayLength,
			DWORD& error) {
			using namespace Framework::Trampoline;
			error = ERROR_BAD_EXE_FORMAT;
			const size_t readLength = requestedLength + kDecodeSlack;
			if (!SpanLengthAllowed(requestedLength)
				|| readLength > (std::numeric_limits<uintptr_t>::max)() - source)
				return false;
			// Zero padding past the read bytes ends any runaway prefix sequence inside the buffer.
			std::array<unsigned char, kMaxSpan + kDecodeSlack + 16> snapshot{};
			if (!ReadPatchMemory(nullptr, source, snapshot.data(), readLength)) {
				error = ERROR_NOACCESS;
				return false;
			}

			const size_t limit = exactLength ? requestedLength : readLength;
			size_t offset = 0;
			bool relocateLeading = false;
			uintptr_t leadingTarget = 0;
			while (offset < requestedLength) {
				PBYTE instruction = snapshot.data() + offset;
				// Large enough for the longest instruction the zero-padded snapshot can hold.
				std::array<unsigned char, sizeof(snapshot) + 16> decoded{};
				PBYTE target = nullptr;
				LONG extra = 0;
				PBYTE next = DetourCopyInstructionEx(decoded.data(), instruction, &target, &extra);
				if (!next || next <= instruction)
					return false;
				Instruction facts{};
				facts.offset = offset;
				facts.length = static_cast<size_t>(next - instruction);
				facts.opcode = *instruction;
				facts.hasTarget = target != nullptr;
				facts.dynamicTarget = target == DETOUR_INSTRUCTION_TARGET_DYNAMIC;
				facts.extra = extra;
				if (!InstructionFits(offset, facts.length, limit))
					return false;
				const Verdict verdict = Classify(facts);
				if (verdict == Verdict::Refuse)
					return false;
				if (verdict == Verdict::RelocateLeading) {
					int32_t relative = 0;
					memcpy(&relative, snapshot.data() + 1, sizeof(relative));
					leadingTarget = source + 5 + static_cast<uintptr_t>(relative);
					relocateLeading = true;
				}
				offset += facts.length;
			}
			const size_t spanLength = offset;
			if ((exactLength && spanLength != requestedLength) || spanLength > readLength
				|| (relocateLeading && !LeadingTargetAllowed(snapshot[0], leadingTarget, source, spanLength)))
				return false;

			const size_t gatewayCapacity = spanLength + 5;
			if (!OutputFits(0, spanLength, gatewayCapacity, 5))
				return false;
			auto* allocated = static_cast<unsigned char*>(VirtualAlloc(nullptr, gatewayCapacity,
				MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE));
			if (!allocated) {
				error = GetLastError();
				return false;
			}
			memcpy(allocated, snapshot.data(), spanLength);
			if (relocateLeading) {
				const uint32_t relative = static_cast<uint32_t>(leadingTarget
					- (reinterpret_cast<uintptr_t>(allocated) + 5));
				memcpy(allocated + 1, &relative, sizeof(relative));
			}
			allocated[spanLength] = 0xE9;
			const uint32_t backRelative = static_cast<uint32_t>(source + spanLength
				- (reinterpret_cast<uintptr_t>(allocated) + spanLength + 5));
			memcpy(allocated + spanLength + 1, &backRelative, sizeof(backRelative));
			gatewayLength = gatewayCapacity;
			DWORD previousProtection = 0;
			if (!VirtualProtect(allocated, gatewayLength, PAGE_EXECUTE_READ, &previousProtection)
				|| !FlushInstructionCache(GetCurrentProcess(), allocated, gatewayLength)) {
				error = GetLastError();
				VirtualFree(allocated, 0, MEM_RELEASE);
				return false;
			}
			original.assign(snapshot.begin(), snapshot.begin() + spanLength);
			error = ERROR_SUCCESS;
			gateway = allocated;
			return true;
		}

	}

	PreparedPatchSet::~PreparedPatchSet() {
		ReleaseNativeEntries();
	}

	void PreparedPatchSet::ReleaseNativeEntries() noexcept {
		for (Site& site : sites_) {
			if (site.ownsNativeEntry && !site.transaction.everPublished && !site.nativeEntryExposed)
				VirtualFree(site.nativeEntry, 0, MEM_RELEASE);
			site.nativeEntry = nullptr;
			site.ownsNativeEntry = false;
		}
	}

	void PreparedPatchSet::Reset() noexcept {
		ReleaseNativeEntries();
		sites_.clear();
		regions_.clear();
		outcome_ = {};
		result_ = PatchResult::Original;
		failure_ = Failure::None;
		failureAddress_ = 0;
		failureCode_ = ERROR_SUCCESS;
		exclusionReason_ = nullptr;
		exclusionThread_ = 0;
		exclusionPass_ = 0;
		prepared_ = false;
		preparationFailed_ = false;
		operationAttempted_ = false;
		useSyscallProtection_ = false;
	}

	bool PreparedPatchSet::Prepare(std::span<const PreparedPatchRequest> requests) {
		bool hasOwnedGateway = false;
		for (const Site& site : sites_)
			hasOwnedGateway = hasOwnedGateway || site.ownsNativeEntry;
		if (!Detail::CanReplacePreparedRecord(result_, hasOwnedGateway))
			return false;
		Reset();
		if (requests.size() > MaxSites) {
			failure_ = Failure::InvalidRequest;
			preparationFailed_ = true;
			return false;
		}
		sites_.reserve(requests.size());
		regions_.reserve(MaxRegions);
		for (const PreparedPatchRequest& request : requests)
			if (!Append(request)) {
				prepared_ = false;
				return false;
			}
		prepared_ = true;
		return true;
	}

	bool PreparedPatchSet::Append(const PreparedPatchRequest& request) {
		// A set that was committed keeps its sites for Rollback; a refused one keeps its first failure.
		if (operationAttempted_ || preparationFailed_)
			return false;
		if (sites_.size() >= MaxSites) {
			failure_ = Failure::InvalidRequest;
			failureAddress_ = reinterpret_cast<uintptr_t>(request.address);
			preparationFailed_ = true;
			return false;
		}
		ResolveProtectionDispatch(useSyscallProtection_);

		Site site{};
		auto fail = [&](Failure failure, uintptr_t address, DWORD error = ERROR_SUCCESS) {
			failure_ = failure;
			failureAddress_ = address;
			failureCode_ = error;
			preparationFailed_ = true;
			if (site.ownsNativeEntry)
				VirtualFree(site.nativeEntry, 0, MEM_RELEASE);
			return false;
		};

		const uintptr_t address = reinterpret_cast<uintptr_t>(request.address);
		if (!address || (!request.replacementTarget && (request.kind == PreparedPatchKind::Jump
			|| request.kind == PreparedPatchKind::Call)))
			return fail(Failure::InvalidRequest, address);

		switch (request.kind) {
		case PreparedPatchKind::Bytes:
			if (request.replacementBytes.empty() || (request.length && request.length != request.replacementBytes.size()))
				return fail(Failure::InvalidRequest, address);
			site.original.resize(request.replacementBytes.size());
			site.replacement.assign(request.replacementBytes.begin(), request.replacementBytes.end());
			if (!Detail::CaptureOriginalBytes(address, request.expectedBytes, site.original,
				TransactionOperations(nullptr)))
				return fail(request.expectedBytes.empty() ? Failure::ReadFailed : Failure::BytesMismatch, address);
			break;
		case PreparedPatchKind::PointerStore:
			if (address % alignof(void*) || request.replacementBytes.size() != sizeof(void*)
				|| (request.length && request.length != sizeof(void*))
				|| (!request.expectedBytes.empty() && request.expectedBytes.size() != sizeof(void*)))
				return fail(Failure::InvalidRequest, address);
			site.original.resize(sizeof(void*));
			site.replacement.assign(request.replacementBytes.begin(), request.replacementBytes.end());
			site.transaction.pointerStore = true;
			if (!Detail::CaptureOriginalBytes(address, request.expectedBytes, site.original,
				TransactionOperations(nullptr)))
				return fail(request.expectedBytes.empty() ? Failure::ReadFailed : Failure::BytesMismatch, address);
			break;
		case PreparedPatchKind::Call: {
			if (!request.expectedTarget || (request.length && request.length != 5)
				|| request.captureNativeEntry && request.expectedTarget == 0)
				return fail(Failure::InvalidRequest, address);
			std::vector<unsigned char> encodedExpected(5);
			encodedExpected[0] = 0xE8;
			const uint32_t expectedRelative = static_cast<uint32_t>(request.expectedTarget - (address + 5));
			memcpy(encodedExpected.data() + 1, &expectedRelative, sizeof(expectedRelative));
			if (!request.expectedBytes.empty() && (request.expectedBytes.size() != 5
				|| memcmp(request.expectedBytes.data(), encodedExpected.data(), 5) != 0))
				return fail(Failure::InvalidRequest, address);
			site.original.resize(5);
			if (!Detail::CaptureOriginalBytes(address, encodedExpected, site.original,
				TransactionOperations(nullptr)))
				return fail(Failure::BytesMismatch, address);
			site.replacement.resize(5);
			site.replacement[0] = 0xE8;
			const uint32_t replacementRelative = static_cast<uint32_t>(request.replacementTarget - (address + 5));
			memcpy(site.replacement.data() + 1, &replacementRelative, sizeof(replacementRelative));
			if (request.captureNativeEntry)
				site.nativeEntry = reinterpret_cast<void*>(request.expectedTarget);
			break;
		}
		case PreparedPatchKind::Jump: {
			const bool dynamicGateway = request.captureNativeEntry && request.expectedBytes.empty()
				&& request.length == 0 && request.minimumLength != 0;
			const size_t length = dynamicGateway ? request.minimumLength
				: request.length ? request.length : request.expectedBytes.size();
			if (length < 5)
				return fail(Failure::InvalidRequest, address);
			if (!request.expectedBytes.empty() && request.length
				&& request.expectedBytes.size() != request.length)
				return fail(Failure::InvalidRequest, address);
			if (request.captureNativeEntry) {
				size_t gatewayLength = 0;
				DWORD error = ERROR_SUCCESS;
				if (!BuildGateway(address, length, !dynamicGateway, site.original,
					site.nativeEntry, gatewayLength, error))
					return fail(Failure::GatewayFailed, address, error);
				site.ownsNativeEntry = true;
				if (!request.expectedBytes.empty()
					&& (request.expectedBytes.size() != site.original.size()
						|| memcmp(request.expectedBytes.data(), site.original.data(), site.original.size()) != 0))
					return fail(Failure::BytesMismatch, address);
				site.transaction.hasNativeEntry = true;
			}
			else {
				site.original.resize(length);
				if (!Detail::CaptureOriginalBytes(address, request.expectedBytes, site.original,
					TransactionOperations(nullptr)))
					return fail(request.expectedBytes.empty() ? Failure::ReadFailed : Failure::BytesMismatch, address);
			}
			site.replacement.assign(site.original.size(), 0x90);
			site.replacement[0] = 0xE9;
			const uint32_t relative = static_cast<uint32_t>(request.replacementTarget - (address + 5));
			memcpy(site.replacement.data() + 1, &relative, sizeof(relative));
			break;
		}
		default:
			return fail(Failure::InvalidRequest, address);
		}

		if (site.original.empty() || site.original.size() != site.replacement.size()
			|| site.original.size() > (std::numeric_limits<uintptr_t>::max)() - address)
			return fail(Failure::InvalidRequest, address);
		const size_t length = site.original.size();
		for (const Site& existing : sites_) {
			if (IsPatchOverlap(address, length, existing.transaction.address, existing.original.size()))
				return fail(Failure::InvalidRequest, address);
		}

		std::vector<Detail::PatchTransactionRegion> queriedRegions;
		if (!QueryRegions(address, length, queriedRegions))
			return fail(Failure::QueryFailed, address, GetLastError());
		if (regions_.size() + queriedRegions.size() > MaxRegions)
			return fail(Failure::InvalidRequest, address);

		site.transaction.address = address;
		site.transaction.original = site.original;
		site.transaction.replacement = site.replacement;
		for (const auto& region : queriedRegions)
			AddTransactionRegion(regions_, region);
		sites_.push_back(std::move(site));
		prepared_ = true;
		return true;
	}

	PatchResult PreparedPatchSet::Commit() noexcept {
		return RunTransaction(false);
	}

	PatchResult PreparedPatchSet::Rollback() noexcept {
		return RunTransaction(true);
	}

	// Exclusion rule: a set that writes anything other than aligned pointer stores runs inside
	// Framework::ThreadExecution::Run, which suspends every other thread of the process, re-checks for
	// threads born after its snapshot and for threads stopped inside a patched span, and retries up
	// to kMaxPasses. It writes without suspending only when the snapshot taken here lists no other
	// thread. When exclusion is not reached nothing is written and the previous state is reported.
	PatchResult PreparedPatchSet::RunTransaction(bool rollback) noexcept {
		operationAttempted_ = true;
		if (preparationFailed_)
			return result_ = ToPatchResult(outcome_);
		failure_ = Failure::None;
		failureAddress_ = 0;
		failureCode_ = ERROR_SUCCESS;
		exclusionReason_ = nullptr;
		exclusionThread_ = 0;
		exclusionPass_ = 0;
		if (!prepared_)
			return result_ = ToPatchResult(outcome_);
		std::array<Detail::PatchTransactionSite, MaxSites> transactionSites{};
		std::array<std::pair<uintptr_t, size_t>, MaxSites> spans{};
		bool needsExclusion = false;
		for (size_t i = 0; i < sites_.size(); ++i) {
			transactionSites[i] = sites_[i].transaction;
			transactionSites[i].everPublished = transactionSites[i].everPublished || sites_[i].nativeEntryExposed;
			spans[i] = { sites_[i].transaction.address, sites_[i].original.size() };
			needsExclusion = needsExclusion || (threadExclusion_ && !sites_[i].transaction.pointerStore);
		}
		DWORD transactionError = ERROR_SUCCESS;
		auto transaction = [&]() noexcept {
			outcome_ = Detail::RunPatchTransaction(
				std::span<Detail::PatchTransactionSite>(transactionSites.data(), sites_.size()), regions_,
				TransactionOperations(&useSyscallProtection_), rollback, PAGE_EXECUTE_READWRITE,
				&failureAddress_);
			transactionError = GetLastError();
			return true;
		};
		if (needsExclusion) {
			AcquireSRWLockExclusive(&Detail::g_exclusionLock);
			Detail::ThreadExclusionOperations::FailureInfo exclusion{};
			bool excluded = false;
			{
				Detail::ThreadExclusionOperations threads(Detail::g_exclusionStorage,
					std::span<const std::pair<uintptr_t, size_t>>(spans.data(), sites_.size()));
				excluded = Framework::ThreadExecution::Run(threads, Framework::ThreadExecution::kMaxPasses,
					transaction);
				exclusion = threads.FailureDetails();
			}
			ReleaseSRWLockExclusive(&Detail::g_exclusionLock);
			if (!excluded) {
				failure_ = Failure::ExecutionSyncFailed;
				failureCode_ = exclusion.error;
				exclusionReason_ = Detail::ExclusionFailureName(exclusion.reason);
				exclusionThread_ = exclusion.threadId;
				exclusionPass_ = exclusion.pass;
				return result_ = ToPatchResult(outcome_);
			}
		}
		else
			transaction();
		for (size_t i = 0; i < sites_.size(); ++i)
			sites_[i].transaction.everPublished = transactionSites[i].everPublished;
		result_ = ToPatchResult(outcome_);
		const PatchByteState expected = rollback ? PatchByteState::Original : PatchByteState::Replacement;
		if (outcome_.bytes != expected || !outcome_.instructionCacheFlushed || !outcome_.protectionRestored) {
			failure_ = Failure::CommitFailed;
			failureCode_ = transactionError;
		}
		return result_;
	}

	void* PreparedPatchSet::NativeEntry(size_t site) const noexcept {
		if (site >= sites_.size())
			return nullptr;
		if (sites_[site].ownsNativeEntry)
			sites_[site].nativeEntryExposed = true;
		return sites_[site].nativeEntry;
	}

	uintptr_t PreparedPatchSet::SiteAddress(size_t site) const noexcept {
		return site < sites_.size() ? sites_[site].transaction.address : 0;
	}

	size_t PreparedPatchSet::SiteLength(size_t site) const noexcept {
		return site < sites_.size() ? sites_[site].original.size() : 0;
	}

	void PreparedPatchSet::Report(const char* group) const {
		if (preparationFailed_) {
			if (failure_ == Failure::BytesMismatch)
				LOG_ERROR(group << " expected bytes did not match at 0x" << std::hex << failureAddress_
					<< std::dec << "; no patch was written" << std::endl);
			else
				LOG_ERROR(group << " could not be prepared at 0x" << std::hex << failureAddress_
					<< std::dec << " (error " << failureCode_ << "); no patch was written" << std::endl);
			return;
		}
		if (prepared_ && !operationAttempted_) {
			LOG_INFO(group << " was prepared; no patch was attempted" << std::endl);
			return;
		}
		if (failure_ == Failure::ExecutionSyncFailed) {
			LOG_ERROR(group << " could not stop other threads: " << (exclusionReason_ ? exclusionReason_ : "unknown")
				<< " (thread " << exclusionThread_ << ", pass " << exclusionPass_ << ", error " << failureCode_
				<< "); the bytes were not touched" << std::endl);
			return;
		}
		if (result_ == PatchResult::Installed) {
			if (outcome_.protectionRestored)
				LOG_INFO(group << " installed all " << sites_.size() << " sites" << std::endl);
			else
				LOG_WARNING(group << " installed all " << sites_.size()
					<< " sites, but page protection was not restored" << std::endl);
		}
		else if (result_ == PatchResult::Original)
			LOG_INFO(group << " is confirmed original at every attempted site" << std::endl);
		else
			LOG_ERROR(group << " is in an unknown state at 0x" << std::hex << failureAddress_
				<< std::dec << " (error " << failureCode_ << "); native pass-through remains active" << std::endl);
	}
}

uint32_t MemUtil::GetTextSectionAddress() {
	HMODULE hModule = GetModuleHandle(NULL);
	if (!hModule) {
		std::cerr << "Failed to get base address of the host process." << std::endl;
		return 0;
	}

	IMAGE_DOS_HEADER* dosHeader = (IMAGE_DOS_HEADER*)hModule;
	IMAGE_NT_HEADERS* ntHeaders = (IMAGE_NT_HEADERS*)((BYTE*)hModule + dosHeader->e_lfanew);

	IMAGE_SECTION_HEADER* sectionHeaders = (IMAGE_SECTION_HEADER*)((BYTE*)&ntHeaders->OptionalHeader +
		ntHeaders->FileHeader.SizeOfOptionalHeader);

	for (int i = 0; i < ntHeaders->FileHeader.NumberOfSections; ++i) {
		if (strncmp((char*)sectionHeaders[i].Name, ".text", 5) == 0) {

			DWORD textSectionVA = sectionHeaders[i].VirtualAddress;
			uint32_t textSectionAddress = (uint32_t)((BYTE*)hModule + textSectionVA);

			return textSectionAddress;
		}
	}

	std::cerr << "Failed to find the .text section." << std::endl;
	return 0;
}

uint32_t MemUtil::GetTextSectionLength() {
	HMODULE hModule = GetModuleHandle(NULL);
	if (!hModule) {
		std::cerr << "Failed to get base address of the host process." << std::endl;
		return 0;
	}

	IMAGE_DOS_HEADER* dosHeader = (IMAGE_DOS_HEADER*)hModule;
	IMAGE_NT_HEADERS* ntHeaders = (IMAGE_NT_HEADERS*)((BYTE*)hModule + dosHeader->e_lfanew);

	IMAGE_SECTION_HEADER* sectionHeaders = (IMAGE_SECTION_HEADER*)((BYTE*)&ntHeaders->OptionalHeader +
		ntHeaders->FileHeader.SizeOfOptionalHeader);

	for (int i = 0; i < ntHeaders->FileHeader.NumberOfSections; ++i) {
		if (strncmp((char*)sectionHeaders[i].Name, ".text", 5) == 0) {
			uint32_t textSectionLength = sectionHeaders[i].Misc.VirtualSize;

			return textSectionLength;
		}
	}

	std::cerr << "Failed to find the .text section." << std::endl;
	return 0;
}

void MemUtil::CheckMemoryProtection(void* address) {
	MEMORY_BASIC_INFORMATION mbi;
	if (VirtualQuery(address, &mbi, sizeof(mbi))) {
		std::cout << "Current Protection: ";

		switch (mbi.Protect) {
		case PAGE_EXECUTE_READ:
			std::cout << "PAGE_EXECUTE_READ" << std::endl;
			break;
		case PAGE_READONLY:
			std::cout << "PAGE_READONLY" << std::endl;
			break;
		case PAGE_READWRITE:
			std::cout << "PAGE_READWRITE" << std::endl;
			break;
		case PAGE_EXECUTE_READWRITE:
			std::cout << "PAGE_EXECUTE_READWRITE" << std::endl;
			break;
		default:
			std::cout << "Other protection flag: " << mbi.Protect << std::endl;
		}
	}
	else {
		std::cerr << "VirtualQuery failed. Error: " << GetLastError() << std::endl;
	}
}
