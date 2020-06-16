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

RandomMemStuff mem;

void* d3d9Device[119];
bool menuEnabled=true; //whole menu is kinda bugged right now :(

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
WNDPROC oWndProc = NULL;

extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	if (menuEnabled && ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam)) //if we keep menuEnabled here, then we should not read hotkeys inside ImGUI's stuff - because they won't even be read when menu is disabled!
		return true;

	return CallWindowProc(oWndProc, hWnd, msg, wParam, lParam);
}

INT currStride;
bool cbEnabled;

HRESULT __stdcall Hook_EndScene(IDirect3DDevice9 *pDevice) {
	static bool init = false;

	if (!init) {
		init = true;

		ImGui::CreateContext();
		ImGuiIO& io = ImGui::GetIO();
		ImFont* font = io.Fonts->AddFontFromMemoryCompressedTTF(RobotoFont_data, RobotoFont_size, 20);

		D3DDEVICE_CREATION_PARAMETERS d3dcp;
		pDevice->GetCreationParameters(&d3dcp);
		hNewWnd = d3dcp.hFocusWindow;

		oWndProc = (WNDPROC)SetWindowLongPtr(hNewWnd, GWLP_WNDPROC, (LONG_PTR)WndProc);


		ImGui_ImplWin32_Init(hNewWnd);
		ImGui_ImplDX9_Init(pDevice);
		ImGui::GetIO().ImeWindowHandle = hNewWnd;


		GenerateTexture(pDevice, &Red, D3DCOLOR_ARGB(255, 000, 255, 255));
		GenerateTexture(pDevice, &Green, D3DCOLOR_RGBA(0, 255, 0, 255));
		GenerateTexture(pDevice, &Blue, D3DCOLOR_ARGB(255, 0, 0, 255));
		GenerateTexture(pDevice, &Yellow, D3DCOLOR_ARGB(255, 255, 255, 0));
		D3DXCreateTextureFromFile(pDevice, L"notes_gradient_normal.dds", &gradientTextureNormal); //if those don't exist, note heads will be "invisible"
		D3DXCreateTextureFromFile(pDevice, L"notes_gradient_seven.dds", &gradientTextureSeven);
		D3DXCreateTextureFromFile(pDevice, L"doesntexist.dds", &nonexistentTexture);

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
		ImGui::SliderInt("Curr Slide", &currStride, 0, 10000);
		//ImGui::Checkbox("Colorblind Mode", &cbEnabled);
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
bool setAllToNoteGradientTexture = false;

bool DiscoEnabled() {
	if (Settings.ReturnToggleValue("DiscoModeEnabled") == "true") {
		return true;
	}
	else {
		return false;
	}
}

HRESULT APIENTRY Hook_DP(LPDIRECT3DDEVICE9 pDevice, D3DPRIMITIVETYPE PrimType, UINT startIndex, UINT primCount) {
	if (pDevice->GetStreamSource(0, &Stream_Data, &Offset, &Stride) == D3D_OK)
		Stream_Data->Release();

	if (Stride == 12) { //Stride 12 = tails PogU
		mem.ToggleCB(mem.Is7StringSong);
		pDevice->SetTexture(1, gradientTextureNormal);
	}

	return oDrawPrimitive(pDevice, PrimType, startIndex, primCount);
}

HRESULT APIENTRY Hook_DIP(LPDIRECT3DDEVICE9 pDevice, D3DPRIMITIVETYPE PrimType, INT BaseVertexIndex, UINT MinVertexIndex, UINT NumVertices, UINT startIndex, UINT primCount)  {
	if (pDevice->GetStreamSource(0, &Stream_Data, &Offset, &Stride) == D3D_OK)
		Stream_Data->Release();

	Mesh current(Stride, primCount, NumVertices);

	if (GetAsyncKeyState(VK_UP) & 1)
		currStride++;
	if (GetAsyncKeyState(VK_DOWN) & 1)
		currStride--;

	if (GetAsyncKeyState(VK_DELETE) & 1)
		Log("Stride == %d && NumVertices == %d && PrimCount == %d && BaseVertexIndex == %d MinVertexIndex == %d && startIndex == %d && mStartregister == %d && PrimType == %d", Stride, NumVertices, primCount, BaseVertexIndex, MinVertexIndex, startIndex, mStartregister, PrimType);

	while (DiscoEnabled()) {
		pDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
		pDevice->SetRenderState(D3DRS_AMBIENT, D3DCOLOR_RGBA(rand() % 256, rand() % 256, rand() % 256, rand() % 256));
		pDevice->SetRenderState(D3DRS_SEPARATEALPHABLENDENABLE, TRUE);
		pDevice->SetRenderState(D3DRS_LOCALVIEWER, TRUE);
		pDevice->SetRenderState(D3DRS_COLORVERTEX, TRUE);
		pDevice->SetRenderState(D3DRS_FOGDENSITY, 15);
		pDevice->SetRenderState(D3DRS_FOGCOLOR, D3DCOLOR_RGBA(rand() % 256, rand() % 256, rand() % 256, rand() % 256));
		pDevice->SetRenderState(D3DRS_FOGVERTEXMODE, 2);
		
		return oDrawIndexedPrimitive(pDevice, PrimType, BaseVertexIndex, MinVertexIndex, NumVertices, startIndex, primCount);
	}

	if (setAllToNoteGradientTexture) {
		pDevice->SetTexture(currStride, gradientTextureSeven);
		return oDrawIndexedPrimitive(pDevice, PrimType, BaseVertexIndex, MinVertexIndex, NumVertices, startIndex, primCount);
	}


	if (NOTE_HEADS || OPEN_STRINGS || INDICATORS || NOTE_HEAD_SYMBOLS || HIGHLIGHTED_NOTE_HEAD || SOME_NOTE_HEAD_STUFF) { //change all pieces of note head's textures
		DWORD origZFunc;
		pDevice->GetRenderState(D3DRS_ZFUNC, &origZFunc);

		/*		if (mem.Is7StringSong)
					pDevice->SetTexture(1, gradientTextureSeven);
				else
					pDevice->SetTexture(1, gradientTextureNormal);
					*/

		mem.ToggleCB(mem.Is7StringSong);
		pDevice->SetTexture(1, gradientTextureNormal);



	}

	else if (IsToBeRemoved(skyline, current) & Settings.ReturnToggleValue("RemoveSkylineEnabled") == "true")
		return D3D_OK;
	/* Buggy ATM
	else if (IsToBeRemoved(skylineLesson, current) & Settings.ReturnToggleValue("RemoveSkylineEnabled") == "true")
		return D3D_OK;
	*/
	else if (IsToBeRemoved(greenscreenwall, current) & Settings.ReturnToggleValue("GreenScreenWallEnabled") == "true")
		return D3D_OK;
	else if (IsToBeRemoved(headstock, current) & Settings.ReturnToggleValue("RemoveHeadstockEnabled") == "true")
		return D3D_OK;
	
	return oDrawIndexedPrimitive(pDevice, PrimType, BaseVertexIndex, MinVertexIndex, NumVertices, startIndex, primCount);
}

HRESULT APIENTRY Hook_BeginScene(LPDIRECT3DDEVICE9 pD3D9) {
	return oBeginScene(pD3D9);
}

HINSTANCE hWnd;
LPDIRECT3D9 pD3D;
LPDIRECT3DDEVICE9 pd3dDevice;

DWORD* GetD3DDevice() {
	HWND tmpWnd = CreateWindowA("BUTTON", "DX", WS_SYSMENU | WS_MINIMIZEBOX, CW_USEDEFAULT, CW_USEDEFAULT, 300, 300, NULL, NULL, hWnd, NULL);
	if (tmpWnd == NULL)
		return NULL;

	pD3D = Direct3DCreate9(D3D_SDK_VERSION);
	if (pD3D == NULL) {
		DestroyWindow(tmpWnd);
		return 0;
	}

	D3DPRESENT_PARAMETERS d3dpp;
	ZeroMemory(&d3dpp, sizeof(d3dpp));
	d3dpp.Windowed = TRUE;
	d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
	d3dpp.hDeviceWindow = tmpWnd;
	d3dpp.BackBufferFormat = D3DFMT_UNKNOWN;

	HRESULT result = pD3D->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, tmpWnd, D3DCREATE_SOFTWARE_VERTEXPROCESSING, &d3dpp, &pd3dDevice);
	if (result != D3D_OK)
	{
		pD3D->Release();
		DestroyWindow(tmpWnd);
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
		oDrawPrimitive = (tDrawPrimitive)MemUtil.TrampHook((PBYTE)vTable[81], (PBYTE)Hook_DP, 7);
	}
	else
		std::cout << "Could not initialize D3D stuff" << std::endl;

	pd3dDevice->Release();
	pD3D->Release();
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
			/*
			if (GetAsyncKeyState('X') & 0x1)  // FUCK OFF
				mem.ShowCurrentTuning();
		*/
			mem.Toggle7StringMode();

		/* Disabled commands
			mem.ToggleLoftWhenSongStarts();
		*/

		//	mem.ToggleCB(cbEnabled); uncomment if you want the checkbox to work
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
