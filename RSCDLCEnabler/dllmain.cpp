#include "d3dx9_42.h"
#include "RandomMemStuff.h"
#include "Enumeration.h"
#include "detours.h"
#include "Functions.h"
#include "Utils.h"
#include "D3D.h"

#include "ImGUI/imgui.h"
#include "ImGUI/imgui_impl_dx9.h"
#include "ImGUI/imgui_impl_win32.h"
#include "ImGUI/RobotoFont.cpp""

const char* windowName = "Rocksmith 2014";
WNDPROC oWndProc;

RandomMemStuff mem;

void* d3d9Device[119];
bool menuEnabled=true; //whole menu is kinda bugged right now :(

UINT mStartregister;
UINT mVectorCount;

LPDIRECT3DTEXTURE9 Red, Green, Blue, Yellow;
LPDIRECT3DTEXTURE9 gradientTextureNormal, gradientTextureSeven;

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


void ReadHotkeys() {
	if (ImGui::IsKeyReleased(Settings.GetKeyBind("ToggleLoftKey"))) //because it runs at the end of each frame, if you use IsKeyPressed, you will likely end up with multiple keypressed registered at once before you release the key
		mem.ToggleLoft();

	if (ImGui::IsKeyReleased(Settings.GetKeyBind("MenuToggleKey")))
		menuEnabled = !menuEnabled;

}

HWND hNewWnd = NULL;

HRESULT __stdcall Hook_EndScene(IDirect3DDevice9 *pDevice) {
	static bool init = false;

	if (!init) {
		init = true;

		ImGui::CreateContext();
		ImGuiIO& io = ImGui::GetIO();
		ImFont* font = io.Fonts->AddFontFromMemoryCompressedTTF(RobotoFont_data, RobotoFont_size, 20);
		
		ImGui_ImplWin32_Init(hNewWnd);
		ImGui_ImplDX9_Init(pDevice);

		GenerateTexture(pDevice, &Red, D3DCOLOR_ARGB(255, 000, 255, 255));
		GenerateTexture(pDevice, &Green, D3DCOLOR_RGBA(0, 255, 0, 255));
		GenerateTexture(pDevice, &Blue, D3DCOLOR_ARGB(255, 0, 0, 255));
		GenerateTexture(pDevice, &Yellow, D3DCOLOR_ARGB(255, 255, 255, 0));
		D3DXCreateTextureFromFile(pDevice, L"notes_gradient_normal.dds", &gradientTextureNormal); //if those don't exist, note heads will be "invisible"
		D3DXCreateTextureFromFile(pDevice, L"notes_gradient_seven.dds", &gradientTextureSeven);

		std::cout << "ImGUI Init";
	}

	ImGui_ImplDX9_NewFrame();
	ImGui_ImplWin32_NewFrame();

	ImGui::NewFrame();

	static auto is_down = false;
	static auto is_clicked = false;
	if (GetAsyncKeyState(VK_INSERT))
	{
		is_clicked = false;
		is_down = true;
	}
	else if (!GetAsyncKeyState(VK_INSERT) && is_down)
	{
		is_clicked = true;
		is_down = false;
	}
	else {
		is_clicked = false;
		is_down = false;
	}

	if (is_clicked)
	{
		menuEnabled = !menuEnabled;
	}

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

LPDIRECT3DVERTEXBUFFER9 Stream_Data;
UINT Offset = 0, Stride = 0;

int stage = 0;
HRESULT APIENTRY Hook_DIP(LPDIRECT3DDEVICE9 pDevice, D3DPRIMITIVETYPE PrimType, INT BaseVertexIndex, UINT MinVertexIndex, UINT NumVertices, UINT startIndex, UINT primCount)  {
	if (pDevice->GetStreamSource(0, &Stream_Data, &Offset, &Stride) == D3D_OK)
		Stream_Data->Release();

	if( NOTE_HEADS || OPEN_STRINGS || INDICATORS || NOTE_HEAD_SYMBOLS || HIGHLIGHTED_NOTE_HEAD) { //change all pieces of textures
		DWORD origZFunc;
		pDevice->GetRenderState(D3DRS_ZFUNC, &origZFunc);

		//if (GetAsyncKeyState(VK_END) & 1)
		//	stage++;

		if (mem.Is7StringSong) 
			pDevice->SetTexture(1, gradientTextureSeven);
		else 
			pDevice->SetTexture(1, gradientTextureNormal);

		Log("Stride == %d && NumVertices == %d && PrimCount == %d && BaseVertexIndex == %d MinVertexIndex == %d && startIndex == %d && mStartregister == %d && PrimType == %d", Stride, NumVertices, primCount, BaseVertexIndex, MinVertexIndex, startIndex, mStartregister, PrimType);
	}
	else if ( SKYLINE1 || SKYLINE2 || SKYLINE3 || SKYLINE4) //disable skyline, SKYLINE4 is for when it's paused
		return D3D_OK;


	return oDrawIndexedPrimitive(pDevice, PrimType, BaseVertexIndex, MinVertexIndex, NumVertices, startIndex, primCount);
}

HRESULT APIENTRY Hook_BeginScene(LPDIRECT3DDEVICE9 pD3D9) {
	return oBeginScene(pD3D9);
}

LRESULT CALLBACK MsgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {

	if(ImGui_ImplWin32_WndProcHandler(hWnd, uMsg, wParam, lParam))
		return true;

	return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

DWORD* GetD3DDevice() {
	WNDCLASSEX wc = { sizeof(WNDCLASSEX), CS_CLASSDC, MsgProc, 0L, 0L, GetModuleHandle(NULL), NULL, NULL, NULL, NULL, L"RS_DX",NULL };
	RegisterClassEx(&wc);
	
	hNewWnd = CreateWindow(L"RS_DX", NULL, WS_OVERLAPPEDWINDOW, 100, 100, 300, 300, GetDesktopWindow(), NULL, wc.hInstance, NULL);
	LPDIRECT3D9 pD3D = Direct3DCreate9(D3D_SDK_VERSION);
	if (!pD3D) {
		UnregisterClass(L"RS_DX", wc.hInstance);
		return NULL;
	}
		

	D3DPRESENT_PARAMETERS d3dpp;
	ZeroMemory(&d3dpp, sizeof(d3dpp));
	d3dpp.Windowed = TRUE;
	d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
	d3dpp.BackBufferFormat = D3DFMT_UNKNOWN;

	LPDIRECT3DDEVICE9 pd3dDevice;
	pD3D->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hNewWnd, D3DCREATE_SOFTWARE_VERTEXPROCESSING, &d3dpp, &pd3dDevice);
	if (pd3dDevice == NULL) {
		pD3D->Release();
		UnregisterClass(L"RS_DX", wc.hInstance);
		return NULL;
	}

	DWORD* pVTable = (DWORD*)pd3dDevice;
	pVTable = (DWORD*)pVTable[0];
	DestroyWindow(hNewWnd);

	return pVTable;
}

void GUI() {
	Sleep(5000);

	DWORD* vTable = GetD3DDevice();

	if (vTable != NULL) {
		oBeginScene = (tBeginScene)MemUtil.TrampHook((PBYTE)vTable[41], (PBYTE)Hook_BeginScene, 7);
		oEndScene = (tEndScene)MemUtil.TrampHook((PBYTE)vTable[42], (PBYTE)Hook_EndScene, 7);
		oReset = (tReset)MemUtil.TrampHook((PBYTE)vTable[16], (PBYTE)Hook_Reset, 7);
		oDrawIndexedPrimitive = (tDrawIndexedPrimitive)MemUtil.TrampHook((PBYTE)vTable[82], (PBYTE)Hook_DIP, 5);
	}
	else
		std::cout << "Could not initialize D3D stuff" << std::endl;
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
		
		/*	if (Settings.ReturnToggleValue("RainbowStringsEnabled") == "true") // Rainbow Strings
				if (GetAsyncKeyState(Settings.GetKeyBind("RainbowStringsKey")) & 0x1)
					mem.DoRainbow();
					//mem.LoadModsWhenSongsLoad("RainbowStrings");
					*/

		// Dev Commands
			if (GetAsyncKeyState('X') & 0x1) 
				mem.ShowCurrentTuning();
		
			mem.Toggle7StringMode();

		/* Disabled commands
			mem.ToggleLoftWhenSongStarts();
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
		AllocConsole();
		freopen("CONIN$", "r", stdin);
		freopen("CONOUT$", "w", stdout);
		DisableThreadLibraryCalls(hModule);

		InitProxy();
		Initialize();
		return TRUE;
	case DLL_PROCESS_DETACH:
		ShutdownProxy();
		ImGui_ImplWin32_Shutdown();
		ImGui_ImplDX9_Shutdown();
		ImGui::DestroyContext();
		return TRUE;
	}
	return TRUE;
}
