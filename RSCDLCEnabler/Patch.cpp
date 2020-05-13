#include "Patch.h"

bool Patch::PatchAdr(LPVOID dst, LPVOID src, size_t len) {
	DWORD dwOldProt, dwDummy;

	if (!VirtualProtect(dst, len, PAGE_EXECUTE_READWRITE, &dwOldProt)) {
		return false;
	}

	memcpy(dst, src, len);

	FlushInstructionCache(GetCurrentProcess(), dst, len);
	VirtualProtect(dst, len, dwOldProt, &dwDummy);

	return true;
}

bool Patch::PlaceHook(void * hookSpot, void * ourFunct, int len)
{
	if (len < 5)
		return false;

	DWORD oldProtect;
	if (!VirtualProtect(hookSpot, len, PAGE_EXECUTE_READWRITE, &oldProtect))
		return false;

	memset(hookSpot, 0x90, len);

	DWORD relativeAddr = ((DWORD)ourFunct - (DWORD)hookSpot) - 5;

	*(BYTE*)hookSpot = 0xE9;
	*(DWORD*)((DWORD)hookSpot + 1) = relativeAddr;

	DWORD backup;
	if (!VirtualProtect(hookSpot, len, oldProtect, &backup))
		return false;

	return true;
}