#include "Main.hpp"
#include <thread>

#ifdef _DEBUG
bool debug = true;
#else
bool debug = false;
#endif

unsigned WINAPI EnumerationThread(void*) {
	while (!D3DHooks::GameLoaded) // We are in no hurry :)
		Sleep(5000);

	Settings::ReadKeyBinds();
	Settings::ReadModSettings();

	int oldDLCCount = Enumeration::GetCurrentDLCCount(), newDLCCount = oldDLCCount;

	while (!D3DHooks::GameClosing) {
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
	if (D3DHooks::menuEnabled && ImGui_ImplWin32_WndProcHandler(hWnd, msg, keyPressed, lParam))
		return true;

	if (msg == WM_KEYUP) {
		if (D3DHooks::GameLoaded) { // Game must not be on the startup videos or it will crash
			if (keyPressed == Settings::GetKeyBind("ToggleLoftKey") && Settings::ReturnSettingValue("ToggleLoftEnabled") == "on") {
				MemHelpers::ToggleLoft();
				std::cout << "Toggle Loft" << std::endl;
			}

			else if (keyPressed == Settings::GetKeyBind("ShowSongTimerKey") && Settings::ReturnSettingValue("ShowSongTimerEnabled") == "on") {
				D3DHooks::showSongTimerOnScreen = !D3DHooks::showSongTimerOnScreen;
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
				D3DHooks::RemoveLyrics = !D3DHooks::RemoveLyrics;
				if (D3DHooks::RemoveLyrics)
					std::cout << "No "; // Keep this without a endl so it appears as "No Karaoke For You" when on and "Karaoke For You" when off.
				std::cout << "Karaoke For You" << std::endl;
			}
			else if (keyPressed == 0x41 && (GetKeyState(VK_CONTROL) & 0x8000)) { //CTRL + A
				Settings::UpdateSettings();
				std::cout << "Value: " << Settings::ReturnSettingValue("ExtendedRangeEnabled") << std::endl;
				std::cout << "Reloaded settings" << std::endl;
			}
			else if (keyPressed == Settings::GetKeyBind("AddVolumeKey") && Settings::ReturnSettingValue("AddVolumeEnabled") == "on") {
				float volume = 0;
				RTPCValue_type type = RTPCValue_GameObject;
				WwiseVariables::Wwise_Sound_Query_GetRTPCValue_Char(mixerInternalNames[currentVolumeIndex].c_str(), 0xffffffff, &volume, &type);

				//std::cout << volume << std::endl;

				if (volume <= 95.0f)
					volume += 5.0f;
				else
					volume = 100.0f; // Incase the volume is 96-99 we can't throw it over 100.

				WwiseVariables::Wwise_Sound_SetRTPCValue_Char(mixerInternalNames[currentVolumeIndex].c_str(), (float)volume, 0xffffffff, 0, AkCurveInterpolation_Linear);
				WwiseVariables::Wwise_Sound_SetRTPCValue_Char(mixerInternalNames[currentVolumeIndex].c_str(), (float)volume, 0x00001234, 0, AkCurveInterpolation_Linear);
			}
			else if (keyPressed == Settings::GetKeyBind("DecreaseVolumeKey") && Settings::ReturnSettingValue("DecreaseVolumeEnabled") == "on") {
				float volume = 0;
				RTPCValue_type type = RTPCValue_GameObject;
				WwiseVariables::Wwise_Sound_Query_GetRTPCValue_Char(mixerInternalNames[currentVolumeIndex].c_str(), 0xffffffff, &volume, &type);

				if (volume >= 5.0f)
					volume -= 5.0f;
				else
					volume = 0.0f; // Incase the volume is 1-4 we can't throw it below 0.

				WwiseVariables::Wwise_Sound_SetRTPCValue_Char(mixerInternalNames[currentVolumeIndex].c_str(), (float)volume, 0xffffffff, 0, AkCurveInterpolation_Linear);
				WwiseVariables::Wwise_Sound_SetRTPCValue_Char(mixerInternalNames[currentVolumeIndex].c_str(), (float)volume, 0x00001234, 0, AkCurveInterpolation_Linear);
			}
			/*else if (keyPressed == VK_F9) { // Disco Mode
				DiscoModeEnabled = !DiscoModeEnabled;

				if (debug) {
					std::cout << "Turning ";
					if (!DiscoModeEnabled)
						std::cout << "off";
					else
						std::cout << "on";
					std::cout << " Disco Mode!" << std::endl;
				}
			}*/

			else if (keyPressed == Settings::GetKeyBind("ChangedSelectedVolumeKey") && Settings::ReturnSettingValue("AddVolumeEnabled") == "on") {
				currentVolumeIndex++;

				if (currentVolumeIndex > (mixerInternalNames.size() - 1)) // There are only so many values we know we can edit.
					currentVolumeIndex = 0;
			}

			//else if (keyPressed == VK_F9) {
			//		lua_State* luaState = lua_open();
			//		luaL_openlibs(luaState);

			//		DumpLuaStateToConsole(*(luaStateClass*)Offsets::ptr_luaopen_ecr);

			//		//printTostdCoutExternal(luaState, "print(_VERSION)"); // Prints our version, not the game's (but they are both the same version | 5.1.4)
			//}
		}

		if (debug) {
			if (keyPressed == VK_INSERT)
				D3DHooks::menuEnabled = !D3DHooks::menuEnabled;
		}
	}
	else if (msg == WM_COPYDATA) {
		COPYDATASTRUCT* pcds = (COPYDATASTRUCT*)lParam;
		if (pcds->dwData == 1)
		{
			std::string currMsg = (char*)pcds->lpData;
			std::cout << currMsg << std::endl;

			if (Contains(currMsg, "update"))
				Settings::ParseSettingUpdate(currMsg);
			else if (Contains(currMsg, "enable"))
				effectQueue.push_back(currMsg);
		}
	}

	if (msg == WM_CLOSE)
		D3DHooks::GameClosing = true;

	return CallWindowProc(D3DHooks::oWndProc, hWnd, msg, keyPressed, lParam);
}

HRESULT APIENTRY D3DHooks::Hook_EndScene(IDirect3DDevice9* pDevice) {
	HRESULT hRet = oEndScene(pDevice);
	uint32_t returnAddress = (uint32_t)_ReturnAddress(); // EndScene is called both by the game and by Steam's overlay renderer, and there's no need to draw our stuff twice

	if (returnAddress > Offsets::baseEnd)
		return hRet;

	static bool init = false; // Has this been ran before (AKA run only once, at startup)

	if (!init) {
		init = true;

		ImGui::CreateContext();
		// ImGuiIO& io = ImGui::GetIO();
		// ImFont* font = io.Fonts->AddFontFromMemoryCompressedTTF(RobotoFont_data, RobotoFont_size, 20);

		D3DDEVICE_CREATION_PARAMETERS d3dcp;
		pDevice->GetCreationParameters(&d3dcp);
		hThisWnd = d3dcp.hFocusWindow;

		oWndProc = (WNDPROC)SetWindowLongPtr(hThisWnd, GWLP_WNDPROC, (LONG_PTR)WndProc);

		ImGui_ImplWin32_Init(hThisWnd);
		ImGui_ImplDX9_Init(pDevice);
		ImGui::GetIO().ImeWindowHandle = hThisWnd;
		ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange;

		D3D::GenerateSolidTexture(pDevice, &Red, D3DCOLOR_ARGB(255, 000, 255, 255));
		D3D::GenerateSolidTexture(pDevice, &Green, D3DCOLOR_ARGB(255, 0, 255, 0));
		D3D::GenerateSolidTexture(pDevice, &Blue, D3DCOLOR_ARGB(255, 0, 0, 255));
		D3D::GenerateSolidTexture(pDevice, &Yellow, D3DCOLOR_ARGB(255, 255, 255, 0));

		/* Used before, not necessary anymore because we generate our texture
		D3DXCreateTextureFromFile(pDevice, L"notes_gradient_normal.dds", &gradientTextureNormal); //if those don't exist, note heads will be "invisible" | 6-String Model
		D3DXCreateTextureFromFile(pDevice, L"notes_gradient_seven.dds", &gradientTextureSeven); // 7-String Note Colors
		D3DXCreateTextureFromFile(pDevice, L"gradient_map_additive.dds", &additiveNoteTexture); // Note Stems
		*/
		D3DXCreateTextureFromFile(pDevice, L"nonexistenttexture.dds", &nonexistentTexture); // Black Notes

		std::cout << "ImGUI Init" << std::endl;

		Settings::UpdateSettings();

		D3D::GenerateTextures(pDevice, D3D::Custom);
		D3D::GenerateTextures(pDevice, D3D::Rainbow);
		//GenerateTextures(pDevice, Random);
		D3D::GenerateTextures(pDevice, D3D::Random_Solid);
	}

	ImGui_ImplDX9_NewFrame();
	ImGui_ImplWin32_NewFrame();

	ImGui::NewFrame();

	if (D3DHooks::menuEnabled) {
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
					strR = (int)currColors.r * 255;
					strG = (int)currColors.g * 255;
					strB = (int)currColors.b * 255;
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
		D3D::GenerateTextures(pDevice, D3D::Custom);
		generateTexture = false;
	}

	int whiteText = 0xFFFFFFFF;

	if (D3DHooks::GameLoaded) { // Draw text on screen || NOTE: NEVER USE SET VALUES. ALWAYS DO MemHelpers::GetWindowSize() X AND Y. THIS MEANS IT WILL SHOW IN THE SAME PLACE ON EVERY RESOLUTION!

		if (Settings::ReturnSettingValue("AddVolumeEnabled") == "on" && MemHelpers::IsInStringArray(D3DHooks::currentMenu, NULL, songModes)) { // If the user wants us to show the volume )
			float volume = 0;
			RTPCValue_type type = RTPCValue_GameObject;
			WwiseVariables::Wwise_Sound_Query_GetRTPCValue_Char(mixerInternalNames[currentVolumeIndex].c_str(), 0xffffffff, &volume, &type);

			if (currentVolumeIndex != 0)
				MemHelpers::DX9DrawText(drawMixerTextName[currentVolumeIndex] + std::to_string((int)volume) + "%", whiteText, (int)(MemHelpers::GetWindowSize()[0] / 54.85), (int)(MemHelpers::GetWindowSize()[1] / 30.85), (int)(MemHelpers::GetWindowSize()[0] / 14.22), (int)(MemHelpers::GetWindowSize()[1] / 8), pDevice);
		}

		if (D3DHooks::showSongTimerOnScreen && MemHelpers::ShowSongTimer() != "") {
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

			MemHelpers::DX9DrawText(std::to_string(hours) + "h:" + std::to_string(minutes) + "m:" + std::to_string(seconds) + "s", whiteText, (int)(MemHelpers::GetWindowSize()[0] / 1.35), (int)(MemHelpers::GetWindowSize()[1] / 30.85), (int)(MemHelpers::GetWindowSize()[0] / 1.45), (int)(MemHelpers::GetWindowSize()[1] / 8), pDevice);
		}

		if (regenerateUserDefinedTexture) {
			Color userDefColor = Settings::ConvertHexToColor(Settings::ReturnSettingValue("SolidNoteColor"));
			//unsigned int red = userDefColor.r * 255, green = userDefColor.g * 255, blue = userDefColor.b * 255;
			//D3D::GenerateSolidTexture(pDevice, &twitchUserDefinedTexture, D3DCOLOR_ARGB(255, red, green, blue));

			ColorList customColorList(16, userDefColor); // I feel like the gradient color looks a bit better
			D3D::GenerateTexture(pDevice, &twitchUserDefinedTexture, customColorList);

			ERMode::customSolidColor.clear();
			for (int str = 0; str < 6;str++)
				ERMode::customSolidColor.push_back(userDefColor);

			regenerateUserDefinedTexture = false;
		}
	}


	return hRet;
}

unsigned WINAPI TimerThread(void*) { // This is likely a waste of resoruces, but we don't really need it to be super precise nor thread safe, so yeah
	while (!D3DHooks::GameClosing) {
		currentRandTexture++;
		currentRandTexture %= randomTextureCount;

		Sleep(StringChangeInterval);
	}

	return 0;
}

bool HandleMessage(std::string currMsg, std::string type) {
	if (Contains(currMsg, "RainbowStrings")) {
		if (type == "enable")
			ERMode::RainbowEnabled = true;
		else
			ERMode::RainbowEnabled = false;
	}
	else if (Contains(currMsg, "DrunkMode")) {
		MemHelpers::ToggleDrunkMode(type == "enable");
		Settings::ParseTwitchToggle(currMsg, type);
	}
	else if (Contains(currMsg, "SolidNotes")) {
		if (!ERMode::ColorsSaved) // Don't apply any effects if we haven't even been in a song yet
			return false;

		if (type == "enable") {
			if (Contains(currMsg, "Random")) { // For Random Colors
				static std::uniform_real_distribution<> urd(0, 9);
				currentRandomTexture = (int)urd(rng);

				ERMode::customSolidColor.clear();
				ERMode::customSolidColor.insert(ERMode::customSolidColor.begin(), randomTextureColors[currentRandomTexture].begin(), randomTextureColors[currentRandomTexture].end());

				//if(twitchUserDefinedTexture != NULL) //determine why this crashes if you send multiple in a row
				//	twitchUserDefinedTexture->Release();

				twitchUserDefinedTexture = randomTextures[currentRandomTexture];
			}
			else { // If colors are not random, set colors which the user defined for this reward
				Settings::ParseSolidColorsMessage(currMsg);
				regenerateUserDefinedTexture = true;
			}
		}
		else
			ERMode::ResetAllStrings();
	}

	Settings::ParseTwitchToggle(currMsg, type);
	return true;
}


void HandleEffect(std::string currEffectMsg) {
	auto msgParts = Settings::SplitByWhitespace(currEffectMsg);
	std::string effectName = msgParts[1];

	std::cout << "Entering the thread for: " << currEffectMsg << std::endl;

	while (MemHelpers::IsInStringArray(effectName, 0, enabledEffects) && !MemHelpers::IsInSong())  // Wait until the current effect is not present any more
		Sleep(150);

	std::cout << enabledEffects.size() << std::endl;

	std::cout << "Enabling " << effectName << std::endl;
	if (HandleMessage(currEffectMsg, "enable")) {
		Sleep(std::stoi(msgParts.back()) * 1000); // Last part of the (new) message is duration

		HandleMessage(currEffectMsg, "disable");

		std::cout << "Disabling " << effectName << std::endl;
		if (MemHelpers::IsInStringArray(effectName, 0, enabledEffects)) // JIC
			enabledEffects.erase(std::find(enabledEffects.begin(), enabledEffects.end(), effectName));
	}
}

unsigned WINAPI HandleEffectQueueThread(void*) { // TODO: This is fairly crude, so if it takes a while to get CC in place, improve synchronization of this (cond_variables, etc.)
	while (!D3DHooks::GameClosing) {
		if (effectQueue.size() > 0 && MemHelpers::IsInSong()) {
			// Okay this is getting sketchy, but otherwise one effect would block other effects from running

			for (auto it = effectQueue.begin(); it != effectQueue.end();) {
				std::string effectName = Settings::SplitByWhitespace(*it)[1];

				if (!MemHelpers::IsInStringArray(effectName, 0, enabledEffects)) { // Check whether there's an empty place for our new effect
					enabledEffects.push_back(effectName);

					std::thread effectThrd(HandleEffect, *it); // Send full effect message to the thread
					effectThrd.detach();

					it = effectQueue.erase(it);
				}
				else
					++it;
			}
		}

		Sleep(250);
	}
	return 0;
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

	oSetVertexDeclaration = (tSetVertexDeclaration)MemUtil::TrampHook((PBYTE)vTable[D3DInfo::SetVertexDeclaration_Index], (PBYTE)D3DHooks::Hook_SetVertexDeclaration, 7);
	oSetVertexShader = (tSetVertexShader)MemUtil::TrampHook((PBYTE)vTable[D3DInfo::SetVertexShader_Index], (PBYTE)D3DHooks::Hook_SetVertexShader, 7);
	oSetVertexShaderConstantF = (tSetVertexShaderConstantF)MemUtil::TrampHook((PBYTE)vTable[D3DInfo::SetVertexShaderConstantF_Index], (PBYTE)D3DHooks::Hook_SetVertexShaderConstantF, 7);
	oSetPixelShader = (tSetPixelShader)MemUtil::TrampHook((PBYTE)vTable[D3DInfo::SetPixelShader_Index], (PBYTE)D3DHooks::Hook_SetPixelShader, 7);
	oSetStreamSource = (tSetStreamSource)MemUtil::TrampHook((PBYTE)vTable[D3DInfo::SetStreamSource_Index], (PBYTE)D3DHooks::Hook_SetStreamSource, 7);

	oReset = (tReset)DetourFunction((PBYTE)vTable[D3DInfo::Reset_Index], (PBYTE)D3DHooks::Hook_Reset);
	//oReset = (tReset)MemUtil::TrampHook((PBYTE)vTable[D3DInfo::Reset_Index], (PBYTE)Hook_Reset, 5); // You'd expect this to work, given the effect is extremely similar to what DetourFunction does, but... it crashes instead
	oEndScene = (tEndScene)MemUtil::TrampHook((PBYTE)vTable[D3DInfo::EndScene_Index], (PBYTE)D3DHooks::Hook_EndScene, 7);
	oDrawIndexedPrimitive = (tDrawIndexedPrimitive)MemUtil::TrampHook((PBYTE)vTable[D3DInfo::DrawIndexedPrimitive_Index], (PBYTE)D3DHooks::Hook_DIP, 5);
	oDrawPrimitive = (tDrawPrimitive)MemUtil::TrampHook((PBYTE)vTable[D3DInfo::DrawPrimitive_Index], (PBYTE)D3DHooks::Hook_DP, 7);
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

	D3DHooks::debug = debug;

	Offsets::Initialize();
	MemHelpers::PatchCDLCCheck();
	Settings::Initialize();

	UpdateSettings();

	ERMode::Initialize();

	GUI();

	ClearLogs(); // Delete's those stupid log files Rocksmith loves making.

	//GuitarSpeak.DrawTunerInGame();

	using namespace D3DHooks;
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

			if (currentMenu == "MainMenu") // Yay We Loaded :P
				GameLoaded = true;

			if (Settings::ReturnSettingValue("ForceProfileEnabled") == "on" && !(MemHelpers::IsInStringArray(currentMenu, NULL, dontAutoEnter))) // "Fork in the toaster" / Spam Enter Method
				AutoEnterGame();
		}
	}
	return 0;
}

void Initialize(void) {
	_beginthreadex(NULL, 0, &MainThread, NULL, 0, 0);
	_beginthreadex(NULL, 0, &EnumerationThread, NULL, 0, 0);
	//_beginthreadex(NULL, 0, &TimerThread, NULL, 0, 0);
	_beginthreadex(NULL, 0, &HandleEffectQueueThread, NULL, 0, 0);
}

BOOL APIENTRY DllMain(HMODULE hModule, uint32_t dwReason, LPVOID lpReserved) { // Do NOT remove the lpReserved parameter. We know it says it's unused, but keep it or the game won't open.
	switch (dwReason) {
	case DLL_PROCESS_ATTACH:
		if (debug) {
			FILE* streamRead, * streamWrite;
			errno_t freOpenSecureRead, freOpenSecureWrite;
			AllocConsole();
			freOpenSecureRead = freopen_s(&streamRead, "CONIN$", "r", stdin);
			freOpenSecureWrite = freopen_s(&streamWrite, "CONOUT$", "w", stdout);
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
