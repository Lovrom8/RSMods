#pragma once
#include "windows.h"
#include <vector>

namespace MemUtil {
	bool bCompare(const BYTE* pData, const byte* bMask, const char* szMask);
	bool PatchAdr(LPVOID dst, LPVOID src, size_t len);
	bool PlaceHook(void* hookSpot, void* ourFunct, int len);
	PBYTE TrampHook(PBYTE src, PBYTE dst, unsigned int len);
	bool IsBadReadPtr(void* p);
	uintptr_t FindDMAAddy(uintptr_t ptr, std::vector<unsigned int> offsets, bool safe = false);
	uint8_t* FindPatternPtr(uint32_t dwAddress, size_t dwLen, uint8_t* pattern, char* mask);
	uintptr_t ReadPtr(uintptr_t adr);

	template <typename T>
	T FindPattern(uint32_t address, size_t size, PBYTE pattern, char* mask);
};

template <typename T>
T MemUtil::FindPattern(uint32_t address, size_t size, PBYTE pattern, char* mask) {
	for (uint32_t i = 0; i < size; i++)
		if (bCompare((PBYTE)(address + i), pattern, mask))
			return (T)(address + i);

	return NULL;
}


