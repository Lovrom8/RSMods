#include "Main.hpp"
#pragma warning(disable: 4838 4018 4996 26817 6031) // Conversion Color->Color | Signed->Unsigned mismatch | freopen potentially not safe | Potentially exposed variable (mesh) | Return value ignored 'freopen'.

#ifdef _DEBUG
bool debug = true;
#else
bool debug = false;
#endif

unsigned WINAPI EnumerationThread(void*) {
	while (!GameLoaded) // We are in no hurry :)
		Sleep(5000);

	Settings::ReadKeyBinds();
	Settings::ReadModSettings();

	int oldDLCCount = Enumeration::GetCurrentDLCCount(), newDLCCount = oldDLCCount;

	while (!GameClosing) {
		if (Settings::ReturnSettingValue("ForceReEnumerationEnabled") == "automatic") {
			oldDLCCount = newDLCCount;
			newDLCCount = Enumeration::GetCurrentDLCCount();

			if (oldDLCCount != newDLCCount)
				Enumeration::ForceEnumeration();
		}

		Sleep(Settings::GetModSetting("CheckForNewSongsInterval"));
		//Sleep(EnumSliderVal);
	}

	return 0;
}

LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM keyPressed, LPARAM lParam) {
	if (menuEnabled && ImGui_ImplWin32_WndProcHandler(hWnd, msg, keyPressed, lParam))
		return true;

	if (msg == WM_KEYUP) {
		if (GameLoaded) { // Game must not be on the startup videos or it will crash
			if (keyPressed == Settings::GetKeyBind("ToggleLoftKey") && Settings::ReturnSettingValue("ToggleLoftEnabled") == "on") {
				MemHelpers::ToggleLoft();
				std::cout << "Toggle Loft" << std::endl;
			}

			else if (keyPressed == Settings::GetKeyBind("ShowSongTimerKey") && Settings::ReturnSettingValue("ShowSongTimerEnabled") == "on") {
				showSongTimerOnScreen = !showSongTimerOnScreen;
				std::cout << "Show Me Dat Timer Bruh" << std::endl;
			}

			else if (keyPressed == Settings::GetKeyBind("ForceReEnumerationKey") && Settings::ReturnSettingValue("ForceReEnumerationEnabled") == "manual") {
				Enumeration::ForceEnumeration();
				std::cout << "ENUMERATE YOU FRICKIN' SOAB" << std::endl;
			}

			else if (keyPressed == Settings::GetKeyBind("RainbowStringsKey") && Settings::ReturnSettingValue("RainbowStringsEnabled") == "on") {
				ERMode::ToggleRainbowMode();
				std::cout << "Rainbows Are Pretty Cool" << std::endl;
			}
			else if (keyPressed == Settings::GetKeyBind("RemoveLyricsKey") && Settings::ReturnSettingValue("RemoveLyricsWhen") == "manual") {
				RemoveLyrics = !RemoveLyrics;
				if (RemoveLyrics)
					std::cout << "No "; // Keep this without a endl so it appears as "No Karaoke For You" when on and "Karaoke For You" when off.
				std::cout << "Karaoke For You" << std::endl;
			}
			else if (keyPressed == 0x41 && (GetKeyState(VK_CONTROL) & 0x8000)) { //CTRL + A
				Settings::UpdateSettings();
				std::cout << "Value: " << Settings::ReturnSettingValue("ExtendedRangeEnabled") << std::endl;
				std::cout << "Reloaded settings" << std::endl;
			}
			else if (keyPressed == Settings::GetKeyBind("AddVolumeKey") && Settings::ReturnSettingValue("AddVolumeEnabled") == "on") {
				float volume = MemHelpers::GetCurrentMusicVolume();

				if (volume < 100.0f) {
					volume += 5.0f;

					SetRTPCValue("Mixer_Music", (float)volume, 0xffffffff, 0, AkCurveInterpolation_Linear);
					SetRTPCValue("Mixer_Music", (float)volume, 0x00001234, 0, AkCurveInterpolation_Linear);
				}


				//RTPCValue_type type;
				//GetRTPCValue("Mixer_Music", 0xffffffff, &volume, &type); Does not return correct value even if the effect is there
				//GetRTPCValue("Mixer_Music", 0x00001234, &volume, &type);
				//std::cout << volume << std::endl;
			}
			else if (keyPressed == Settings::GetKeyBind("DecreaseVolumeKey") && Settings::ReturnSettingValue("DecreaseVolumeEnabled") == "on") {
				float volume = MemHelpers::GetCurrentMusicVolume();

				if (volume > 0.0f) {
					volume -= 5.0f;

					SetRTPCValue("Mixer_Music", (float)volume, 0xffffffff, 0, AkCurveInterpolation_Linear);
					SetRTPCValue("Mixer_Music", (float)volume, 0x00001234, 0, AkCurveInterpolation_Linear);
				}
			}
			else if (keyPressed == VK_F9) {
				DiscoModeEnabled = !DiscoModeEnabled;

				if (debug) {
					std::cout << "Turning ";
					if (!DiscoModeEnabled)
						std::cout << "off";
					else
						std::cout << "on";
					std::cout << " Disco Mode!" << std::endl;
				}
			}
		}

		if (debug) {
			if (keyPressed == VK_INSERT)
				menuEnabled = !menuEnabled;
		}
	}

	if (msg == WM_CLOSE)
		GameClosing = true;

	return CallWindowProc(oWndProc, hWnd, msg, keyPressed, lParam);
}

ColorMap GetCustomColors(int strIdx, bool CB) {
	RSColor iniColor;
	std::string ext;

	if (CB)
		ext = "_CB";
	else
		ext = "_N";

	iniColor = Settings::GetCustomColors(CB)[strIdx];
	int H;
	float S, L;
	CollectColors::RGB2HSL(iniColor.r, iniColor.g, iniColor.b, H, S, L);

	ColorMap customColors = {
		{"Ambient" + ext, CollectColors::GetAmbientStringColor(H, CB)},
		{"Disabled" + ext, CollectColors::GetDisabledStringColor(H, S, L, CB)},
		{"Enabled" + ext, iniColor},
		{"Glow" + ext, CollectColors::GetGlowStringColor(H)},
		{"PegsTuning" + ext, CollectColors::GetGlowStringColor(H)},
		{"PegsReset" + ext, CollectColors::GetPegResetColor()},
		{"PegsSuccess" + ext, CollectColors::GetPegSuccessColor(CB)},
		{"PegsInTune" + ext, CollectColors::GetPegInTuneColor(H, CB)},
		{"PegsOutTune" + ext, CollectColors::GetPegOutTuneColor()},
		{"TextIndicator" + ext, CollectColors::GetRegTextIndicatorColor(H, CB)},
		{"ForkParticles" + ext, CollectColors::GetRegForkParticlesColor(H, CB)},
		{"NotewayNormal" + ext, CollectColors::GetNotewayNormalColor(H, S, L, CB)},
		{"NotewayAccent" + ext, CollectColors::GetNotewayAccentColor(H, CB)},
		{"NotewayPreview" + ext, CollectColors::GetNotewayPreviewColor(H, CB)},
		{"GC_Main" + ext, CollectColors::GetGuitarcadeMainColor(H, strIdx, CB)},
		{"GC_Add" + ext, CollectColors::GetGuitarcadeAdditiveColor(H, strIdx, CB)},
		{"GC_UI" + ext, CollectColors::GetGuitarcadeUIColor(H, strIdx, CB)}
	};

	return customColors;
}

void SetCustomColors() {
	for (int strIdx = 0; strIdx < 6;strIdx++) {
		ColorMap customColorsFull;

		ColorMap normalColors = GetCustomColors(strIdx, false);
		ColorMap cbColors = GetCustomColors(strIdx, true);


		customColorsFull.insert(normalColors.begin(), normalColors.end());
		customColorsFull.insert(cbColors.begin(), cbColors.end());

		ERMode::SetCustomColors(strIdx, customColorsFull);
	}
}

RSColor GenerateRandomColor() {
	RSColor rndColor;

	static std::uniform_real_distribution<> urd(0.0f, 1.0f);
	rndColor.r = urd(rng);
	rndColor.g = urd(rng);
	rndColor.b = urd(rng);

	return rndColor;
}

void GenerateTexture(IDirect3DDevice9* pDevice, IDirect3DTexture9** ppTexture, ColorList colorSet) {
	while (GetModuleHandleA("gdiplus.dll") == NULL) // JIC, to prevent crashing
		Sleep(500);

	using namespace Gdiplus;
	UINT width = 256, height = 128;
	int lineHeight = 8;

	if (Ok != GdiplusStartup(&token_, &inp, NULL))
		std::cout << "GDI+ failed to start up!" << std::endl;

	Bitmap bmp(width, height, PixelFormat32bppARGB);
	Graphics graphics(&bmp);
	RSColor currColor;

	REAL blendPositions[] = { 0.0f, 0.4f, 1.0f };

	for (int i = 0; i < 16;i++) {
		currColor = colorSet[i]; // If we are in range of 0-7, grab the normal colors, otherwise grab CB colors

		Gdiplus::Color middleColor(currColor.r * 255, currColor.g * 255, currColor.b * 255);

		Gdiplus::Color gradientColors[] = { Gdiplus::Color::Black, middleColor , Gdiplus::Color::White };
		LinearGradientBrush linGrBrush( // Base texture for note gradients (top / normal)
			Point(0, 0),
			Point(width, lineHeight),
			Gdiplus::Color::Black,
			Gdiplus::Color::White);
		LinearGradientBrush whiteCoverupBrush( // Coverup for some spotty gradients (top / normal)
			Point(width - 3, lineHeight * 5),
			Point(width, height),
			Gdiplus::Color::White,
			Gdiplus::Color::White);

		linGrBrush.SetInterpolationColors(gradientColors, blendPositions, 3);
		graphics.FillRectangle(&linGrBrush, 0, i * lineHeight, width, lineHeight);
		graphics.FillRectangle(&whiteCoverupBrush, width - 3, i * lineHeight, width, lineHeight); // Don't hate me for this hacky fix to the black bars.
	}

	// Uncomment if you want to save the generated texture
	/*CLSID pngClsid;
	CLSIDFromString(L"{557CF406-1A04-11D3-9A73-000F81EF32E}", &pngClsid); //for BMP: {557cf400-1a04-11d3-9a73-0000f81ef32e}
	bmp.Save(L"generatedTexture.png", &pngClsid); */

	BitmapData bitmapData;
	D3DLOCKED_RECT lockedRect;

	D3DXCreateTexture(pDevice, width, height, 1, 0, D3DFMT_A8R8G8B8, D3DPOOL_MANAGED, ppTexture);

	(*ppTexture)->LockRect(0, &lockedRect, 0, 0);

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

	(*ppTexture)->UnlockRect(0);

	//D3DXSaveTextureToFileA("generatedTexture_d3d.dds", D3DXIFF_DDS, (*ppTexture), 0);

	SetCustomColors();
}

void GenerateTextures(IDirect3DDevice9* pDevice, TextureType type) {
	ColorList colorSet;

	if (type == Random || type == Random_Solid) {
		for (int textIdx = 0; textIdx < randomTextureCount;textIdx++) {
			if (type == Random_Solid) {
				RSColor iniColor = GenerateRandomColor();

				for (int i = 0; i < 16; i++)
					colorSet[i] = iniColor;
			}
			else {
				for (int i = 0; i < 16; i++)
					colorSet[i] = GenerateRandomColor();
			}

			GenerateTexture(pDevice, &randomTextures[textIdx], colorSet);
			colorSet.clear();
		}
	}
	else if (type == Rainbow) {
		float h = 0.0f, stringOffset = 20.0f;
		int currTexture = 0;

		Color c;
		ColorList colorsRainbow;

		while (h < 360.f) {
			h += rainbowSpeed;

			for (int i = 0; i < 8;i++) { // There's two extra colors per string, so we may need to think about this a bit more
				c.setH(h + (stringOffset * i));

				colorsRainbow.push_back(c);
			}

			colorSet.insert(colorSet.begin(), colorsRainbow.begin(), colorsRainbow.end()); // Both CB and regular colors will still look the same in rainbow mode
			colorSet.insert(colorSet.end(), colorsRainbow.begin(), colorsRainbow.end());

			GenerateTexture(pDevice, &rainbowTextures[currTexture], colorSet);

			colorSet.clear();
			colorsRainbow.clear();

			currTexture++;
		}
	}
	else if (type == Custom) {
		ColorList colorsN = Settings::GetCustomColors(false);
		ColorList colorsCB = Settings::GetCustomColors(true);

		colorSet.insert(colorSet.begin(), colorsN.begin(), colorsN.end());
		colorSet.insert(colorSet.end(), colorsCB.begin(), colorsCB.end());

		GenerateTexture(pDevice, &ourTexture, colorSet);
	}
}

HRESULT __stdcall Hook_EndScene(IDirect3DDevice9* pDevice) {
	HRESULT hRet = oEndScene(pDevice);
	uint32_t returnAddress = (uint32_t)_ReturnAddress(); // EndScene is called both by the game and by Steam's overlay renderer, and there's no need to draw our stuff twice

	if (returnAddress > Offsets::baseEnd)
		return hRet;

	static bool init = false; // Has this been ran before (AKA run only once, at startup)

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
		ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange;

		GenerateTexture(pDevice, &Red, D3DCOLOR_ARGB(255, 000, 255, 255));
		GenerateTexture(pDevice, &Green, D3DCOLOR_ARGB(255, 0, 255, 0));
		GenerateTexture(pDevice, &Blue, D3DCOLOR_ARGB(255, 0, 0, 255));
		GenerateTexture(pDevice, &Yellow, D3DCOLOR_ARGB(255, 255, 255, 0));

		/* Used before, not necessary anymore because we generate our texture
		D3DXCreateTextureFromFile(pDevice, L"notes_gradient_normal.dds", &gradientTextureNormal); //if those don't exist, note heads will be "invisible" | 6-String Model
		D3DXCreateTextureFromFile(pDevice, L"notes_gradient_seven.dds", &gradientTextureSeven); // 7-String Note Colors
		D3DXCreateTextureFromFile(pDevice, L"doesntexist.dds", &nonexistentTexture); // Black Notes
		D3DXCreateTextureFromFile(pDevice, L"gradient_map_additive.dds", &additiveNoteTexture); // Note Stems
		*/

		std::cout << "ImGUI Init" << std::endl;

		Settings::UpdateSettings();

		GenerateTextures(pDevice, Custom);
		GenerateTextures(pDevice, Rainbow);
		//GenerateTextures(pDevice, Random);
		//GenerateTextures(pDevice, Random_Solid);
	}

	ImGui_ImplDX9_NewFrame();
	ImGui_ImplWin32_NewFrame();

	ImGui::NewFrame();

	if (menuEnabled) {
		ImGui::Begin("RS Modz");

		static bool CB = false;
		static std::string previewValue = "Select a string";
		if (ImGui::BeginCombo("String Colors", previewValue.c_str())) {
			for (int n = 0; n < 6; n++)
			{
				const bool is_selected = (selectedString == n);
				if (ImGui::Selectable(comboStringsItems[n], is_selected, ImGuiSelectableFlags_::ImGuiSelectableFlags_DontClosePopups))
					selectedString = n;

				if (is_selected) {
					previewValue = std::to_string(selectedString);

					RSColor currColors = Settings::GetCustomColors(CB)[selectedString];
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
			Settings::SetStringColors(selectedString, Color(strR, strG, strB), CB);
			generateTexture = true;
		}

		if (ImGui::Button("Restore default colors"))
			ERMode::ResetString(selectedString);

		ImGui::End();
	}

	ImGui::EndFrame();
	ImGui::Render();

	ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());

	if (generateTexture) {
		GenerateTextures(pDevice, Custom);
		generateTexture = false;
	}

	int whiteText = 0xFFFFFFFF;

	if (GameLoaded) { // Draw text on screen || NOTE: NEVER USE SET VALUES. ALWAYS DO MemHelpers::GetWindowSize() X AND Y. THIS MEANS IT WILL SHOW IN THE SAME PLACE ON EVERY RESOLUTION!
		// Consistent Fonts: Width = (MemHelpers::GetWindowSize()[0] / 96), Height = (MemHelpers::GetWindowSize()[1] / 72)
		if (Settings::ReturnSettingValue("AddVolumeEnabled") == "on" && MemHelpers::IsInStringArray(currentMenu, NULL, songModes)){ // If the user wants us to show the volume
			float songVolume = MemHelpers::GetCurrentMusicVolume();

			MemHelpers::DX9DrawText("Current Song Volume: " + std::to_string((int)songVolume), whiteText, (MemHelpers::GetWindowSize()[0] / 96), (MemHelpers::GetWindowSize()[1] / 72), FW_NORMAL, (MemHelpers::GetWindowSize()[0] / 54.85), (MemHelpers::GetWindowSize()[1] / 30.85), (MemHelpers::GetWindowSize()[0] / 14.22), (MemHelpers::GetWindowSize()[1] / 8), pDevice);
		}

		if (showSongTimerOnScreen && MemHelpers::ShowSongTimer() != "") {
			std::string currentSongTimeString = MemHelpers::ShowSongTimer();
			size_t stringSize;

			int currentSongTime = std::stoi(currentSongTimeString, &stringSize); // We don't need to tell them the EXACT microsecond it is, just a second is fine.
			int seconds = currentSongTime, minutes = 0, hours = 0; // Can't leave them uninitialized or the (minutes >= 60) will freak out and throw a warning.

			if (currentSongTime / 60 > 0) { // Handles Seconds -> Minutes conversion
				minutes = currentSongTime / 60;
				seconds = currentSongTime - (minutes * 60);
			}

			if (minutes >= 60) { // Handles Minutes -> Hours conversion
				hours++;
				minutes = 0;
			}

			MemHelpers::DX9DrawText(std::to_string(hours) + ":" + std::to_string(minutes) + ":" + std::to_string(seconds), whiteText, (MemHelpers::GetWindowSize()[0] / 96), (MemHelpers::GetWindowSize()[1] / 72), FW_NORMAL, (MemHelpers::GetWindowSize()[0] / 1.07), (MemHelpers::GetWindowSize()[1] / 30.85), (MemHelpers::GetWindowSize()[0] / 1.13), (MemHelpers::GetWindowSize()[1] / 8), pDevice);
		}
	}


	return hRet;
}

HRESULT APIENTRY Hook_Reset(IDirect3DDevice9* pDevice, D3DPRESENT_PARAMETERS* pPresentationParameters) { // Gotta do this so that ALT TAB-ing out of the game doesn't mess the whole thing up
	ImGui_ImplDX9_InvalidateDeviceObjects();

	MemHelpers::DX9DrawText("ERROR: Resetting Text (Lost)", 0xFF000000, 20, 15, FW_HEAVY, 0, 150, 300, 450, pDevice, 1);

	HRESULT ResetReturn = oReset(pDevice, pPresentationParameters);

	ImGui_ImplDX9_CreateDeviceObjects();

	MemHelpers::DX9DrawText("ERROR: Resetting Text (Reset)", 0xFF000000, 20, 15, FW_HEAVY, 0, 150, 300, 450, pDevice, 2);

	return ResetReturn;
}

unsigned WINAPI TimerThread(void*) { // This is likely a waste of resoruces, but we don't really need it to be super precise nor thread safe, so yeah
	while (!GameClosing) {
		currentRandTexture++;
		currentRandTexture %= randomTextureCount;

		Sleep(StringChangeInterval);
	}

	return 0;
}

HRESULT APIENTRY Hook_DP(IDirect3DDevice9* pDevice, D3DPRIMITIVETYPE PrimType, UINT StartIndex, UINT PrimCount) {
	if (pDevice->GetStreamSource(0, &Stream_Data, &Offset, &Stride) == D3D_OK)
		Stream_Data->Release();

	if (Settings::ReturnSettingValue("ExtendedRangeEnabled") == "on" && Stride == 12) { //Stride 12 = tails
		MemHelpers::ToggleCB(MemHelpers::IsExtendedRangeSong());
		pDevice->SetTexture(1, ourTexture); //For random textures, use randomTextures[currentRandTexture]
	}

	return oDrawPrimitive(pDevice, PrimType, StartIndex, PrimCount);
}

HRESULT APIENTRY Hook_DIP(IDirect3DDevice9* pDevice, D3DPRIMITIVETYPE PrimType, INT BaseVertexIndex, UINT MinVertexIndex, UINT NumVertices, UINT StartIndex, UINT PrimCount) {
	static bool calculatedCRC = false, calculatedHeadstocks = false, calculatedSkyline = false;

	if (pDevice->GetStreamSource(0, &Stream_Data, &Offset, &Stride) == D3D_OK)
		Stream_Data->Release();

	// This could potentially lead to game locking up (because DIP is called multiple times per frame) if that value is not filled, but generally it should work 
	if (Settings::ReturnSettingValue("ExtendedRangeEnabled").length() < 2) { // Due to some weird reasons, sometimes settings decide to go missing - this may solve the problem
		Settings::UpdateSettings();
		GenerateTextures(pDevice, Custom);
		std::cout << "Reloaded settings" << std::endl;
	}

	if (setAllToNoteGradientTexture) {
		pDevice->SetTexture(currStride, gradientTextureSeven);
		return oDrawIndexedPrimitive(pDevice, PrimType, BaseVertexIndex, MinVertexIndex, NumVertices, StartIndex, PrimCount);
	}

	Mesh current(Stride, PrimCount, NumVertices);
	ThiccMesh currentThicc(Stride, PrimCount, NumVertices, StartIndex, StartRegister, PrimType, decl->Type, VectorCount, NumElements);

	if (debug) {
		if (GetAsyncKeyState(VK_PRIOR) & 1 && currIdx < std::size(allMeshes) - 1)// Page up
			currIdx++;
		if (GetAsyncKeyState(VK_NEXT) & 1 && currIdx > 0) // Page down
			currIdx--;

		if (GetAsyncKeyState(VK_END) & 1) // Toggle logging
			startLogging = !startLogging;

		if (GetAsyncKeyState(VK_F8) & 1) { // Save logged meshes to file
			for (auto mesh : allMeshes) {
				//Log(mesh.ToString().c_str());
			}
		}

		if (GetAsyncKeyState(VK_F7) & 1) { // Save only removed 
			for (auto mesh : removedMeshes) {
				//Log(mesh.ToString().c_str());
			}
		}

		if (GetAsyncKeyState(VK_CONTROL) & 1)
			//Log("{ %d, %d, %d, %d, %d, %d, %d, %d, %d }, ", Stride, PrimCount, NumVertices, StartIndex, StartRegister, PrimType, decl->Type, VectorCount, NumElements);

			if (startLogging) {
				if (std::find(allMeshes.begin(), allMeshes.end(), currentThicc) == allMeshes.end()) // Make sure we don't log what we'd already logged
					allMeshes.push_back(currentThicc);
				//Log("{ %d, %d, %d},", Stride, PrimCount, NumVertices); // Log Current Texture -> Mesh
				//Log("{ %d, %d, %d, %d, %d, %d, %d, %d, %d }, ", Stride, PrimCount, NumVertices, StartIndex, StartRegister, PrimType, decl->Type, VectorCount, NumElements); // Log Current Texture -> ThiccMesh
				//Log("%s", currentMenu.c_str()); // Log Current Menu
			}

		if (std::size(allMeshes) > 0 && allMeshes.at(currIdx) == currentThicc) {
			currStride = Stride;
			currNumVertices = NumVertices;
			currPrimCount = PrimCount;
			currStartIndex = StartIndex;
			currStartRegister = StartRegister;
			currPrimType = PrimType;
			currDeclType = decl->Type;
			currVectorCount = VectorCount;
			currNumElements = NumElements;
			//pDevice->SetTexture(1, Yellow);
			return D3D_OK;
		}

		if (IsExtraRemoved(removedMeshes, currentThicc))
			return D3D_OK;
	}

	// Mods

	if (Settings::ReturnSettingValue("DiscoModeEnabled") == "on") {
		// Need Lovro's Help With This :(
		//if (DiscoModeInitialSetting.find(pDevice) == DiscoModeInitialSetting.end()) { // We haven't saved this pDevice's initial values yet
		//	DWORD initialAlphaValue = (DWORD)pDevice, initialSeperateValue = (DWORD)pDevice;
		//	pDevice->GetRenderState(D3DRS_ALPHABLENDENABLE, (DWORD*)initialAlphaValue);
		//	pDevice->GetRenderState(D3DRS_SEPARATEALPHABLENDENABLE, (DWORD*)initialSeperateValue);
		//	
		//	DiscoModeInitialSetting.insert({ pDevice, std::make_pair(initialAlphaValue, initialSeperateValue) });
		//}
		//else { // We've seen this pDevice value before.
		//	if (DiscoModeEnabled) { // Key was pressed to have Disco Mode on
		//		pDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE); // Make AMPS Semi-Transparent <- Is the one that makes things glitchy.
		//		pDevice->SetRenderState(D3DRS_SEPARATEALPHABLENDENABLE, TRUE); // Sticky Colors
		//	}

		//	else { // Disco mode was turned off, we need to revert the settings so there is no trace of disco mode.
		//		for (auto pDeviceList : DiscoModeInitialSetting) {
		//			pDeviceList.first->SetRenderState(D3DRS_ALPHABLENDENABLE, *(DWORD*)pDeviceList.second.first); // Needs to have *(DWORD*) since it only sets DWORD not DWORD*
		//			pDeviceList.first->SetRenderState(D3DRS_SEPARATEALPHABLENDENABLE, *(DWORD*)pDeviceList.second.second); // Needs to have *(DWORD*) since it only sets DWORD not DWORD*
		//		}
		//	}
		//}
	}

	if (Settings::ReturnSettingValue("ExtendedRangeEnabled") == "on" && MemHelpers::IsExtendedRangeSong() || Settings::GetModSetting("CustomStringColors") == 2) { // Extended Range Mode
		MemHelpers::ToggleCB(MemHelpers::IsExtendedRangeSong());

		if (IsToBeRemoved(sevenstring, current))  // Change all pieces of note head's textures
			pDevice->SetTexture(1, ourTexture);
		else if (FRETNUM_AND_MISS_INDICATOR) { // Colors for note stems (part below the note), and note accents
			pDevice->GetTexture(1, &pBaseTexture);
			pCurrTexture = (IDirect3DTexture9*)pBaseTexture;

			if (!pBaseTexture)
				return oDrawIndexedPrimitive(pDevice, PrimType, BaseVertexIndex, MinVertexIndex, NumVertices, StartIndex, PrimCount);

			if (CRCForTexture(pCurrTexture, crc)) { // 0x00090000 - fretnums on noteway, 0x005a00b9 - noteway lanes, 0x00035193 -noteway bgd, 0x00004a4a-noteway lanes left and right of chord shape
				//if (startLogging)
				//	Log("0x%08x", crc);

				if (crc == 0x02a50002)  // Same checksum for stems and accents, because they use the same texture
					pDevice->SetTexture(1, ourTexture);
			}

			return oDrawIndexedPrimitive(pDevice, PrimType, BaseVertexIndex, MinVertexIndex, NumVertices, StartIndex, PrimCount);
		}
	}

	if (GreenScreenWall && IsExtraRemoved(greenScreenWallMesh, currentThicc))
		return D3D_OK;


	if (MemHelpers::IsInStringArray(currentMenu, NULL, songModes)) {
		if (Settings::ReturnSettingValue("FretlessModeEnabled") == "on" && IsExtraRemoved(fretless, currentThicc))
			return D3D_OK;
		if (Settings::ReturnSettingValue("RemoveInlaysEnabled") == "on" && IsExtraRemoved(inlays, currentThicc))
			return D3D_OK;
		if (Settings::ReturnSettingValue("RemoveLaneMarkersEnabled") == "on" && IsExtraRemoved(laneMarkers, currentThicc))
			return D3D_OK;
		if (RemoveLyrics && Settings::ReturnSettingValue("RemoveLyrics") == "on" && IsExtraRemoved(lyrics, currentThicc))
			return D3D_OK;
	}

	else if (MemHelpers::IsInStringArray(currentMenu, NULL, tuningMenus) && Settings::ReturnSettingValue("RemoveHeadstockEnabled") == "on" && RemoveHeadstockInThisMenu)
	{
		if (IsExtraRemoved(tuningLetters, currentThicc)) // This is called to remove those pesky tuning letters that share the same texture values as fret numbers and chord fingerings
			return D3D_OK;
		if (IsExtraRemoved(tunerHighlight, currentThicc)) // This is called to remove the tuner's highlights
			return D3D_OK;
		if (IsExtraRemoved(leftyFix, currentThicc)) // Lefties need their own little place in life...
			return D3D_OK;
	}

	if (toggleSkyline && Stride == 16) {
		if (DrawSkylineInMenu) { // If the user is in "Song" mode for Toggle Skyline and is NOT in a song -> draw the UI
			SkylineOff = false;
			return oDrawIndexedPrimitive(pDevice, PrimType, BaseVertexIndex, MinVertexIndex, NumVertices, StartIndex, PrimCount);
		}

		// Skyline Removal
		pDevice->GetTexture(1, &pBaseTextures[1]);
		pCurrTextures[1] = (IDirect3DTexture9*)pBaseTextures[1];

		if (pBaseTextures[1]) {  // There's only two textures in Stage 1 for meshes with Stride = 16, so we could as well skip CRC calcuation and just check if !pBaseTextures[1] and return D3D_OK directly
			if (CRCForTexture(pCurrTextures[1], crc)) {
				if (crc == 0x65b846aa || crc == 0xbad9e064) { // Purple rectangles + orange line beneath them
					SkylineOff = true;
					return D3D_OK;
				}
			}
		}

		pDevice->GetTexture(0, &pBaseTextures[0]);
		pCurrTextures[0] = (IDirect3DTexture9*)pBaseTextures[0];

		if (pBaseTextures[0]) {
			if (CRCForTexture(pCurrTextures[0], crc)) {
				if (crc == 0xc605fbd2 || crc == 0xff1c61ff) {  // There's a few more of textures used in Stage 0, so doing the same is no-go; Shadow-ish thing in the background + backgrounds of rectangles
					SkylineOff = true;
					return D3D_OK;
				}
			}
		}
	}

	else if (Settings::ReturnSettingValue("RemoveHeadstockEnabled") == "on") {
		if (Stride == 44 || Stride == 56 || Stride == 60 || Stride == 68 || Stride == 76 || Stride == 84) { // If we call GetTexture without any filtering, it causes a lockup when ALT-TAB-ing/changing fullscreen to windowed and vice versa

			if (!RemoveHeadstockInThisMenu) // This user has RemoveHeadstock only on during the song. So if we aren't in the song, we need to draw the headstock texture.
				return oDrawIndexedPrimitive(pDevice, PrimType, BaseVertexIndex, MinVertexIndex, NumVertices, StartIndex, PrimCount);

			pDevice->GetTexture(1, &pBaseTextures[1]);
			pCurrTextures[1] = (IDirect3DTexture9*)pBaseTextures[1];

			if (resetHeadstockCache && IsExtraRemoved(headstockThicc, currentThicc)) {
				if (!pBaseTextures[1]) //if there's no texture for Stage 1
					return D3D_OK;

				if (CRCForTexture(pCurrTextures[1], crc)) {
					if (crc == 0x008d5439 || crc == 0x000d4439 || crc == 0x00000000 || crc == 0xa55470f6 || crc == 0x008f4039)
						AddToTextureList(headstockTexturePointers, pCurrTextures[1]);
				}

				//Log("0x%08x", crc);

				if (headstockTexturePointers.size() == 3) {
					calculatedHeadstocks = true;
					resetHeadstockCache = false;
					std::cout << "Calculated headstock CRCs (Menu: " << currentMenu << " )" << std::endl;
				}

				return D3D_OK;
			}

			if (calculatedHeadstocks)
				if (std::find(std::begin(headstockTexturePointers), std::end(headstockTexturePointers), pCurrTextures[1]) != std::end(headstockTexturePointers))
					return D3D_OK;
		}
	}

	return oDrawIndexedPrimitive(pDevice, PrimType, BaseVertexIndex, MinVertexIndex, NumVertices, StartIndex, PrimCount); // KEEP THIS LINE. It means for it to display the graphics!
}

HRESULT APIENTRY Hook_SetVertexDeclaration(LPDIRECT3DDEVICE9 pDevice, IDirect3DVertexDeclaration9* pdecl) {
	if (pdecl != NULL)
		pdecl->GetDeclaration(decl, &NumElements);

	return oSetVertexDeclaration(pDevice, pdecl);
}

HRESULT APIENTRY Hook_SetVertexShaderConstantF(LPDIRECT3DDEVICE9 pDevice, UINT StartRegister, const float* pConstantData, UINT Vector4fCount) {
	if (pConstantData != NULL) {
		StartRegister = StartRegister;
		VectorCount = Vector4fCount;
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

	if (Stride == 32 && NumElements == 8 && VectorCount == 4 && decl->Type == 2) { // Remove Line Markers
		pStreamData->GetDesc(&desc);
		vertexBufferSize = desc.Size;
	}

	return oSetStreamSource(pDevice, StreamNumber, pStreamData, OffsetInBytes, Stride);
}

void GUI() {
	uint32_t d3d9Base, adr, * vTable = NULL;
	while ((d3d9Base = (uint32_t)GetModuleHandleA("d3d9.dll")) == NULL) //aight ffio ;)
		Sleep(500);

	adr = MemUtil::FindPattern<uint32_t>(d3d9Base, Offsets::d3dDevice_SearchLen, (PBYTE)Offsets::d3dDevice_Pattern, Offsets::d3dDevice_Mask) + 2; //and that's it... (:

	if (!adr) {
		std::cout << "Could not find D3D9 device pointer" << std::endl;
		return;
	}

	if (!*(uint32_t*)adr) { // Wing it
		MessageBoxA(NULL, "Could not find DX9 device, please restart the game!", "Error", NULL);
		return;
	}

	vTable = *(uint32_t**)adr;
	if (!vTable || vTable < (uint32_t*)Offsets::baseHandle) {
		MessageBoxA(NULL, "Could not find D3D device's vTable address \n Restart the game and if you still get this error after a few tries, please report the error!", "Error", NULL);
		return;
	}

	oSetVertexDeclaration = (tSetVertexDeclaration)MemUtil::TrampHook((PBYTE)vTable[D3DInfo::SetVertexDeclaration_Index], (PBYTE)Hook_SetVertexDeclaration, 7);
	oSetVertexShader = (tSetVertexShader)MemUtil::TrampHook((PBYTE)vTable[D3DInfo::SetVertexShader_Index], (PBYTE)Hook_SetVertexShader, 7);
	oSetVertexShaderConstantF = (tSetVertexShaderConstantF)MemUtil::TrampHook((PBYTE)vTable[D3DInfo::SetVertexShaderConstantF_Index], (PBYTE)Hook_SetVertexShaderConstantF, 7);
	oSetPixelShader = (tSetPixelShader)MemUtil::TrampHook((PBYTE)vTable[D3DInfo::SetPixelShader_Index], (PBYTE)Hook_SetPixelShader, 7);
	oSetStreamSource = (tSetStreamSource)MemUtil::TrampHook((PBYTE)vTable[D3DInfo::SetStreamSource_Index], (PBYTE)Hook_SetStreamSource, 7);

	oReset = (tReset)DetourFunction((PBYTE)vTable[D3DInfo::Reset_Index], (PBYTE)Hook_Reset);
	//oReset = (tReset)MemUtil::TrampHook((PBYTE)vTable[D3DInfo::Reset_Index], (PBYTE)Hook_Reset, 5); // You'd expect this to work, given the effect is extremely similar to what DetourFunction, but... it crashes instead
	oEndScene = (tEndScene)MemUtil::TrampHook((PBYTE)vTable[D3DInfo::EndScene_Index], (PBYTE)Hook_EndScene, 7);
	oDrawIndexedPrimitive = (tDrawIndexedPrimitive)MemUtil::TrampHook((PBYTE)vTable[D3DInfo::DrawIndexedPrimitive_Index], (PBYTE)Hook_DIP, 5);
	oDrawPrimitive = (tDrawPrimitive)MemUtil::TrampHook((PBYTE)vTable[D3DInfo::DrawPrimitive_Index], (PBYTE)Hook_DP, 7);
}

void InitEngineFunctions() {
	SetRTPCValue = (tSetRTPCValue)Offsets::func_SetRTPCValue;
	GetRTPCValue = (tGetRTPCValue)Offsets::func_GetRTPCValue;
}

void AutoEnterGame() {	//very big brain || "Fork in the toaster"
	PostMessage(FindWindow(NULL, L"Rocksmith 2014"), WM_KEYDOWN, VK_RETURN, 0);
	Sleep(30);
	PostMessage(FindWindow(NULL, L"Rocksmith 2014"), WM_KEYUP, VK_RETURN, 0);
}

void UpdateSettings() { // Live updates from the INI
	Settings::UpdateSettings();
	Sleep(500);
	CustomSongTitles::LoadSettings();
	Sleep(500);
	CustomSongTitles::HookSongListsKoko();
	Sleep(500);
}

void ClearLogs() { // Not taken from here: https://stackoverflow.com/questions/6935279/delete-all-txt-in-a-directory-with-c :P
	if (debug) {
		std::string command = "del /Q ";
		std::string path = "*.mdmp";
		system(command.append(path).c_str());

		std::cout << "Deleting Useless Log Files" << std::endl;
	}
}

unsigned WINAPI MainThread(void*) {
	std::ifstream RSModsFileInput("RSMods.ini"); // Check if this file exists
	if (!RSModsFileInput) {
		std::ofstream RSModsFileOutput("RSMods.ini"); // If we don't call this, the game will crash for some reason :(
		RSModsFileOutput.close();
	}

	Offsets::Initialize();
	MemHelpers::PatchCDLCCheck();
	Settings::Initialize();

	UpdateSettings();

	ERMode::Initialize();

	GUI();

	ClearLogs(); // Delete's those stupid log files Rocksmith loves making.

	//GuitarSpeak.DrawTunerInGame();
	while (!GameClosing) {
		Sleep(250); // We don't need to call these settings always, we just want it to run every 1/4 of a second so the user doesn't notice it.
		
		if (GameLoaded) {// If Game Is Loaded (No need to run these while the game is loading.)
			currentMenu = MemHelpers::GetCurrentMenu(false); // This loads without checking if memory is safe... This can cause crashes if used else where.
			
			//std::cout << currentMenu << std::endl;

			if (MemHelpers::IsInStringArray(currentMenu, NULL, lessonModes)) // Is User In A Lesson
				LessonMode = true;
			else
				LessonMode = false;

			if (Settings::ReturnSettingValue("RemoveHeadstockEnabled") == "on" && Settings::ReturnSettingValue("RemoveHeadstockWhen") == "startup")
				RemoveHeadstockInThisMenu = true; // In this case, the user always wants to remove the headstock. This value should never turn to false in this mode.

			if (LessonMode && Settings::ReturnSettingValue("ToggleLoftEnabled") == "on" && Settings::ReturnSettingValue("ToggleLoftWhen") != "manual") { // Is User In A Lesson Mode AND set to turn loft off
				if (LoftOff)
					MemHelpers::ToggleLoft();
				LoftOff = false;
				GreenScreenWall = true;
			}

			if (!LoftOff && !LessonMode && Settings::ReturnSettingValue("ToggleLoftEnabled") == "on" && Settings::ReturnSettingValue("ToggleLoftWhen") == "startup") { // Turn the loft off on startup
				MemHelpers::ToggleLoft();
				LoftOff = true;
				GreenScreenWall = false;
			}
			if (!SkylineOff && Settings::ReturnSettingValue("RemoveSkylineEnabled") == "on" && Settings::ReturnSettingValue("ToggleSkylineWhen") == "startup") // Turn the skyline off on startup
				toggleSkyline = true;

			if (!RemoveLyrics && Settings::ReturnSettingValue("RemoveLyricsWhen") == "startup")
				RemoveLyrics = true;

			if (MemHelpers::IsInStringArray(currentMenu, NULL, songModes)) { // If User Is Entering Song
				GuitarSpeakPresent = false;

				if (Settings::ReturnSettingValue("RemoveHeadstockEnabled") == "on" && Settings::ReturnSettingValue("RemoveHeadstockWhen") == "song")
					RemoveHeadstockInThisMenu = true;

				if (Settings::ReturnSettingValue("ToggleLoftEnabled") == "on" && Settings::ReturnSettingValue("ToggleLoftWhen") == "song") { // Turn the loft off when entering a song
					if (!LoftOff)
						MemHelpers::ToggleLoft();
					LoftOff = true;
				}

				if (Settings::ReturnSettingValue("RemoveSkylineEnabled") == "on" && Settings::ReturnSettingValue("ToggleSkylineWhen") == "song") { // Turn the skyline off when entering a song
					if (!SkylineOff)
						toggleSkyline = true;
					DrawSkylineInMenu = false;
				}
			}
			else { // If User Is Exiting Song / In A Menu
				if (Settings::ReturnSettingValue("RemoveHeadstockEnabled") == "on" && Settings::ReturnSettingValue("RemoveHeadstockWhen") == "song")
					RemoveHeadstockInThisMenu = false;

				if (Settings::ReturnSettingValue("ToggleLoftEnabled") == "on" && Settings::ReturnSettingValue("ToggleLoftWhen") == "song") { // Turn the loft back on after exiting a song
					if (LoftOff) {
						MemHelpers::ToggleLoft();
						LoftOff = false;
					}
					if (!LessonMode)
						GreenScreenWall = false;
				}
				if (SkylineOff && Settings::ReturnSettingValue("RemoveSkylineEnabled") == "on" && Settings::ReturnSettingValue("ToggleSkylineWhen") == "song") { // Turn the skyline back on after exiting a song
					toggleSkyline = true;
					DrawSkylineInMenu = true;
				}

				if (!GuitarSpeakPresent && Settings::ReturnSettingValue("GuitarSpeak") == "on") { // Guitar Speak
					GuitarSpeakPresent = true;
					if (!GuitarSpeak::TimerTick()) // If we are in a menu where we don't want to read bad values
						GuitarSpeakPresent = false;
				}

				//UpdateSettings(); // A little flaky right now, it likes to crash after a couple setting changes. | Disco mode skyline modification doesn't change back?

				if (Settings::ReturnSettingValue("RemoveHeadstockEnabled") == "on" && (!(MemHelpers::IsInStringArray(currentMenu, NULL, tuningMenus)) || currentMenu == "MissionMenu")) // Can we reset the headstock cache without the user noticing?
					resetHeadstockCache = true;
			}

			if (previousMenu != currentMenu && MemHelpers::IsInStringArray(currentMenu, NULL, tuningMenus)) { // If the current menu is not the same as the previous menu and if it's one of menus where you tune your guitar (i.e. headstock is shown), reset the cache because user may want to change the headstock style
				resetHeadstockCache = true;
				headstockTexturePointers.clear();
			}
			previousMenu = currentMenu;

			if (enableColorBlindCheckboxGUI)
				MemHelpers::ToggleCB(cbEnabled);

			if (ERMode::IsRainbowEnabled())
				ERMode::DoRainbow();
			else
				ERMode::Toggle7StringMode();
		}
		else { // Game Hasn't Loaded Yet
			currentMenu = MemHelpers::GetCurrentMenu(true); // This is the safe version of checking the current menu. It is only used while the game boots to help performance.

			if (currentMenu == "MainMenu") { // Yay We Loaded :P
				GameLoaded = true;
				InitEngineFunctions(); // Anti-crash or not, let's try atleast
			}


			if (Settings::ReturnSettingValue("ForceProfileEnabled") == "on" && !(MemHelpers::IsInStringArray(currentMenu, NULL, dontAutoEnter))) // "Fork in the toaster" / Spam Enter Method
				AutoEnterGame();
		}
	}
	return 0;
}

void Initialize(void) {
	_beginthreadex(NULL, 0, &MainThread, NULL, 0, 0);
	_beginthreadex(NULL, 0, &EnumerationThread, NULL, 0, 0);
	_beginthreadex(NULL, 0, &TimerThread, NULL, 0, 0);
}

BOOL APIENTRY DllMain(HMODULE hModule, uint32_t dwReason, LPVOID lpReserved) {
	switch (dwReason) {
	case DLL_PROCESS_ATTACH:
		if (debug) {
			AllocConsole();
			freopen("CONIN$", "r", stdin);
			freopen("CONOUT$", "w", stdout);
		}
		DisableThreadLibraryCalls(hModule);

		D3DX9_42::InitProxy();
		Initialize();
		return TRUE;
	case DLL_PROCESS_DETACH:
		D3DX9_42::ShutdownProxy();
		ImGui_ImplWin32_Shutdown();
		ImGui_ImplDX9_Shutdown();
		ImGui::DestroyContext();
		return TRUE;
	}
	return TRUE;
}
