#pragma once
#include "windows.h"
#include <vector>

namespace MemUtil {
	bool bCompare(const BYTE* pData, const byte* bMask, const char* szMask);
	bool PatchAdr(LPVOID address, LPVOID changeToMake, size_t len);
	bool PlaceHook(void* hookSpot, void* ourFunct, int len);
	PBYTE TrampHook(PBYTE src, PBYTE dst, unsigned int len);
	bool IsBadReadPtr(void* pointer);
	uintptr_t FindDMAAddy(uintptr_t ptr, std::vector<unsigned int> offsets, bool safe = false);
	uintptr_t ReadPtr(uintptr_t adr);
	template <typename T>
	bool SetStaticValue(uintptr_t staticValue, T data, unsigned int lengthOfData);

	template <typename T>
	T FindPattern(uint32_t address, size_t size, PBYTE pattern, char* mask);
};

template <typename T>
/// <summary>
/// Scans memory chunk for a pattern.
/// </summary>
/// <typeparam name="T"> - Type of value to return</typeparam>
/// <param name="address"> - Address to start the search at.</param>
/// <param name="size"> - Size of search.</param>
/// <param name="pattern"> - Pattern to look for.</param>
/// <param name="mask"> - Mask of what bytes we know (notated with an "x") and what bytes we dont (notated with a "?").</param>
/// <returns>Value if found or NULL if not.</returns>
T MemUtil::FindPattern(uint32_t address, size_t size, PBYTE pattern, char* mask) {
	for (uint32_t i = 0; i < size; i++)
		if (bCompare((PBYTE)(address + i), pattern, mask))
			return (T)(address + i);

	return NULL;
}

template <typename T>
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

	// Save old Virtual Protect status, but allow us to Execute, Read, and Write to the executable's memory so we can make this change.
	if (!VirtualProtect((LPVOID)staticValue, lengthOfData, PAGE_EXECUTE_READWRITE, &dwOldProt)) {
		return false;
	}

	// Write what we had in data to the staticValue.
	*(T*)staticValue = data;

	// Resets the virtual protect to the status we saved earlier in this function. 
	VirtualProtect((LPVOID)staticValue, lengthOfData, dwOldProt, &dwDummy);

	return true;
}