#include "stdafx.h"
#include "MemUtil.hpp"

typedef enum _MEMORY_INFORMATION_CLASS {
	MemoryBasicInformation,
	MemoryWorkingSetList,
	MemorySectionName
} MEMORY_INFORMATION_CLASS;

EXTERN_C NTSTATUS NtQueryVirtualMemory(__in HANDLE ProcessHandle,
	__in_opt PVOID BaseAddress,
	__in MEMORY_INFORMATION_CLASS MemoryInformationClass,
	__out PVOID MemoryInformation,
	__in SIZE_T MemoryInformationLength,
	__out_opt PSIZE_T ReturnLength);

EXTERN_C NTSTATUS NtProtectVirtualMemory(
	IN HANDLE ProcessHandle,
	IN OUT PVOID* BaseAddress,
	IN OUT PSIZE_T RegionSize,
	IN ULONG NewProtect,
	OUT PULONG OldProtect
);

/// <summary>
/// Compares memory chunk to pattern. 
/// </summary>
/// <param name="pData"> - Data in memory.</param>
/// <param name="bMask"> - Pattern to look for.</param>
/// <param name="szMask"> - Mask of what bytes we know (notated with an "x") and what bytes we dont (notated with a "?").</param>
/// <returns></returns>
bool MemUtil::bCompare(const BYTE* pData, const byte* bMask, const char* szMask) {
	for (; *szMask; ++szMask, ++pData, ++bMask) {
		if (*szMask == 'x' && *pData != *bMask)
			return 0;
	}
		
	return (*szMask) == NULL;
}

bool MemUtil::PatchAdr(uintptr_t address, LPVOID changeToMake, size_t len, bool addBaseHandle) {
	uintptr_t addr = address + (addBaseHandle ? Offsets::baseHandle : 0);
	return PatchAdr((LPVOID)(addr), changeToMake, len);
}

bool MemUtil::PatchAdr(VersioningStruct<uintptr_t>& address, LPVOID changeToMake, size_t len, bool addBaseHandle) {
	uintptr_t addr = address.Get() + (addBaseHandle ? Offsets::baseHandle : 0);
	return PatchAdr((LPVOID)(addr), changeToMake, len);
}

/// <summary>
/// Write x86 ASM (HEX) to address.
/// </summary>
/// <param name="address"> - Pointer you want to edit</param>
/// <param name="changeToMake"> - Edit you want to make</param>
/// <param name="len"> - How long is the edit</param>
/// <returns></returns>
bool MemUtil::PatchAdr(LPVOID address, LPVOID changeToMake, size_t len) {
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
	if (len < 5)
	{
		return nullptr;
	}

	// Create the gateway (len + 5 for the overwritten bytes + the jmp)
	auto gateway = (PBYTE)VirtualAlloc(nullptr, len + 5, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);

	// Makes sure gateway doesn't equal null
	if (!gateway)
	{
		return nullptr;
	}
		
	// Put the bytes that will be overwritten in the gateway
	memcpy(gateway, src, len);

	// Get the gateway to destination addy
	auto gateJmpAddy = (uintptr_t)(src - gateway - 5);

	// Add the jmp opcode to the end of the gateway
	*(gateway + len) = (unsigned char)0xE9;

	// Add the address to the jmp
	*(uintptr_t*)(gateway + len + 1) = gateJmpAddy;

	// Place the hook at the destination
	if (PlaceHook(src, dst, len))
		return gateway;
	else
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
uintptr_t MemUtil::FindDMAAddy(uintptr_t ptr, const std::vector<unsigned int>& offsets, bool safe)
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

NTSTATUS MemUtil::HookedVirtualProtect(LPVOID address, SIZE_T len, ULONG newProtection, ULONG& oldProtection)
{
	return NtProtectVirtualMemory(GetCurrentProcess(), &address, &len, newProtection, &oldProtection);
}

NTSTATUS MemUtil::HookedQueryVirtualMemory(LPVOID address, PMEMORY_BASIC_INFORMATION memoryBuffer, SIZE_T dwLength)
{
	SIZE_T returnLength = 0;
	return NtQueryVirtualMemory(GetCurrentProcess(), address, MemoryBasicInformation, memoryBuffer, dwLength, &returnLength);
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
