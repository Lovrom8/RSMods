#include "stdafx.h"
#include "MemUtil.hpp"
#include "SettingEdits.hpp"
#include "Lib/Detours/detours.h"

namespace {
	struct BootPatch {
		void* address;
		std::vector<unsigned char> original;
		std::vector<unsigned char> replacement;
		const char* member;
		DWORD protection = 0;
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

	bool StageBootPatch(void* address, const void* replacement, size_t length) {
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
			std::vector<unsigned char>(length), g_bootBundle.member };
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
	bool protectionRestored = true;
	// Preflight protection and unchanged originals before the first game write.
	for (BootPatch& patch : g_bootBundle.patches) {
		if (g_bootBundle.failed)
			break;
		g_bootBundle.member = patch.member;
		DWORD oldProtection = 0, ignored = 0;
		if (!MatchesBytes(reinterpret_cast<uintptr_t>(patch.address), patch.original)
			|| !NT_SUCCESS(HookedVirtualProtect(patch.address, patch.original.size(),
				PAGE_EXECUTE_READWRITE, oldProtection))) {
			FailBootPatchBundle();
			break;
		}
		patch.protection = oldProtection;
		if (!NT_SUCCESS(HookedVirtualProtect(patch.address, patch.original.size(),
			oldProtection, ignored))) {
			FailBootPatchBundle();
			protectionRestored = NT_SUCCESS(HookedVirtualProtect(patch.address,
				patch.original.size(), oldProtection, ignored));
		}
	}
	g_bootBundle.staging = false;
	size_t attempted = 0;
	if (!g_bootBundle.failed) {
		for (const BootPatch& patch : g_bootBundle.patches) {
			++attempted;
			if (!PatchAdr(patch.address, patch.replacement.data(), patch.replacement.size())) {
				g_bootBundle.failed = patch.member;
				break;
			}
		}
	}
	bool restored = protectionRestored;
	if (g_bootBundle.failed) {
		while (attempted > 0) {
			const BootPatch& patch = g_bootBundle.patches[--attempted];
			const bool bytesRestored = PatchAdr(patch.address, patch.original.data(), patch.original.size());
			DWORD ignored = 0;
			const bool pageRestored = NT_SUCCESS(HookedVirtualProtect(patch.address,
				patch.original.size(), patch.protection, ignored));
			if (!bytesRestored || !pageRestored) {
				restored = false;
				LOG_ERROR("(BOOT BUNDLE) Rollback failed: " << patch.member << " at 0x"
					<< std::hex << reinterpret_cast<uintptr_t>(patch.address) << std::dec << std::endl);
			}
		}
		LOG_WARNING("(BOOT BUNDLE) Failed member: " << g_bootBundle.failed
			<< "; bundle stayed off; rollback=" << (restored ? "complete" : "incomplete") << std::endl);
		// Keep gateways alive if a hook could still reference one after failed rollback.
		if (restored)
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
	if (g_bootBundle.staging)
		return StageBootPatch(address, changeToMake, len);
	DWORD dwOldProt, dwDummy, ret;


	// Save old Virtual Protect status, but allow us to Execute, Read, and Write to the executable's memory so we can place our hook.
	ret = HookedVirtualProtect(address, len, PAGE_EXECUTE_READWRITE, dwOldProt);
	if (!NT_SUCCESS(ret))
	{
		return false;
	}

	// Write what we had in changeToMake to address, for len bytes.
	memcpy(address, changeToMake, len);

	// Force the CPU to dump it's cached instruction. Also resets the virtual protect to the status we saved earlier in this function. 
	FlushInstructionCache(GetCurrentProcess(), address, len);

	ret = HookedVirtualProtect(address, len, dwOldProt, dwDummy);
	if (!NT_SUCCESS(ret))
	{
		return false;
	}

	return true;
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
	std::vector<unsigned char> patch(static_cast<size_t>(len), 0x90);
	patch[0] = 0xE9;
	const uint32_t relative = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(ourFunct)
		- reinterpret_cast<uintptr_t>(hookSpot) - 5);
	memcpy(patch.data() + 1, &relative, sizeof(relative));
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
PBYTE MemUtil::TrampHook(PBYTE src, PBYTE dst, unsigned int len)
{
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

	// The gateway is freshly written executable code; make sure no stale copy runs.
	FlushInstructionCache(GetCurrentProcess(), gateway, gatewaySize);

	// Place the hook at the destination
	if (PlaceHook(src, dst, hookLen)) {
		if (g_bootBundle.staging)
			g_bootBundle.gateways.push_back(gateway);
		return gateway;
	}

	VirtualFree(gateway, 0, MEM_RELEASE);
	FailBootPatchBundle();
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
		<< policy.overrideValue << "; path=documented first; syscall fallback="
		<< (policy.useWineApis ? "disabled" : "enabled on documented failure") << std::endl);
}

namespace {
	void LogMemoryCall(const char* operation, void* address, const char* path,
		NTSTATUS status, DWORD error, ULONGLONG start) {
		const ULONGLONG ms = GetTickCount64() - start;
		if (ms >= 50 || !NT_SUCCESS(status))
			LOG_WARNING("(MEMORY) " << operation << " address=0x" << std::hex
				<< reinterpret_cast<uintptr_t>(address) << " path=" << path
				<< " status=0x" << status << " win32=" << error << std::dec
				<< " ms=" << ms << std::endl);
	}
}

namespace MemUtil {
	namespace Detail {
		static uint32_t ssnProtect = 0x50;
		static uint32_t ssnQuery = 0x23;

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

		static uint32_t ExtractSyscallFromPE(const std::vector<BYTE>& buf, const char* funcName) {
			if (buf.empty()) return 0;

			auto dos = (PIMAGE_DOS_HEADER)buf.data();
			if (dos->e_magic != IMAGE_DOS_SIGNATURE) return 0;

			auto nt = (PIMAGE_NT_HEADERS)(buf.data() + dos->e_lfanew);
			if (nt->Signature != IMAGE_NT_SIGNATURE) return 0;

			DWORD exportRva = nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress;
			if (!exportRva) return 0;

			auto RvaToRaw = [&](DWORD rva) -> DWORD {
				PIMAGE_SECTION_HEADER sec = IMAGE_FIRST_SECTION(nt);
				for (WORD i = 0; i < nt->FileHeader.NumberOfSections; i++, sec++) {
					if (rva >= sec->VirtualAddress && rva < sec->VirtualAddress + sec->Misc.VirtualSize) {
						return rva - sec->VirtualAddress + sec->PointerToRawData;
					}
				}
				return 0;
				};

			DWORD expOffset = RvaToRaw(exportRva);
			if (!expOffset) return 0;

			auto exp = (PIMAGE_EXPORT_DIRECTORY)(buf.data() + expOffset);
			DWORD* names = (DWORD*)(buf.data() + RvaToRaw(exp->AddressOfNames));
			WORD* ords = (WORD*)(buf.data() + RvaToRaw(exp->AddressOfNameOrdinals));
			DWORD* funcs = (DWORD*)(buf.data() + RvaToRaw(exp->AddressOfFunctions));

			for (DWORD i = 0; i < exp->NumberOfNames; i++) {
				const char* name = (const char*)(buf.data() + RvaToRaw(names[i]));
				if (strcmp(name, funcName) == 0) {
					DWORD funcRva = funcs[ords[i]];
					BYTE* code = (BYTE*)(buf.data() + RvaToRaw(funcRva));
					if (code[0] == 0xB8) { // mov eax, <SSN>
						return *(uint32_t*)(code + 1);
					}
					break;
				}
			}

			return 0;
		}

		static uint32_t GetSyscallNumber(const char* funcName, uint32_t fallbackSSN) {
			HMODULE hNtdll = GetModuleHandleA("ntdll.dll");
			if (hNtdll) {
				BYTE* pFunc = (BYTE*)GetProcAddress(hNtdll, funcName);
				if (pFunc && pFunc[0] == 0xB8) {
					return *(uint32_t*)(pFunc + 1);
				}
			}

			// If memory was modified/hooked by VMProtect, parse fresh unhooked ntdll from disk
			static const std::vector<BYTE> ntdllDiskBuf = ReadNtdllFromDisk();
			uint32_t diskSsn = ExtractSyscallFromPE(ntdllDiskBuf, funcName);
			if (diskSsn != 0) {
				return diskSsn;
			}

			return fallbackSSN;
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
		const BOOL ok = VirtualProtect(address, len, newProtection, &oldProtection);
		const DWORD error = ok ? ERROR_SUCCESS : GetLastError();
		LogMemoryCall("VirtualProtect", address, "documented",
			ok ? STATUS_SUCCESS : STATUS_UNSUCCESSFUL, error, start);
		if (ok)
			return STATUS_SUCCESS;
		if (MemoryHostPolicy().useWineApis)
			return STATUS_UNSUCCESSFUL;

		start = GetTickCount64();
		static const bool initialized = [] {
			Detail::ssnProtect = Detail::GetSyscallNumber("NtProtectVirtualMemory", 0x50);
			return true;
		}();
		(void)initialized;
		PVOID baseAddress = address;
		SIZE_T regionSize = len;
		const NTSTATUS status = Detail::Syscall_NtProtectVirtualMemory(GetCurrentProcess(),
			&baseAddress, &regionSize, newProtection, &oldProtection);
		LogMemoryCall("VirtualProtect", address, "syscall", status, 0, start);
		return status;
	}

	NTSTATUS HookedQueryVirtualMemory(LPVOID address,
		PMEMORY_BASIC_INFORMATION memoryBuffer, SIZE_T dwLength) {
		ULONGLONG start = GetTickCount64();
		const SIZE_T result = VirtualQuery(address, memoryBuffer, dwLength);
		const DWORD error = result ? ERROR_SUCCESS : GetLastError();
		LogMemoryCall("VirtualQuery", address, "documented",
			result ? STATUS_SUCCESS : STATUS_UNSUCCESSFUL, error, start);
		if (result)
			return STATUS_SUCCESS;
		if (MemoryHostPolicy().useWineApis)
			return STATUS_UNSUCCESSFUL;

		start = GetTickCount64();
		static const bool initialized = [] {
			Detail::ssnQuery = Detail::GetSyscallNumber("NtQueryVirtualMemory", 0x23);
			return true;
		}();
		(void)initialized;
		SIZE_T returnLength = 0;
		const NTSTATUS status = Detail::Syscall_NtQueryVirtualMemory(GetCurrentProcess(),
			address, 0, memoryBuffer, dwLength, &returnLength);
		LogMemoryCall("VirtualQuery", address, "syscall", status, 0, start);
		return status;
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
