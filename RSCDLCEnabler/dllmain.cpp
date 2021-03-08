#include "Main.hpp"

#ifdef _DEBUG
bool debug = true;
#else
bool debug = false;
#endif


/// <summary>
/// Handle Force Enumeration
/// </summary>
/// <returns>NULL. Loops while game is open.</returns>
unsigned WINAPI EnumerationThread() {
	while (!D3DHooks::GameLoaded)
		Sleep(5000);

	Settings::ReadKeyBinds();
	Settings::ReadModSettings();

	int oldDLCCount = Enumeration::GetCurrentDLCCount(), newDLCCount = oldDLCCount;

	// Look for new dlc files loop. If there is any, trigger Enumeration, else sleep. Break when game is closing.
	while (!D3DHooks::GameClosing) {
		if (Settings::ReturnSettingValue("ForceReEnumerationEnabled") == "automatic") {
			oldDLCCount = newDLCCount;
			newDLCCount = Enumeration::GetCurrentDLCCount();

			if (oldDLCCount != newDLCCount)
				Enumeration::ForceEnumeration();
		}

		Sleep(Settings::GetModSetting("CheckForNewSongsInterval"));
	}

	return 0;
}

/// <summary>
/// Send Midi Data Async
/// </summary>
/// <returns>NULL. Loops while game is open.</returns>
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

/// <summary>
/// Handle Keypress Inputs. Used to toggle mods on / off. WARNING: RUNS (almost) EVERY FRAME
/// </summary>
/// <param name="hWnd"> - ID of Rocksmith</param>
/// <param name="msg"> - Reason Function was called. KEYUP = Keypress | COPYDATA = Settings Update | CLOSE = Game Closing.</param>
/// <param name="keyPressed"> - Virtual Key of the key pressed. Reference here: https://docs.microsoft.com/en-us/windows/win32/inputdev/virtual-key-codes </param>
/// <param name="lParam"> - Data Sent</param>
/// <returns>Verification that message was sent.</returns>
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM keyPressed, LPARAM lParam) {

	// Trigger Mod on Keypress
	if (msg == WM_KEYUP) {
		if (D3DHooks::GameLoaded) { // Game must not be on the startup videos or it will crash
			if (keyPressed == Settings::GetKeyBind("ToggleLoftKey") && Settings::ReturnSettingValue("ToggleLoftEnabled") == "on") { // Toggle Loft
				MemHelpers::ToggleLoft();
				std::cout << "Toggle Loft" << std::endl;
			}

			else if (keyPressed == Settings::GetKeyBind("ShowSongTimerKey") && Settings::ReturnSettingValue("ShowSongTimerEnabled") == "on") { // Show Song Timer
				D3DHooks::showSongTimerOnScreen = !D3DHooks::showSongTimerOnScreen;
				std::cout << "Show Me Dat Timer Bruh" << std::endl;
			}

			else if (keyPressed == Settings::GetKeyBind("ForceReEnumerationKey") && Settings::ReturnSettingValue("ForceReEnumerationEnabled") == "manual") { // Force Enumeration (Manual | Keypress)
				Enumeration::ForceEnumeration();
				std::cout << "ENUMERATE YOU FRICKIN' SOAB" << std::endl;
			}

			else if (keyPressed == Settings::GetKeyBind("RainbowStringsKey") && Settings::ReturnSettingValue("RainbowStringsEnabled") == "on") { // Rainbow Strings
				ERMode::ToggleRainbowMode();
				std::cout << "Rainbows Are Pretty Cool" << std::endl;
			}

			else if (keyPressed == Settings::GetKeyBind("RainbowNotesKey") && Settings::ReturnSettingValue("RainbowNotesEnabled") == "on") { // Rainbow Notes
				ERMode::ToggleRainbowNotes();
				std::cout << "Those notes do be kinda pretty" << std::endl;
			}

			else if (keyPressed == Settings::GetKeyBind("RemoveLyricsKey") && Settings::ReturnSettingValue("RemoveLyricsWhen") == "manual") { // Remove Lyrics (Manual | Keypress)
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
			else if (keyPressed == Settings::GetKeyBind("MasterVolumeKey") && Settings::ReturnSettingValue("VolumeControlEnabled") == "on") { // Master Volume
				if ((GetKeyState(VK_CONTROL) & 0x8000)) // Is Control Pressed
					VolumeControl::DecreaseVolume(Settings::GetModSetting("VolumeControlInterval"), "Master_Volume");
				else
					VolumeControl::IncreaseVolume(Settings::GetModSetting("VolumeControlInterval"), "Master_Volume");
			}
			else if (keyPressed == Settings::GetKeyBind("SongVolumeKey") && Settings::ReturnSettingValue("VolumeControlEnabled") == "on") { // Song Volume
				if ((GetKeyState(VK_CONTROL) & 0x8000)) // Is Control Pressed
					VolumeControl::DecreaseVolume(Settings::GetModSetting("VolumeControlInterval"), "Mixer_Music");
				else
					VolumeControl::IncreaseVolume(Settings::GetModSetting("VolumeControlInterval"), "Mixer_Music");
			}
			else if (keyPressed == Settings::GetKeyBind("Player1VolumeKey") && Settings::ReturnSettingValue("VolumeControlEnabled") == "on") { // P1 Guitar & Bass Volume
				if ((GetKeyState(VK_CONTROL) & 0x8000)) // Is Control Pressed
					VolumeControl::DecreaseVolume(Settings::GetModSetting("VolumeControlInterval"), "Mixer_Player1");
				else
					VolumeControl::IncreaseVolume(Settings::GetModSetting("VolumeControlInterval"), "Mixer_Player1");
			}
			else if (keyPressed == Settings::GetKeyBind("Player2VolumeKey") && Settings::ReturnSettingValue("VolumeControlEnabled") == "on") { // P2 Guitar & Bass Volume
				if ((GetKeyState(VK_CONTROL) & 0x8000)) // Is Control Pressed
					VolumeControl::DecreaseVolume(Settings::GetModSetting("VolumeControlInterval"), "Mixer_Player2");
				else
					VolumeControl::IncreaseVolume(Settings::GetModSetting("VolumeControlInterval"), "Mixer_Player2");
			}
			else if (keyPressed == Settings::GetKeyBind("MicrophoneVolumeKey") && Settings::ReturnSettingValue("VolumeControlEnabled") == "on") { // Mic Volume
				if ((GetKeyState(VK_CONTROL) & 0x8000)) // Is Control Pressed
					VolumeControl::DecreaseVolume(Settings::GetModSetting("VolumeControlInterval"), "Mixer_Mic");
				else
					VolumeControl::IncreaseVolume(Settings::GetModSetting("VolumeControlInterval"), "Mixer_Mic");
			}
			else if (keyPressed == Settings::GetKeyBind("VoiceOverVolumeKey") && Settings::ReturnSettingValue("VolumeControlEnabled") == "on") { // Voice-Over Volume
				if ((GetKeyState(VK_CONTROL) & 0x8000)) // Is Control Pressed
					VolumeControl::DecreaseVolume(Settings::GetModSetting("VolumeControlInterval"), "Mixer_VO");
				else
					VolumeControl::IncreaseVolume(Settings::GetModSetting("VolumeControlInterval"), "Mixer_VO");
			}
			else if (keyPressed == Settings::GetKeyBind("SFXVolumeKey") && Settings::ReturnSettingValue("VolumeControlEnabled") == "on") { // SFX Volume
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

			else if (keyPressed == Settings::GetKeyBind("ChangedSelectedVolumeKey") && Settings::ReturnSettingValue("VolumeControlEnabled") == "on") { // Show Selected Volume
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
			

			else if (keyPressed == Settings::GetKeyBind("RRSpeedKey") && Settings::ReturnSettingValue("RRSpeedAboveOneHundred") == "on" && (MemHelpers::IsInStringArray(D3DHooks::currentMenu, NULL, fastRRModes)) && useNewSongSpeed) { // Riff Repeater over 100%
				bool prepToTurnOff = false;

				if (realSongSpeed >= 100.f)
					prepToTurnOff = true;

				// Convert UI Speed -> Real Speed
				float UISpeed = MemHelpers::RiffRepeaterSpeed();
				realSongSpeed = ((60 / (104.539 - (0.4393 * UISpeed))) * 100);

				// Add / Subtract User's Interval

				if (GetAsyncKeyState(VK_SHIFT) < 0)
					realSongSpeed -= (float)Settings::GetModSetting("RRSpeedInterval");
				else
					realSongSpeed += (float)Settings::GetModSetting("RRSpeedInterval");
				
				// Set Limits
				if (realSongSpeed > 428.f)
					realSongSpeed = 428.f;

				if (realSongSpeed < 100.f)
					realSongSpeed = 100.f;

				// Convert Real Speed -> UI Speed
				UISpeed = ((60 / (realSongSpeed / 100) - 104.539) / -0.4393);

				// Save new speed, and save it to a file (for streamers to use as an on-screen overlay)
				MemHelpers::RiffRepeaterSpeed(UISpeed);
				saveNewRRSpeedToFile = true;

				if (prepToTurnOff && realSongSpeed == 100.f) // Disable UI if we bug out of the mode.
					useNewSongSpeed = false;
			}

			if (MemHelpers::IsInStringArray(D3DHooks::currentMenu, NULL, tuningMenus) && keyPressed == VK_DELETE)
				Midi::userWantsToUseAutoTuning = true;
		}

		if (debug) {
			if (keyPressed == VK_INSERT) // Debug menu | ImGUI
				D3DHooks::menuEnabled = !D3DHooks::menuEnabled;

		}
	}

	// Update settings from GUI. Done on GUI open AND on GUI setting save.
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

			// **Deprecated** Twitch Integration. Use CrowdControl.
			else if (Contains(currMsg, "TurboSpeed"))
				userWantsRRSpeedEnabled = Contains(currMsg, "enable");
			else if (Contains(currMsg, "enable"))
				effectQueue.push_back(currMsg);
			else if (Contains(currMsg, "Reconnect"))
				CrowdControl::StartServerLoop();
		}
	}

	// Failsafe for mouse input falling through the menu
	//if (D3DHooks::menuEnabled && (msg == WM_LBUTTONDOWN || msg == WM_LBUTTONUP || msg == WM_RBUTTONDOWN || msg == WM_RBUTTONUP || msg == WM_MBUTTONDOWN || msg == WM_MBUTTONUP || msg == WM_MOUSEWHEEL || msg == WM_MOUSEMOVE))
	//	return false;

	// If Game is closing, else get ImGUI setup.
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

/// <summary>
/// Hook on game boot. Initialize all our own UI elements (text on screen, and ImGUI).
/// </summary>
/// <param name="pDevice"> - Device Pointer</param>
/// <returns>HRESULT of the official EndScene</returns>
HRESULT APIENTRY D3DHooks::Hook_EndScene(IDirect3DDevice9* pDevice) {
	HRESULT hRet = oEndScene(pDevice); // Get real result so we can call it later.
	uint32_t returnAddress = (uint32_t)_ReturnAddress(); // EndScene is called both by the game and by Steam's overlay renderer, and there's no need to draw our stuff twice

	if (returnAddress > Offsets::baseEnd)
		return hRet;

	static bool init = false; 

	// Has this been ran before (AKA run only once, at startup)
	if (!init) {
		init = true;

		// Create ImGUI
		ImGui::CreateContext();
		ImGuiIO& io = ImGui::GetIO();
		ImFont* font = io.Fonts->AddFontFromMemoryCompressedTTF(RobotoFont_data, RobotoFont_size, 20);

		// Hook WndProc (Keypress manager)
		D3DDEVICE_CREATION_PARAMETERS d3dcp;
		pDevice->GetCreationParameters(&d3dcp);
		hThisWnd = d3dcp.hFocusWindow;
		oWndProc = (WNDPROC)SetWindowLongPtr(hThisWnd, GWLP_WNDPROC, (LONG_PTR)WndProc);

		// Init ImGUI
		ImGui_ImplWin32_Init(hThisWnd);
		ImGui_ImplDX9_Init(pDevice);
		ImGui::GetIO().ImeWindowHandle = hThisWnd;
		//ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange;
		ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

		// **DEPRECATED** Generate solid color textures. Useful for testing
		//D3D::GenerateSolidTexture(pDevice, &Red, D3DCOLOR_ARGB(255, 000, 255, 255));
		//D3D::GenerateSolidTexture(pDevice, &Green, D3DCOLOR_ARGB(255, 0, 255, 0));
		//D3D::GenerateSolidTexture(pDevice, &Blue, D3DCOLOR_ARGB(255, 0, 0, 255));
		//D3D::GenerateSolidTexture(pDevice, &Yellow, D3DCOLOR_ARGB(255, 255, 255, 0));

		// **DEPRECATED** Generate texture from dds file.
		//D3DXCreateTextureFromFile(pDevice, L"notes_gradient_normal.dds", &gradientTextureNormal); //if those don't exist, note heads will be "invisible" | 6-String Model
		//D3DXCreateTextureFromFile(pDevice, L"notes_gradient_seven.dds", &gradientTextureSeven); // 7-String Note Colors
		//D3DXCreateTextureFromFile(pDevice, L"gradient_map_additive.dds", &additiveNoteTexture); // Note Stems
		
		D3DXCreateTextureFromFile(pDevice, L"nonexistenttexture.dds", &nonexistentTexture); // Black Notes

		std::cout << "ImGUI Init" << std::endl;

		Settings::UpdateSettings();

		// Generate textures to be called later
		D3D::GenerateTextures(pDevice, D3D::Strings);
		D3D::GenerateTextures(pDevice, D3D::Notes);
		D3D::GenerateTextures(pDevice, D3D::Rainbow);
		//GenerateTextures(pDevice, Random);
		D3D::GenerateTextures(pDevice, D3D::Random_Solid);
		D3D::GenerateTextures(pDevice, D3D::Noteway);
		D3D::GenerateTextures(pDevice, D3D::Gutter);
		D3D::GenerateTextures(pDevice, D3D::FretNums);

		D3DXCreateTextureFromFile(pDevice, L"headstock.png", &customHeadstockTexture); // Custom Headstock

		// Green Screen Wall textures. Look at the uses of the textures for more information.
		D3DXCreateTextureFromFile(pDevice, L"stage0.png", &customGreenScreenWall_Stage0); // Background Tile
		D3DXCreateTextureFromFile(pDevice, L"stage1.png", &customGreenScreenWall_Stage1); // Noise
		D3DXCreateTextureFromFile(pDevice, L"stage2.png", &customGreenScreenWall_Stage2); // Caustic (Indirect)
		D3DXCreateTextureFromFile(pDevice, L"stage3.png", &customGreenScreenWall_Stage3); // Narnia / Venue Fade In Mask.
		D3DXCreateTextureFromFile(pDevice, L"stage4.png", &customGreenScreenWall_Stage4); // White square
		D3DXCreateTextureFromFile(pDevice, L"stage5.png", &customGreenScreenWall_Stage5); // Pipes and wall trim
		D3DXCreateTextureFromFile(pDevice, L"stage6.png", &customGreenScreenWall_Stage6); // N Mask of Background tile
	}

	// Start new ImGUI Frame
	ImGui_ImplDX9_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();

	if (D3DHooks::menuEnabled) {

		// Draw ImGUI UI
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

	// Render ImGUI
	ImGui::EndFrame();
	ImGui::Render();

	ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());

	// Restart mouse and keyboard in game
	ImGui::CaptureKeyboardFromApp(false);
	ImGui::CaptureMouseFromApp(false);

	if (generateTexture) {
		D3D::GenerateTextures(pDevice, D3D::Strings);
		generateTexture = false;
	}

	int whiteText = 0xFFFFFFFF;
	Resolution WindowSize = MemHelpers::GetWindowSize();

	// Draw text on screen || NOTE: NEVER USE SET VALUES. Always do division of WindowSize X AND Y so every resolution should have the text in around the same spot.
	if (D3DHooks::GameLoaded) {

		if (Settings::ReturnSettingValue("VolumeControlEnabled") == "on" && (MemHelpers::IsInStringArray(D3DHooks::currentMenu, NULL, songModes) || AutomatedSelectedVolume)) { // Show Selected Volume
			float volume = 0;
			RTPCValue_type type = RTPCValue_GameObject;
			WwiseVariables::Wwise_Sound_Query_GetRTPCValue_Char(mixerInternalNames[currentVolumeIndex].c_str(), 0xffffffff, &volume, &type);

			if (currentVolumeIndex != 0)
				MemHelpers::DX9DrawText(drawMixerTextName[currentVolumeIndex] + std::to_string((int)volume) + "%", whiteText, (int)(WindowSize.width / 54.85), (int)(WindowSize.height / 30.85), (int)(WindowSize.width / 14.22), (int)(WindowSize.height / 8), pDevice);
		}

		if ((D3DHooks::showSongTimerOnScreen && MemHelpers::ShowSongTimer() != "")) { // Show Song Timer
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

		if (Settings::ReturnSettingValue("RRSpeedAboveOneHundred") == "on" && MemHelpers::IsInStringArray(currentMenu, NULL, fastRRModes) || MemHelpers::IsInStringArray(currentMenu, NULL, scoreScreens)) { // Riff Repeater over 100%
			
			// Set song speed every frame (approx) as the value continously falls down.
			if (!MemHelpers::IsInStringArray(currentMenu, NULL, scoreScreens))
				MemHelpers::RiffRepeaterSpeed(((60 / (realSongSpeed / 100) - 104.539) / -0.4393));

			if (useNewSongSpeed)
				MemHelpers::DX9DrawText("Riff Repeater Speed: " + std::to_string((int)roundf(realSongSpeed)) + "%", whiteText, (int)(WindowSize.width / 2.35), (int)(WindowSize.height / 30.85), (int)(WindowSize.width / 2.50), (int)(WindowSize.height / 8), pDevice);
		}

		if (Settings::ReturnSettingValue("ShowCurrentNoteOnScreen") == "on" && GuitarSpeak::GetCurrentNoteName() != (std::string)"") { // Show Current Note On Screen
			if (MemHelpers::IsInStringArray(currentMenu, NULL, songModes))
				MemHelpers::DX9DrawText(GuitarSpeak::GetCurrentNoteName(), whiteText, (int)(WindowSize.width / 5.5), (int)(WindowSize.height / 1.75), (int)(WindowSize.width / 5.75), (int)(WindowSize.height / 8), pDevice);
			else // Show outside of the song at the top of the screen.
				MemHelpers::DX9DrawText("Current Note: " + GuitarSpeak::GetCurrentNoteName(), whiteText, (int)(WindowSize.width / 3.87), (int)(WindowSize.height / 30.85), (int)(WindowSize.width / 4), (int)(WindowSize.height / 8), pDevice);
		}

		// Regenerate Twitch Solid Note Color for a new color
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

/// <summary>
/// Handle Twitch Toggle Message.
/// </summary>
/// <param name="currMsg"> - Mod to Trigger.</param>
/// <param name="type"> - Should We Enable or Disable.</param>
/// <returns>BOOL. If effects triggered.</returns>
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

/// <summary>
/// Trigger Twitch Effect
/// </summary>
/// <param name="currEffectMsg"> - Mod to Trigger.</param>
void HandleEffect(std::string currEffectMsg) {
	// Split message into chunks
	auto msgParts = Settings::SplitByWhitespace(currEffectMsg);
	std::string effectName = msgParts[1];
	std::cout << "Entering the thread for: " << currEffectMsg << std::endl;

	// Don't allow the current effect to apply twice. Also blocks mods from triggering when not in a song.
	while (MemHelpers::IsInStringArray(effectName, 0, enabledEffects) && !MemHelpers::IsInSong())  
		Sleep(150);
	std::cout << enabledEffects.size() << std::endl;

	// Enable Effect
	std::cout << "Enabling " << effectName << std::endl;
	if (HandleMessage(currEffectMsg, "enable")) {
		// Sleep for the duration of the effect.
		Sleep(std::stoi(msgParts.back()) * 1000);

		// Disable the effect after it's done
		HandleMessage(currEffectMsg, "disable");
		std::cout << "Disabling " << effectName << std::endl;
		if (MemHelpers::IsInStringArray(effectName, 0, enabledEffects)) // JIC
			enabledEffects.erase(std::find(enabledEffects.begin(), enabledEffects.end(), effectName));
	}
}

/// <summary>
/// Manage Queue of Twitch Effects. TODO: This is fairly crude, so if it takes a while to get CC in place, improve synchronization of this (cond_variables, etc.)
/// </summary>
/// <returns>NULL. Loops while game is open.</returns>
unsigned WINAPI HandleEffectQueueThread() {
	while (!D3DHooks::GameClosing) {

		// If in a song, and there is an effect.
		if (effectQueue.size() > 0 && MemHelpers::IsInSong()) {
			for (auto it = effectQueue.begin(); it != effectQueue.end();) {
				std::string effectName = Settings::SplitByWhitespace(*it)[1];

				// Make sure we don't already have this effect in the queue.
				if (!MemHelpers::IsInStringArray(effectName, 0, enabledEffects)) { 
					enabledEffects.push_back(effectName);

					// Send full effect message to the thread
					std::thread effectThrd(HandleEffect, *it); 
					effectThrd.detach();

					// Remove effect from queue.
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

/// <summary>
/// Hook Game Functions For Our Own Uses (On Alt-Tab, Draw UI, etc).
/// </summary>
void GUI() {
	uint32_t d3d9Base, adr, * vTable = NULL;

	// Find D3D Device
	while ((d3d9Base = (uint32_t)GetModuleHandleA("d3d9.dll")) == NULL)
		Sleep(500);
	adr = MemUtil::FindPattern<uint32_t>(d3d9Base, Offsets::d3dDevice_SearchLen, (PBYTE)Offsets::d3dDevice_Pattern, Offsets::d3dDevice_Mask) + 2;

	bool runningThroughWine = adr == (uint32_t)2;

	std::cout << "Running in Wine: " << std::boolalpha << runningThroughWine << std::endl;

	// Proton / Wine Check
	if (runningThroughWine) {
		std::cout << "Performing Wine check" << std::endl;
		adr = MemUtil::FindPattern<uint32_t>(0x2000000, 0x2B778CC, (PBYTE)Offsets::d3dDevice_Pattern, Offsets::d3dDevice_Mask) + 2;
		std::cout << adr << std::endl;
	}

	// Null-check
	if (!adr) {
		std::cout << "Could not find D3D9 device pointer" << std::endl;
		return;
	}

	// Null-Check x2
	if (!*(uint32_t*)adr) { // Wing it
		MessageBoxA(NULL, "Could not find DX9 device, please restart the game!", "Error", NULL);
		return;
	}

	vTable = *(uint32_t**)adr;

	// Third times the charm?
	if (!vTable || vTable < (uint32_t*)Offsets::baseHandle) {
		MessageBoxA(NULL, "Could not find D3D device's vTable address \n Restart the game and if you still get this error after a few tries, please report the error!", "Error", NULL);
		return;
	}

	// Hook D3D functions to use for our own D3D work. Reference D3DHooks

	if (!runningThroughWine) {
		oSetVertexDeclaration = (tSetVertexDeclaration)MemUtil::TrampHook((PBYTE)vTable[D3DInfo::SetVertexDeclaration_Index], (PBYTE)D3DHooks::Hook_SetVertexDeclaration, 7); // https://docs.microsoft.com/en-us/windows/win32/api/d3d9helper/nf-d3d9helper-idirect3ddevice9-setvertexdeclaration
		oSetVertexShaderConstantF = (tSetVertexShaderConstantF)MemUtil::TrampHook((PBYTE)vTable[D3DInfo::SetVertexShaderConstantF_Index], (PBYTE)D3DHooks::Hook_SetVertexShaderConstantF, 7); // https://docs.microsoft.com/en-us/windows/win32/api/d3d9helper/nf-d3d9helper-idirect3ddevice9-setvertexshaderconstantf
		oReset = (tReset)DetourFunction((PBYTE)vTable[D3DInfo::Reset_Index], (PBYTE)D3DHooks::Hook_Reset); // https://docs.microsoft.com/en-us/windows/win32/api/d3d9helper/nf-d3d9helper-idirect3ddevice9-reset
		//oReset = (tReset)MemUtil::TrampHook((PBYTE)vTable[D3DInfo::Reset_Index], (PBYTE)Hook_Reset, 5); // You'd expect this to work, given the effect is extremely similar to what DetourFunction does, but... it crashes instead
	}

	oSetVertexShader = (tSetVertexShader)MemUtil::TrampHook((PBYTE)vTable[D3DInfo::SetVertexShader_Index], (PBYTE)D3DHooks::Hook_SetVertexShader, 7); // https://docs.microsoft.com/en-us/windows/win32/api/d3d9helper/nf-d3d9helper-idirect3ddevice9-setvertexshader
	oSetPixelShader = (tSetPixelShader)MemUtil::TrampHook((PBYTE)vTable[D3DInfo::SetPixelShader_Index], (PBYTE)D3DHooks::Hook_SetPixelShader, 7); // https://docs.microsoft.com/en-us/windows/win32/api/d3d9helper/nf-d3d9helper-idirect3ddevice9-setpixelshader
	oSetStreamSource = (tSetStreamSource)MemUtil::TrampHook((PBYTE)vTable[D3DInfo::SetStreamSource_Index], (PBYTE)D3DHooks::Hook_SetStreamSource, 7); // https://docs.microsoft.com/en-us/windows/win32/api/d3d9helper/nf-d3d9helper-idirect3ddevice9-setstreamsource
	oEndScene = (tEndScene)MemUtil::TrampHook((PBYTE)vTable[D3DInfo::EndScene_Index], (PBYTE)D3DHooks::Hook_EndScene, 7); // https://docs.microsoft.com/en-us/windows/win32/api/d3d9helper/nf-d3d9helper-idirect3ddevice9-endscene
	oDrawIndexedPrimitive = (tDrawIndexedPrimitive)MemUtil::TrampHook((PBYTE)vTable[D3DInfo::DrawIndexedPrimitive_Index], (PBYTE)D3DHooks::Hook_DIP, 5); // https://docs.microsoft.com/en-us/windows/win32/api/d3d9helper/nf-d3d9helper-idirect3ddevice9-drawindexedprimitive
	oDrawPrimitive = (tDrawPrimitive)MemUtil::TrampHook((PBYTE)vTable[D3DInfo::DrawPrimitive_Index], (PBYTE)D3DHooks::Hook_DP, 7); // https://docs.microsoft.com/en-us/windows/win32/api/d3d9helper/nf-d3d9helper-idirect3ddevice9-drawprimitive
}

/// <summary>
/// Press enter. Normally used in a loop to skip most of the login dialog. "Fork in the toaster" method
/// </summary>
void AutoEnterGame() {
	PostMessage(FindWindow(NULL, L"Rocksmith 2014"), WM_KEYDOWN, VK_RETURN, 0);
	Sleep(30);
	PostMessage(FindWindow(NULL, L"Rocksmith 2014"), WM_KEYUP, VK_RETURN, 0);
}

/// <summary>
/// Force a Steam screenshot. Requires the default "F12" screenshot key for Steam.
/// </summary>
void TakeScreenshot() {
	if (!takenScreenshotOfThisScreen) {
		takenScreenshotOfThisScreen = true;
		Sleep(8000); // The menu title changes while the animation is running so we are giving it so time to show the actual results. (8 seconds)

		// Press F12
		PostMessage(FindWindow(NULL, L"Rocksmith 2014"), WM_KEYDOWN, VK_F12, 0);
		std::cout << "Took screenshot" << std::endl;
		Sleep(30);
		PostMessage(FindWindow(NULL, L"Rocksmith 2014"), WM_KEYUP, VK_F12, 0);
	}
}

/// <summary>
/// Update settings so users don't need to restart the game for every mod they want to toggle on / off.
/// </summary>
void UpdateSettings() { // Live updates from the INI
	Settings::UpdateSettings();
	Sleep(500);
	CustomSongTitles::LoadSettings();
	Sleep(500);
	CustomSongTitles::HookSongListsKoko();
	Sleep(500);
}

/// <summary>
/// Remove .mdmp dump files for when Rocksmith crashes. **ONLY EFFECTS DEBUG BUILDS**
/// </summary>
void ClearMDMPs() {
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

/// <summary>
/// Write a log of Riff Repeater speed if it's above 100%. Mainly used for streamers who want to create their own UI for the RR Speed.
/// </summary>
/// <returns>NULL. Loops while game is open.</returns>
unsigned WINAPI StreamerLogThread() {
	while (!D3DHooks::GameClosing) {
		if (Settings::ReturnSettingValue("RRSpeedAboveOneHundred") == "on" && MemHelpers::IsInStringArray(D3DHooks::currentMenu, NULL, fastRRModes) && saveNewRRSpeedToFile) {
			std::ofstream rrText = std::ofstream("riff_repeater_speed.txt", std::ofstream::out);
			rrText << std::to_string((int)realSongSpeed) << std::endl;
			saveNewRRSpeedToFile = false;
		}
		Sleep(50);
	}

	return 0;
}

/// <summary>
/// Main Thread where we trigger the mods to startup.
/// </summary>
/// <returns>NULL. Loops while game is open.</returns>
unsigned WINAPI MainThread() {
	std::ifstream RSModsFileInput("RSMods.ini"); // Check if this file exists
	if (!RSModsFileInput) {
		std::ofstream RSModsFileOutput("RSMods.ini"); // If we don't call this, the game will crash for some reason :(
		RSModsFileOutput.close();
	}

	bool movedToExternalDisplay = false; // User wants to move the display to a specific location on boot.

	// Initialize Functions
	D3DHooks::debug = debug;
	Offsets::Initialize();
	MemHelpers::PatchCDLCCheck();
	Settings::Initialize();
	UpdateSettings();
	ERMode::Initialize();
	GUI();
	ClearMDMPs();
	Midi::InitMidi();

	using namespace D3DHooks;
	while (!GameClosing) {
		Sleep(250); // We don't need to call these settings always, we just want it to run every 1/4 of a second so the user doesn't notice it.

		// Move Rocksmith to second monitor on boot (if specified)
		if (!movedToExternalDisplay && Settings::ReturnSettingValue("SecondaryMonitor") == "on") {
			LaunchOnExternalMonitor::SendRocksmithToScreen(Settings::GetModSetting("SecondaryMonitorXPosition"), Settings::GetModSetting("SecondaryMonitorYPosition")); // Move to Location in INI
			movedToExternalDisplay = true;
		}

		// If Game Is Loaded (No need to run these while the game is loading.)
		if (GameLoaded) {
			currentMenu = MemHelpers::GetCurrentMenu(); // This loads without checking if memory is safe... This can cause crashes if used else where.

			//std::cout << currentMenu << std::endl;

			// Scan for MIDI devices for Automated Tuning / True-Tuning
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

			// Remove Headstock (Always Off)
			if (Settings::ReturnSettingValue("RemoveHeadstockEnabled") == "on" && Settings::ReturnSettingValue("RemoveHeadstockWhen") == "startup")
				RemoveHeadstockInThisMenu = true; // In this case, the user always wants to remove the headstock. This value should never turn to false in this mode.

			// Toggle Loft (In Song / Always Off). Turn off in Lesson Mode (or video won't appear). Emulate effect with GreenScreenWall.
			if (LessonMode && Settings::ReturnSettingValue("ToggleLoftEnabled") == "on" && Settings::ReturnSettingValue("ToggleLoftWhen") != "manual") { // Is User In A Lesson Mode AND set to turn loft off
				if (LoftOff)
					MemHelpers::ToggleLoft();
				LoftOff = false;
				GreenScreenWall = true;
			}

			// Show Selected Volume
			if (!AutomatedSelectedVolume && Settings::ReturnSettingValue("VolumeControlEnabled") == "on" && Settings::ReturnSettingValue("ShowSelectedVolumeWhen") == "automatic") {
				AutomatedSelectedVolume = true;
				currentVolumeIndex = 1;
			}
			
			// Toggle Loft off (Always Off)
			if (!LoftOff && !LessonMode && Settings::ReturnSettingValue("ToggleLoftEnabled") == "on" && Settings::ReturnSettingValue("ToggleLoftWhen") == "startup") {
				MemHelpers::ToggleLoft();
				LoftOff = true;
				GreenScreenWall = false;
			}

			// Toggle Skyline (Always Off)
			if (!SkylineOff && Settings::ReturnSettingValue("RemoveSkylineEnabled") == "on" && Settings::ReturnSettingValue("ToggleSkylineWhen") == "startup") // Turn the skyline off on startup
				toggleSkyline = true;

			// Remove Lyrics (Always Off)
			if (!RemoveLyrics && Settings::ReturnSettingValue("RemoveLyricsWhen") == "startup")
				RemoveLyrics = true;

			// Riff Repeater above 100%
			userWantsRRSpeedEnabled = (Settings::ReturnSettingValue("RRSpeedAboveOneHundred") == "on"); // Set To True if you want the user / streamer to have RR open every song (for over 100% RR speed)
			if (MemHelpers::IsInStringArray(currentMenu, NULL, fastRRModes) && userWantsRRSpeedEnabled && !automatedSongSpeedInThisSong) // This won't work in SA so we need to exclude it.
				MemHelpers::AutomatedOpenRRSpeedAbuse();

			/// If User Is Entering Song

			if (MemHelpers::IsInStringArray(currentMenu, NULL, songModes)) {
				GuitarSpeakPresent = false;

				// Remove Headstock (In Song)
				if (Settings::ReturnSettingValue("RemoveHeadstockEnabled") == "on" && Settings::ReturnSettingValue("RemoveHeadstockWhen") == "song")
					RemoveHeadstockInThisMenu = true;

				// Toggle Loft (In Song)
				if (Settings::ReturnSettingValue("ToggleLoftEnabled") == "on" && Settings::ReturnSettingValue("ToggleLoftWhen") == "song") {
					if (!LoftOff)
						MemHelpers::ToggleLoft();
					LoftOff = true;
				}

				// Remove Skyline (In Song)
				if (Settings::ReturnSettingValue("RemoveSkylineEnabled") == "on" && Settings::ReturnSettingValue("ToggleSkylineWhen") == "song") {
					if (!SkylineOff)
						toggleSkyline = true;
					DrawSkylineInMenu = false;
				}

				// MIDI Auto Tuning / Auto True-Tuning
				if (Settings::ReturnSettingValue("AutoTuneForSong") == "on" && !Midi::alreadyAutomatedTuningInThisSong && Midi::userWantsToUseAutoTuning) {
					Midi::AutomateDownTuning();
					Midi::AutomateTrueTuning();
				}

				// Show Song Timer (In Song)
				if (!AutomatedSongTimer && Settings::ReturnSettingValue("ShowSongTimerEnabled") == "on" && Settings::ReturnSettingValue("ShowSongTimerWhen") == "automatic") { // User always wants to see the song timer.
					AutomatedSongTimer = true;
					showSongTimerOnScreen = true;
				}
				
				// Show Selected Volume (In Song)
				if (!AutomatedSelectedVolume && Settings::ReturnSettingValue("VolumeControlEnabled") == "on" && Settings::ReturnSettingValue("ShowSelectedVolumeWhen") == "song") {
					currentVolumeIndex = 1;
					AutomatedSelectedVolume = true;
				}
					
			}

			/// If User Is Exiting A Song / In A Menu

			else {
				// Turn off Riff Repeater Speed above 100%
				if (!MemHelpers::IsInStringArray(currentMenu, NULL, scoreScreens)) {
					automatedSongSpeedInThisSong = false; 
					realSongSpeed = 100.f;
				}
				
				// Turn off Show Song Timer (In Song)
				if (AutomatedSongTimer && Settings::ReturnSettingValue("ShowSongTimerEnabled") == "on" && Settings::ReturnSettingValue("ShowSongTimerWhen") == "automatic") { // User always wants to see the song timer.
					AutomatedSongTimer = false;
					showSongTimerOnScreen = false;
				}	

				// Turn off Show Selected Volume (In Song)
				if (AutomatedSelectedVolume && Settings::ReturnSettingValue("VolumeControl") == "on" && Settings::ReturnSettingValue("ShowSelectedVolumeWhen") == "song") { // User only wants to see selected volume in game.
					currentVolumeIndex = 0;
					AutomatedSelectedVolume = false;
				}

				// Turn off MIDI Auto Tuning / Auto True-Tuning
				if (Midi::alreadyAutomatedTuningInThisSong) {
					Midi::RevertAutomatedTuning();
					Midi::userWantsToUseAutoTuning = false;
				}	

				// Turn off Remove Headstock (In Song)
				if (Settings::ReturnSettingValue("RemoveHeadstockEnabled") == "on" && Settings::ReturnSettingValue("RemoveHeadstockWhen") == "song")
					RemoveHeadstockInThisMenu = false;

				// Turn loft back on. If leaving lesson mode, turn off GreenScreenWall.
				if (Settings::ReturnSettingValue("ToggleLoftEnabled") == "on" && Settings::ReturnSettingValue("ToggleLoftWhen") == "song") {
					if (LoftOff) {
						MemHelpers::ToggleLoft();
						LoftOff = false;
					}
					if (!LessonMode)
						GreenScreenWall = false;
				}

				// Turn off Remove Skyline (In Song)
				if (SkylineOff && Settings::ReturnSettingValue("RemoveSkylineEnabled") == "on" && Settings::ReturnSettingValue("ToggleSkylineWhen") == "song") { // Turn the skyline back on after exiting a song
					toggleSkyline = true;
					DrawSkylineInMenu = true;
				}

				// Turn on Guitar Speak
				if (!GuitarSpeakPresent && Settings::ReturnSettingValue("GuitarSpeak") == "on") { // Guitar Speak
					GuitarSpeakPresent = true;
					if (!GuitarSpeak::RunGuitarSpeak()) // If we are in a menu where we don't want to read bad values
						GuitarSpeakPresent = false;
				}

				// Reset Headstock Cache (so we aren't running the same textures over and over again)
				if (Settings::ReturnSettingValue("RemoveHeadstockEnabled") == "on" && (!(MemHelpers::IsInStringArray(currentMenu, NULL, tuningMenus)) || currentMenu == "MissionMenu"))
					resetHeadstockCache = true;
			}
			
			/// "Other" menus. These will normally state what menus they need to be in.

			// Screenshot Scores
			if (Settings::ReturnSettingValue("ScreenShotScores") == "on" && MemHelpers::IsInStringArray(currentMenu, NULL, scoreScreens)) // Screenshot Scores
				TakeScreenshot();
			else
				takenScreenshotOfThisScreen = false;

			// If the current menu is not the same as the previous menu and if it's one of menus where you tune your guitar (i.e. headstock is shown), reset the cache because user may want to change the headstock style
			if (previousMenu != currentMenu && MemHelpers::IsInStringArray(currentMenu, NULL, tuningMenus)) { 
				resetHeadstockCache = true;
				headstockTexturePointers.clear();
			}

			previousMenu = currentMenu;

			// Toggle Rainbow Strings / Rainbow Notes effect(s) if enabled.
			if (ERMode::IsRainbowEnabled() || ERMode::IsRainbowNotesEnabled())
				ERMode::DoRainbow();
			else
				ERMode::Toggle7StringMode();
		}

		/// Game Hasn't Loaded Yet

		else {
			
			// Disable Controllers: XInput (Xbox) and DirectInput (other)
			//DisableControllers::DisableControllers();

			// Change Current Menu status to the current menu while the game is loading.
			currentMenu = MemHelpers::GetCurrentMenu(true); // This is the safe version of checking the current menu. It is only used while the game boots to help performance.

			if (currentMenu == "MainMenu") // Yay We Loaded :P
				GameLoaded = true;

			// Auto Load Profile. AKA "Fork in the toaster".
			if (Settings::ReturnSettingValue("ForceProfileEnabled") == "on" && !(MemHelpers::IsInStringArray(currentMenu, NULL, dontAutoEnter))) {
				// If the user user says "I want to always load this profile"
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


/// <summary>
/// Unhook all threads to take advantage of multi-threading.
/// </summary>
void Initialize() {
	std::thread(MainThread).detach(); // Mod Toggle based on menus
	std::thread(EnumerationThread).detach(); // Force Enumeration
	std::thread(HandleEffectQueueThread).detach(); // Twitch Effects
	std::thread(MidiThread).detach(); // MIDI Auto Tuning / True Tuning
	std::thread(StreamerLogThread).detach(); // RR Speed Above 100% Log

	// Probably check ini setting before starting this thing
	CrowdControl::StartServer(); // Twitch Effects Server
}

/// <summary>
/// Hook into the game for us to run our own code. **DISPLAYS DEBUG CONSOLE ON DEBUG BUILD**
/// </summary>
/// <param name="hModule"></param>
/// <param name="dwReason"></param>
/// <param name="lpReserved"></param>
/// <returns>Always returns TRUE</returns>
BOOL APIENTRY DllMain(HMODULE hModule, uint32_t dwReason, LPVOID lpReserved) {

	bool debugLogPresent = std::ifstream("RSMods_debug.txt").good();
	std::ofstream clearDebugLog = std::ofstream("RSMods_debug.txt");

	switch (dwReason) {
	case DLL_PROCESS_ATTACH:
		// Give debug console
		if (debug) {
			FILE* streamRead, * streamWrite;
			AllocConsole();
			freopen_s(&streamRead, "CONIN$", "r", stdin);
			freopen_s(&streamWrite, "CONOUT$", "w", stdout);
		}
		else if (debugLogPresent) { // Dump console to log (mainly for debugging release build issues)
			// Clear log so it isn't full of junk from the last launch
			clearDebugLog.open("RSMods_debug.txt", std::ofstream::out | std::ofstream::trunc);
			clearDebugLog.close();

			// Attach log to stdout / std::cout so we can get the logs.
			FILE* debugLog;
			freopen_s(&debugLog, "RSMods_debug.txt", "w", stdout);
		}

		DisableThreadLibraryCalls(hModule); // Disables the DLL_THREAD_ATTACH and DLL_THREAD_DETACH notifications. | https://docs.microsoft.com/en-us/windows/win32/api/libloaderapi/nf-libloaderapi-disablethreadlibrarycalls
		D3DX9_42::InitProxy(); // Proxy all real D3DX9_42.dll commands to the actual D3DX9_42.dll.
		Initialize(); // Inject our code.
		return TRUE;
	case DLL_PROCESS_DETACH:
		D3DX9_42::ShutdownProxy(); // Kill Proxy to D3DX9_42.dll

		// Shutdown ImGUI
		ImGui_ImplWin32_Shutdown();
		ImGui_ImplDX9_Shutdown();
		ImGui::DestroyContext();
		return TRUE;
	}
	return TRUE;
}
