#include "d3dx9_42.h"
#include "RandomMemStuff.h"
#include "Enumeration.h"
#include "detours.h"
#include "Functions.h"

#include "ImGUI/imgui.h"
#include "ImGUI/imgui_impl_dx9.h"
#include "ImGUI/imgui_impl_win32.h"
#include "ImGUI/RobotoFont.cpp""

const char* windowName = "Rocksmith 2014";
WNDPROC oWndProc;

RandomMemStuff mem;

IDirect3DDevice9* pD3DDevice = nullptr;
void* d3d9Device[119];
bool menuEnabled=false;

void PatchCDLCCheck() {
	uint8_t* VerifySignatureOffset = Offsets.cdlcCheckAdr;

	if (VerifySignatureOffset) {
		if (!MemUtil.PatchAdr(VerifySignatureOffset + 8, (UINT*)Offsets.patch_CDLCCheck, 2))
			printf("Failed to patch verify_signature!\n");
		else
			printf("Patch verify_signature success!\n");
	}
}

int EnumSliderVal = 10000;

DWORD WINAPI EnumerationThread(void*) { //pls don't let me regret doing this
	Sleep(30000); //wait for half a minute, should give enough time for the initial stuff to get done

	Settings.ReadKeyBinds(); //YIC
	Settings.ReadModSettings();

	int oldDLCCount = Enumeration.GetCurrentDLCCount(), newDLCCount = oldDLCCount;

	while (true) {
		if (Settings.ReturnToggleValue("ForceReEnumerationEnabled") == "automatic") {
			oldDLCCount = newDLCCount;
			newDLCCount = Enumeration.GetCurrentDLCCount();

			if (oldDLCCount != newDLCCount)
				Enumeration.ForceEnumeration();
		}

		//Sleep(Settings.GetModSetting("CheckForNewSongsInterval"));
		Sleep(EnumSliderVal);
	}

	return 0;
}


LRESULT __stdcall WndProc(const HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {

	if (true && ImGui_ImplWin32_WndProcHandler(hWnd, uMsg, wParam, lParam))
		return true;

	return CallWindowProc(oWndProc, hWnd, uMsg, wParam, lParam);
}


void ReadHotkeys() {
	if (ImGui::IsKeyReleased(Settings.GetKeyBind("ToggleLoftKey"))) //because it runs at the end of each frame, if you use IsKeyPressed, you will likely end up with multiple keypressed registered at once before you release the key
		mem.ToggleLoft();

	if (ImGui::IsKeyReleased(Settings.GetKeyBind("MenuToggleKey")))
		menuEnabled = !menuEnabled;

}

HRESULT __stdcall Hook_EndScene(IDirect3DDevice9 *pDevice) {
	static bool init = false;

	if (!init) {
		init = true;

		ImGui::CreateContext();
		ImGuiIO& io = ImGui::GetIO();
		ImFont* font = io.Fonts->AddFontFromMemoryCompressedTTF(RobotoFont_data, RobotoFont_size, 20);

		ImGui_ImplWin32_Init(FindWindowA(NULL, windowName));
		ImGui_ImplDX9_Init(pDevice);
	}

	ImGui_ImplDX9_NewFrame();
	ImGui_ImplWin32_NewFrame();

	ImGui::NewFrame();

	if (menuEnabled) {
		ImGui::Begin("RS Modz");
		ImGui::SliderInt("Enumeration Interval", &EnumSliderVal, 100, 100000);
		ImGui::End();
	}

	/* New place for keybinds */
	ReadHotkeys();

	ImGui::EndFrame();
	ImGui::Render();

	ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());

	return oEndScene(pDevice); // Call original ensdcene so the game can draw
}

HRESULT APIENTRY Hook_Reset(LPDIRECT3DDEVICE9 pD3D9, D3DPRESENT_PARAMETERS* pPresentationParameters) { //gotta do this so that ALT TAB-ing out of the game doesn't mess the whole thing up
	ImGui_ImplDX9_InvalidateDeviceObjects();

	HRESULT ResetReturn = oReset(pD3D9, pPresentationParameters);

	ImGui_ImplDX9_CreateDeviceObjects(); 

	return ResetReturn;
}

bool GetD3D9Device(void** pTable, size_t Size) {
	HWND window = FindWindowA(NULL, windowName);
	oWndProc = (WNDPROC)SetWindowLongPtr(window, GWL_WNDPROC, (LONG_PTR)WndProc);

	IDirect3D9* pD3D = Direct3DCreate9(D3D_SDK_VERSION);
	if (!pD3D)
		return false;

	IDirect3DDevice9* Device = nullptr; //create a dummy D3D9 device
	D3DPRESENT_PARAMETERS d3dpp{ 0 };
	d3dpp.hDeviceWindow = window, d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD, d3dpp.Windowed = false;

	HRESULT dummyDeviceCreated = pD3D->CreateDevice(0, D3DDEVTYPE_HAL, d3dpp.hDeviceWindow, D3DCREATE_SOFTWARE_VERTEXPROCESSING, &d3dpp, &Device);
	if (dummyDeviceCreated != S_OK)
	{
		d3dpp.Windowed = !d3dpp.Windowed;

		dummyDeviceCreated = pD3D->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, d3dpp.hDeviceWindow, D3DCREATE_SOFTWARE_VERTEXPROCESSING, &d3dpp, &Device);

		if (dummyDeviceCreated != S_OK)
		{
			pD3D->Release();
			return false;
		}
	}

	memcpy(pTable, *reinterpret_cast<void***>(Device), Size);

	if (Device)
		Device->Release(), Device = nullptr;
	pD3D->Release();

	return true;
}

void GUI() {
	Sleep(5000);
	if (GetD3D9Device(d3d9Device, sizeof(d3d9Device))) {
		oEndScene = (tEndScene)MemUtil.TrampHook((PBYTE)d3d9Device[42], (PBYTE)Hook_EndScene, 7);
		oReset = (f_Reset)MemUtil.TrampHook((PBYTE)d3d9Device[16], (PBYTE)Hook_Reset, 7);
	}
}

DWORD WINAPI MainThread(void*) {	
	Offsets.Initialize();
	PatchCDLCCheck();

	mem.LoadSettings();
	Settings.ReadKeyBinds();
	mem.HookSongListsKoko();

	GUI();

	while (true) {

		Sleep(100);
		
		// Mods
			/*if (Settings.ReturnToggleValue("ToggleLoftEnabled") == "true") 
				if (GetAsyncKeyState(Settings.GetKeyBind("ToggleLoftKey")) & 0x1) // Toggle Loft
					mem.ToggleLoft();*/
		
			/* Buggy?
				if (Settings.ReturnToggleValue("AddVolumeEnabled") == "true") // Add Volume / Volume Up
					if (GetAsyncKeyState(Settings.GetKeyBind("AddVolumeKey")) & 0x1) 
						mem.AddVolume(5);
			
				if (Settings.ReturnToggleValue("DecreaseVolumeEnabled") == "true")  // Decrease Volume / Volume Down
					if (GetAsyncKeyState(Settings.GetKeyBind("DecreaseVolumeKey")) & 0x1)
						mem.DecreaseVolume(5);
			*/
			if (Settings.ReturnToggleValue("ShowSongTimerEnabled") == "true")  // Show Song Timer
				if (GetAsyncKeyState(Settings.GetKeyBind("ShowSongTimerKey")) & 0x1)
					mem.ShowSongTimer();
				
			if (Settings.ReturnToggleValue("ForceReEnumerationEnabled") == "manual") // Force ReEnumeration (Manual)
				if (GetAsyncKeyState(Settings.GetKeyBind("ForceReEnumerationKey")) & 0x1) 
					Enumeration.ForceEnumeration();
		
			if (Settings.ReturnToggleValue("RainbowStringsEnabled") == "true") // Rainbow Strings
				if (GetAsyncKeyState(Settings.GetKeyBind("RainbowStringsKey")) & 0x1)
					mem.DoRainbow();
					//mem.LoadModsWhenSongsLoad("RainbowStrings");

		// Dev Commands
			if (GetAsyncKeyState('X') & 0x1) 
				mem.ShowCurrentTuning();
		
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
	//CreateThread(NULL, 0, EnumerationThread, NULL, NULL, 0);
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
