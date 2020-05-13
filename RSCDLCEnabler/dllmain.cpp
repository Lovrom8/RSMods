#include <windows.h>
#include <string>

#include "d3dx9_42.h"
#include "FindSignature.h"
#include "Patch.h"
#include <cstdlib>
#include "RandomMemStuff.h"

DWORD WINAPI MainThread(void*) {	
	RandomMemStuff mem;
	Patch patch;

	uint8_t* VerifySignatureOffset = FindPattern(0x01377000, 0x00DDE000, (uint8_t*)"\xE8\x00\x00\x00\x00\x83\xC4\x20\x88\xC3", "x????xxxxx");

	if (VerifySignatureOffset) {
		if (!patch.PatchAdr(VerifySignatureOffset + 8, "\xB3\x01", 2))
			printf("Failed to patch verify_signature!\n");
		else
			printf("Patch verify_signature success!\n");
	}

	bool loftEnabled = true;

	mem.LoadSettings();

	while (true) {

		Sleep(100);
		 
		/*if (GetAsyncKeyState(VK_INSERT) & 1) {
			mem.AddVolume(5);
		}

		if (GetAsyncKeyState(VK_END) & 1) {
			mem.DecreaseVolume(5);
		}*/

		if (GetAsyncKeyState('L') & 1) {
			mem.HookSongLists();
		}

		if (GetAsyncKeyState('K')) {
			mem.HookSongListsKoko();
		}

		if (GetAsyncKeyState('T') & 1) {
			mem.ToggleLoft();
		}

		//mem.ToggleLoftWhenSongStarts();

	}
	

	return 0;
}


void Initialize(void) {
	CreateThread(NULL, 0, MainThread, NULL, NULL, 0);
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD dwReason, LPVOID lpReserved) {
	switch (dwReason) {
	case DLL_PROCESS_ATTACH:
		InitProxy();
		Initialize();
		return TRUE;
	case DLL_PROCESS_DETACH:
		ShutdownProxy();
		return TRUE;
	}
	return TRUE;
}
