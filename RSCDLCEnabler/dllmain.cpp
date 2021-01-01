#include "Main.hpp"

#ifdef _DEBUG
bool debug = true;
#else
bool debug = false;
#endif

unsigned WINAPI EnumerationThread() {
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

unsigned WINAPI MidiThread() {
	while (!D3DHooks::GameClosing) {
		if (Midi::sendPC)
			Midi::SendProgramChange(Midi::dataToSendPC);
		if (Midi::sendCC)
			Midi::SendControlChange(Midi::dataToSendCC);
		Sleep(Midi::sleepFor); // Sleep for 1/33rd of a second so we don't drain resources.
	}
	
	return 0;
}


LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM keyPressed, LPARAM lParam) {
	// Failsafe for mouse input falling through the menu
	//if (D3DHooks::menuEnabled && (msg == WM_LBUTTONDOWN || msg == WM_LBUTTONUP || msg == WM_RBUTTONDOWN || msg == WM_RBUTTONUP || msg == WM_MBUTTONDOWN || msg == WM_MBUTTONUP || msg == WM_MOUSEWHEEL || msg == WM_MOUSEMOVE))
	//	return false;

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
			else if (keyPressed == Settings::GetKeyBind("MasterVolumeKey") && Settings::ReturnSettingValue("VolumeControlEnabled") == "on") {
				if ((GetKeyState(VK_CONTROL) & 0x8000)) // Is Control Pressed
					VolumeControl::DecreaseVolume(Settings::GetModSetting("VolumeControlInterval"), "Master_Volume");
				else
					VolumeControl::IncreaseVolume(Settings::GetModSetting("VolumeControlInterval"), "Master_Volume");
			}
			else if (keyPressed == Settings::GetKeyBind("SongVolumeKey") && Settings::ReturnSettingValue("VolumeControlEnabled") == "on") {
				if ((GetKeyState(VK_CONTROL) & 0x8000)) // Is Control Pressed
					VolumeControl::DecreaseVolume(Settings::GetModSetting("VolumeControlInterval"), "Mixer_Music");
				else
					VolumeControl::IncreaseVolume(Settings::GetModSetting("VolumeControlInterval"), "Mixer_Music");
			}
			else if (keyPressed == Settings::GetKeyBind("Player1VolumeKey") && Settings::ReturnSettingValue("VolumeControlEnabled") == "on") {
				if ((GetKeyState(VK_CONTROL) & 0x8000)) // Is Control Pressed
					VolumeControl::DecreaseVolume(Settings::GetModSetting("VolumeControlInterval"), "Mixer_Player1");
				else
					VolumeControl::IncreaseVolume(Settings::GetModSetting("VolumeControlInterval"), "Mixer_Player1");
			}
			else if (keyPressed == Settings::GetKeyBind("Player2VolumeKey") && Settings::ReturnSettingValue("VolumeControlEnabled") == "on") {
				if ((GetKeyState(VK_CONTROL) & 0x8000)) // Is Control Pressed
					VolumeControl::DecreaseVolume(Settings::GetModSetting("VolumeControlInterval"), "Player2VolumeKey");
				else
					VolumeControl::IncreaseVolume(Settings::GetModSetting("VolumeControlInterval"), "Player2VolumeKey");
			}
			else if (keyPressed == Settings::GetKeyBind("MicrophoneVolumeKey") && Settings::ReturnSettingValue("VolumeControlEnabled") == "on") {
				if ((GetKeyState(VK_CONTROL) & 0x8000)) // Is Control Pressed
					VolumeControl::DecreaseVolume(Settings::GetModSetting("VolumeControlInterval"), "Mixer_Mic");
				else
					VolumeControl::IncreaseVolume(Settings::GetModSetting("VolumeControlInterval"), "Mixer_Mic");
			}
			else if (keyPressed == Settings::GetKeyBind("VoiceOverVolumeKey") && Settings::ReturnSettingValue("VolumeControlEnabled") == "on") {
				if ((GetKeyState(VK_CONTROL) & 0x8000)) // Is Control Pressed
					VolumeControl::DecreaseVolume(Settings::GetModSetting("VolumeControlInterval"), "Mixer_VO");
				else
					VolumeControl::IncreaseVolume(Settings::GetModSetting("VolumeControlInterval"), "Mixer_VO");
			}
			else if (keyPressed == Settings::GetKeyBind("SFXVolumeKey") && Settings::ReturnSettingValue("VolumeControlEnabled") == "on")
			{
				if ((GetKeyState(VK_CONTROL) & 0x8000)) // Is Control Pressed
					VolumeControl::DecreaseVolume(Settings::GetModSetting("VolumeControlInterval"), "Mixer_SFX");
				else
					VolumeControl::IncreaseVolume(Settings::GetModSetting("VolumeControlInterval"), "Mixer_SFX");
			}
			

			/*else if (keyPressed == VK_F9) { // Disco Mode | Current State: No easy way to toggle it off.
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

			else if (keyPressed == Settings::GetKeyBind("ChangedSelectedVolumeKey") && Settings::ReturnSettingValue("VolumeControlEnabled") == "on") {
				currentVolumeIndex++;

				if (currentVolumeIndex > (mixerInternalNames.size() - 1)) // There are only so many values we know we can edit.
					currentVolumeIndex = 0;
			}

			//else if (keyPressed == VK_F9) // Controller Killswitch | Current State: Kills XInput Controllers (Xbox), but won't kill DirectInput (else)
			//	DisableControllers::DisableControllers();

			//else if (keyPressed == VK_F9) { // Current State: Will restart the sound system, but requires the user to change input methods, and switch back. When the user encounters a tone-bug, the audio will never come back.
			//	std::cout << "Trying to restart Wwise Music Engine" << std::endl;
			//	WwiseVariables::Wwise_Music_Term(); // Kill all sounds
			//	WwiseVariables::Wwise_Music_Init((AkMusicSettings*)0x01384964); // Restart the music

			//	WwiseVariables::Rocksmith_SetAudioInputCallbacks((AkAudioInputPluginExecuteCallbackFunc*)0x01c9f415, (AkAudioInputPluginGetFormatCallbackFunc*)0x01c9f34e, (AkAudioInputPluginGetGainCallbackFunc*)0x01c9f2ae, (int*)0x01c9f31e);

			//	WwiseVariables::Wwise_Sound_PostEvent_Char("w_='", (AkGameObjectID)0x3E8, 0, NULL, NULL, 0, NULL, 0); // Fix event char
			//	WwiseVariables::Wwise_Sound_PostEvent_Char("w_='", (AkGameObjectID)0x3E9, 0, NULL, NULL, 0, NULL, 0);
			//	WwiseVariables::Wwise_Sound_PostEvent_Char("w_='", (AkGameObjectID)0x3EA, 0, NULL, NULL, 0, NULL, 0);
			//	WwiseVariables::Wwise_Sound_PostEvent_Char("w_='", (AkGameObjectID)0x3EB, 0, NULL, NULL, 0, NULL, 0);
			//	WwiseVariables::Wwise_Sound_PostEvent_Char("w_='", (AkGameObjectID)0x3EC, 0, NULL, NULL, 0, NULL, 0);
			//	WwiseVariables::Wwise_Sound_PostEvent_Char("w_='", (AkGameObjectID)0x3ED, 0, NULL, NULL, 0, NULL, 0);
			//	WwiseVariables::Wwise_Sound_PostEvent_Char("w_='", (AkGameObjectID)0x3EE, 0, NULL, NULL, 0, NULL, 0);
			//	WwiseVariables::Wwise_Sound_PostEvent_Char("w_='", (AkGameObjectID)0x3EF, 0, NULL, NULL, 0, NULL, 0);
			//	WwiseVariables::Wwise_Sound_PostEvent_Char("w_='", (AkGameObjectID)0x3F0, 0, NULL, NULL, 0, NULL, 0);
			//	WwiseVariables::Wwise_Sound_PostEvent_Char("w_='", (AkGameObjectID)0x3F1, 0, NULL, NULL, 0, NULL, 0);
			//	WwiseVariables::Wwise_Sound_PostEvent_Char("w_='", (AkGameObjectID)0x3F2, 0, NULL, NULL, 0, NULL, 0);

			//	std::cout << "Restart Complete!" << std::endl;
			//}
			

			else if (keyPressed == Settings::GetKeyBind("RRSpeedKey") && Settings::ReturnSettingValue("RRSpeedAboveOneHundred") == "on" && (MemHelpers::IsInStringArray(D3DHooks::currentMenu, NULL, learnASongModes))) { // Song Speed (RR speed)
				newSongSpeed = MemHelpers::RiffRepeaterSpeed();

				if (GetAsyncKeyState(VK_SHIFT) < 0)
					newSongSpeed -= (float)Settings::GetModSetting("RRSpeedInterval");
				else
					newSongSpeed += (float)Settings::GetModSetting("RRSpeedInterval");
				
				if (newSongSpeed > 200.f)
					newSongSpeed = 200.f;

				if (newSongSpeed < 15.f)
					newSongSpeed = 15.f;

				MemHelpers::RiffRepeaterSpeed(newSongSpeed);
				saveNewRRSpeedToFile = true;
			}

			if (MemHelpers::IsInStringArray(D3DHooks::currentMenu, NULL, tuningMenus) && keyPressed == VK_DELETE)
				Midi::userWantsToUseAutoTuning = true;
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

			if (Contains(currMsg, "update")) {
				if (Contains(currMsg, "all"))
					Settings::UpdateSettings();
				else
					Settings::ParseSettingUpdate(currMsg);
			}
			else if (Contains(currMsg, "TurboSpeed"))
				userWantsRRSpeedEnabled = Contains(currMsg, "enable");
			else if (Contains(currMsg, "enable"))
				effectQueue.push_back(currMsg);
			else if (Contains(currMsg, "Reconnect"))
				CrowdControl::StartServerLoop();
		}
	}

	if (msg == WM_CLOSE)
		D3DHooks::GameClosing = true;
	else {
		ImGuiIO& io = ImGui::GetIO();

		POINT mPos;
		GetCursorPos(&mPos);
		ScreenToClient(hWnd, &mPos);
		ImGui::GetIO().MousePos.x = mPos.x;
		ImGui::GetIO().MousePos.y = mPos.y;

		if (D3DHooks::menuEnabled && ImGui_ImplWin32_WndProcHandler(hWnd, msg, keyPressed, lParam))
			return true;
	}

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
		ImGuiIO& io = ImGui::GetIO();
		ImFont* font = io.Fonts->AddFontFromMemoryCompressedTTF(RobotoFont_data, RobotoFont_size, 20);

		D3DDEVICE_CREATION_PARAMETERS d3dcp;
		pDevice->GetCreationParameters(&d3dcp);
		hThisWnd = d3dcp.hFocusWindow;

		oWndProc = (WNDPROC)SetWindowLongPtr(hThisWnd, GWLP_WNDPROC, (LONG_PTR)WndProc);

		ImGui_ImplWin32_Init(hThisWnd);
		ImGui_ImplDX9_Init(pDevice);
		ImGui::GetIO().ImeWindowHandle = hThisWnd;
		//ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange;
		ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

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
		D3D::GenerateTextures(pDevice, D3D::Noteway);
	}

	ImGui_ImplDX9_NewFrame();
	ImGui_ImplWin32_NewFrame();

	ImGui::NewFrame();

	if (D3DHooks::menuEnabled) {
		ImGui::Begin("RS Modz");

		// Stop mouse and keyboard in game
		ImGui::CaptureKeyboardFromApp(true);
		ImGui::CaptureMouseFromApp(true);

		static std::string previewValue = "Select a device";
		if (ImGui::BeginCombo("MIDI devices", previewValue.c_str())) {
			for (int n = 0; n < Midi::NumberOfPorts; n++)
			{
				const bool is_selected = (selectedDevice == n);
				if (ImGui::Selectable(Midi::midiOutDevices[n].szPname, is_selected, ImGuiSelectableFlags_::ImGuiSelectableFlags_DontClosePopups))
					selectedDevice = n;

				if (is_selected) {
					previewValue = Midi::midiOutDevices[n].szPname;
					Midi::SelectedMidiDevice = n;
				}
			}
			ImGui::EndCombo();
		}

		// Sliders
		ImGui::SliderInt("Program Change", &Midi::MidiPC, 0, 127);
		ImGui::SliderInt("Control Change", &Midi::MidiCC, 0, 127);

		// Submit Buttons
		if (ImGui::Button("Send PC MIDI Message"))
			Midi::SendDataToThread_PC(Midi::MidiPC);

		if (ImGui::Button("Send CC MIDI Message"))
			Midi::SendDataToThread_CC(Midi::MidiCC);

		/* STRING COLOR TESTING

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

		*/

		ImGui::End();
	}

	ImGui::EndFrame();
	ImGui::Render();

	ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());

	// Restart mouse and keyboard in game
	ImGui::CaptureKeyboardFromApp(false);
	ImGui::CaptureMouseFromApp(false);

	if (generateTexture) {
		D3D::GenerateTextures(pDevice, D3D::Custom);
		generateTexture = false;
	}

	int whiteText = 0xFFFFFFFF;
	Resolution WindowSize = MemHelpers::GetWindowSize();

	if (D3DHooks::GameLoaded) { // Draw text on screen || NOTE: NEVER USE SET VALUES. Always do division of WindowSize X AND Y so every resolution should have the text in around the same spot.

		if (Settings::ReturnSettingValue("VolumeControlEnabled") == "on" && MemHelpers::IsInStringArray(D3DHooks::currentMenu, NULL, songModes)) { // If the user wants us to show the volume )
			float volume = 0;
			RTPCValue_type type = RTPCValue_GameObject;
			WwiseVariables::Wwise_Sound_Query_GetRTPCValue_Char(mixerInternalNames[currentVolumeIndex].c_str(), 0xffffffff, &volume, &type);

			if (currentVolumeIndex != 0)
				MemHelpers::DX9DrawText(drawMixerTextName[currentVolumeIndex] + std::to_string((int)volume) + "%", whiteText, (int)(WindowSize.width / 54.85), (int)(WindowSize.height / 30.85), (int)(WindowSize.width / 14.22), (int)(WindowSize.height / 8), pDevice);
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

			MemHelpers::DX9DrawText(std::to_string(hours) + "h:" + std::to_string(minutes) + "m:" + std::to_string(seconds) + "s", whiteText, (int)(WindowSize.width / 1.35), (int)(WindowSize.height / 30.85), (int)(WindowSize.width / 1.45), (int)(WindowSize.height / 8), pDevice);
		}

		if (Settings::ReturnSettingValue("RRSpeedAboveOneHundred") == "on" && MemHelpers::IsInStringArray(currentMenu, NULL, fastRRModes)) {
			MemHelpers::RiffRepeaterSpeed(newSongSpeed);
			if (useNewSongSpeed)
				MemHelpers::DX9DrawText("Riff Repeater Speed: " + std::to_string((int)MemHelpers::RiffRepeaterSpeed()) + "%", whiteText, (int)(WindowSize.width / 2.35), (int)(WindowSize.height / 30.85), (int)(WindowSize.width / 2.50), (int)(WindowSize.height / 8), pDevice);
		}

		if (Settings::ReturnSettingValue("ShowCurrentNoteOnScreen") == "on" && GuitarSpeak::GetCurrentNoteName() != (std::string)"") {
			if (MemHelpers::IsInStringArray(currentMenu, NULL, songModes))
				MemHelpers::DX9DrawText(GuitarSpeak::GetCurrentNoteName(), whiteText, (int)(WindowSize.width / 5.5), (int)(WindowSize.height / 1.75), (int)(WindowSize.width / 5.75), (int)(WindowSize.height / 8), pDevice);
			else
				MemHelpers::DX9DrawText("Current Note: " + GuitarSpeak::GetCurrentNoteName(), whiteText, (int)(WindowSize.width / 3.87), (int)(WindowSize.height / 30.85), (int)(WindowSize.width / 4), (int)(WindowSize.height / 8), pDevice);
		}
			

		if (D3DHooks::regenerateUserDefinedTexture) {
			Color userDefColor = Settings::ConvertHexToColor(Settings::ReturnSettingValue("SolidNoteColor"));
			//unsigned int red = userDefColor.r * 255, green = userDefColor.g * 255, blue = userDefColor.b * 255;
			//D3D::GenerateSolidTexture(pDevice, &twitchUserDefinedTexture, D3DCOLOR_ARGB(255, red, green, blue));

			ColorList customColorList(16, userDefColor); // I feel like the gradient color looks a bit better
			D3D::GenerateTexture(pDevice, &twitchUserDefinedTexture, customColorList);

			ERMode::customSolidColor.clear();
			for (int str = 0; str < 6;str++)
				ERMode::customSolidColor.push_back(userDefColor);

			D3DHooks::regenerateUserDefinedTexture = false;
		}
	}
	return hRet;
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
				D3DHooks::regenerateUserDefinedTexture = true; //Smh... that's why namespaces are important my children
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

unsigned WINAPI HandleEffectQueueThread() { // TODO: This is fairly crude, so if it takes a while to get CC in place, improve synchronization of this (cond_variables, etc.)
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

void TakeScreenshot() { // Simple Screenshot function (requires F12 to be the screenshot key | F12 is default for Steam Screenshots)
	if (!takenScreenshotOfThisScreen) {
		takenScreenshotOfThisScreen = true;
		Sleep(8000); // The menu title changes while the animation is running so we are giving it so time to show the actual results. (8 seconds)
		PostMessage(FindWindow(NULL, L"Rocksmith 2014"), WM_KEYDOWN, VK_F12, 0);
		std::cout << "Took screenshot" << std::endl;
		Sleep(30);
		PostMessage(FindWindow(NULL, L"Rocksmith 2014"), WM_KEYUP, VK_F12, 0);
	}
}

void UpdateSettings() { // Live updates from the INI
	Settings::UpdateSettings();
	Sleep(500);
	CustomSongTitles::LoadSettings();
	Sleep(500);
	CustomSongTitles::HookSongListsKoko();
	Sleep(500);
}

void ClearLogs() {
	if (debug) { 
		bool didWeDeleteFiles = false;

		std::filesystem::path rocksmithFolder = std::filesystem::current_path();
		for (const auto& file : std::filesystem::directory_iterator(rocksmithFolder)) {
			if (file.path().extension().string() == (std::string)".mdmp")
			{
				std::filesystem::remove(file.path());
				didWeDeleteFiles = true;
			}
		}

		if (didWeDeleteFiles)
			std::cout << "Deleting Useless Log Files" << std::endl;
	}
}

unsigned WINAPI StreamerLogThread() {
	while (!D3DHooks::GameClosing) {
		if (Settings::ReturnSettingValue("RRSpeedAboveOneHundred") == "on" && MemHelpers::IsInStringArray(D3DHooks::currentMenu, NULL, fastRRModes) && saveNewRRSpeedToFile) {
			std::ofstream rrText = std::ofstream("riff_repeater_speed.txt", std::ofstream::out);
			rrText << std::to_string((int)newSongSpeed) << std::endl;
			saveNewRRSpeedToFile = false;
		}
		Sleep(50);
	}

	return 0;
}

unsigned WINAPI MainThread() {
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

	Midi::InitMidi();
	//GuitarSpeak.DrawTunerInGame();

	if (Settings::ReturnSettingValue("RRSpeedAboveOneHundred") == "on")
		userWantsRRSpeedEnabled = true; // Set To True if you want the user / streamer to have RR open every song (for over 100% RR speed)

	using namespace D3DHooks;
	while (!GameClosing) {
		Sleep(250); // We don't need to call these settings always, we just want it to run every 1/4 of a second so the user doesn't notice it.

		if (GameLoaded) {// If Game Is Loaded (No need to run these while the game is loading.)
			currentMenu = MemHelpers::GetCurrentMenu(false); // This loads without checking if memory is safe... This can cause crashes if used else where.

			//std::cout << currentMenu << std::endl;

			if (!Midi::scannedForMidiDevices && Settings::ReturnSettingValue("AutoTuneForSong") == "on") {
				Midi::scannedForMidiDevices = true;
				Midi::ReadMidiSettingsFromINI(Settings::ReturnSettingValue("ChordsMode"), Settings::GetModSetting("TuningPedal"), Settings::ReturnSettingValue("AutoTuneForSongDevice"));
			}

			/// If User Is Entering / In Lesson Mode

			if (MemHelpers::IsInStringArray(currentMenu, NULL, lessonModes))
				LessonMode = true;
			else
				LessonMode = false;

			/// Always on Mods (If the user specifies them to be on)

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


			if (MemHelpers::IsInStringArray(currentMenu, NULL, fastRRModes) && userWantsRRSpeedEnabled && !automatedSongSpeedInThisSong) // This won't work in SA so we need to exclude it.
				MemHelpers::AutomatedOpenRRSpeedAbuse();

			/// If User Is Entering Song

			if (MemHelpers::IsInStringArray(currentMenu, NULL, songModes)) {
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

				if (Settings::ReturnSettingValue("AutoTuneForSong") == "on" && !Midi::alreadyAutomatedTuningInThisSong && Midi::userWantsToUseAutoTuning) {
					Midi::AutomateDownTuning();
					Midi::AutomateTrueTuning();
				}
			}

			/// If User Is Exiting A Song / In A Menu

			else {
				automatedSongSpeedInThisSong = false; // Riff Repeater Speed above 100%
				newSongSpeed = 100.f;

				if (Midi::alreadyAutomatedTuningInThisSong) {
					Midi::RevertAutomatedTuning();
					Midi::userWantsToUseAutoTuning = false;
				}	

				if (Settings::ReturnSettingValue("RemoveHeadstockEnabled") == "on" && Settings::ReturnSettingValue("RemoveHeadstockWhen") == "song") // If the user only wants to see the headstock in menus, then we need to stop removing it.
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
					if (!GuitarSpeak::RunGuitarSpeak()) // If we are in a menu where we don't want to read bad values
						GuitarSpeakPresent = false;
				}

				//UpdateSettings(); // A little flaky right now, it likes to crash after a couple setting changes. | Disco mode skyline modification doesn't change back?

				if (Settings::ReturnSettingValue("RemoveHeadstockEnabled") == "on" && (!(MemHelpers::IsInStringArray(currentMenu, NULL, tuningMenus)) || currentMenu == "MissionMenu")) // Can we reset the headstock cache without the user noticing?
					resetHeadstockCache = true;
			}
			
			/// "Other" menus. These will normally state what menus they need to be in.

			if (Settings::ReturnSettingValue("ScreenShotScores") == "on" && MemHelpers::IsInStringArray(currentMenu, NULL, scoreScreens)) // Screenshot Scores
				TakeScreenshot();
			else
				takenScreenshotOfThisScreen = false;

			if (previousMenu != currentMenu && MemHelpers::IsInStringArray(currentMenu, NULL, tuningMenus)) { // If the current menu is not the same as the previous menu and if it's one of menus where you tune your guitar (i.e. headstock is shown), reset the cache because user may want to change the headstock style
				resetHeadstockCache = true;
				headstockTexturePointers.clear();
			}

			previousMenu = currentMenu;

			if (enableColorBlindCheckboxGUI)
				MemHelpers::ToggleCB(cbEnabled);

			if (ERMode::IsRainbowEnabled()) // Rainbow mode / ER Mode
				ERMode::DoRainbow();
			else
				ERMode::Toggle7StringMode();
		}

		/// Game Hasn't Loaded Yet

		else {
			//DisableControllers::DisableControllers();
			currentMenu = MemHelpers::GetCurrentMenu(true); // This is the safe version of checking the current menu. It is only used while the game boots to help performance.

			if (currentMenu == "MainMenu") // Yay We Loaded :P
				GameLoaded = true;

			if (Settings::ReturnSettingValue("ForceProfileEnabled") == "on" && !(MemHelpers::IsInStringArray(currentMenu, NULL, dontAutoEnter))) { // "Fork in the toaster" / Spam Enter Method
				if (Settings::ReturnSettingValue("ProfileToLoad") != (std::string)"" && currentMenu == (std::string)"ProfileSelect") {
					selectedUser = MemHelpers::CurrentSelectedUser();
					if (selectedUser == Settings::ReturnSettingValue("ProfileToLoad")) // The profile we're looking for
						AutoEnterGame();
					else if (selectedUser == (std::string)"New profile") // Yeah, the profile they're looking for doesn't exist :(
						std::cout << "(Auto Load) Invalid Profile Name" << std::endl;
					else { // Not the profile we're looking for
						PostMessage(FindWindow(NULL, L"Rocksmith 2014"), WM_KEYDOWN, VK_DOWN, 0);
						Sleep(30);
						PostMessage(FindWindow(NULL, L"Rocksmith 2014"), WM_KEYUP, VK_DOWN, 0);
					}
				}
				else
					AutoEnterGame();
			}
		}
	}
	return 0;
}

void Initialize() {
	std::thread(MainThread).detach();
	std::thread(EnumerationThread).detach();
	std::thread(HandleEffectQueueThread).detach();
	std::thread(MidiThread).detach();
	std::thread(StreamerLogThread).detach();

	// Probably check ini setting before starting this thing
	CrowdControl::StartServer();
}

BOOL APIENTRY DllMain(HMODULE hModule, uint32_t dwReason, LPVOID lpReserved) { // Do NOT remove the lpReserved parameter. We know it says it's unused, but keep it or the game won't open.
	switch (dwReason) {
	case DLL_PROCESS_ATTACH:
		if (debug) {
			FILE* streamRead, * streamWrite;
			AllocConsole();
			freopen_s(&streamRead, "CONIN$", "r", stdin);
			freopen_s(&streamWrite, "CONOUT$", "w", stdout);
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
