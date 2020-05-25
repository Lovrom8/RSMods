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
	mem.HookSongListsKoko();

	while (true) {

		Sleep(100);
		
		// Mods
			if (GetAsyncKeyState(Settings.GetKeyBind("ToggleLoftKey")) & Settings.ReturnToggleValue("ToggleLoftEnabled") == "true" & 0x1) { // Toggle Loft
				mem.ToggleLoft();
			}
			// Buggy
			if (GetAsyncKeyState(Settings.GetKeyBind("AddVolumeKey")) & Settings.ReturnToggleValue("AddVolumeEnabled") == "true" & 0x1) { // Add Volume / Volume Up
				mem.AddVolume(5);
			}
			if (GetAsyncKeyState(Settings.GetKeyBind("DecreaseVolumeKey")) & Settings.ReturnToggleValue("DecreaseVolumeEnabled") == "true" & 0x1) { // Decrease Volume / Volume Down
				mem.DecreaseVolume(5);
			}
			if (GetAsyncKeyState(Settings.GetKeyBind("ShowSongTimerKey")) & Settings.ReturnToggleValue("ShowSongTimerEnabled") == "true" & 0x1) { // Show Song Timer
				mem.ShowSongTimer();
			}
			if (GetAsyncKeyState(Settings.GetKeyBind("ForceReEnumerationKey")) & Settings.ReturnToggleValue("ForceReEnumerationEnabled") == "true" & 0x8000) { // Force ReEnumeration (Manual)
				mem.EnumerateBrah();
			}
			if (GetAsyncKeyState(Settings.GetKeyBind("RainbowStringsKey")) & Settings.ReturnToggleValue("RainbowStringsEnabled") == "true" & 0x1) { // Rainbow Strings
				mem.DoRainbow();
			}
		// Dev Commands
			if (GetAsyncKeyState('X') & 0x1) {
				mem.ShowCurrentTuning();
			}
		/* Disabled commands
			mem.ToggleLoftWhenSongStarts();
			mem.Toggle7StringMode();
			mem.HookSongListsKoko();
		*/
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
