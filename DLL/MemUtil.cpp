#include "stdafx.h"
#include "MemUtil.hpp"
#include "SettingEdits.hpp"
#include "Lib/Detours/detours.h"
#include <optional>

namespace {
	struct BootPatch {
		void* address;
		std::vector<unsigned char> original;
		std::vector<unsigned char> replacement;
		const char* member;
		bool pointerStore;
	};
	struct BootBundle {
		bool staging = false;
		const char* member = "startup";
		const char* failed = nullptr;
		std::vector<BootPatch> patches;
		std::vector<void*> gateways;
	};
	thread_local BootBundle g_bootBundle;
	std::atomic<bool> g_bootBundleEnabled{ true };

	bool StageBootPatch(void* address, const void* replacement, size_t length, bool pointerStore = false) {
		if (g_bootBundle.failed)
			return false;
		const uintptr_t start = reinterpret_cast<uintptr_t>(address);
		for (const BootPatch& previous : g_bootBundle.patches) {
			const uintptr_t other = reinterpret_cast<uintptr_t>(previous.address);
			if (start < other + previous.original.size() && other < start + length) {
				MemUtil::FailBootPatchBundle();
				return false;
			}
		}
		BootPatch patch{ address, std::vector<unsigned char>(length),
			std::vector<unsigned char>(length), g_bootBundle.member, pointerStore };
		for (size_t i = 0; i < length; ++i) {
			if (!MemUtil::TryRead(reinterpret_cast<uintptr_t>(address) + i, patch.original[i])) {
				MemUtil::FailBootPatchBundle();
				return false;
			}
		}
		memcpy(patch.replacement.data(), replacement, length);
		g_bootBundle.patches.push_back(std::move(patch));
		return true;
	}

	void StorePatchBytes(void* address, const void* bytes, size_t length, bool pointerStore) {
		if (!pointerStore) {
			memcpy(address, bytes, length);
			return;
		}
		void* value = nullptr;
		memcpy(&value, bytes, sizeof(value));
		InterlockedExchangePointer(reinterpret_cast<PVOID volatile*>(address), value);
	}

	struct ProtectedRegion {
		void* base;
		SIZE_T size;
		ULONG protection;
		void* queryBase;
		SIZE_T querySize;
	};

	// VirtualProtect reports only the first page's old protection, so record each uniform region.
	bool QueryRegions(void* address, size_t length, std::vector<ProtectedRegion>& regions) {
		regions.clear();
		uintptr_t cursor = reinterpret_cast<uintptr_t>(address);
		const uintptr_t end = cursor + length;
		while (cursor < end) {
			MEMORY_BASIC_INFORMATION info{};
			if (!NT_SUCCESS(MemUtil::HookedQueryVirtualMemory(reinterpret_cast<LPVOID>(cursor), &info, sizeof(info)))
				|| info.State != MEM_COMMIT || info.RegionSize == 0)
				return false;
			const uintptr_t regionEnd = reinterpret_cast<uintptr_t>(info.BaseAddress) + info.RegionSize;
			const uintptr_t stop = regionEnd < end ? regionEnd : end;
			regions.push_back({ reinterpret_cast<void*>(cursor), stop - cursor, info.Protect,
				info.BaseAddress, info.RegionSize });
			cursor = stop;
		}
		return !regions.empty();
	}

	void AddBundleRegion(std::vector<ProtectedRegion>& regions, const ProtectedRegion& candidate) {
		const uintptr_t candidateStart = reinterpret_cast<uintptr_t>(candidate.base);
		const uintptr_t candidateEnd = candidateStart + candidate.size;
		for (ProtectedRegion& region : regions) {
			if (region.queryBase != candidate.queryBase || region.querySize != candidate.querySize
				|| region.protection != candidate.protection)
				continue;
			const uintptr_t start = reinterpret_cast<uintptr_t>(region.base);
			const uintptr_t end = start + region.size;
			const uintptr_t mergedStart = start < candidateStart ? start : candidateStart;
			const uintptr_t mergedEnd = end > candidateEnd ? end : candidateEnd;
			region.base = reinterpret_cast<void*>(mergedStart);
			region.size = mergedEnd - mergedStart;
			return;
		}
		regions.push_back(candidate);
	}

	// Restores the first count regions, retrying each once; false if any stays changed.
	bool RestoreRegions(const std::vector<ProtectedRegion>& regions, size_t count) {
		bool restored = true;
		for (size_t i = 0; i < count; ++i) {
			const ProtectedRegion& region = regions[i];
			ULONG ignored = 0;
			if (NT_SUCCESS(MemUtil::HookedVirtualProtect(region.base, region.size, region.protection, ignored)))
				continue;
			if (!NT_SUCCESS(MemUtil::HookedVirtualProtect(region.base, region.size, region.protection, ignored)))
				restored = false;
		}
		return restored;
	}

	// On failure the regions already opened are restored and restored reports whether that held.
	bool OpenRegions(const std::vector<ProtectedRegion>& regions, bool& restored) {
		restored = true;
		for (size_t i = 0; i < regions.size(); ++i) {
			ULONG ignored = 0;
			if (!NT_SUCCESS(MemUtil::HookedVirtualProtect(regions[i].base, regions[i].size,
				PAGE_EXECUTE_READWRITE, ignored))) {
				restored = RestoreRegions(regions, i);
				return false;
			}
		}
		return true;
	}

	// The outcome is decided by reading the bytes back; protectionRestored covers every region.
	MemUtil::PatchResult WriteBytes(void* address, const void* bytes, size_t length,
		bool& protectionRestored, bool pointerStore = false) {
		using MemUtil::PatchResult;
		protectionRestored = true;
		std::vector<unsigned char> original(length);
		std::vector<ProtectedRegion> regions;
		if (!QueryRegions(address, length, regions))
			return PatchResult::Original;
		if (!OpenRegions(regions, protectionRestored))
			return protectionRestored ? PatchResult::Original : PatchResult::RollbackIncomplete;

		memcpy(original.data(), address, length);
		StorePatchBytes(address, bytes, length, pointerStore);
		PatchResult result = PatchResult::Installed;
		if (!FlushInstructionCache(GetCurrentProcess(), address, length)) {
			// Without a flush a stale copy could still run, so put the original back and read it.
			StorePatchBytes(address, original.data(), length, pointerStore);
			const bool rollbackFlushed = FlushInstructionCache(GetCurrentProcess(), address, length) != FALSE;
			result = rollbackFlushed && memcmp(address, original.data(), length) == 0
				? PatchResult::Original : PatchResult::RollbackIncomplete;
		}
		else if (memcmp(address, bytes, length) != 0) {
			result = PatchResult::RollbackIncomplete;
		}

		protectionRestored = RestoreRegions(regions, regions.size());
		if (!protectionRestored) {
			if (result == PatchResult::Original)
				result = PatchResult::RollbackIncomplete;
			LOG_WARNING("(MEMORY) Page protection at 0x" << std::hex << reinterpret_cast<uintptr_t>(address)
				<< std::dec << " stayed writable after a patch" << std::endl);
		}
		return result;
	}

	std::vector<unsigned char> JumpPatch(void* site, void* target, size_t length) {
		std::vector<unsigned char> patch(length, 0x90);
		patch[0] = 0xE9;
		const uint32_t relative = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(target)
			- reinterpret_cast<uintptr_t>(site) - 5);
		memcpy(patch.data() + 1, &relative, sizeof(relative));
		return patch;
	}
}

void MemUtil::BeginBootPatchBundle() {
	g_bootBundle = {};
	g_bootBundle.staging = true;
	g_bootBundleEnabled.store(false);
	LOG_INFO("(BOOT BUNDLE) Staging startup members; install messages are provisional until commit" << std::endl);
}

bool MemUtil::BootPatchMember(const char* name) {
	g_bootBundle.member = name;
	return !g_bootBundle.failed;
}

void MemUtil::FailBootPatchBundle() {
	if (g_bootBundle.staging && !g_bootBundle.failed)
		g_bootBundle.failed = g_bootBundle.member;
}

bool MemUtil::BootPatchBundleEnabled() {
	return g_bootBundleEnabled.load();
}

bool MemUtil::BootPatchStaged(uintptr_t address) {
	for (const BootPatch& patch : g_bootBundle.patches) {
		const uintptr_t start = reinterpret_cast<uintptr_t>(patch.address);
		if (address >= start && address - start < patch.original.size())
			return true;
	}
	return false;
}

bool MemUtil::CommitBootPatchBundle() {
	bool restored = true;
	std::vector<ProtectedRegion> regions;
	// Query every member once and retain each unique region's pre-install protection.
	for (const BootPatch& patch : g_bootBundle.patches) {
		if (g_bootBundle.failed)
			break;
		g_bootBundle.member = patch.member;
		std::vector<ProtectedRegion> patchRegions;
		if (!QueryRegions(patch.address, patch.original.size(), patchRegions)) {
			FailBootPatchBundle();
			break;
		}
		for (const ProtectedRegion& region : patchRegions)
			AddBundleRegion(regions, region);
	}
	g_bootBundle.staging = false;
	size_t attempted = 0;
	bool sourceWriteAttempted = false;
	if (!g_bootBundle.failed) {
		bool openRestored = true;
		if (!OpenRegions(regions, openRestored)) {
			restored = restored && openRestored;
			g_bootBundle.failed = g_bootBundle.member;
		} else {
			// Do not write any member until every original has survived the open.
			for (const BootPatch& patch : g_bootBundle.patches) {
				g_bootBundle.member = patch.member;
				if (!MatchesBytes(reinterpret_cast<uintptr_t>(patch.address), patch.original)) {
					g_bootBundle.failed = patch.member;
					break;
				}
			}
			if (!g_bootBundle.failed) {
				for (const BootPatch& patch : g_bootBundle.patches) {
					g_bootBundle.member = patch.member;
					++attempted;
					sourceWriteAttempted = true;
					StorePatchBytes(patch.address, patch.replacement.data(),
						patch.replacement.size(), patch.pointerStore);
					if (!FlushInstructionCache(GetCurrentProcess(), patch.address, patch.replacement.size())
						|| memcmp(patch.address, patch.replacement.data(), patch.replacement.size()) != 0) {
						g_bootBundle.failed = patch.member;
						break;
					}
				}
			}
			if (g_bootBundle.failed) {
				while (attempted > 0) {
					const BootPatch& patch = g_bootBundle.patches[--attempted];
					StorePatchBytes(patch.address, patch.original.data(),
						patch.original.size(), patch.pointerStore);
					const bool rollbackFlushed = FlushInstructionCache(GetCurrentProcess(),
						patch.address, patch.original.size()) != FALSE;
					if (!rollbackFlushed
						|| memcmp(patch.address, patch.original.data(), patch.original.size()) != 0) {
						restored = false;
						LOG_ERROR("(BOOT BUNDLE) Rollback failed: " << patch.member << " at 0x"
							<< std::hex << reinterpret_cast<uintptr_t>(patch.address) << std::dec << std::endl);
					}
				}
			}
			if (!RestoreRegions(regions, regions.size()))
				restored = false;
		}
	}
	if (g_bootBundle.failed) {
		LOG_WARNING("(BOOT BUNDLE) Failed member: " << g_bootBundle.failed
			<< "; bundle stayed off; rollback=" << (restored ? "complete" : "incomplete") << std::endl);
		// A gateway stays alive once a source write could have published it.
		if (!sourceWriteAttempted)
			for (void* gateway : g_bootBundle.gateways)
				VirtualFree(gateway, 0, MEM_RELEASE);
	} else {
		g_bootBundleEnabled.store(true);
		LOG_INFO("(BOOT BUNDLE) Installed all startup members" << std::endl);
	}
	g_bootBundle.patches.clear();
	g_bootBundle.gateways.clear();
	return g_bootBundleEnabled.load();
}

bool MemUtil::BootPatchBundleStaging() { return g_bootBundle.staging; }

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
	if (g_bootBundle.staging)
		return StageBootPatch(address, changeToMake, len) ? PatchResult::Installed : PatchResult::Original;
	bool protectionRestored = true;
	return WriteBytes(address, changeToMake, len, protectionRestored);
}

MemUtil::PatchResult MemUtil::WritePointerPatch(LPVOID address, LPCVOID value) {
	if (reinterpret_cast<uintptr_t>(address) % alignof(void*) != 0)
		return PatchResult::Original;
	if (g_bootBundle.staging)
		return StageBootPatch(address, value, sizeof(void*), true)
			? PatchResult::Installed : PatchResult::Original;
	bool protectionRestored = true;
	return WriteBytes(address, value, sizeof(void*), protectionRestored, true);
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
	const std::vector<unsigned char> patch = JumpPatch(hookSpot, ourFunct, static_cast<size_t>(len));
	return PatchAdr(hookSpot, patch.data(), patch.size());
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
	if (!src || !dst)
	{
		FailBootPatchBundle();
		return nullptr;
	}

	// Calculate safe instruction length without splitting instructions
	unsigned int hookLen = 0;
	while (hookLen < 5) {
		PBYTE next = DetourCopyInstruction(nullptr, src + hookLen, nullptr);
		if (!next || next <= src + hookLen) {
			hookLen = (len >= 5) ? len : 5;
			break;
		}
		hookLen = (unsigned int)(next - src);
	}

	if (len > hookLen) {
		hookLen = len;
	}

	// Relocating the stolen prologue can make it grow (a short jump promotes to a
	// near jump, and so on), so give the gateway generous headroom over hookLen.
	const unsigned int gatewaySize = hookLen * 2 + 16;

	// Create the gateway (relocated prologue + the jmp back to the original).
	auto gateway = (PBYTE)VirtualAlloc(nullptr, gatewaySize, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);

	// Makes sure gateway doesn't equal null
	if (!gateway)
	{
		FailBootPatchBundle();
		return nullptr;
	}

	// Copy the overwritten bytes into the gateway, relocating any relative instructions
	// as we go, while keeping in mind any changes done by other tools (Steam, Rivatuner, ...). 
	// DetourCopyInstructionEx rewrites each relative operand for the gateway's address, and
	// reports via `extra` how many bytes the relocated instruction grew by.
	PBYTE srcPos = src;
	PBYTE dstPos = gateway;
	while (srcPos < src + hookLen) {
		PBYTE target = nullptr;
		LONG extra = 0;
		PBYTE srcNext = DetourCopyInstructionEx(dstPos, srcPos, &target, &extra);
		if (!srcNext || srcNext <= srcPos) {
			VirtualFree(gateway, 0, MEM_RELEASE);
			FailBootPatchBundle();
			return nullptr;
		}
		dstPos += (srcNext - srcPos) + extra;
		srcPos = srcNext;
	}

	// Jump from the end of the relocated prologue back into the original function,
	// continuing at the first instruction boundary past the bytes we stole (srcPos).
	*dstPos = (unsigned char)0xE9;
	*(uint32_t*)(dstPos + 1) = (uint32_t)(srcPos - (dstPos + 5));

	// Seal the gateway before any hook can reach it, and make sure no stale copy runs.
	ULONG writable = 0;
	if (!NT_SUCCESS(HookedVirtualProtect(gateway, gatewaySize, PAGE_EXECUTE_READ, writable))
		|| !FlushInstructionCache(GetCurrentProcess(), gateway, gatewaySize)) {
		VirtualFree(gateway, 0, MEM_RELEASE);
		FailBootPatchBundle();
		return nullptr;
	}

	const bool staging = g_bootBundle.staging;
	if (original)
		*original = reinterpret_cast<uintptr_t>(gateway);
	const std::vector<unsigned char> patch = JumpPatch(src, dst, hookLen);
	const PatchResult outcome = WritePatch(src, patch.data(), patch.size());
	if (result)
		*result = outcome;
	if (outcome == PatchResult::Installed) {
		if (staging)
			g_bootBundle.gateways.push_back(gateway);
		return gateway;
	}

	FailBootPatchBundle();
	if (outcome == PatchResult::RollbackIncomplete) {
		LOG_ERROR("(MEMORY) Hook at 0x" << std::hex << reinterpret_cast<uintptr_t>(src) << std::dec
			<< " may still be live; its trampoline is kept" << std::endl);
		return gateway;
	}
	// A staged write never reached the source; a direct one may have been live for a moment.
	if (staging)
		VirtualFree(gateway, 0, MEM_RELEASE);
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
		for (HKEY root : { HKEY_LOCAL_MACHINE, HKEY_CURRENT_USER }) {
			HKEY key = nullptr;
			if (RegOpenKeyExW(root, L"Software\\Wine", 0, KEY_READ, &key) == ERROR_SUCCESS) {
				RegCloseKey(key);
				return "Wine";
			}
		}
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
	}

	NTSTATUS HookedVirtualProtect(LPVOID address, SIZE_T len, ULONG newProtection,
		ULONG& oldProtection) {
		ULONGLONG start = GetTickCount64();
		// On Windows the game's own guard slows or refuses the documented call, so go to the kernel first.
		if (!MemoryHostPolicy().useWineApis) {
			static const bool resolved = [] {
				const std::optional<uint32_t> number = Detail::GetSyscallNumber("NtProtectVirtualMemory");
				if (!number)
					return false;
				Detail::ssnProtect = *number;
				return true;
			}();
			if (resolved) {
				PVOID baseAddress = address;
				SIZE_T regionSize = len;
				const NTSTATUS status = Detail::Syscall_NtProtectVirtualMemory(GetCurrentProcess(),
					&baseAddress, &regionSize, newProtection, &oldProtection);
				if (NT_SUCCESS(status)) {
					LogMemoryCall("VirtualProtect", address, len, true, newProtection,
						"syscall", status, 0, start);
					return status;
				}
				LogSyscallFailureOnce("VirtualProtect", address, len, true, newProtection, status, start);
			} else {
				LogUnresolvedSyscallOnce("VirtualProtect", address, len, true, newProtection, start);
			}
			start = GetTickCount64();
		}
		const BOOL ok = VirtualProtect(address, len, newProtection, &oldProtection);
		const DWORD error = ok ? ERROR_SUCCESS : GetLastError();
		LogMemoryCall("VirtualProtect", address, len, true, newProtection, "documented",
			ok ? STATUS_SUCCESS : STATUS_UNSUCCESSFUL, error, start);
		return ok ? STATUS_SUCCESS : STATUS_UNSUCCESSFUL;
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

	PreparedPatchSet::~PreparedPatchSet() {
		ReleaseNativeEntries();
	}

	void PreparedPatchSet::ReleaseNativeEntries() noexcept {
		for (size_t i = 0; i < siteCount_; ++i) {
			if (sites_[i].ownsNativeEntry)
				VirtualFree(sites_[i].nativeEntry, 0, MEM_RELEASE);
			sites_[i].nativeEntry = nullptr;
			sites_[i].ownsNativeEntry = false;
		}
	}

	bool PreparedPatchSet::Prepare(std::span<const PreparedPatchRequest> requests) {
		if (operationAttempted_ && result_ != PatchResult::Original) {
			failure_ = Failure::InvalidRequest;
			return false;
		}
		for (size_t i = 0; i < siteCount_; ++i) {
			if (!sites_[i].ownsNativeEntry)
				continue;
			failure_ = Failure::InvalidRequest;
			failureAddress_ = sites_[i].address;
			return false;
		}
		ReleaseNativeEntries();
		sites_ = {};
		regions_ = {};
		siteCount_ = 0;
		regionCount_ = 0;
		result_ = PatchResult::Original;
		failure_ = Failure::None;
		failureAddress_ = 0;
		failureCode_ = ERROR_SUCCESS;
		prepared_ = false;
		operationAttempted_ = false;

		if (requests.empty() || requests.size() > MaxSites) {
			failure_ = requests.empty() ? Failure::InvalidRequest : Failure::Capacity;
			return false;
		}

		useWineApis_ = MemoryHostPolicy().useWineApis;
		const std::optional<uint32_t> protectNumber = Detail::GetSyscallNumber("NtProtectVirtualMemory");
		protectSyscallResolved_ = protectNumber.has_value();
		if (protectNumber)
			Detail::ssnProtect = *protectNumber;
		(void)Detail::GetSyscallNumber("NtQueryVirtualMemory");

		for (const PreparedPatchRequest& request : requests) {
			Site& site = sites_[siteCount_];
			site.address = reinterpret_cast<uintptr_t>(request.address);
			if (!site.address || request.replacementTarget == 0) {
				failure_ = Failure::InvalidRequest;
				failureAddress_ = site.address;
				return false;
			}

			if (request.kind == PreparedPatchKind::Call) {
				site.length = 5;
				if (!request.expectedTarget) {
					failure_ = Failure::InvalidRequest;
					failureAddress_ = site.address;
					return false;
				}
				site.original[0] = 0xE8;
				const uint32_t expectedRelative = static_cast<uint32_t>(request.expectedTarget - (site.address + 5));
				memcpy(site.original.data() + 1, &expectedRelative, sizeof(expectedRelative));
				site.replacement[0] = 0xE8;
				const uint32_t replacementRelative = static_cast<uint32_t>(request.replacementTarget - (site.address + 5));
				memcpy(site.replacement.data() + 1, &replacementRelative, sizeof(replacementRelative));
				if (request.captureNativeEntry)
					site.nativeEntry = reinterpret_cast<void*>(request.expectedTarget);
			}
			else {
				site.length = request.expectedBytes.size();
				if (site.length < 5 || site.length > MaxPatchBytes) {
					failure_ = Failure::InvalidRequest;
					failureAddress_ = site.address;
					return false;
				}
				memcpy(site.original.data(), request.expectedBytes.data(), site.length);
				memset(site.replacement.data(), 0x90, site.length);
				site.replacement[0] = 0xE9;
				const uint32_t relative = static_cast<uint32_t>(request.replacementTarget - (site.address + 5));
				memcpy(site.replacement.data() + 1, &relative, sizeof(relative));
			}

			const uintptr_t siteEnd = site.address + site.length;
			if (siteEnd < site.address) {
				failure_ = Failure::InvalidRequest;
				failureAddress_ = site.address;
				return false;
			}
			for (size_t previous = 0; previous < siteCount_; ++previous) {
				const Site& existing = sites_[previous];
				if (site.address < existing.address + existing.length
					&& existing.address < siteEnd) {
					failure_ = Failure::InvalidRequest;
					failureAddress_ = site.address;
					return false;
				}
			}
			if (!MatchesBytes(site.address, std::span<const unsigned char>(site.original.data(), site.length))) {
				failure_ = Failure::BytesMismatch;
				failureAddress_ = site.address;
				return false;
			}
			++siteCount_;

			if (request.captureNativeEntry && request.kind == PreparedPatchKind::Jump) {
				auto* gateway = static_cast<unsigned char*>(VirtualAlloc(nullptr, site.length + 5,
					MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE));
				if (!gateway) {
					failure_ = Failure::GatewayFailed;
					failureAddress_ = site.address;
					failureCode_ = GetLastError();
					return false;
				}
				memcpy(gateway, site.original.data(), site.length);
				gateway[site.length] = 0xE9;
				const uint32_t backRelative = static_cast<uint32_t>(site.address + site.length
					- (reinterpret_cast<uintptr_t>(gateway) + site.length + 5));
				memcpy(gateway + site.length + 1, &backRelative, sizeof(backRelative));
				DWORD previousProtection = 0;
				if (!VirtualProtect(gateway, site.length + 5, PAGE_EXECUTE_READ, &previousProtection)
					|| !FlushInstructionCache(GetCurrentProcess(), gateway, site.length + 5)) {
					failure_ = Failure::GatewayFailed;
					failureAddress_ = site.address;
					failureCode_ = GetLastError();
					VirtualFree(gateway, 0, MEM_RELEASE);
					return false;
				}
				site.nativeEntry = gateway;
				site.ownsNativeEntry = true;
			}
			else if (request.captureNativeEntry && request.kind != PreparedPatchKind::Call) {
				failure_ = Failure::InvalidRequest;
				failureAddress_ = site.address;
				return false;
			}

			uintptr_t cursor = site.address;
			while (cursor < siteEnd) {
				MEMORY_BASIC_INFORMATION info{};
				if (!NT_SUCCESS(HookedQueryVirtualMemory(reinterpret_cast<LPVOID>(cursor), &info, sizeof(info)))
					|| info.State != MEM_COMMIT || !info.RegionSize) {
					failure_ = Failure::QueryFailed;
					failureAddress_ = cursor;
					failureCode_ = GetLastError();
					return false;
				}
				const uintptr_t queryBase = reinterpret_cast<uintptr_t>(info.BaseAddress);
				const uintptr_t queryEnd = queryBase + info.RegionSize;
				if (queryBase > cursor || queryEnd <= cursor) {
					failure_ = Failure::QueryFailed;
					failureAddress_ = cursor;
					return false;
				}
				const uintptr_t regionEnd = queryEnd < siteEnd ? queryEnd : siteEnd;
				bool merged = false;
				for (size_t i = 0; i < regionCount_; ++i) {
					Region& region = regions_[i];
					if (region.queryBase != queryBase || region.querySize != info.RegionSize
						|| region.protection != info.Protect)
						continue;
					const uintptr_t oldEnd = region.base + region.size;
					const uintptr_t begin = region.base < cursor ? region.base : cursor;
					const uintptr_t end = oldEnd > regionEnd ? oldEnd : regionEnd;
					region.base = begin;
					region.size = end - begin;
					merged = true;
					break;
				}
				if (!merged) {
					if (regionCount_ == regions_.size()) {
						failure_ = Failure::Capacity;
						failureAddress_ = cursor;
						return false;
					}
					regions_[regionCount_++] = { cursor, regionEnd - cursor, queryBase,
						info.RegionSize, info.Protect, false };
				}
				cursor = regionEnd;
			}
		}

		prepared_ = true;
		return true;
	}

	bool PreparedPatchSet::SetRegionProtection(Region& region, ULONG protection) noexcept {
		if (!useWineApis_ && protectSyscallResolved_) {
			PVOID baseAddress = reinterpret_cast<PVOID>(region.base);
			SIZE_T regionSize = region.size;
			ULONG previous = 0;
			const NTSTATUS status = Detail::Syscall_NtProtectVirtualMemory(GetCurrentProcess(),
				&baseAddress, &regionSize, protection, &previous);
			if (NT_SUCCESS(status))
				return true;
		}
		DWORD previous = 0;
		if (VirtualProtect(reinterpret_cast<LPVOID>(region.base), region.size, protection, &previous))
			return true;
		failureCode_ = GetLastError();
		return false;
	}

	bool PreparedPatchSet::RestoreOpenedRegions() noexcept {
		bool restored = true;
		for (size_t i = regionCount_; i > 0; --i) {
			Region& region = regions_[i - 1];
			if (!region.opened)
				continue;
			if (!SetRegionProtection(region, region.protection)
				&& !SetRegionProtection(region, region.protection)) {
				restored = false;
				failureAddress_ = region.base;
			}
			region.opened = false;
		}
		return restored;
	}

	bool PreparedPatchSet::MatchesOriginal(size_t count) const noexcept {
		for (size_t i = 0; i < count; ++i)
			if (memcmp(reinterpret_cast<const void*>(sites_[i].address), sites_[i].original.data(), sites_[i].length) != 0)
				return false;
		return true;
	}

	PatchResult PreparedPatchSet::Commit() noexcept {
		operationAttempted_ = true;
		if (!prepared_) {
			result_ = PatchResult::Original;
			return result_;
		}
		failure_ = Failure::None;
		failureAddress_ = 0;
		failureCode_ = ERROR_SUCCESS;
		size_t opened = 0;
		for (; opened < regionCount_; ++opened) {
			if (!SetRegionProtection(regions_[opened], PAGE_EXECUTE_READWRITE)) {
				failure_ = Failure::OpenFailed;
				failureAddress_ = regions_[opened].base;
				const bool restored = RestoreOpenedRegions();
				result_ = restored && MatchesOriginal(siteCount_)
					? PatchResult::Original : PatchResult::RollbackIncomplete;
				return result_;
			}
			regions_[opened].opened = true;
		}

		for (size_t i = 0; i < siteCount_; ++i) {
			if (memcmp(reinterpret_cast<const void*>(sites_[i].address), sites_[i].original.data(), sites_[i].length) == 0)
				continue;
			failure_ = Failure::CommitMismatch;
			failureAddress_ = sites_[i].address;
			const bool restored = RestoreOpenedRegions();
			result_ = restored && MatchesOriginal(siteCount_)
				? PatchResult::Original : PatchResult::RollbackIncomplete;
			return result_;
		}

		size_t attempted = 0;
		bool writeFailed = false;
		for (; attempted < siteCount_; ++attempted) {
			Site& site = sites_[attempted];
			memcpy(reinterpret_cast<void*>(site.address), site.replacement.data(), site.length);
			if (!FlushInstructionCache(GetCurrentProcess(), reinterpret_cast<LPCVOID>(site.address), site.length)) {
				failure_ = Failure::FlushFailed;
				failureAddress_ = site.address;
				failureCode_ = GetLastError();
				writeFailed = true;
				++attempted;
				break;
			}
			if (memcmp(reinterpret_cast<const void*>(site.address), site.replacement.data(), site.length) != 0) {
				failure_ = Failure::ReadbackFailed;
				failureAddress_ = site.address;
				writeFailed = true;
				++attempted;
				break;
			}
		}
		if (writeFailed) {
			bool rollbackFlushed = true;
			for (size_t i = attempted; i > 0; --i) {
				Site& site = sites_[i - 1];
				memcpy(reinterpret_cast<void*>(site.address), site.original.data(), site.length);
				if (!FlushInstructionCache(GetCurrentProcess(), reinterpret_cast<LPCVOID>(site.address), site.length)) {
					rollbackFlushed = false;
					failureCode_ = GetLastError();
				}
			}
			const bool restored = RestoreOpenedRegions();
			result_ = rollbackFlushed && restored && MatchesOriginal(siteCount_)
				? PatchResult::Original : PatchResult::RollbackIncomplete;
			return result_;
		}

		const bool restored = RestoreOpenedRegions();
		if (!restored) {
			failure_ = Failure::RestoreFailed;
			result_ = PatchResult::RollbackIncomplete;
			return result_;
		}
		for (size_t i = 0; i < siteCount_; ++i) {
			if (memcmp(reinterpret_cast<const void*>(sites_[i].address), sites_[i].replacement.data(), sites_[i].length) != 0) {
				failure_ = Failure::ReadbackFailed;
				failureAddress_ = sites_[i].address;
				result_ = PatchResult::RollbackIncomplete;
				return result_;
			}
		}
		result_ = PatchResult::Installed;
		return result_;
	}

	PatchResult PreparedPatchSet::Rollback() noexcept {
		operationAttempted_ = true;
		failure_ = Failure::None;
		failureAddress_ = 0;
		failureCode_ = ERROR_SUCCESS;
		if (!prepared_) {
			result_ = PatchResult::Original;
			return result_;
		}
		if (result_ == PatchResult::Original && MatchesOriginal(siteCount_)) {
			result_ = PatchResult::Original;
			return result_;
		}

		for (size_t i = 0; i < regionCount_; ++i) {
			if (!SetRegionProtection(regions_[i], PAGE_EXECUTE_READWRITE)) {
				failure_ = Failure::OpenFailed;
				failureAddress_ = regions_[i].base;
				const bool restored = RestoreOpenedRegions();
				result_ = restored && MatchesOriginal(siteCount_)
					? PatchResult::Original : PatchResult::RollbackIncomplete;
				return result_;
			}
			regions_[i].opened = true;
		}

		for (size_t i = 0; i < siteCount_; ++i) {
			const Site& site = sites_[i];
			const void* bytes = reinterpret_cast<const void*>(site.address);
			if (memcmp(bytes, site.original.data(), site.length) == 0
				|| memcmp(bytes, site.replacement.data(), site.length) == 0)
				continue;
			failure_ = Failure::CommitMismatch;
			failureAddress_ = site.address;
			const bool restored = RestoreOpenedRegions();
			result_ = restored && MatchesOriginal(siteCount_)
				? PatchResult::Original : PatchResult::RollbackIncomplete;
			return result_;
		}

		bool flushed = true;
		for (size_t i = 0; i < siteCount_; ++i) {
			Site& site = sites_[i];
			if (memcmp(reinterpret_cast<const void*>(site.address), site.original.data(), site.length) != 0)
				memcpy(reinterpret_cast<void*>(site.address), site.original.data(), site.length);
			if (!FlushInstructionCache(GetCurrentProcess(), reinterpret_cast<LPCVOID>(site.address), site.length)) {
				flushed = false;
				failureCode_ = GetLastError();
			}
		}
		const bool restored = RestoreOpenedRegions();
		result_ = flushed && restored && MatchesOriginal(siteCount_)
			? PatchResult::Original : PatchResult::RollbackIncomplete;
		if (result_ == PatchResult::RollbackIncomplete && failure_ == Failure::None)
			failure_ = !flushed ? Failure::FlushFailed : Failure::RestoreFailed;
		return result_;
	}

	void* PreparedPatchSet::NativeEntry(size_t site) const noexcept {
		return site < siteCount_ ? sites_[site].nativeEntry : nullptr;
	}

	void PreparedPatchSet::Report(const char* group) const {
		if (prepared_ && !operationAttempted_) {
			LOG_INFO(group << " was prepared; no patch was attempted" << std::endl);
			return;
		}
		if (!prepared_ && failure_ != Failure::None) {
			if (failure_ == Failure::BytesMismatch)
				LOG_ERROR(group << " expected bytes did not match at 0x" << std::hex << failureAddress_
					<< std::dec << "; no patch was written" << std::endl);
			else
				LOG_ERROR(group << " could not be prepared at 0x" << std::hex << failureAddress_
					<< std::dec << " (error " << failureCode_ << "); no patch was written" << std::endl);
			return;
		}
		if (result_ == PatchResult::Installed)
			LOG_INFO(group << " installed all " << siteCount_ << " sites" << std::endl);
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
