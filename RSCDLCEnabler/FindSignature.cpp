#include "FindSignature.h"

bool bCompare(const BYTE* pData, const byte * bMask, const char* szMask) {
	for (; *szMask; ++szMask, ++pData, ++bMask)
		if (*szMask == 'x' && *pData != *bMask)
			return 0;
	return (*szMask) == NULL;
}

uint8_t* FindPattern(uint32_t dwAddress, size_t dwLen, uint8_t* bMask, char* szMask) {
	for (DWORD i = 0; i < dwLen; i++)
		if (bCompare((BYTE*)(dwAddress + i), bMask, szMask))
			return (byte*)(dwAddress + i);
	return NULL;
}