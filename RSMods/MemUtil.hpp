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