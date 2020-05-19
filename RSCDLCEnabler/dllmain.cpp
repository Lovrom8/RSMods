#include "d3dx9_42.h"
#include "RandomMemStuff.h"

void PatchCDLCCheck() {
	uint8_t* VerifySignatureOffset = Offsets.cdlcCheckAdr;

	if (VerifySignatureOffset) {
		if (!MemUtil.PatchAdr(VerifySignatureOffset + 8, (UINT*)Offsets.patch_CDLCCheck, 2))
			printf("Failed to patch verify_signature!\n");
		else
			printf("Patch verify_signature success!\n");
	}
}

DWORD WINAPI MainThread(void*) {	
	RandomMemStuff mem;

	Offsets.Initialize();
	PatchCDLCCheck();

	mem.LoadSettings();
	Settings.ReadKeyBinds();

	while (true) {

		Sleep(100);
		 
		/*if (GetAsyncKeyState(VK_INSERT) & 1) {
			mem.AddVolume(5);
		}

		if (GetAsyncKeyState(VK_END) & 1) {
			mem.DecreaseVolume(5);
		}*/

		//mem.ToggleLoftWhenSongStarts();
		
		if (GetAsyncKeyState('J') & 0x1) {
			mem.DoRainbow();
		}

		if (GetAsyncKeyState('X') & 0x1) {
			mem.ShowCurrentTuning();
		}

		if (GetAsyncKeyState(Settings.GetKeyBind("CustomSongListTitles")) & 0x1) {
			mem.HookSongListsKoko();
		}

		if (GetAsyncKeyState(Settings.GetKeyBind("ToggleLoft")) & 0x1) {
			mem.ToggleLoft();
		}

		if (GetAsyncKeyState(Settings.GetKeyBind("ForceEnumeration")) & 0x8000) {
			mem.EnumerateBrah();
		}
	
		//mem.ToggleLoftWhenSongStarts();
		//mem.Toggle7StringMode();
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
