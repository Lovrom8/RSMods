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

#include <intrin.h>
#pragma intrinsic(_ReturnAddress)

const char* windowName = "Rocksmith 2014";

bool menuEnabled = false;
bool enableColorBlindCheckboxGUI = false;
bool GameLoaded = false;
bool ifYourNameIsFfio = false; // AKA if your game lags like shit when you run the texture checks || AKA we have too many mods :P

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

			//SetRTPCValue("Mixer_Music", (float)volume, 0xffffffff, 0, AkCurveInterpolation_Linear);
			//GetRTPCValue("Mixer_Music", 0xffffffff, &volume, &type);
			//std::cout << volume << std::endl;
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
	HRESULT hRet = oEndScene(pDevice);
	DWORD dwReturnAddress = (DWORD)_ReturnAddress(); //EndScene is called both by the game and by Steam's overlay renderer, and there's no need to draw our stuff twice

	if (dwReturnAddress > Offsets.baseEnd)
		return hRet;

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
		D3DXCreateTextureFromFile(pDevice, L"normalGradient.bmp", &normalBMP);
		D3DXCreateTextureFromFile(pDevice, L"additiveGradient.bmp", &additiveBMP);

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

	return hRet;
}

HRESULT APIENTRY Hook_Reset(LPDIRECT3DDEVICE9 pDevice, D3DPRESENT_PARAMETERS* pPresentationParameters) { //gotta do this so that ALT TAB-ing out of the game doesn't mess the whole thing up
	ImGui_ImplDX9_InvalidateDeviceObjects();

	HRESULT ResetReturn = oReset(pDevice, pPresentationParameters);

	ImGui_ImplDX9_CreateDeviceObjects();

	return ResetReturn;
}

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

	if (Settings.ReturnToggleValue("ExtendedRangeEnabled") == "true" && Stride == 12) { //Stride 12 = tails PogU
		MemHelpers.ToggleCB(MemHelpers.IsExtendedRangeSong());
		pDevice->SetTexture(1, gradientTextureNormal);
	}


	return oDrawPrimitive(pDevice, PrimType, startIndex, primCount);
}


bool startLogging = false;
HRESULT APIENTRY Hook_DIP(LPDIRECT3DDEVICE9 pDevice, D3DPRIMITIVETYPE PrimType, INT BaseVertexIndex, UINT MinVertexIndex, UINT NumVertices, UINT startIndex, UINT primCount) {
	static bool calculatedCRC = false, calculatedHeadstocks = false, calculatedSkyline = false;

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

	//std::vector<DWORD> crcs = { 0x00004a4a, 0x00035193, 0x00090000, 0x005a00b9, 0x02a50002, 0xb8059160 };
	if (FRETNUM_AND_MISS_INDICATOR && numElements == 7 && mVectorCount == 4 && decl->Type == 2) {
		pDevice->GetTexture(1, &pBaseTexture);
		//pDevice->SetTexture(1, additiveBMP);
		pCurrTexture = (LPDIRECT3DTEXTURE9)pBaseTexture;

		if (calculatedCRC) {
			if (pCurrTexture == stemTexture)
				pDevice->SetTexture(1, additiveNoteTexture);
		}
		else if (CRCForTexture(pCurrTexture, crc)) {
			//	if(crc == crcs[currIdx])
			if (crc == 0x02a50002) {
				stemTexture = pCurrTexture;
				std::cout << "Calculated stem CRC" << std::endl;
				calculatedCRC = true;
			}

			//Log("0x%08x", crc);
		}

		return oDrawIndexedPrimitive(pDevice, PrimType, BaseVertexIndex, MinVertexIndex, NumVertices, startIndex, primCount);
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
		//pDevice->SetTexture(1, Yellow);
		return D3D_OK;
	}

	if (IsExtraRemoved(removedMeshes, currentThicc))
		return D3D_OK;

	if (Settings.ReturnToggleValue("ExtendedRangeEnabled") == "true" && MemHelpers.IsExtendedRangeSong() && IsToBeRemoved(sevenstring, current)) { //change all pieces of note head's textures
		DWORD origZFunc;
		pDevice->GetRenderState(D3DRS_ZFUNC, &origZFunc);

		/*if (MemHelpers.IsExtendedRangeSong())
			pDevice->SetTexture(1, gradientTextureSeven);
		else
			 pDevice->SetTexture(1, gradientTextureNormal);*/
			 //either this or CB based Extended Range mode - otherwise you may get some silly effects


		MemHelpers.ToggleCB(MemHelpers.IsExtendedRangeSong());
		//pDevice->SetTexture(1, normalBMP);
		pDevice->SetTexture(1, gradientTextureNormal);
	}

	if (!ifYourNameIsFfio) { // If you run Skyline, Greenscreen, Headstock, fretless, and inlays all at the same time you get shitty FPS;
		if (Settings.ReturnToggleValue("RemoveSkylineEnabled") == "true" && Stride == 16) {
			pDevice->GetTexture(1, &pBaseTextures[1]);
			pCurrTextures[1] = (LPDIRECT3DTEXTURE9)pBaseTextures[1];

			if (pBaseTextures[1]) {  // There's only two textures in Stage 1 for meshes with Stride = 16, so we could as well skip CRC calcuation and just check if !pBaseTextures[1] and return D3D_OK directly
				if (CRCForTexture(pCurrTextures[1], crc))
					if (crc == 0x65b846aa || crc == 0xbad9e064) // Purple rectangles + 
						return D3D_OK;
			}

			pDevice->GetTexture(0, &pBaseTextures[0]);
			pCurrTextures[0] = (LPDIRECT3DTEXTURE9)pBaseTextures[0];

			if (pBaseTextures[0]){
				if (CRCForTexture(pCurrTextures[0], crc))
					if (crc == 0xc605fbd2 || crc == 0xff1c61ff) // There's a few more of textures used in Stage 0, so doing the same is no-go; Shadow-ish thing in the background + backgrounds of rectangles
						return D3D_OK;
			}
		}
		else if (Settings.ReturnToggleValue("RemoveHeadstockEnabled") == "true") { //TODO: when we confirm whether it's better for performance, add CRCs for other headstock types
			if (Stride == 60 || Stride == 44 || Stride == 76 || Stride == 68) { // If we call GetTexture without any filtering, it causes a lockup when ALT-TAB-ing/changing fullscreen to windowed and vice versa
				pDevice->GetTexture(1, &pBaseTextures[1]);
				pCurrTextures[1] = (LPDIRECT3DTEXTURE9)pBaseTextures[1];

				if (calculatedHeadstocks) {
					for (auto pTexture : headstockTexutrePointers)
						if (pTexture == pCurrTextures[1])
							return D3D_OK;
				}
				else if (IsExtraRemoved(headstockThicc, currentThicc)) {
					if (!pBaseTextures[1]) //if there's no texture for Stage 1
						return D3D_OK;

					if (CRCForTexture(pCurrTextures[1], crc)) {
						if (crc == 0x008d5439 || crc == 0x000d4439 || crc == 0x00000000 || crc == 0xa55470f6) //00000s for some reason
							AddToTextureList(headstockTexutrePointers, pCurrTextures[1]);
					}

					//Log("0x%08x", crc);

					if (headstockTexutrePointers.size() == 4) { //for your usual 3+3 there's only 4 textures in total, ALSO we need to add the rest :P
						calculatedHeadstocks = true;
						std::cout << "Calculated headstock CRCs" << std::endl;
					}

					return D3D_OK;
				}
			}

			if (IsExtraRemoved(tuningLetters, currentThicc) && (std::find(std::begin(getRidOfTuningLettersOnTheseMenus), std::end(getRidOfTuningLettersOnTheseMenus), MemHelpers.GetCurrentMenu().c_str()) != std::end(getRidOfTuningLettersOnTheseMenus))) // This is called to remove those pesky tuning letters that share the same texture values as fret numbers and chord fingerings
				return D3D_OK;

		}
		else if (Settings.ReturnToggleValue("GreenScreenWallEnabled") == "true" && IsExtraRemoved(greenscreenwall, currentThicc))
			return D3D_OK;
		else if (Settings.ReturnToggleValue("FretlessModeEnabled") == "true" && IsExtraRemoved(fretless, currentThicc))
			return D3D_OK;
		else if (Settings.ReturnToggleValue("RemoveInlaysEnabled") == "true" && IsExtraRemoved(inlays, currentThicc))
			return D3D_OK;
		else if (Settings.ReturnToggleValue("RemoveLaneMarkersEnabled") == "true" && IsExtraRemoved(laneMarkers, currentThicc))
			return D3D_OK;
	}
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

	if (Stride == 32 && numElements == 8 && mVectorCount == 4 && decl->Type == 2) { // Remove Line Markers
		pStreamData->GetDesc(&desc);

		vertexBufferSize = desc.Size;
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
	CustomSongTitles.HookSongListsKoko();

	Settings.ReadKeyBinds();
	Settings.ReadModSettings();
	Settings.ReadStringColors();

	GUI();
	InitEngineFunctions();

	while (true) {
		Sleep(300);

		if (GameLoaded)
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
				if (!LoftOff) // Only toggle loft once
					MemHelpers.ToggleLoft();
				LoftOff = true;
			}
			if (LoftOff && !(std::find(std::begin(songModes), std::end(songModes), MemHelpers.GetCurrentMenu().c_str()) != std::end(songModes))) // If Not In A Song
			{
				MemHelpers.ToggleLoft();
				LoftOff = false;
			}
		}

		//if(GameLoaded)
		//	std::cout << MemHelpers.IsExtendedRangeSong() << std::endl;

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
