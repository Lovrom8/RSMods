#include "stdafx.h"
#include "MemUtil.hpp"
#include "Lib/Detours/detours.h"

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
	DWORD dwOldProt, dwDummy, ret;

	clock_t before = clock();

	// Save old Virtual Protect status, but allow us to Execute, Read, and Write to the executable's memory so we can place our hook.
	ret = HookedVirtualProtect(address, len, PAGE_EXECUTE_READWRITE, dwOldProt);
	if (!NT_SUCCESS(ret))
	{
		printf_s("MemUtil::PatchAdr Failed 1: Addr: 0x%X | Status: 0x%08X | Time to run in sec: %f\n", (uintptr_t)address, ret, (float)(clock() - before) / CLOCKS_PER_SEC); // Can't use log here, need to use printf.
		return false;
	}

	// Write what we had in changeToMake to address, for len bytes.
	memcpy(address, changeToMake, len);

	// Force the CPU to dump it's cached instruction. Also resets the virtual protect to the status we saved earlier in this function. 
	FlushInstructionCache(GetCurrentProcess(), address, len);

	ret = HookedVirtualProtect(address, len, dwOldProt, dwDummy);
	if (!NT_SUCCESS(ret))
	{
		printf_s("MemUtil::PatchAdr Failed 2: Addr: 0x%X | Status: 0x%08X\n", (uintptr_t)address, ret); // Can't use log here, need to use printf.
		return false;
	}

	return true;
}

bool MemUtil::PlaceHook(VersioningStruct<uintptr_t>& hookSpot, void* ourFunct, int len, bool addBaseHandle) {
	uintptr_t addr = hookSpot.Get() + (addBaseHandle ? Offsets::baseHandle : 0);
	return PlaceHook((void*)(addr), ourFunct, len);
}

/// <summary>
/// Place x86 ASM (__asm) hook
/// </summary>
/// <param name="hookSpot"> - Where should we hook?</param>
/// <param name="ourFunct"> - What should we run (x86 ASM)</param>
/// <param name="len"> - How long is the first command (x86 ASM length)</param>
/// <returns>Can we place the hook?</returns>
bool MemUtil::PlaceHook(void* hookSpot, void* ourFunct, int len)
{
	// Verify that len is at least 5. A relative jump instruction is 5 bytes, so we cannot work with any value less than that.
	if (len < 5)
		return false;

	// Save old Virtual Protect status, but allow us to Execute, Read, and Write to the executable's memory so we can place our hook.
	DWORD oldProtect;
	DWORD ret;
	clock_t before = clock();

	ret = HookedVirtualProtect(hookSpot, len, PAGE_EXECUTE_READWRITE, oldProtect);
	if (!NT_SUCCESS(ret))
	{
		printf_s("MemUtil::PlaceHook Failed 1: Addr: 0x%X | Status: 0x%08X | Time to run in sec: %f\n", (uintptr_t)hookSpot, ret, (float)(clock() - before) / CLOCKS_PER_SEC); // Can't use log here, need to use printf.
		return false;
	}

	// Place x86 assembly NOP characters where we want our hook to be placed.
	memset(hookSpot, 0x90, len);

	// Get the relative address from our hook to where our function is actually stored, minus the length of the jump instruction (5).
	uint32_t relativeAddr = ((uint32_t)ourFunct - (uint32_t)hookSpot) - 5;

	// Place relative jump at hookSpot.
	*(BYTE*)hookSpot = 0xE9;
	*(uint32_t*)((uint32_t)hookSpot + 1) = relativeAddr;

	// Reset the virtual protect to the status we saved earlier in this function. 
	DWORD backup;

	ret = HookedVirtualProtect(hookSpot, len, oldProtect, backup);
	if (!NT_SUCCESS(ret))
	{
		printf_s("MemUtil::PlaceHook Failed 2: Addr: 0x%X | Status: 0x%08X\n", (uintptr_t)hookSpot, ret); // Can't use log here, need to use printf.
		return false;
	}

	return true;
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
	if (PlaceHook(src, dst, hookLen))
		return gateway;

	VirtualFree(gateway, 0, MEM_RELEASE);
	return nullptr;
}

/// <summary>
/// Will reading this pointer kill the game?
/// </summary>
/// <param name="p"> - Pointer</param>
/// <returns>True - Bad Pointer, do not read. False - Safe to read.</returns>
bool MemUtil::IsBadReadPtr(void* pointer)
{
	//NOTE: We are very aware this is not exactly the optimal (neither completely thread safe nor very fast) way to handle pointers to a non-initialized variable.
	//      but for now it will have to do the job until we figure out a better current menu check.

	MEMORY_BASIC_INFORMATION mbi = { 0 };
	if (NT_SUCCESS(HookedQueryVirtualMemory(pointer, &mbi, sizeof(mbi))))
	{
		uint32_t mask = (PAGE_READONLY | PAGE_READWRITE | PAGE_WRITECOPY | PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE | PAGE_EXECUTE_WRITECOPY);
		bool badPtr = !(mbi.Protect & mask);

		// Check the page is not a guard page
		if (mbi.Protect & (PAGE_GUARD | PAGE_NOACCESS)) badPtr = true;

		return badPtr;
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
bool MemUtil::IsRunningOnWine()
{
	static bool checked = false;
	static bool isWine = false;

	if (!checked) {
		HMODULE hNtdll = GetModuleHandleA("ntdll.dll");
		if (hNtdll && GetProcAddress(hNtdll, "wine_get_version") != nullptr) {
			isWine = true;
		}
		checked = true;
	}

	return isWine;
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

		static __declspec(naked) NTSTATUS NTAPI Syscall_NtProtectVirtualMemory(HANDLE ProcessHandle, PVOID* BaseAddress, PSIZE_T RegionSize, ULONG NewProtect, PULONG OldProtect)
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

		static __declspec(naked) NTSTATUS NTAPI Syscall_NtQueryVirtualMemory(HANDLE ProcessHandle, PVOID BaseAddress, ULONG MemoryInformationClass, PVOID MemoryInformation, SIZE_T MemoryInformationLength, PSIZE_T ReturnLength)
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

	NTSTATUS HookedVirtualProtect(LPVOID address, SIZE_T len, ULONG newProtection, ULONG& oldProtection)
	{
		if (IsRunningOnWine()) {
			DWORD oldProt = 0;
			if (VirtualProtect(address, len, newProtection, &oldProt)) {
				oldProtection = oldProt;
				return STATUS_SUCCESS;
			}
			return STATUS_UNSUCCESSFUL;
		}

		static bool initialized = false;
		if (!initialized) {
			Detail::ssnProtect = Detail::GetSyscallNumber("NtProtectVirtualMemory", 0x50);
			initialized = true;
		}

		PVOID baseAddress = address;
		SIZE_T regionSize = len;
		ULONG oldProt = 0;
		NTSTATUS status = Detail::Syscall_NtProtectVirtualMemory(GetCurrentProcess(), &baseAddress, &regionSize, newProtection, &oldProt);

		if (NT_SUCCESS(status)) {
			oldProtection = oldProt;
			return status;
		}

		return status;
	}

	NTSTATUS HookedQueryVirtualMemory(LPVOID address, PMEMORY_BASIC_INFORMATION memoryBuffer, SIZE_T dwLength)
	{
		if (IsRunningOnWine()) {
			if (VirtualQuery(address, memoryBuffer, dwLength) != 0) {
				return STATUS_SUCCESS;
			}
			return STATUS_UNSUCCESSFUL;
		}

		static bool initialized = false;
		if (!initialized) {
			Detail::ssnQuery = Detail::GetSyscallNumber("NtQueryVirtualMemory", 0x23);
			initialized = true;
		}

		SIZE_T returnLength = 0;
		return Detail::Syscall_NtQueryVirtualMemory(GetCurrentProcess(), address, 0 /* MemoryBasicInformation */, memoryBuffer, dwLength, &returnLength);
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
