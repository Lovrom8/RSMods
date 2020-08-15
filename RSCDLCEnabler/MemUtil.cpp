#include "MemUtil.hpp"

bool MemUtil::bCompare(const BYTE* pData, const byte* bMask, const char* szMask) {
	for (; *szMask; ++szMask, ++pData, ++bMask)
		if (*szMask == 'x' && *pData != *bMask)
			return 0;
	return (*szMask) == NULL;
}

bool MemUtil::PatchAdr(LPVOID dst, LPVOID src, size_t len) {
	DWORD dwOldProt, dwDummy;

	if (!VirtualProtect(dst, len, PAGE_EXECUTE_READWRITE, &dwOldProt)) {
		return false;
	}

	memcpy(dst, src, len);

	FlushInstructionCache(GetCurrentProcess(), dst, len);
	VirtualProtect(dst, len, dwOldProt, &dwDummy);

	return true;
}

bool MemUtil::PlaceHook(void* hookSpot, void* ourFunct, int len)
{
	if (len < 5)
		return false;

	DWORD oldProtect;
	if (!VirtualProtect(hookSpot, len, PAGE_EXECUTE_READWRITE, &oldProtect))
		return false;

	memset(hookSpot, 0x90, len);

	uint32_t relativeAddr = ((uint32_t)ourFunct - (uint32_t)hookSpot) - 5;

	*(BYTE*)hookSpot = 0xE9;
	*(uint32_t*)((uint32_t)hookSpot + 1) = relativeAddr;

	DWORD backup;
	if (!VirtualProtect(hookSpot, len, oldProtect, &backup))
		return false;

	return true;
}

PBYTE MemUtil::TrampHook(PBYTE src, PBYTE dst, unsigned int len)
{
	if (len < 5) return 0;

	// Create the gateway (len + 5 for the overwritten bytes + the jmp)
	PBYTE gateway = (PBYTE)VirtualAlloc(0, len + 5, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);

	// Put the bytes that will be overwritten in the gateway
	memcpy(gateway, src, len);

	// Get the gateway to destination addy
	uintptr_t gateJmpAddy = (uintptr_t)(src - gateway - 5);

	// Add the jmp opcode to the end of the gateway
	*(gateway + len) = (char)0xE9;

	// Add the address to the jmp
	*(uintptr_t*)(gateway + len + 1) = gateJmpAddy;

	// Place the hook at the destination
	if (PlaceHook(src, dst, len))
		return gateway;
	else
		return nullptr;
}

bool MemUtil::IsBadReadPtr(void* p) //NOTE: We are very aware this is not exactly the optimal (neither completely thread safe nor very fast) way to handle pointers to a non-initialized variable, but for now it will have to do the job until we figure out a better current menu check 
{
	MEMORY_BASIC_INFORMATION mbi = { 0 };
	if (::VirtualQuery(p, &mbi, sizeof(mbi)))
	{
		uint32_t mask = (PAGE_READONLY | PAGE_READWRITE | PAGE_WRITECOPY | PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE | PAGE_EXECUTE_WRITECOPY);
		bool b = !(mbi.Protect & mask);
		// check the page is not a guard page
		if (mbi.Protect & (PAGE_GUARD | PAGE_NOACCESS)) b = true;

		return b;
	}
	return true;
}


uintptr_t MemUtil::FindDMAAddy(uintptr_t ptr, std::vector<unsigned int> offsets, bool safe)
{
	uintptr_t addr = ptr;

	for (unsigned int i = 0; i < offsets.size(); ++i)
	{
		if (safe && IsBadReadPtr((void*)addr))
			return NULL;

		if (!addr)
			return NULL;

		bool safeExport = safe;
		std::vector<unsigned int> offsetsExport = offsets;
		addr = *(uintptr_t*)addr;

		if (addr == NULL)
			return NULL;

		addr += offsets[i];
	}
	return addr;
}

uintptr_t MemUtil::ReadPtr(uintptr_t adr) {
	if (adr == NULL)
		return NULL;

	return *(uintptr_t*)adr;
}
