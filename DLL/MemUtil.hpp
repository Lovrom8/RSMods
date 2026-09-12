#pragma once

#include "Offsets.hpp"
#include "winternl.h"
#include <span>
#include <string_view>

#ifndef STATUS_SUCCESS
#define STATUS_SUCCESS ((NTSTATUS)0x00000000L)
#endif

#ifndef STATUS_UNSUCCESSFUL
#define STATUS_UNSUCCESSFUL ((NTSTATUS)0xC0000001L)
#endif

namespace MemUtil {
	bool bCompare(const BYTE* pData, const byte* bMask, const char* szMask);
	bool PatchAdr(VersioningStruct<uintptr_t>& address, LPCVOID changeToMake, size_t len, bool addBaseHandle = false);
	bool PatchAdr(LPVOID address, LPCVOID changeToMake, size_t len);
	bool PatchAdr(uintptr_t address, LPCVOID changeToMake, size_t len, bool addBaseHandle);
	bool PatchAdr(uintptr_t address, std::string_view data, bool addBaseHandle);
	bool PlaceHook(VersioningStruct<uintptr_t>& hookSpot, void* ourFunct, int len, bool addBaseHandle = false);
	bool PlaceHook(void* hookSpot, void* ourFunct, int len);
	PBYTE TrampHook(PBYTE src, PBYTE dst, unsigned int len);
	bool IsBadReadPtr(void* pointer);
	uintptr_t FindDMAAddy(uintptr_t ptr, std::span<const unsigned int> offsets, bool safe = false);
	uintptr_t ReadPtr(uintptr_t adr);
	template <typename T>
		requires std::is_trivially_copyable_v<T>
	bool SetStaticValue(uintptr_t staticValue, T data, unsigned int lengthOfData);

	template <typename T>
	T FindPattern(uint32_t address, size_t size, PBYTE pattern, const char* mask);

	template <typename T>
		requires std::is_trivially_copyable_v<T>
	T ReadValue(uintptr_t adr, bool addBaseHandle);

	bool IsRunningOnWine();
	NTSTATUS HookedVirtualProtect(LPVOID address, SIZE_T len, ULONG newProtection, ULONG& oldProtection);
	NTSTATUS HookedQueryVirtualMemory(LPVOID address, PMEMORY_BASIC_INFORMATION memoryBuffer, SIZE_T dwLength);

	uint32_t GetTextSectionAddress();
	uint32_t GetTextSectionLength();
	void CheckMemoryProtection(void* address);
};

template <typename T>
/// <summary>
/// Scans memory chunk for a pattern.
/// </summary>
/// <typeparam name="T"> - Type of value to return</typeparam>
/// <param name="address"> - Address to start the search at.</param>
/// <param name="size"> - Size of search.</param>
/// <param name="pattern"> - Pattern to look for.</param>
/// <param name="mask"> - Mask of what bytes we know (notated with an "x") and what bytes we don't (notated with a "?").</param>
/// <returns>Value if found or NULL if not.</returns>
T MemUtil::FindPattern(uint32_t address, size_t size, PBYTE pattern, const char* mask) {
	for (uint32_t i = 0; i < size; i++) {
		if (bCompare(reinterpret_cast<PBYTE>(address + i), pattern, mask)) {
			return T(address + i);
		}
	}

	return T{};
}

template <typename T> requires std::is_trivially_copyable_v<T>
/// <summary>
/// Sets a static value in the executable, utilizing VirtualProtect.
/// </summary>
/// <typeparam name="T"> - Type of the data.</typeparam>
/// <param name="staticValue"> - Address of the static value.</param>
/// <param name="data"> - Data we should change the staticValue to.</param>
/// <param name="lengthOfData"> - Length of the data (needed for VirtualProtect).</param>
/// <returns>Successfully able to set the value.</returns>
bool MemUtil::SetStaticValue(uintptr_t staticValue, T data, unsigned int lengthOfData) {
	DWORD dwOldProt, dwDummy;

	// Change memory protection to allow writing
	NTSTATUS status = HookedVirtualProtect(reinterpret_cast<LPVOID>(staticValue), lengthOfData, PAGE_EXECUTE_READWRITE, dwOldProt);
	if (!NT_SUCCESS(status)) {
		return false;
	}

	// Write the data
	*reinterpret_cast<T*>(staticValue) = data;

	// Restore original protection
	status = HookedVirtualProtect(reinterpret_cast<LPVOID>(staticValue), lengthOfData, dwOldProt, dwDummy);
	if (!NT_SUCCESS(status)) {
		return false;
	}

	return true;
}

template <typename T> requires std::is_trivially_copyable_v<T>
T MemUtil::ReadValue(uintptr_t address, bool addBaseHandle) {
	if (address == 0) {
		return T{};
	}

	const uintptr_t addr = address + (addBaseHandle ? Offsets::baseHandle : 0);
	return *reinterpret_cast<T*>(addr);
}