#include "d3dx9_42.h"
#include "MemHelpers.h"
#include <gdiplus.h>
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
#pragma comment (lib, "gdiplus.lib")

/// Set Global Variables
	// Console
		const char* windowName = "Rocksmith 2014"; // Console Window Title
	// GUI Settings
		bool menuEnabled = false; // Do we show the user the imGUI settings menu?
		bool enableColorBlindCheckboxGUI = false; // Do we allow the user to change Colorblind mode in the imGUI menu?
	// Mod Settings
		bool resetHeadstockCache = true; // Do we want to reset the headstock cache? Triggers when opening tuning menu
		bool toggleSkyline = false; // Do we want to toggle the skyline right now? Triggers to false when turned on/ off
		int EnumSliderVal = 10000; // Sleep every X ms for enumeration
		bool LoftOff = false; // Is the loft disabled right now? Toggles when loft turns off (True - No Loft, False - Loft)
		bool SkylineOff = false; // Is the skyline disabled right now? Toggles when skyline turns off (True - No Skyline, False - Skyline)
		bool DrawSkylineInMenu = false; // If the user is in "Song" mode of Toggle Skyline, should we draw the skyline in this menu (True - Skyline, False - No Skyline)
	// Misc
		std::string previousMenu, currentMenu; // What is the last menu, and the current menu?
		bool GameLoaded = false; // Has the game gotten to the main menu where you can pick the gamemodes?
		bool lowPerformancePC = false; // Does your game lag with all of our mods? Toggle on to disable us running mods for an original DLL experience.
		bool setAllToNoteGradientTexture = false; // Should we override the 6-string note textures with the 7-string note textures?
		bool startLogging = false; // Should we log what's happening in Hook_DIP? Logs to log.txt in your RS2014 directory

/// End Global Variable Section


DWORD WINAPI EnumerationThread(void*) { //pls don't let me regret doing this
	Sleep(30000); //wait for half a minute, should give enough time for the initial stuff to get done

	Settings.ReadKeyBinds();
	Settings.ReadModSettings();

	int oldDLCCount = Enumeration.GetCurrentDLCCount(), newDLCCount = oldDLCCount;

	while (true) {
		if (Settings.ReturnSettingValue("ForceReEnumerationEnabled") == "automatic") {
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
		//if (wParam == Settings.GetKeyBind(Settings.ReturnSettingValue("ShowSongTimerEnabled")))

		if (keyPressed == VK_DELETE) {
			float volume;
			RTPCValue_type type;

			//SetRTPCValue("Mixer_Music", (float)volume, 0xffffffff, 0, AkCurveInterpolation_Linear);
			//GetRTPCValue("Mixer_Music", 0xffffffff, &volume, &type);
			//std::cout << volume << std::endl;
		}

		if (keyPressed == Settings.GetKeyBind("ToggleLoftKey") && Settings.ReturnSettingValue("ToggleLoftEnabled") == "on" && GameLoaded) { // Game must not be on the startup videos or it will crash
			MemHelpers.ToggleLoft();
			std::cout << "Toggle Loft" << std::endl;
		}

		else if (keyPressed == Settings.GetKeyBind("AddVolumeKey") && Settings.ReturnSettingValue("AddVolumeEnabled") == "on") {
			//MemHelpers.AddVolume(5); TODO: find out how to use Set/GetRTPCValue to change volume
			std::cout << "Adding 5 Volume" << std::endl;
		}

		else if (keyPressed == Settings.GetKeyBind("DecreaseVolumeKey") && Settings.ReturnSettingValue("DecreaseVolumeEnabled") == "on") {
			//MemHelpers.DecreaseVolume(5);
			std::cout << "Subtracting 5 Volume" << std::endl;
		}

		else if (keyPressed == Settings.GetKeyBind("ShowSongTimerKey") && Settings.ReturnSettingValue("ShowSongTimerEnabled") == "on") {
			MemHelpers.ShowSongTimer();
			std::cout << "Show Me Dat Timer Bruh" << std::endl;
		}

		else if (keyPressed == Settings.GetKeyBind("ForceReEnumerationKey") && Settings.ReturnSettingValue("ForceReEnumerationEnabled") == "manual") {
			Enumeration.ForceEnumeration();
			std::cout << "ENUMERATE YOU FRICKIN' SOAB" << std::endl;
		}

		else if (keyPressed == Settings.GetKeyBind("RainbowStringsKey") && Settings.ReturnSettingValue("RainbowStringsEnabled") == "on") {
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

void GenerateTexture(IDirect3DDevice9* pDevice) {
	Sleep(2000); //it has an unusually weird tendency of crashing if you call it too early :(

	using namespace Gdiplus;
	GdiplusStartupInput inp;
	GdiplusStartupOutput outp;
	ULONG_PTR token_;

	UINT width = 256, height = 128;

	if (Ok != GdiplusStartup(&token_, &inp, NULL))
		std::cout << "GDI+ failed to start up!" << std::endl;

	Bitmap bmp(256, 128, PixelFormat32bppARGB);
	Graphics graphics(&bmp);

	REAL blendPositions[] = { 0.0f, 0.4f, 1.0f };
	//Gdiplus::Color middleColors[] = { Gdiplus::Color::Red, Gdiplus::Color::Yellow, Gdiplus::Color::Cyan, Gdiplus::Color::Orange, Gdiplus::Color::Green,  Gdiplus::Color::Purple, Gdiplus::Color::Cyan, Gdiplus::Color::Gray };

	for (int i = 0; i < 8;i++) {
		RSColor iniColor = Settings.GetCustomColors(false)[i];
		Gdiplus::Color middleColor(iniColor.r * 255, iniColor.g * 255, iniColor.b * 255);

		Gdiplus::Color gradientColors[] = { Gdiplus::Color::Black, middleColor , Gdiplus::Color::White };
		LinearGradientBrush linGrBrush(
			Point(0, 0),
			Point(256, 8),
			Gdiplus::Color::Black,
			Gdiplus::Color::White);

		linGrBrush.SetInterpolationColors(gradientColors, blendPositions, 3);
		graphics.FillRectangle(&linGrBrush, 0, i * 8, 256, 8);
	}

	for (int i = 0; i < 8;i++) {
		RSColor iniColor = Settings.GetCustomColors(true)[i];
		Gdiplus::Color middleColor(iniColor.r * 255, iniColor.g * 255, iniColor.b * 255);

		Gdiplus::Color gradientColors[] = { Gdiplus::Color::Black, middleColor , Gdiplus::Color::White };
		LinearGradientBrush linGrBrush(
			Point(0, 0),
			Point(256, 8),
			Gdiplus::Color::Black,
			Gdiplus::Color::White);

		linGrBrush.SetInterpolationColors(gradientColors, blendPositions, 3);
		graphics.FillRectangle(&linGrBrush, 0, (i + 8) * 8, 256, 8);
	}

	CLSID pngClsid;
	CLSIDFromString(L"{557CF406-1A04-11D3-9A73-0000F81EF32E}", &pngClsid); //for BMP: {557cf400-1a04-11d3-9a73-0000f81ef32e}
	bmp.Save(L"kekw.png", &pngClsid);

	//Gdiplus::GdiplusShutdown(token_); freaking thing crashes if you do the right thing D:, i.e. if you try cleaning after yourself

	BitmapData bitmapData;

	D3DLOCKED_RECT lockedRect;

	D3DXCreateTexture(pDevice, width, height, 1, 0, D3DFMT_A8R8G8B8, D3DPOOL_MANAGED, &ourTexture);
	ourTexture->LockRect(0, &lockedRect, 0, 0);

	bmp.LockBits(&Rect(0, 0, width, height), ImageLockModeRead, PixelFormat32bppARGB, &bitmapData);
	unsigned char* pSourcePixels = (unsigned char*)bitmapData.Scan0;
	unsigned char* pDestPixels = (unsigned char*)lockedRect.pBits;

	for (int y = 0; y < height; ++y)
	{
		// copy a row
		memcpy(pDestPixels, pSourcePixels, width * 4);   // 4 bytes per pixel

		// advance row pointers
		pSourcePixels += bitmapData.Stride;
		pDestPixels += lockedRect.Pitch;
	}

	ourTexture->UnlockRect(0);

	D3DXSaveTextureToFileA("kekw_d3d.dds", D3DXIFF_DDS, ourTexture, 0);
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

		D3DXCreateTextureFromFile(pDevice, L"notes_gradient_normal.dds", &gradientTextureNormal); //if those don't exist, note heads will be "invisible" | 6-String Model
		D3DXCreateTextureFromFile(pDevice, L"notes_gradient_seven.dds", &gradientTextureSeven); // 7-String Note Colors 
		D3DXCreateTextureFromFile(pDevice, L"doesntexist.dds", &nonexistentTexture); // Black Notes
		D3DXCreateTextureFromFile(pDevice, L"gradient_map_additive.dds", &additiveNoteTexture); // Note Stems
		D3DXCreateTextureFromFile(pDevice, L"normalGradient.bmp", &normalBMP);
		D3DXCreateTextureFromFile(pDevice, L"additiveGradient.bmp", &additiveBMP);

		std::cout << "ImGUI Init" << std::endl;

		GenerateTexture(pDevice);
	}

	ImGui_ImplDX9_NewFrame();
	ImGui_ImplWin32_NewFrame();

	ImGui::NewFrame();

	if (menuEnabled) {
		ImGui::Begin("RS Modz");
		ImGui::SliderInt("Enumeration Interval", &EnumSliderVal, 100, 100000);
		/*ImGui::InputInt("Curr Slide", &currStride);
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
		*/

		static bool CB = false;
		static std::string previewValue = "Select a string";
		if (ImGui::BeginCombo("String Colors", previewValue.c_str())) {
			for (int n = 0; n < 6; n++)
			{
				const bool is_selected = (selectedString == n);
				if (ImGui::Selectable(comboStringsItems[n], is_selected, ImGuiSelectableFlags_::ImGuiSelectableFlags_DontClosePopups))
					selectedString = n;

				if (is_selected) {
					std::cout << selectedString << std::endl;
					previewValue = std::to_string(selectedString);

					RSColor currColors = Settings.GetCustomColors(false)[selectedString];
					strR = currColors.r * 255;
					strG = currColors.g * 255;
					strB = currColors.b * 255;
				}
			}
			ImGui::EndCombo();
		}
		
		ImGui::SliderInt("R", &strR, 0, 255);
		ImGui::SliderInt("G", &strG, 0, 255);
		ImGui::SliderInt("B", &strB, 0, 255);
		ImGui::Checkbox("CB", &CB);

		if (ImGui::Button("Generate Texture")) {
			Settings.SetStringColors(selectedString, Color(strR, strG, strB), CB);
			generateTexture = true;
		}

		ImGui::End();
	}

	ImGui::EndFrame();
	ImGui::Render();

	ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());

	if (generateTexture) {
		GenerateTexture(pDevice);
		generateTexture = false;
	}
		
	return hRet;
}

HRESULT APIENTRY Hook_Reset(LPDIRECT3DDEVICE9 pDevice, D3DPRESENT_PARAMETERS* pPresentationParameters) { //gotta do this so that ALT TAB-ing out of the game doesn't mess the whole thing up
	ImGui_ImplDX9_InvalidateDeviceObjects();

	HRESULT ResetReturn = oReset(pDevice, pPresentationParameters);

	ImGui_ImplDX9_CreateDeviceObjects();

	return ResetReturn;
}

bool DiscoEnabled() {
	if (Settings.ReturnSettingValue("DiscoModeEnabled") == "on") {
		return true;
	}
	else {
		return false;
	}
}

HRESULT APIENTRY Hook_DP(LPDIRECT3DDEVICE9 pDevice, D3DPRIMITIVETYPE PrimType, UINT startIndex, UINT primCount) {
	if (pDevice->GetStreamSource(0, &Stream_Data, &Offset, &Stride) == D3D_OK)
		Stream_Data->Release();

	if (Settings.ReturnSettingValue("ExtendedRangeEnabled") == "on" && Stride == 12) { //Stride 12 = tails PogU
		MemHelpers.ToggleCB(MemHelpers.IsExtendedRangeSong());
		pDevice->SetTexture(1, gradientTextureNormal);
	}

	return oDrawPrimitive(pDevice, PrimType, startIndex, primCount);
}


HRESULT APIENTRY Hook_DIP(LPDIRECT3DDEVICE9 pDevice, D3DPRIMITIVETYPE PrimType, INT BaseVertexIndex, UINT MinVertexIndex, UINT NumVertices, UINT startIndex, UINT primCount) {
	static bool calculatedCRC = false, calculatedHeadstocks = false, calculatedSkyline = false;

	if (pDevice->GetStreamSource(0, &Stream_Data, &Offset, &Stride) == D3D_OK)
		Stream_Data->Release();

	if (GetAsyncKeyState(VK_PRIOR) & 1 && currIdx < std::size(allMeshes) - 1)//page up
		currIdx++;
	if (GetAsyncKeyState(VK_NEXT) & 1 && currIdx > 0) //page down
		currIdx--;

	if (GetAsyncKeyState(VK_END) & 1) // Toggle logging
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
		//Log("{ %d, %d, %d},", Stride, primCount, NumVertices); // Log Current Texture -> Mesh
		//Log("{ %d, %d, %d, %d, %d, %d, %d, %d, %d }, ", Stride, primCount, NumVertices, startIndex, mStartregister, PrimType, decl->Type, mVectorCount, numElements); // Log Current Texture -> ThiccMesh
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

	if (Settings.ReturnSettingValue("ExtendedRangeEnabled") == "on" && MemHelpers.IsExtendedRangeSong() && IsToBeRemoved(sevenstring, current)) { //change all pieces of note head's textures
		DWORD origZFunc;
		pDevice->GetRenderState(D3DRS_ZFUNC, &origZFunc);

		/*if (MemHelpers.IsExtendedRangeSong())
			pDevice->SetTexture(1, gradientTextureSeven);
		else
			 pDevice->SetTexture(1, gradientTextureNormal);*/
			 //either this or CB based Extended Range mode - otherwise you may get some silly effects


		MemHelpers.ToggleCB(MemHelpers.IsExtendedRangeSong());
		//pDevice->SetTexture(1, normalBMP);
		pDevice->SetTexture(1, ourTexture);
	}

	if (!lowPerformancePC) {
		if (toggleSkyline && Stride == 16) {
			if (DrawSkylineInMenu) { // If the user is in "Song" mode for Toggle Skyline and is NOT in a song -> draw the UI
				SkylineOff = false;
				return oDrawIndexedPrimitive(pDevice, PrimType, BaseVertexIndex, MinVertexIndex, NumVertices, startIndex, primCount);
			}

			// Skyline Removal
			pDevice->GetTexture(1, &pBaseTextures[1]);
			pCurrTextures[1] = (LPDIRECT3DTEXTURE9)pBaseTextures[1];

			if (pBaseTextures[1]) {  // There's only two textures in Stage 1 for meshes with Stride = 16, so we could as well skip CRC calcuation and just check if !pBaseTextures[1] and return D3D_OK directly
				if (CRCForTexture(pCurrTextures[1], crc)) {
					if (crc == 0x65b846aa || crc == 0xbad9e064) { // Purple rectangles + orange line beneath them
						SkylineOff = true;
						return D3D_OK;
					}
				}
			}

			pDevice->GetTexture(0, &pBaseTextures[0]);
			pCurrTextures[0] = (LPDIRECT3DTEXTURE9)pBaseTextures[0];

			if (pBaseTextures[0]) {
				if (CRCForTexture(pCurrTextures[0], crc)) {
					if (crc == 0xc605fbd2 || crc == 0xff1c61ff) {  // There's a few more of textures used in Stage 0, so doing the same is no-go; Shadow-ish thing in the background + backgrounds of rectangles
						SkylineOff = true;
						return D3D_OK;
					}
				}
			}
		}
		else if (Settings.ReturnSettingValue("RemoveHeadstockEnabled") == "on") { //TODO: confirm this caching thingy works for different headstocks
			if (Stride == 44 || Stride == 56 || Stride == 60 || Stride == 68 || Stride == 76 || Stride == 84) { // If we call GetTexture without any filtering, it causes a lockup when ALT-TAB-ing/changing fullscreen to windowed and vice versa
				pDevice->GetTexture(1, &pBaseTextures[1]);
				pCurrTextures[1] = (LPDIRECT3DTEXTURE9)pBaseTextures[1];

				if (resetHeadstockCache && IsExtraRemoved(headstockThicc, currentThicc)) {
					if (!pBaseTextures[1]) //if there's no texture for Stage 1
						return D3D_OK;

					if (CRCForTexture(pCurrTextures[1], crc)) {
						if (crc == 0x008d5439 || crc == 0x000d4439 || crc == 0x00000000 || crc == 0xa55470f6 || crc == 0x008f4039)
							AddToTextureList(headstockTexutrePointers, pCurrTextures[1]);
					}

					//Log("0x%08x", crc);

					if (headstockTexutrePointers.size() == 3) {
						calculatedHeadstocks = true;
						resetHeadstockCache = false;
						std::cout << "Calculated headstock CRCs (Menu: " << currentMenu << " )" << std::endl;
					}

					return D3D_OK;
				}

				if (calculatedHeadstocks)
					if (std::find(std::begin(headstockTexutrePointers), std::end(headstockTexutrePointers), pCurrTextures[1]) != std::end(headstockTexutrePointers))
						return D3D_OK;
			}

			if (IsExtraRemoved(tuningLetters, currentThicc) && (std::find(std::begin(tuningMenus), std::end(tuningMenus), MemHelpers.GetCurrentMenu().c_str()) != std::end(tuningMenus))) // This is called to remove those pesky tuning letters that share the same texture values as fret numbers and chord fingerings
				return D3D_OK;
		}
		else if (Settings.ReturnSettingValue("GreenScreenWallEnabled") == "on" && IsExtraRemoved(greenscreenwall, currentThicc))
			return D3D_OK;
		else if (Settings.ReturnSettingValue("FretlessModeEnabled") == "on" && IsExtraRemoved(fretless, currentThicc))
			return D3D_OK;
		else if (Settings.ReturnSettingValue("RemoveInlaysEnabled") == "on" && IsExtraRemoved(inlays, currentThicc))
			return D3D_OK;
		else if (Settings.ReturnSettingValue("RemoveLaneMarkersEnabled") == "on" && IsExtraRemoved(laneMarkers, currentThicc))
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

void AutoEnterGame() { //very big brain || "Fork in the toaster"
	PostMessage(FindWindow(NULL, L"Rocksmith 2014"), WM_KEYDOWN, VK_RETURN, 0);
	Sleep(30);
	PostMessage(FindWindow(NULL, L"Rocksmith 2014"), WM_KEYUP, VK_RETURN, 0);
}



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
		Sleep(2000); //TODO: determine a more appropriate delay - since we don't use this for keybinds anymore, we may as well check less often

		currentMenu = MemHelpers.GetCurrentMenu();

		if (GameLoaded) // If Game Is Loaded (No need to run these while the game is loading.)
		{
			ERMode.Toggle7StringMode();
			if (!LoftOff && Settings.ReturnSettingValue("ToggleLoftEnabled") == "on" && Settings.ReturnSettingValue("ToggleLoftWhen") == "startup") { // Runs on startup, only once
				MemHelpers.ToggleLoft();
				LoftOff = true;
			}
			if (!SkylineOff && Settings.ReturnSettingValue("RemoveSkylineEnabled") == "on" && Settings.ReturnSettingValue("ToggleSkylineWhen") == "startup") { // Runs on startup, only once
				toggleSkyline = true;
			}

			if (std::find(std::begin(songModes), std::end(songModes), currentMenu.c_str()) != std::end(songModes)) // If User Is Entering Song
			{
				if (Settings.ReturnSettingValue("ToggleLoftEnabled") == "on" && Settings.ReturnSettingValue("ToggleLoftWhen") == "song")
				{
					if (!LoftOff)
						MemHelpers.ToggleLoft();
					LoftOff = true;
				}

				if (Settings.ReturnSettingValue("RemoveSkylineEnabled") == "on" && Settings.ReturnSettingValue("ToggleSkylineWhen") == "song")
				{
					if (!SkylineOff)
						toggleSkyline = true;
					DrawSkylineInMenu = false;
				}
			}
			else // If User Is Exiting Song
			{
				if (LoftOff && Settings.ReturnSettingValue("ToggleLoftEnabled") == "on" && Settings.ReturnSettingValue("ToggleLoftWhen") == "song") {
					MemHelpers.ToggleLoft();
					LoftOff = false;
				}

				if (SkylineOff && Settings.ReturnSettingValue("RemoveSkylineEnabled") == "on" && Settings.ReturnSettingValue("ToggleSkylineWhen") == "song") {
					toggleSkyline = true;
					DrawSkylineInMenu = true;
				}
			}

			if (previousMenu != currentMenu && std::find(std::begin(tuningMenus), std::end(tuningMenus), currentMenu) != std::end(tuningMenus)) { // If the current menu is not the same as the previous menu and if it's one of menus where you tune your guitar (i.e. headstock is shown), reset the cache because user may want to change the headstock style
				resetHeadstockCache = true;
				headstockTexutrePointers.clear();
			}
			previousMenu = currentMenu;
		}
		else // Game Hasn't Loaded Yet
		{
			if (MemHelpers.GetCurrentMenu() == "MainMenu") // Yay We Loaded :P
				GameLoaded = true;

			if (Settings.ReturnSettingValue("ForceProfileEnabled") == "on") // "Fork in the toaster" / Spam Enter Method
				AutoEnterGame();
			//	std::cout << MemHelpers.IsExtendedRangeSong() << std::endl;
		}

		if (enableColorBlindCheckboxGUI)
			MemHelpers.ToggleCB(cbEnabled);

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
