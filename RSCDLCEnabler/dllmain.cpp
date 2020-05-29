#include "d3dx9_42.h"
#include "RandomMemStuff.h"
#include "Enumeration.h"
#include "detours.h"
#include "Functions.h"


#include "ImGUI/imgui.h"
#include "ImGUI/imgui_impl_dx9.h"
#include "ImGUI/imgui_impl_win32.h"
#include "ImGUI/RobotoFont.cpp""


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
	Sleep(30000); //wait for a minute, should give enough time for the initial stuff to get done

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

const char* windowName = "Rocksmith 2014";
WNDPROC oWndProc;

extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
typedef LRESULT(CALLBACK* WNDPROC)(HWND, UINT, WPARAM, LPARAM);


LRESULT __stdcall WndProc(const HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {

	if (true && ImGui_ImplWin32_WndProcHandler(hWnd, uMsg, wParam, lParam))
		return true;

	return CallWindowProc(oWndProc, hWnd, uMsg, wParam, lParam);
}


HRESULT __stdcall Hooked_EndScene(IDirect3DDevice9 * pDevice) 
{
	static bool init = false;
	if (!init) {
		/*char fontPath[MAX_PATH];
		GetWindowsDirectoryA(fontPath, sizeof(fontPath));
		strcat_s(fontPath, "\\Fonts\\calibri.ttf");*/

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

	ImGui::Begin("RS Modz");
	ImGui::SliderInt("Enumeration Interval", &EnumSliderVal, 100, 100000);
	ImGui::End();

	ImGui::EndFrame();
	ImGui::Render();

	ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());

	return oEndScene(pDevice); // Call original ensdcene so the game can draw
}

uintptr_t ptr_createD3D_Device = 0x135DD50;
void GUI() {
	Sleep(5000);
	HWND  window = FindWindowA(NULL, windowName);
	oWndProc = (WNDPROC)SetWindowLongPtr(window, GWL_WNDPROC, (LONG_PTR)WndProc);
	
	IDirect3D9 * pD3D = Direct3DCreate9(D3D_SDK_VERSION);
	if (!pD3D)
		return;
		
	D3DPRESENT_PARAMETERS d3dpp{ 0 };
	d3dpp.hDeviceWindow = window, d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD, d3dpp.Windowed = TRUE;

	IDirect3DDevice9 *Device = nullptr; //create a dummy D3D9 device
	if (FAILED(pD3D->CreateDevice(0, D3DDEVTYPE_HAL, d3dpp.hDeviceWindow, D3DCREATE_SOFTWARE_VERTEXPROCESSING, &d3dpp, &Device)))
	{
		pD3D->Release();
		return;
	}

	void ** pVTable = *reinterpret_cast<void***>(Device);

	if (Device)
		Device->Release(), Device = nullptr;

	oEndScene = (f_EndScene)DetourFunction((PBYTE)pVTable[42], (PBYTE)Hooked_EndScene);
}


DWORD WINAPI MainThread(void*) {	
	RandomMemStuff mem;

	Offsets.Initialize();
	PatchCDLCCheck();

	mem.LoadSettings();
	Settings.ReadKeyBinds();
	mem.HookSongListsKoko();

	GUI();

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
			if (GetAsyncKeyState(Settings.GetKeyBind("ForceReEnumerationKey")) & Settings.ReturnToggleValue("ForceReEnumerationEnabled") == "manual" & 0x8000) { // Force ReEnumeration (Manual)
				Enumeration.ForceEnumeration();
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
	CreateThread(NULL, 0, EnumerationThread, NULL, NULL, 0);
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
