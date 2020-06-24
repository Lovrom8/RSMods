#include "d3dx9_42.h"
#include "MemHelpers.h"
#include "Enumeration.h"
#include "detours.h"
#include "Functions.h"
#include "Utils.h"
#include "D3D.h"
#include "CustomSongTitles.h"
#include "Offsets.h"
#include "MemUtil.h"
#include "Settings.h"
#include "ExtendedRangeMode.h"

#include "ImGUI/imgui.h"
#include "ImGUI/imgui_impl_dx9.h"
#include "ImGUI/imgui_impl_win32.h"
#include "ImGUI/RobotoFont.cpp""

const char* windowName = "Rocksmith 2014";

bool menuEnabled = false;
bool enableColorBlindCheckboxGUI = false;
bool GameLoaded = false;

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

		if (keyPressed == Settings.GetKeyBind("ToggleLoftKey") && Settings.ReturnToggleValue("ToggleLoftEnabled") == "true" && GameLoaded) { // Game must not be on the startup videos or it will crash
			MemHelpers.ToggleLoft();
			std::cout << "Toggle Loft" << std::endl;
		}

		else if (keyPressed == Settings.GetKeyBind("AddVolumeKey") && Settings.ReturnToggleValue("AddVolumeEnabled") == "true") {
			//MemHelpers.AddVolume(5); TODO: find out how to use Set/GetRTPCValue to change volume
			std::cout << "Adding 5 Volume" << std::endl;
		}

		else if (keyPressed == Settings.GetKeyBind("DecreaseVolumeKey") && Settings.ReturnToggleValue("DecreaseVolumeEnabled") == "true") {
			//MemHelpers.DecreaseVolume(5);
			std::cout << "Subtracting 5 Volume" << std::endl;
		}

		else if (keyPressed == Settings.GetKeyBind("ShowSongTimerKey") && Settings.ReturnToggleValue("ShowSongTimerEnabled") == "true") {
			MemHelpers.ShowSongTimer();
			std::cout << "Show Me Dat Timer Bruh" << std::endl;
		}

		else if (keyPressed == Settings.GetKeyBind("ForceReEnumerationKey") && Settings.ReturnToggleValue("ForceReEnumerationEnabled") == "manual") {
			Enumeration.ForceEnumeration();
			std::cout << "ENUMERATE YOU FRICKIN' SOAB" << std::endl;
		}

		else if (keyPressed == Settings.GetKeyBind("RainbowStringsKey") && Settings.ReturnToggleValue("RainbowStringsEnabled") == "true") {
			ERMode.ToggleRainbowMode();
			std::cout << "Rainbows Are Pretty Cool" << std::endl;
		}

		if (keyPressed == VK_INSERT)
			menuEnabled = !menuEnabled;
	}

	if (menuEnabled && ImGui_ImplWin32_WndProcHandler(hWnd, msg, keyPressed, lParam)) //if we keep menuEnabled here, then we should not read hotkeys inside ImGUI's stuff - because they won't even be read when menu is disabled!
		return true;

	return CallWindowProc(oWndProc, hWnd, msg, keyPressed, lParam);
}

HRESULT __stdcall Hook_EndScene(IDirect3DDevice9* pDevice) {
	static bool init = false;

	if (!init) {
		init = true;

		ImGui::CreateContext();
		ImGuiIO& io = ImGui::GetIO();
		ImFont* font = io.Fonts->AddFontFromMemoryCompressedTTF(RobotoFont_data, RobotoFont_size, 20);

		D3DDEVICE_CREATION_PARAMETERS d3dcp;
		pDevice->GetCreationParameters(&d3dcp);
		hThisWnd = d3dcp.hFocusWindow;

		oWndProc = (WNDPROC)SetWindowLongPtr(hThisWnd, GWLP_WNDPROC, (LONG_PTR)WndProc);


		ImGui_ImplWin32_Init(hThisWnd);
		ImGui_ImplDX9_Init(pDevice);
		ImGui::GetIO().ImeWindowHandle = hThisWnd;


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
		ImGui::InputInt("Curr Slide", &currStride);
		ImGui::InputInt("Curr PrimCount", &currPrimCount);
		ImGui::InputInt("Curr NumVertices", &currNumVertices);
		ImGui::InputInt("Curr StartIndex", &currStartIndex);
		ImGui::InputInt("Curr StarRegister", &currStartRegister);
		ImGui::InputInt("Curr PrimType", &currPrimType);
		ImGui::InputInt("Curr DeclType", &currDeclType);
		ImGui::InputInt("Curr VectorCount", &currVectorCount);
		ImGui::InputInt("Curr NumElements", &currNumElements);
		ImGui::InputInt("Curr Idx", &currIdx);

		if (enableColorBlindCheckboxGUI)
			ImGui::Checkbox("Colorblind Mode", &cbEnabled);

		if (ImGui::Button("Add"))
			if (allMeshes.size() > 0)
				removedMeshes.push_back(allMeshes[currIdx]);

		ImGui::End();

		ImGui::Begin("LOOSE CANNONS");
		/*ImGui::ListBoxHeader("Logged models");
		for (auto mesh : allMeshes) {
			ImGui::Selectable(mesh.ToString().c_str(), false);
		}*/

		ImGui::ListBoxHeader("Removed models");
		counter = 0;
		for (auto mesh : removedMeshes) {
			if (ImGui::Selectable(mesh.ToString().c_str(), false))
				selectedIdx = counter;

			counter++;
		}

		ImGui::ListBoxFooter();
		if (ImGui::Button("Remove"))
			removedMeshes.erase(removedMeshes.begin() + selectedIdx);

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
		MemHelpers.ToggleCB(MemHelpers.IsExtendedRangeSong());
		pDevice->SetTexture(1, gradientTextureNormal);
	}

	return oDrawPrimitive(pDevice, PrimType, startIndex, primCount);
}

UINT sthSize;
bool startLogging = false;
HRESULT APIENTRY Hook_DIP(LPDIRECT3DDEVICE9 pDevice, D3DPRIMITIVETYPE PrimType, INT BaseVertexIndex, UINT MinVertexIndex, UINT NumVertices, UINT startIndex, UINT primCount) {
	if (pDevice->GetStreamSource(0, &Stream_Data, &Offset, &Stride) == D3D_OK)
		Stream_Data->Release();

	if (GetAsyncKeyState(VK_PRIOR) & 1 && currIdx < std::size(allMeshes) - 1)//page up
		currIdx++;
	if (GetAsyncKeyState(VK_NEXT) & 1 && currIdx > 0) //page down
		currIdx--;

	if (GetAsyncKeyState(VK_END) & 1)
		startLogging = !startLogging;

	if (GetAsyncKeyState(VK_F8) & 1) { //save logged meshes to file
		for (auto mesh : allMeshes) {
			Log(mesh.ToString().c_str());
		}
	}

	if (GetAsyncKeyState(VK_F7) & 1) { //save only removed 
		for (auto mesh : removedMeshes) {
			Log(mesh.ToString().c_str());
		}
	}

	if (GetAsyncKeyState(VK_CONTROL) & 1)
		Log("{ %d, %d, %d, %d, %d, %d, %d, %d, %d }, ", Stride, primCount, NumVertices, startIndex, mStartregister, PrimType, decl->Type, mVectorCount, numElements);

	while (DiscoEnabled()) {
		pDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE); // Make AMPS Semi-Transparent <- Is the one that breaks things
		pDevice->SetRenderState(D3DRS_SEPARATEALPHABLENDENABLE, TRUE); // Sticky Colors

		return oDrawIndexedPrimitive(pDevice, PrimType, BaseVertexIndex, MinVertexIndex, NumVertices, startIndex, primCount);
	}

	if (setAllToNoteGradientTexture) {
		pDevice->SetTexture(currStride, gradientTextureSeven);
		return oDrawIndexedPrimitive(pDevice, PrimType, BaseVertexIndex, MinVertexIndex, NumVertices, startIndex, primCount);
	}

	if (FRETNUM_AND_MISS_INDICATOR && numElements == 7 && mVectorCount == 4 && decl->Type == 2 && sthSize == 128) { //almost stems.. but still doesn't work completely
		//Log("{ %d, %d, %d, %d, %d, %d, %d, %d, %d }, ", Stride, primCount, NumVertices, startIndex, mStartregister, PrimType, decl->Type, mVectorCount, numElements);
		pDevice->SetTexture(1, additiveNoteTexture);
	}

	Mesh current(Stride, primCount, NumVertices);
	ThiccMesh currentThicc(Stride, primCount, NumVertices, startIndex, mStartregister, PrimType, decl->Type, mVectorCount, numElements);

	if (startLogging) {
		if (std::find(allMeshes.begin(), allMeshes.end(), currentThicc) == allMeshes.end()) //make sure we don't log what we'd already logged
			allMeshes.push_back(currentThicc);
		//Log("{ %d, %d, %d},", Stride, primCount, NumVertices); // Log Current Texture (Small)
		//Log("{ %d, %d, %d, %d, %d, %d, %d, %d, %d }, ", Stride, primCount, NumVertices, startIndex, mStartregister, PrimType, decl->Type, mVectorCount, numElements); // Log Current Texture (Thicc)
		//Log("%s", MemHelpers.GetCurrentMenu().c_str()); // Log Current Menu
	}

	if (std::size(allMeshes) > 0 && allMeshes.at(currIdx) == currentThicc) {
		currStride = Stride;
		currNumVertices = NumVertices;
		currPrimCount = primCount;
		currStartIndex = startIndex;
		currStartRegister = mStartregister;
		currPrimType = PrimType;
		currDeclType = decl->Type;
		currVectorCount = mVectorCount;
		currNumElements = numElements;
		pDevice->SetTexture(1, Yellow);
		//return D3D_OK;
	}

	if (IsExtraRemoved(removedMeshes, currentThicc))
		return D3D_OK;

	//return oDrawIndexedPrimitive(pDevice, PrimType, BaseVertexIndex, MinVertexIndex, NumVertices, startIndex, primCount);

	if (IsToBeRemoved(sevenstring, current)) { //change all pieces of note head's textures
		DWORD origZFunc;
		pDevice->GetRenderState(D3DRS_ZFUNC, &origZFunc);

		/*if (MemHelpers.IsExtendedRangeSong())
			pDevice->SetTexture(1, gradientTextureSeven);
		else
			 pDevice->SetTexture(1, gradientTextureNormal);*/
			 //either this or CB based Extended Range mode - otherwise you may get some silly effects


		MemHelpers.ToggleCB(MemHelpers.IsExtendedRangeSong());
		pDevice->SetTexture(1, gradientTextureNormal);
	}

	else if (IsToBeRemoved(skyline, current) & Settings.ReturnToggleValue("RemoveSkylineEnabled") == "true")
		return D3D_OK;

	/* Buggy ATM
	else if (IsToBeRemoved(skylineLesson, current) & Settings.ReturnToggleValue("RemoveSkylineEnabled") == "true")
		return D3D_OK;
	*/

	else if (IsExtraRemoved(greenscreenwall, currentThicc) & Settings.ReturnToggleValue("GreenScreenWallEnabled") == "true")
		return D3D_OK;

	else if (IsExtraRemoved(headstockThicc, currentThicc) & Settings.ReturnToggleValue("RemoveHeadstockEnabled") == "true")
		return D3D_OK;

	if (IsExtraRemoved(tuningLetters, currentThicc) && (std::find(std::begin(getRidOfTuningLettersOnTheseMenus), std::end(getRidOfTuningLettersOnTheseMenus), MemHelpers.GetCurrentMenu().c_str()) != std::end(getRidOfTuningLettersOnTheseMenus))) // This is called to remove those pesky tuning letters that share the same texture values as fret numbers and chord fingerings
		return D3D_OK;

	else if (IsExtraRemoved(fretless, currentThicc) & Settings.ReturnToggleValue("FretlessModeEnabled") == "true")
		return D3D_OK;
	else if (IsExtraRemoved(inlays, currentThicc) & Settings.ReturnToggleValue("RemoveInlaysEnabled") == "true")
		return D3D_OK;
	else if (IsExtraRemoved(laneMarkers, currentThicc) & Settings.ReturnToggleValue("RemoveLaneMarkersEnabled") == "true")
		return D3D_OK;
	return oDrawIndexedPrimitive(pDevice, PrimType, BaseVertexIndex, MinVertexIndex, NumVertices, startIndex, primCount);
}

HRESULT APIENTRY Hook_BeginScene(LPDIRECT3DDEVICE9 pD3D9) {
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

HRESULT APIENTRY Hook_SetStreamSource(LPDIRECT3DDEVICE9 pDevice, UINT StreamNumber, IDirect3DVertexBuffer9* pStreamData, UINT OffsetInBytes, UINT Stride) {
	D3DVERTEXBUFFER_DESC desc;

	if (Stride == 32 && numElements == 7 && mVectorCount == 4 && decl->Type == 2) {
		pStreamData->GetDesc(&desc);

		if (desc.Size == 128) { //desc has multiple properties, so you may find something different that would differentiate two meshes if you are lucky

			//std::cout << StreamNumber << " " << OffsetInBytes << std::endl;
			//std::cout << "---------" << std::endl;

		}

		sthSize = desc.Size;
	}


	return oSetStreamSource(pDevice, StreamNumber, pStreamData, OffsetInBytes, Stride);
}


void GUI() {
	DWORD d3d9Base, adr, * vTable = NULL;
	while ((d3d9Base = (DWORD)GetModuleHandleA("d3d9.dll")) == NULL) //aight ffio ;)
		Sleep(500);

	adr = MemUtil.FindPattern(d3d9Base, 0x128000, (PBYTE)"\xC7\x06\x00\x00\x00\x00\x89\x86\x00\x00\x00\x00\x89\x86", "xx????xx????xx") + 2; //and that's it... (:

	if (!adr) {
		std::cout << "Could not find D3D9 device pointer" << std::endl;
		return;
	}

	vTable = *(DWORD**)adr;
	if (!vTable) {
		std::cout << "Could not find game device's vTable address" << std::endl;
		return;
	}

	oSetVertexDeclaration = (tSetVertexDeclaration)MemUtil.TrampHook((PBYTE)vTable[D3DInfo::SetVertexDeclaration_Index], (PBYTE)Hook_SetVertexDeclaration, 7);
	oSetVertexShader = (tSetVertexShader)MemUtil.TrampHook((PBYTE)vTable[D3DInfo::SetVertexShader_Index], (PBYTE)Hook_SetVertexShader, 7);
	oSetVertexShaderConstantF = (tSetVertexShaderConstantF)MemUtil.TrampHook((PBYTE)vTable[D3DInfo::SetVertexShaderConstantF_Index], (PBYTE)Hook_SetVertexShaderConstantF, 7);
	oSetPixelShader = (tSetPixelShader)MemUtil.TrampHook((PBYTE)vTable[D3DInfo::SetPixelShader_Index], (PBYTE)Hook_SetPixelShader, 7);
	oSetStreamSource = (tSetStreamSource)MemUtil.TrampHook((PBYTE)vTable[D3DInfo::SetStreamSource_Index], (PBYTE)Hook_SetStreamSource, 7);

	oReset = (tReset)DetourFunction((PBYTE)vTable[D3DInfo::Reset_Index], (PBYTE)Hook_Reset);
	oBeginScene = (tBeginScene)MemUtil.TrampHook((PBYTE)vTable[D3DInfo::BeginScene_Index], (PBYTE)Hook_BeginScene, 7);
	oEndScene = (tEndScene)MemUtil.TrampHook((PBYTE)vTable[D3DInfo::EndScene_Index], (PBYTE)Hook_EndScene, 7);
	oDrawIndexedPrimitive = (tDrawIndexedPrimitive)MemUtil.TrampHook((PBYTE)vTable[D3DInfo::DrawIndexedPrimitive_Index], (PBYTE)Hook_DIP, 5);
	oDrawPrimitive = (tDrawPrimitive)MemUtil.TrampHook((PBYTE)vTable[D3DInfo::DrawPrimitive_Index], (PBYTE)Hook_DP, 7);
}

void InitEngineFunctions() {
	SetRTPCValue = (tSetRTPCValue)Offsets.func_SetRTPCValue;
	GetRTPCValue = (tGetRTPCValue)Offsets.func_GetRTPCValue;
}

void AutoEnterGame() { //very big brain
	PostMessage(FindWindow(NULL, L"Rocksmith 2014"), WM_KEYDOWN, VK_RETURN, 0);
	Sleep(30);
	PostMessage(FindWindow(NULL, L"Rocksmith 2014"), WM_KEYUP, VK_RETURN, 0);
}

bool LoftOff = false;

DWORD WINAPI MainThread(void*) {
	Offsets.Initialize();
	MemHelpers.PatchCDLCCheck();

	CustomSongTitles.LoadSettings();
	Settings.ReadKeyBinds();
	CustomSongTitles.HookSongListsKoko();

	GUI();
	InitEngineFunctions();

	while (true) {
		Sleep(300);
		ERMode.Toggle7StringMode();

		/*
		 Dev Commands

		if (GetAsyncKeyState('X') & 0x1)
			MemHelpers.ShowCurrentTuning();

		  Disabled commands
			MemHelpers.ToggleLoftWhenSongStarts();
		*/

		if (GameLoaded && !LoftOff && Settings.ReturnToggleValue("ToggleLoftEnabled") == "true" && Settings.ReturnToggleValue("ToggleLoftWhen") == "startup") {
			MemHelpers.ToggleLoft();
			LoftOff = true;
		}

		if (GameLoaded && Settings.ReturnToggleValue("ToggleLoftEnabled") == "true" && Settings.ReturnToggleValue("ToggleLoftWhen") == "song")
		{
			if (std::find(std::begin(songModes), std::end(songModes), MemHelpers.GetCurrentMenu().c_str()) != std::end(songModes)) // If in a song
			{
				if(!LoftOff) // Only toggle loft once
					MemHelpers.ToggleLoft();
				LoftOff = true;
			}
			if (LoftOff && !(std::find(std::begin(songModes), std::end(songModes), MemHelpers.GetCurrentMenu().c_str()) != std::end(songModes))) // If Not In A Song
			{
				MemHelpers.ToggleLoft();
				LoftOff = false;
			}
		}
		
		if(GameLoaded)
			std::cout << MemHelpers.IsExtendedRangeSong() << std::endl;

		if (enableColorBlindCheckboxGUI)
			MemHelpers.ToggleCB(cbEnabled);

		if (!GameLoaded) {
			if (MemHelpers.GetCurrentMenu() == "MainMenu")
				GameLoaded = true;

			if (!GameLoaded && Settings.ReturnToggleValue("ForceProfileEnabled") == "true")
				AutoEnterGame();
		}

		ERMode.DoRainbow();
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
