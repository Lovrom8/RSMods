#include "RandomMemStuff.h"


RandomMemStuff::RandomMemStuff()
{

}

uintptr_t RandomMemStuff::FindDMAAddy(uintptr_t ptr, std::vector<unsigned int> offsets)
{
	uintptr_t addr = ptr;
	for (unsigned int i = 0; i < offsets.size(); ++i)
	{
		addr = *(uintptr_t*)addr;

		if (addr == NULL)
			return NULL;

		addr += offsets[i];
	}
	return addr;
}

void RandomMemStuff::AddVolume(float add) {
	uintptr_t addr = FindDMAAddy( (uintptr_t)GetModuleHandle(NULL) + 0x00F4E91C, { 0x28, 0x7C0, 0x214, 0x7F4, 0xDC });

	float val = *(float*)addr;

	if (val+add >= 100.0f)
		return;

	 *(float*)addr = val+add;
}

void RandomMemStuff::DecreaseVolume(float remove) {
	uintptr_t addr = FindDMAAddy((uintptr_t)GetModuleHandle(NULL) + 0x00F4E91C, { 0x28, 0x7C0, 0x214, 0x7F4, 0xDC });

	float val = *(float*)addr;

	if (val + remove <= 0.0f)
		return;

	*(float*)addr = val + remove;
}

void RandomMemStuff::ToggleLoft() {
	uintptr_t addr = FindDMAAddy((uintptr_t)GetModuleHandle(NULL) + 0x00F5C4EC, { 0x108, 0x14, 0x28, 0x7C  });

	if (*(float*)addr == 10)
		*(float*)addr = 10000;
	else
		*(float*)addr = 10;
}

void RandomMemStuff::ToggleLoftWhenSongStarts() {
	uintptr_t addrTimer = FindDMAAddy((uintptr_t)GetModuleHandle(NULL) + 0x00F5C5AC, { 0xB0, 0x538, 0x8 });
	uintptr_t addrLoft = FindDMAAddy((uintptr_t)GetModuleHandle(NULL) + 0x00F5C4EC, { 0x108, 0x14, 0x28, 0x7C });

	if (*(float*)addrLoft != 10)
		return;

	return;
	if (*(float*)addrTimer >= 0.1 )
		*(float*)addrLoft = 10000;
}

void RandomMemStuff::ShowSongTimer() {
	uintptr_t addrTimer = FindDMAAddy((uintptr_t)GetModuleHandle(NULL) + 0x01567AB0, { 0x80, 0x20, 0x10C, 0x244 });
	
	if (!addrTimer)
		return;

	std::string valStr = std::to_string(*(float*)addrTimer);
	MessageBoxA(NULL, valStr.c_str(), "", 0);

	
	/*
	float val = *(float*)addrTimer;

	std::string valStr = std::to_string(val);
	MessageBoxA(NULL, valStr.c_str(), "", 0);
		
	uintptr_t addrState = FindDMAAddy((uintptr_t)GetModuleHandle(NULL) + 0x00F5C494, { 0xBC, 0x0 });
	//uintptr_t addrState = FindDMAAddy((uintptr_t)GetModuleHandle(NULL) + 0x00F5C5AC, { 0x18, 0x18, 0xC, 0x27C });
	std::string game_stage = *(std::string*)addrState;
	MessageBoxA(NULL, game_stage.c_str(), "", 0);
	/**/
}

void RandomMemStuff::PatchSongLists() {
	Patch((BYTE*)0x01529f98, (UINT*)"\x58\x58\x90\x90\x90", 5);
	Patch((BYTE*)0x0152a006, (UINT*)"\x5A\x5A\x90\x90\x90", 5);
}

bool placeHook(void * hookSpot, void * ourFunct, int len)
{
	if (len < 5)
	{
		return false;
	}

	DWORD oldProtect;
	if (!VirtualProtect(hookSpot, len, PAGE_EXECUTE_READWRITE, &oldProtect))
	{
		return false;
	}

	memset(hookSpot, 0x90, len);

	DWORD relativeAddr = ((DWORD)ourFunct - (DWORD)hookSpot) - 5;

	*(BYTE*)hookSpot = 0xE9;
	*(DWORD*)((DWORD)hookSpot + 1) = relativeAddr;

	DWORD backup;
	if (!VirtualProtect(hookSpot, len, oldProtect, &backup))
	{
		return false;
	}

	return true;
}

DWORD hookBackAddr;
void __declspec(naked) hookFunc() {
	//ESI = INDEX
	//EAX = char* str "$[36969]SONG LIST"
	__asm {
		mov ecx, dword ptr ds : [0x135CB7C]
		pushad
		mov ebx, 0x33
		add ebx, esi
		mov byte ptr[eax + 0x6], bl  
		mov byte ptr[eax + 0x2], 0x30 //third char (that is, the first digit); 0x30 = 0, 0x31 = 1, ...
		//mov byte ptr[eax + 0x3], 0x34 //adapt to your needs
		popad
		jmp[hookBackAddr]
	}
}

void RandomMemStuff::HookSongLists() {
	PatchSongLists();

	int len = 6;
	DWORD hookAddr = 0x01529f2b;
	hookBackAddr = hookAddr + len;

	placeHook((void*)hookAddr, hookFunc, len);

	//std::string game_stage = "hooked";
	//MessageBoxA(NULL, game_stage.c_str(), "", 0);
}

RandomMemStuff::~RandomMemStuff()
{
}
