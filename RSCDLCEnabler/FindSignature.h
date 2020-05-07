#ifndef FindSignature_h__
#define FindSignature_h__

#include <windows.h>
#include <inttypes.h>

uint8_t* FindPattern(uint32_t dwAddress, size_t dwLen, uint8_t* bMask, char* szMask);

#endif // FindSignature_h__