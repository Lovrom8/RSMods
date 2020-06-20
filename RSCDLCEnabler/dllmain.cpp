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

bool menuEnabled = true; //whole menu is kinda bugged right now :(
bool enableColorBlindCheckboxGUI = false;
LPDIRECT3DDEVICE9 device;
D3DPRESENT_PARAMETERS d3dpp;

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

	Settings.ReadKeyBinds();
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

LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM keyPressed, LPARAM lParam) {
	if (msg == WM_KEYUP) {
		//if (wParam == Settings.GetKeyBind(Settings.ReturnToggleValue("ShowSongTimerEnabled")))

		if (keyPressed == VK_DELETE) {
			float volume;
			RTPCValue_type type;

			SetRTPCValue("Mixer_SFX", 75.0, AK_INVALID_GAME_OBJECT, 0, AkCurveInterpolation_Linear);
			GetRTPCValue("Mixer_SFX", AK_INVALID_GAME_OBJECT, &volume, &type);
			std::cout << volume << " " << type << std::endl;
		}

		if (keyPressed == Settings.GetKeyBind("ToggleLoftKey") & Settings.ReturnToggleValue("ToggleLoftEnabled") == "true") {
			mem.ToggleLoft();
			std::cout << "Toggle Loft" << std::endl;
		}

		else if (keyPressed == Settings.GetKeyBind("AddVolumeKey") & Settings.ReturnToggleValue("AddVolumeEnabled") == "true") {
			mem.AddVolume(5);
			std::cout << "Adding 5 Volume" << std::endl;
		}

		else if (keyPressed == Settings.GetKeyBind("DecreaseVolumeKey") & Settings.ReturnToggleValue("DecreaseVolumeEnabled") == "true") {
			mem.DecreaseVolume(5);
			std::cout << "Subtracting 5 Volume" << std::endl;
		}

		else if (keyPressed == Settings.GetKeyBind("ShowSongTimerKey") & Settings.ReturnToggleValue("ShowSongTimerEnabled") == "true") {
			mem.ShowSongTimer();
			std::cout << "Show Me Dat Timer Bruh" << std::endl;
		}

		else if (keyPressed == Settings.GetKeyBind("ForceReEnumerationKey") & Settings.ReturnToggleValue("ForceReEnumerationEnabled") == "manual") {
			Enumeration.ForceEnumeration();
			std::cout << "ENUMERATE YOU FRICKIN' SOAB" << std::endl;
		}

		else if (keyPressed == Settings.GetKeyBind("RainbowStringsKey") & Settings.ReturnToggleValue("RainbowStringsEnabled") == "true") {
			mem.DoRainbow();
			std::cout << "Rainbows Are Pretty Cool" << std::endl;
		}

		if (keyPressed == VK_INSERT)
			menuEnabled = !menuEnabled;

	}

	if (msg == WM_ACTIVATEAPP) {
		if (device) {
			std::cout << "dingus" << std::endl;
			//ImGui_ImplDX9_InvalidateDeviceObjects();
			device->Reset(&d3dpp);
			//ImGui_ImplDX9_CreateDeviceObjects();
		}
	}

	if (menuEnabled && ImGui_ImplWin32_WndProcHandler(hWnd, msg, keyPressed, lParam)) //if we keep menuEnabled here, then we should not read hotkeys inside ImGUI's stuff - because they won't even be read when menu is disabled!
		return true;

	return CallWindowProc(oWndProc, hWnd, msg, keyPressed, lParam);
}

INT currStride;
bool cbEnabled;

HRESULT __stdcall Hook_EndScene(IDirect3DDevice9* pDevice) {
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
		D3DXCreateTextureFromFile(pDevice, L"gradient_map_additive.dds", &additiveNoteTexture);

		std::cout << "ImGUI Init" << std::endl;
	}

	ImGui_ImplDX9_NewFrame();
	ImGui_ImplWin32_NewFrame();

	ImGui::NewFrame();

	if (menuEnabled) {
		ImGui::Begin("RS Modz");
		ImGui::SliderInt("Enumeration Interval", &EnumSliderVal, 100, 100000);
		ImGui::SliderInt("Curr Slide", &currStride, 0, 10000);
		if (enableColorBlindCheckboxGUI)
			ImGui::Checkbox("Colorblind Mode", &cbEnabled);
		ImGui::End();
	}

	ImGui::EndFrame();
	ImGui::Render();

	ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());

	return oEndScene(pDevice); // Call original ensdcene so the game can draw
}

HRESULT APIENTRY Hook_Reset(LPDIRECT3DDEVICE9 pDevice, D3DPRESENT_PARAMETERS* pPresentationParameters) { //gotta do this so that ALT TAB-ing out of the game doesn't mess the whole thing up
	ImGui_ImplDX9_InvalidateDeviceObjects();

	HRESULT ResetReturn = oReset(pDevice, pPresentationParameters);

	ImGui_ImplDX9_CreateDeviceObjects();

	return ResetReturn;
}

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

HRESULT APIENTRY Hook_DIP(LPDIRECT3DDEVICE9 pDevice, D3DPRIMITIVETYPE PrimType, INT BaseVertexIndex, UINT MinVertexIndex, UINT NumVertices, UINT startIndex, UINT primCount) {
	if (pDevice->GetStreamSource(0, &Stream_Data, &Offset, &Stride) == D3D_OK)
		Stream_Data->Release();

	Mesh current(Stride, primCount, NumVertices);

	if (GetAsyncKeyState(VK_UP) & 1)
		currStride++;
	if (GetAsyncKeyState(VK_DOWN) & 1)
		currStride--;

	if (GetAsyncKeyState(VK_CONTROL) & 1)
		Log("Stride == %d && NumVertices == %d && PrimCount == %d && BaseVertexIndex == %d MinVertexIndex == %d && startIndex == %d && mStartregister == %d && PrimType == %d", Stride, NumVertices, primCount, BaseVertexIndex, MinVertexIndex, startIndex, mStartregister, PrimType);
	//Log("Stride == %d && NumVertices == %d && PrimCount == %d && BaseVertexIndex == %d MinVertexIndex == %d && startIndex == %d && mStartregister == %d && PrimType == %d", Stride, NumVertices, primCount, BaseVertexIndex, MinVertexIndex, startIndex, mStartregister, PrimType);
	while (DiscoEnabled()) {
		pDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE); // Make AMPS Semi-Transparent <- Is the one that breaks things
		pDevice->SetRenderState(D3DRS_SEPARATEALPHABLENDENABLE, TRUE); // Sticky Colors

		return oDrawIndexedPrimitive(pDevice, PrimType, BaseVertexIndex, MinVertexIndex, NumVertices, startIndex, primCount);
	}

	if (setAllToNoteGradientTexture) {
		pDevice->SetTexture(currStride, gradientTextureSeven);
		return oDrawIndexedPrimitive(pDevice, PrimType, BaseVertexIndex, MinVertexIndex, NumVertices, startIndex, primCount);
	}
	

	if (FRETNUM_AND_MISS_INDICATOR && numElements == 7 && mVectorCount == 4 && decl->Type == 2) { //almost stems.. but still doesn't work completely
		////Log("Offset: %d %d %d", primCount, decl->Usage, decl->UsageIndex);
//	Log("%d %d %d %d", MinVertexIndex, startIndex, primCount, BaseVertexIndex );
		//Log("Stride == %d && NumVertices == %d && PrimCount == %d && BaseVertexIndex == %d MinVertexIndex == %d && startIndex == %d && mStartregister == %d && PrimType == %d", Stride, NumVertices, primCount, BaseVertexIndex, MinVertexIndex, startIndex, mStartregister, PrimType);
		//Stride  NumVertices PrimCount BaseVertexIndex MinVertexIndex startIndex mStartregister PrimType mVectorCount  decl->Type numElements
		//pDevice->SetTexture(1, additiveNoteTexture	);
	}


	if (IsToBeRemoved(sevenstring, current)) { //change all pieces of note head's textures
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

	else if (IsToBeRemoved(fretless, current)) // Add GUI check
		return D3D_OK;

	return oDrawIndexedPrimitive(pDevice, PrimType, BaseVertexIndex, MinVertexIndex, NumVertices, startIndex, primCount);
}

HRESULT APIENTRY Hook_BeginScene(LPDIRECT3DDEVICE9 pD3D9) {
	device = pD3D9;
	return oBeginScene(pD3D9);
}

HRESULT APIENTRY Hook_SetVertexDeclaration(LPDIRECT3DDEVICE9 pDevice, IDirect3DVertexDeclaration9* pdecl) {
	if (pdecl != NULL)
		pdecl->GetDeclaration(decl, &numElements);

	return oSetVertexDeclaration(pDevice, pdecl);
}

HRESULT APIENTRY Hook_SetVertexShaderConstantF(LPDIRECT3DDEVICE9 pDevice, UINT StartRegister, const float* pConstantData, UINT Vector4fCount) {
	if (pConstantData != NULL) {
		mStartregister = StartRegister;
		mVectorCount = Vector4fCount;
	}

	return oSetVertexShaderConstantF(pDevice, StartRegister, pConstantData, Vector4fCount);
}

HRESULT APIENTRY Hook_SetVertexShader(LPDIRECT3DDEVICE9 pDevice, IDirect3DVertexShader9* veShader) {
	if (veShader != NULL) {
		vShader = veShader;
		vShader->GetFunction(NULL, &vSize);
	}

	return oSetVertexShader(pDevice, veShader);
}

HRESULT APIENTRY Hook_SetPixelShader(LPDIRECT3DDEVICE9 pDevice, IDirect3DPixelShader9* piShader) {
	if (piShader != NULL) {
		pShader = piShader;
		pShader->GetFunction(NULL, &pSize);
	}

	return oSetPixelShader(pDevice, piShader);
}


DWORD* GetD3DDevice() {
	HWND tmpWnd = CreateWindowA("BUTTON", "DX", WS_SYSMENU | WS_MINIMIZEBOX, CW_USEDEFAULT, CW_USEDEFAULT, 300, 300, NULL, NULL, hWnd, NULL);
	if (tmpWnd == NULL)
		return NULL;

	pD3D = Direct3DCreate9(D3D_SDK_VERSION);
	if (pD3D == NULL) {
		DestroyWindow(tmpWnd);
		return 0;
	}

	//D3DPRESENT_PARAMETERS d3dpp;
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
	while (!GetModuleHandleA("d3d9.dll")) //aight ffio ;)
		Sleep(500);

	DWORD* vTable = GetD3DDevice();
	if (vTable != NULL) {
		oSetVertexDeclaration = (tSetVertexDeclaration)MemUtil.TrampHook((PBYTE)vTable[87], (PBYTE)Hook_SetVertexDeclaration, 7);
		oSetVertexShader = (tSetVertexShader)MemUtil.TrampHook((PBYTE)vTable[92], (PBYTE)Hook_SetVertexShader, 5);
		oSetVertexShaderConstantF = (tSetVertexShaderConstantF)MemUtil.TrampHook((PBYTE)vTable[94], (PBYTE)Hook_SetVertexShaderConstantF, 7);
		oSetPixelShader = (tSetPixelShader)MemUtil.TrampHook((PBYTE)vTable[107], (PBYTE)Hook_SetPixelShader, 7);


		//oReset = (tReset)MemUtil.TrampHook((PBYTE)vTable[16], (PBYTE)Hook_Reset, 7);
		oBeginScene = (tBeginScene)MemUtil.TrampHook((PBYTE)vTable[41], (PBYTE)Hook_BeginScene, 7);
		oEndScene = (tEndScene)MemUtil.TrampHook((PBYTE)vTable[42], (PBYTE)Hook_EndScene, 7);
		oDrawIndexedPrimitive = (tDrawIndexedPrimitive)MemUtil.TrampHook((PBYTE)vTable[82], (PBYTE)Hook_DIP, 5);
		oDrawPrimitive = (tDrawPrimitive)MemUtil.TrampHook((PBYTE)vTable[81], (PBYTE)Hook_DP, 7);

	}
	else
		std::cout << "Could not initialize D3D stuff" << std::endl;
	pd3dDevice->Release();
	pD3D->Release();
}

void InitStuff() {
	SetRTPCValue = (tSetRTPCValue)Offsets.func_SetRTPCValue;
	GetRTPCValue = (tGetRTPCValue)Offsets.func_GetRTPCValue;
}

DWORD WINAPI MainThread(void*) {
	Offsets.Initialize();
	PatchCDLCCheck();

	mem.LoadSettings();
	Settings.ReadKeyBinds();
	mem.HookSongListsKoko();

	GUI();
	InitStuff();

	while (true) {

		Sleep(100);
		mem.Toggle7StringMode();



		/* Mods (Moved to LRESULT WINAPI WndProc)
			if (Settings.ReturnToggleValue("ToggleLoftEnabled") == "true")
				if (GetAsyncKeyState(Settings.GetKeyBind("ToggleLoftKey")) & 0x1) // Toggle Loft
					mem.ToggleLoft();

			if (Settings.ReturnToggleValue("AddVolumeEnabled") == "true") // Add Volume / Volume Up
				if (GetAsyncKeyState(Settings.GetKeyBind("AddVolumeKey")) & 0x1)
					mem.AddVolume(5);

			if (Settings.ReturnToggleValue("DecreaseVolumeEnabled") == "true")  // Decrease Volume / Volume Down
				if (GetAsyncKeyState(Settings.GetKeyBind("DecreaseVolumeKey")) & 0x1)
					mem.DecreaseVolume(5);

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

		 Dev Commands

			if (GetAsyncKeyState('X') & 0x1)
				mem.ShowCurrentTuning();


		  Disabled commands
			mem.ToggleLoftWhenSongStarts();
		*/

		if (enableColorBlindCheckboxGUI)
			mem.ToggleCB(cbEnabled);
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
