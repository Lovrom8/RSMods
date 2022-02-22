#include "Main.hpp"

#ifdef _DEBUG
bool debug = true; // You ARE on a debug build.
#else
bool debug = false; // You are NOT on a debug build.
#endif

#ifdef _WWISE_LOGS
bool wwiseLogging = true; // You ARE on a Wwise logging build.
#else
bool wwiseLogging = false; // You are NOT on a Wwise logging build.
#endif

#ifndef _LOG_INIT
#define _LOG_INIT Log LOG; // Use log in place of cout or cerr, to help consolidate logging.
#endif;

#ifndef _RSMODS_VERSION
#define _RSMODS_VERSION "RSMODS Version: 1.2.6.5 SRC. DEBUG: " << std::boolalpha << debug << ". Wwise Logs: " << std::boolalpha << wwiseLogging << "."
#endif

/// <summary>
/// Handle Force Enumeration
/// </summary>
/// <returns>NULL. Loops while game is open.</returns>
unsigned WINAPI EnumerationThread() {

	// User has not seen the main menu yet.
	// Please wait until the user has seen the main menu before doing anything.
	while (!D3DHooks::GameLoaded)
		Sleep(5000);


	// Read the users Keybinds and Mod Settings, so verify we have the latest data from the INI.
	Settings::ReadKeyBinds();
	Settings::ReadModSettings();

	// Set an inital count for number of DLC songs.
	int oldDLCCount = Enumeration::GetCurrentDLCCount(), newDLCCount = oldDLCCount;

	// Look for new dlc files. Break when game is closing.
	while (!D3DHooks::GameClosing) {
		if (Settings::ReturnSettingValue("ForceReEnumerationEnabled") == "automatic") {
			oldDLCCount = newDLCCount;
			newDLCCount = Enumeration::GetCurrentDLCCount();

			// If there is any new dlc files, trigger Enumeration.
			if (oldDLCCount != newDLCCount)
				Enumeration::ForceEnumeration();
		}

		// Sleep for a user provided interval (milliseconds).
		Sleep(Settings::GetModSetting("CheckForNewSongsInterval"));
	}

	return 0;
}

/// <summary>
/// Send Midi Data Async. Only really used in debug builds to test MIDI commands.
/// Secondary purpose of remaking D3D textures every 32 ticks (~ 1 second).
/// </summary>
/// <returns>NULL. Loops while game is open.</returns>
unsigned WINAPI MidiThread() {
	// Initial some values.
	int everyXcyclesCheckD3DTextures = 31;
	int currentCount = 0;

	while (!D3DHooks::GameClosing) {
		// If this is the 32nd loop, remake the D3D textures.
		// This allows us to have real-time updates to textures.
		if (currentCount == 31) {
			currentCount = 0;
			D3DHooks::RecreateTextureTimer = true;
		}

		// If we have sent a Midi PC value to this thread, send the Midi value.
		if (Midi::sendPC)
			Midi::SendProgramChange(Midi::dataToSendPC);

		// If we have sent a Midi CC value to this thread, send the Midi value.
		if (Midi::sendCC)
			Midi::SendControlChange(Midi::dataToSendCC);

		// Sleep for ~ 1/33rd of a second so we don't waste that many cycles.
		Sleep(Midi::sleepFor); 
		currentCount++;
	}
	
	return 0;
}

unsigned WINAPI RiffRepeaterThread() {
	std::string previousSongKey = "";

	// We can only user Riff Repeater while the game is open, so verify it's open. Runs every 100 ms.
	while (!D3DHooks::GameClosing) {
		Sleep(100);

		// Get SongKey for the song. We need this to reference the audio event by name.
		const auto songKey = MemHelpers::GetSongKey();

		// If this isn't the song we saw on the last loop.
		if (songKey != previousSongKey) {
			previousSongKey = songKey;

			// If we have cached this event, then use the cached version, and tell the other threads to enable the Riff Repeater speed mod.
			if (RiffRepeater::SongObjectIDs.find("Play_" + previousSongKey) != RiffRepeater::SongObjectIDs.end()) {
				RiffRepeater::currentSongID = RiffRepeater::SongObjectIDs.find("Play_" + previousSongKey)->second;
				RiffRepeater::readyToLogSongID = false;
				RiffRepeater::loggedCurrentSongID = true;
			}
			// We have not seen this event yet, so we need to log the internal ID for the song and cache it.
			else {
				RiffRepeater::loggedCurrentSongID = false;
				RiffRepeater::readyToLogSongID = true; // Wait until the user gets into the song so we can grab this ID.
			}
		}
		
		// If we have recently changed the Riff Repeater speed through the mod, then save the new value to a text file.
		// This is primarily for streamers who want to make a custom overlay with the song speed.
		if (Settings::ReturnSettingValue("RRSpeedAboveOneHundred") == "on" && MemHelpers::Contains(D3DHooks::currentMenu, fastRRModes) && saveNewRRSpeedToFile) {
			std::ofstream rrText = std::ofstream("riff_repeater_speed.txt", std::ofstream::out);
			rrText << std::to_string((int)realSongSpeed) << std::endl;
			saveNewRRSpeedToFile = false;
		}
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

	// Makes ALT + ENTER cause F11 to be pressed.
	// This is mainly so a user can use a common shortcut, that works in most games now-a-days.
	if (msg == WM_SYSCOMMAND && keyPressed == SC_KEYMENU) {
		if (lParam == VK_RETURN) 
			WndProc(hWnd, WM_KEYUP, VK_F11, 0);

		return true;
	}
	
	// Prevent a weird bug when trying to play Guitarcade or Score Attack with a key combination.
	if (msg == WM_SYSCOMMAND && keyPressed == SC_MOVE + 0x2 && MemHelpers::Contains(D3DHooks::currentMenu, onlineModes))
		return true;

	// Trigger Mod on Keypress
	if (msg == WM_KEYUP) {
		if (D3DHooks::GameLoaded) { // Game must not be on the startup videos or it will crash

			// Toggle Loft mod
			if (keyPressed == Settings::GetKeyBind("ToggleLoftKey") && Settings::ReturnSettingValue("ToggleLoftEnabled") == "on") {
				MemHelpers::ToggleLoft();
				std::cout << "Triggered Mod: Toggle Loft" << std::endl;
			}

			// Show Song Timer mod
			else if (keyPressed == Settings::GetKeyBind("ShowSongTimerKey") && Settings::ReturnSettingValue("ShowSongTimerEnabled") == "on") {
				D3DHooks::showSongTimerOnScreen = !D3DHooks::showSongTimerOnScreen;
				std::cout << "Triggered Mod: Show Song Timer" << std::endl;
			}

			// Force Enumeration mod
			else if (keyPressed == Settings::GetKeyBind("ForceReEnumerationKey") && Settings::ReturnSettingValue("ForceReEnumerationEnabled") == "manual") {
				Enumeration::ForceEnumeration();
				std::cout << "Triggered Mod: Force Enumeration" << std::endl;
			}

			// Rainbow Strings mod
			else if (keyPressed == Settings::GetKeyBind("RainbowStringsKey") && Settings::ReturnSettingValue("RainbowStringsEnabled") == "on") {
				ERMode::ToggleRainbowMode();

				if (!ERMode::RainbowEnabled)
					ERMode::ResetAllStrings();

				std::cout << "Triggered Mod: Rainbow Strings" << std::endl;
			}

			// Rainbow Notes mod
			else if (keyPressed == Settings::GetKeyBind("RainbowNotesKey") && Settings::ReturnSettingValue("RainbowNotesEnabled") == "on") {
				ERMode::ToggleRainbowNotes();
				std::cout << "Triggered Mod: Rainbow Notes" << std::endl;
			}

			// Remove Lyrics mod
			else if (keyPressed == Settings::GetKeyBind("RemoveLyricsKey") && Settings::ReturnSettingValue("RemoveLyricsWhen") == "manual") {
				D3DHooks::RemoveLyrics = !D3DHooks::RemoveLyrics;
				std::cout << "Triggered Mod: Remove Lyrics" << std::endl;
			}

			// Control + A. Force us to read the Settings from the INI again, to renew our cached values.
			else if (keyPressed == 0x41 && (GetKeyState(VK_CONTROL) & 0x8000)) {
				Settings::UpdateSettings();
				std::cout << "Triggered Setting Update" << std::endl;
			}

			// Change Master Volume mod
			else if (keyPressed == Settings::GetKeyBind("MasterVolumeKey") && Settings::ReturnSettingValue("VolumeControlEnabled") == "on") {
				if ((GetKeyState(VK_CONTROL) & 0x8000)) // Is Control Pressed
					VolumeControl::DecreaseVolume(Settings::GetModSetting("VolumeControlInterval"), "Master_Volume");
				else
					VolumeControl::IncreaseVolume(Settings::GetModSetting("VolumeControlInterval"), "Master_Volume");
			}

			// Change Song Volume mod
			else if (keyPressed == Settings::GetKeyBind("SongVolumeKey") && Settings::ReturnSettingValue("VolumeControlEnabled") == "on") {
				if ((GetKeyState(VK_CONTROL) & 0x8000)) // Is Control Pressed
					VolumeControl::DecreaseVolume(Settings::GetModSetting("VolumeControlInterval"), "Mixer_Music");
				else
					VolumeControl::IncreaseVolume(Settings::GetModSetting("VolumeControlInterval"), "Mixer_Music");
			}

			// Change P1 Guitar & P1 Bass Volume mod
			else if (keyPressed == Settings::GetKeyBind("Player1VolumeKey") && Settings::ReturnSettingValue("VolumeControlEnabled") == "on") {
				if ((GetKeyState(VK_CONTROL) & 0x8000)) // Is Control Pressed
					VolumeControl::DecreaseVolume(Settings::GetModSetting("VolumeControlInterval"), "Mixer_Player1");
				else
					VolumeControl::IncreaseVolume(Settings::GetModSetting("VolumeControlInterval"), "Mixer_Player1");
			}

			// Change P2 Guitar & P2 Bass Volume mod
			else if (keyPressed == Settings::GetKeyBind("Player2VolumeKey") && Settings::ReturnSettingValue("VolumeControlEnabled") == "on") {
				if ((GetKeyState(VK_CONTROL) & 0x8000)) // Is Control Pressed
					VolumeControl::DecreaseVolume(Settings::GetModSetting("VolumeControlInterval"), "Mixer_Player2");
				else
					VolumeControl::IncreaseVolume(Settings::GetModSetting("VolumeControlInterval"), "Mixer_Player2");
			}

			// Change Microphone Volume mod
			else if (keyPressed == Settings::GetKeyBind("MicrophoneVolumeKey") && Settings::ReturnSettingValue("VolumeControlEnabled") == "on") {
				if ((GetKeyState(VK_CONTROL) & 0x8000)) // Is Control Pressed
					VolumeControl::DecreaseVolume(Settings::GetModSetting("VolumeControlInterval"), "Mixer_Mic");
				else
					VolumeControl::IncreaseVolume(Settings::GetModSetting("VolumeControlInterval"), "Mixer_Mic");
			}

			// Change Voice-Over Volume mod
			else if (keyPressed == Settings::GetKeyBind("VoiceOverVolumeKey") && Settings::ReturnSettingValue("VolumeControlEnabled") == "on") {
				if ((GetKeyState(VK_CONTROL) & 0x8000)) // Is Control Pressed
					VolumeControl::DecreaseVolume(Settings::GetModSetting("VolumeControlInterval"), "Mixer_VO");
				else
					VolumeControl::IncreaseVolume(Settings::GetModSetting("VolumeControlInterval"), "Mixer_VO");
			}
			
			// Change SFX Volume mod
			else if (keyPressed == Settings::GetKeyBind("SFXVolumeKey") && Settings::ReturnSettingValue("VolumeControlEnabled") == "on") {
				if ((GetKeyState(VK_CONTROL) & 0x8000)) // Is Control Pressed
					VolumeControl::DecreaseVolume(Settings::GetModSetting("VolumeControlInterval"), "Mixer_SFX");
				else
					VolumeControl::IncreaseVolume(Settings::GetModSetting("VolumeControlInterval"), "Mixer_SFX");
			}

			// Changed Displayed Volume mod
			else if (keyPressed == Settings::GetKeyBind("ChangedSelectedVolumeKey") && Settings::ReturnSettingValue("VolumeControlEnabled") == "on") {
			currentVolumeIndex++;

			if (currentVolumeIndex > (mixerInternalNames.size() - 1)) // There are only so many values we know we can edit, so loop back.
				currentVolumeIndex = 0;
			}

			// Change Tuning Offset mod
			else if (keyPressed == Settings::GetKeyBind("TuningOffsetKey") && Settings::ReturnSettingValue("AutoTuneForSong") == "on") {
				if (GetKeyState(VK_CONTROL) & 0x8000) // Is Control Pressed
					Midi::tuningOffset--;
				else
					Midi::tuningOffset++;

				// Cap what values can be set (as we don't want to have to specify every value, and some pedals don't support more than an octave).
				if (Midi::tuningOffset < -3)
					Midi::tuningOffset = -3;
				else if (Midi::tuningOffset > 12)
					Midi::tuningOffset = 12;

				std::cout << "Triggered Mod Setting: Tuning Offset is now set to " << Midi::tuningOffset << std::endl;
			}


			/* else if (keyPressed == VK_F9) { // Disco Mode | Current State: No easy way to toggle it off.
				DiscoModeEnabled = !DiscoModeEnabled;

				if (debug) {
					std::cout << "Turning ";
					if (!DiscoModeEnabled)
						std::cout << "off";
					else
						std::cout << "on";
					std::cout << " Disco Mode!" << std::endl;
				}
			} */

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

			// Toggle Extended Range mod
			else if (keyPressed == Settings::GetKeyBind("ToggleExtendedRangeKey"))
			{
				D3DHooks::UseERExclusivelyInThisSong = !D3DHooks::UseERExclusivelyInThisSong;
				MemHelpers::ToggleCB(D3DHooks::UseERExclusivelyInThisSong);

				std::cout << "Triggered Mod: Toggle Extended Range" << std::endl;
			}

			// Looping mod. Set loop starting point.
			else if (keyPressed == Settings::GetKeyBind("LoopStartKey") && Settings::ReturnSettingValue("AllowLooping") == "on" && MemHelpers::Contains(D3DHooks::currentMenu, fastRRModes)) {
				if (GetKeyState(VK_CONTROL) & 0x8000) { // Is Control Pressed.
					loopStart = NULL;
					loopEnd = NULL;
				}
				else {
					// Set the start of the loop to the current time in the song.
					loopStart = MemHelpers::SongTimer();

					// If end point of the loop comes at the same time as, or before, the start of the loop, reset it to 0.
					if (loopEnd <= loopStart) {
						loopEnd = NULL;
					}
				}
			}

			// Looping mod. Set loop ending point.
			else if (keyPressed == Settings::GetKeyBind("LoopEndKey") && Settings::ReturnSettingValue("AllowLooping") == "on" && MemHelpers::Contains(D3DHooks::currentMenu, fastRRModes)) {
				if (GetKeyState(VK_CONTROL) & 0x8000) { // Is Control Pressed.
					loopEnd = NULL;
				}
				else {
					// Set the end of the loop to the current time in the song.
					loopEnd = MemHelpers::SongTimer();

					// If end point of the loop comes at the same time as, or before, the start of the loop, reset it to 0.
					if (loopEnd <= loopStart) {
						loopEnd = NULL;
					}
				}
			}

			// Rewind song by X seconds mod.
			else if (keyPressed == Settings::GetKeyBind("RewindKey") && Settings::ReturnSettingValue("AllowRewind") == "on" && MemHelpers::Contains(D3DHooks::currentMenu, learnASongPlaying)) {
				// SongTimer is stored in seconds, while RewindBy is stored in milliseconds.
				// We need milliseconds to send to Wwise, so change SongTimer to milliseconds, then subtract the Rewind value.
				AkTimeMs seekTo = (AkTimeMs)((MemHelpers::SongTimer() * 1000) - Settings::GetModSetting("RewindBy")); 
																													  
				// RewindBy is greater than the amount of time we've been in the song.
				// Reset seekTo to 0 to prevent seeking to a negative time.
				if (seekTo < 0) {
					std::cout << "(REWIND) Tried to seek to " << seekTo << "ms into the song. Resetting to 0." << std::endl;
					seekTo = 0;
				}

				// Send event to Wwise to rewind the song.
				// Or more accurately, move to the seek time since Wwise doesn't have a rewind function.
				Wwise::SoundEngine::SeekOnEvent(std::string("Play_" + MemHelpers::GetSongKey()).c_str(), 0x1234, seekTo, false);

				// Tell Rocksmith to make all notes before the section we want the user to play to be greyed out.
				// While this isn't absolutely necessary, it is best to have this run just in case.
				// Our seek time needs to be stored as milliseconds when sending to Wwise, but we need to have it in seconds when setting the GreyNoteTimer.
				MemHelpers::SetGreyNoteTimer(seekTo / 1000.f);

				std::cout << "(REWIND) Seeked to " << seekTo << "ms." << std::endl;
			}

			// Auto Tuning via MIDI mod. 
			// Checks if we are in a tuning menu, and the user tried to skip tuning.
			if (Settings::ReturnSettingValue("AutoTuneForSongWhen") == "manual" && MemHelpers::Contains(D3DHooks::currentMenu, tuningMenus) && keyPressed == VK_DELETE) {
				Midi::userWantsToUseAutoTuning = true;
			}
		}

		// If we are using a debug version of the mods.
		if (debug) {
			if (keyPressed == VK_INSERT) { // Debug menu. ImGUI
				D3DHooks::menuEnabled = !D3DHooks::menuEnabled;
			}
		}
	}

	// Repeatedly trigger mod on key hold
	else if (msg == WM_KEYDOWN) {
		// Game must not be on the startup videos or it might crash
		if (D3DHooks::GameLoaded) { 
			// Riff Repeater > 100% mod.
			if (keyPressed == Settings::GetKeyBind("RRSpeedKey") && Settings::ReturnSettingValue("RRSpeedAboveOneHundred") == "on" && (MemHelpers::Contains(D3DHooks::currentMenu, fastRRModes)) && RiffRepeater::loggedCurrentSongID) {
				
				// Get the current speed of Riff Repeater.
				realSongSpeed = RiffRepeater::GetSpeed(true);

				// Add / Subtract User specified Interval
				if (GetKeyState(VK_CONTROL) & 0x8000)
					realSongSpeed -= (float)Settings::GetModSetting("RRSpeedInterval");
				else
					realSongSpeed += (float)Settings::GetModSetting("RRSpeedInterval");

				// Set limits to the speed
				// Cap at 400. Plugin only goes down to 25. 10000 / 25 = 400.
				// Cap at 25. Plugin only goes up to 400. 10000 / 400 = 25.

				if (realSongSpeed > 400.f) 
					realSongSpeed = 400.f;

				if (realSongSpeed < 25.f) 
					realSongSpeed = 25.f;

				// Save new speed, and save it to a file (for streamers to use as a custom on-screen overlay)
				RiffRepeater::SetSpeed(realSongSpeed, true);
				RiffRepeater::EnableTimeStretch();
				saveNewRRSpeedToFile = true;

				std::cout << "Triggered Mod: Song Speed set to " << realSongSpeed << "% which is equivalent to " << RiffRepeater::ConvertSpeed(realSongSpeed) << " Wwise RTPC." << std::endl;
			}
		}
	}

	// Update settings from GUI.
	// Done on GUI setting save.
	else if (msg == WM_COPYDATA) {
		COPYDATASTRUCT* pcds = (COPYDATASTRUCT*)lParam;
		if (pcds->dwData == 1)
		{
			std::string currMsg = (char*)pcds->lpData;
			std::cout << currMsg << std::endl;

			// GUI sent a "update" message. We need to re-read the INI.
			if (Contains(currMsg, "update")) {
				if (Contains(currMsg, "all"))
					Settings::UpdateSettings();
				else
					Settings::ParseSettingUpdate(currMsg);
			}

			// GUI sent a "WwiseEvent" message. We need to trigger a Voice-Over so the user can test their Sound Pack.
			else if (Contains(currMsg, "WwiseEvent")) {
				auto msgParts = Settings::SplitByWhitespace(currMsg);

				if (msgParts.size() == 2)
					VoiceOverControl::PlayVoiceOver(msgParts[1]);
			}

			// **Deprecated** Twitch Integration. Use CrowdControl.
			else if (Contains(currMsg, "TurboSpeed"))
				if (Contains(currMsg, "enable")) {
					RiffRepeater::EnableTimeStretch();
				}
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

	// Has this been ran before (AKA run only once, at startup)
	if (!ImGuiInit) {
		ImGuiInit = true;

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
		//ImGui::CaptureKeyboardFromApp(true);
		//ImGui::CaptureMouseFromApp(true);

		static std::string previewValue = "Select a device";
		if (ImGui::BeginCombo("MIDI devices", previewValue.c_str())) {
			for (int n = 0; n < Midi::NumberOfOutPorts; n++)
			{
				const bool is_selected = (selectedDevice == n);
				if (ImGui::Selectable(Midi::midiOutDevices[n].szPname, is_selected, ImGuiSelectableFlags_::ImGuiSelectableFlags_DontClosePopups))
					selectedDevice = n;

				if (is_selected) {
					previewValue = Midi::midiOutDevices[n].szPname;
					Midi::SelectedMidiOutDevice = n;
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

		// ImGUI Calibration menu.
		ImGui::Begin("Calibration");

		static float NewNoiseFloor = -59.3134f;
		static float NewToneBalance = -19.4793f;
		static float NewInputVolume = -3.22313f;
		static float NewInputVolumeReturn = -3.22313f;

		ImGui::SliderFloat("Noise Floor", &NewNoiseFloor, -100.f, 10.f);
		ImGui::SliderFloat("Tone Balance", &NewToneBalance, -100.f, 10.f);
		ImGui::SliderFloat("Input Volume", &NewInputVolume, -100.f, 10.f);
		ImGui::SliderFloat("Input Volume Return", &NewInputVolumeReturn, -100.f, 10.f);

		if (ImGui::Button("Calibrate"))
		{
			// Noise Floor
			Wwise::SoundEngine::SetRTPCValue("P1_NoiseFloor", NewNoiseFloor, 0x1234, 0, AkCurveInterpolation_Linear);
			Wwise::SoundEngine::SetRTPCValue("P1_NoiseFloor", NewNoiseFloor, AK_INVALID_GAME_OBJECT, 0, AkCurveInterpolation_Linear);

			// Tone Balance
			Wwise::SoundEngine::SetRTPCValue("Meter_Tone_Balance_Return", NewToneBalance, 0x1234, 0, AkCurveInterpolation_Linear);
			Wwise::SoundEngine::SetRTPCValue("Meter_Tone_Balance_Return", NewToneBalance, AK_INVALID_GAME_OBJECT, 0, AkCurveInterpolation_Linear);

			// Input Volume
			Wwise::SoundEngine::SetRTPCValue("P1_InputVol_Calibration", NewInputVolume, 0x1234, 0, AkCurveInterpolation_Linear);
			Wwise::SoundEngine::SetRTPCValue("P1_InputVol_Calibration", NewInputVolume, AK_INVALID_GAME_OBJECT, 0, AkCurveInterpolation_Linear);

			// Input Volume Return
			Wwise::SoundEngine::SetRTPCValue("P1_InputVol_Calibration_Return", NewInputVolumeReturn, 0x1234, 0, AkCurveInterpolation_Linear);
			Wwise::SoundEngine::SetRTPCValue("P1_InputVol_Calibration_Return", NewInputVolumeReturn, AK_INVALID_GAME_OBJECT, 0, AkCurveInterpolation_Linear);
		}

		ImGui::End();


		// ImGUI Microhones menu.
		ImGui::Begin("Microphones");

		static std::string previewMicrophone = "Select a Microphone";
		static std::string selectedMicrophone = "";
		static std::vector<std::string> microphones;

		if (microphones.size() == 0) {
			for (std::map<std::string, LPWSTR>::iterator it = AudioDevices::activeMicrophones.begin(); it != AudioDevices::activeMicrophones.end(); ++it) {
				microphones.push_back(it->first);
			}
		}
		
		// Drop-down list of all microphones.
		if (ImGui::BeginCombo("Microphones", previewMicrophone.c_str())) {
			for (int n = 0; n < microphones.size(); n++)
			{
				const bool is_selected = (selectedMicrophone == microphones.at(n));
				if (ImGui::Selectable(microphones.at(n).c_str(), is_selected, ImGuiSelectableFlags_::ImGuiSelectableFlags_DontClosePopups))
					selectedMicrophone = microphones.at(n);

				if (is_selected) {
					previewMicrophone = microphones.at(n);
				}
			}
			ImGui::EndCombo();
		}

		// Button for testing setting volume on the microphone.
		if (ImGui::Button("Random Volume"))
			AudioDevices::SetMicrophoneVolume(selectedMicrophone, rand() % 100);

		ImGui::End();

		// ImGUI Voicelines menu.
		ImGui::Begin("Voicelines");

		static std::string previewVoiceline = "Select a voiceline";
		static std::vector<VoiceOver> selectedVoiceOverList = VoiceOverControl::VO_ResultsScreens;

		// Drop-down list of all voice-overs in the selected Voice-Over list.
		if (ImGui::BeginCombo("Voicelines", previewVoiceline.c_str())) {
			for (int n = 0; n < selectedVoiceOverList.size(); n++)
			{
				const bool is_selected = (VoiceOverControl::selectedVoiceOver.EventName == selectedVoiceOverList.at(n).EventName);
				if (ImGui::Selectable(selectedVoiceOverList.at(n).Text.c_str(), is_selected, ImGuiSelectableFlags_::ImGuiSelectableFlags_DontClosePopups))
					VoiceOverControl::selectedVoiceOver = selectedVoiceOverList.at(n);

				if (is_selected) {
					previewVoiceline = selectedVoiceOverList.at(n).Text;
					VoiceOverControl::selectedVoiceOver = selectedVoiceOverList.at(n);
				}
			}
			ImGui::EndCombo();
		}

		// Play Button
		if (ImGui::Button("Play selected voiceline"))
			VoiceOverControl::PlayVoiceOver(VoiceOverControl::selectedVoiceOver);

		ImGui::End();
	}

	// Render ImGUI
	ImGui::EndFrame();
	ImGui::Render();
	ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());

	// Restart mouse and keyboard in game
	ImGui::CaptureKeyboardFromApp(false);
	ImGui::CaptureMouseFromApp(false);

	// Regenerate string colors when the user changes their string colors in the GUI.
	// Needed to have real-time textures.
	if (generateTexture) {
		D3D::GenerateTextures(pDevice, D3D::Strings);

		generateTexture = false;
	}

	int whiteText = 0xFFFFFFFF;
	Resolution WindowSize = MemHelpers::GetWindowSize();

	// Draw text on screen
	// NOTE: NEVER USE SET VALUES. Always do division of WindowSize width AND heigh so every resolution should have the text in around the same spot.
	if (D3DHooks::GameLoaded) {

		// Show Selected Volume mod.
		if (Settings::ReturnSettingValue("VolumeControlEnabled") == "on" && (MemHelpers::IsInSong() || AutomatedSelectedVolume)) {
			float volume = 0;
			RTPCValue_type type = RTPCValue_GameObject;
			Wwise::SoundEngine::Query::GetRTPCValue(mixerInternalNames[currentVolumeIndex].c_str(), AK_INVALID_GAME_OBJECT, &volume, &type);

			if (currentVolumeIndex != 0) {
				MemHelpers::DX9DrawText(
						drawMixerTextName[currentVolumeIndex] + std::to_string(static_cast<int>(volume)) + "%",
						whiteText,
						static_cast<int>(WindowSize.width / 96.0f),  // 20 pixels from left in 1920x1080 resolution
						static_cast<int>(WindowSize.height / 54.0f), // 20 pixels from top 
						static_cast<int>(WindowSize.width / 19.2f),  // 120 pixels from left
						static_cast<int>(WindowSize.height / 16.0f), // 120 pixels from top
						pDevice);
			}
		}

		// Show Song Timer mod.
		if ((D3DHooks::showSongTimerOnScreen && MemHelpers::SongTimer() != 0.f)) {
			MemHelpers::DX9DrawText(
					ConvertFloatTimeToStringTime(MemHelpers::SongTimer()),
					whiteText,
					static_cast<int>(WindowSize.width - WindowSize.width / 16.0f), // 120 pixels left from right edge in 1920x1080 resolution
					static_cast<int>(WindowSize.height / 54.0f),                   // 20 pixels from top
					static_cast<int>(WindowSize.width - WindowSize.width / 96.0f), // 20 left from right edge
					static_cast<int>(WindowSize.height / 16.0f),                   // 120 pixels from top
					pDevice,
					{ NULL, NULL },
					DT_RIGHT | DT_NOCLIP);
		}

		// Riff Repeater > 100% mod.
		if ((Settings::ReturnSettingValue("RRSpeedAboveOneHundred") == "on" && RiffRepeater::loggedCurrentSongID &&
		(MemHelpers::Contains(currentMenu, fastRRModes) || MemHelpers::Contains(currentMenu, scoreScreens))) || RiffRepeater::currentlyEnabled_Above100) {
			realSongSpeed = RiffRepeater::GetSpeed(true); // While this should almost always be the same value, the user might enable riff repeater, which could cause this number to be wrong.

			MemHelpers::DX9DrawText(
					"Song Speed: " + std::to_string(static_cast<int>(roundf(realSongSpeed))) + "%",
					whiteText,
					static_cast<int>(WindowSize.width / 2.0f - WindowSize.width / 38.4f), // 50 pixels left of center in 1920x1080 resolution
					static_cast<int>(WindowSize.height / 54.0f),                          // 20 pixels from top
					static_cast<int>(WindowSize.width / 2.0f + WindowSize.width / 38.4f), // 50 pixels right of center
					static_cast<int>(WindowSize.height / 16.0f),                          // 120 pixels from top
					pDevice,
					{ NULL, NULL },
					DT_CENTER | DT_NOCLIP);
		}

		// Show Current Note On Screen mod
		if (Settings::ReturnSettingValue("ShowCurrentNoteOnScreen") == "on" && GuitarSpeak::GetCurrentNoteName() != (std::string)"") {

			if (MemHelpers::IsInSong())
				MemHelpers::DX9DrawText(
					GuitarSpeak::GetCurrentNoteName(),
					whiteText,
					(int)(WindowSize.width / 5.5),		// 349 pixels left of the center in 1920x1080 resolution.
					(int)(WindowSize.height / 1.75),	// 617 pixels from the top
					(int)(WindowSize.width / 5.75),		// 334 pixels right of center
					(int)(WindowSize.height / 8),		// 135 pixels from the top
					pDevice);
			else // Show outside of the song at the top of the screen.
				MemHelpers::DX9DrawText(
					"Current Note: " + GuitarSpeak::GetCurrentNoteName(),
					whiteText,
					(int)(WindowSize.width / 3.87),		// 496 pixels left of the center in 1920x1080 resolution
					(int)(WindowSize.height / 30.85),	// 35 pixels from the top
					(int)(WindowSize.width / 4),		// 480 pixel right of the center
					(int)(WindowSize.height / 8),		// 135 pixels from the top
					pDevice);
		}

		// Show current tuning for Auto Tune via MIDI mod.
		if (Settings::ReturnSettingValue("AutoTuneForSong") == "on" && Settings::GetKeyBind("TuningOffsetKey") != NULL && MemHelpers::Contains(currentMenu, tuningMenus)) {
			MemHelpers::DX9DrawText(
				"Auto Tune For: " + Midi::GetTuningOffsetName(Midi::tuningOffset),
				whiteText, 
				(int)(WindowSize.width / 5.5),		// 349 pixels left of the center in 1920x1080 resolution
				(int)(WindowSize.height / 30.85),	// 35 pixels from the top
				(int)(WindowSize.width / 5.65),		// 339 pixels right of center
				(int)(WindowSize.height / 8),		// 135 pixels from the top
				pDevice);
		}

		// Looping mod.
		if (Settings::ReturnSettingValue("AllowLooping") == "on" && (loopStart != NULL || loopEnd != NULL)) {

			// Only enable looping in learn a song modes (learn a song & non-stop play)
			if (MemHelpers::Contains(currentMenu, learnASongModes)) {

				// Display loop start/end times
				MemHelpers::DX9DrawText(
						"Loop: " + ConvertFloatTimeToStringTime(loopStart) + " - " + ConvertFloatTimeToStringTime(loopEnd),
						whiteText,
						static_cast<int>(WindowSize.width / 2.0f - WindowSize.width / 38.4f), // 50 pixels left of center in 1920x1080 resolution
						static_cast<int>(WindowSize.height / 21.6f),                          // 50 pixels from top
						static_cast<int>(WindowSize.width / 2.0f + WindowSize.width / 38.4f), // 50 pixels right of center
						static_cast<int>(WindowSize.height / 7.2f),                           // 150 pixels from top
						pDevice,
						{ NULL, NULL },
						DT_CENTER | DT_NOCLIP);

				// Prevent the user from creating a loop that starts at a negative timestamp.
				if ((Settings::GetModSetting("LoopingLeadUp") / 1000.f) >= loopStart) { 
					roughLoopStart = 0.f;
				}
				else {
					roughLoopStart = loopStart - (Settings::GetModSetting("LoopingLeadUp") / 1000.f);
				}

				// If we are paused, reset the grey note timer.
				if (MemHelpers::Contains(currentMenu, lasPauseMenus)) {
					// Resets grey note timer to loopStart. This makes it so notes in the loop are not deactivated.
					// Deactivated notes are greyed out, and do not register with note detection.
					// As an added bonus the game also automatically adds a bit of lead time so the player has some time to prepare.
					if (MemHelpers::GetGreyNoteTimer() != loopStart) {
						MemHelpers::SetGreyNoteTimer(loopStart);
					}
				}

				// If not paused AND we are at the end of the loop, seek to the start of the loop.
				else if (loopStart != NULL && loopEnd != NULL && (MemHelpers::SongTimer() >= loopEnd)) {
					Wwise::SoundEngine::SeekOnEvent(std::string("Play_" + MemHelpers::GetSongKey()).c_str(), 0x1234, (AkTimeMs)(roughLoopStart * 1000), false);
				}
			}
			// Difference between learnASongModes & fastRRModes is the inclusion of RR. This means that this check is only gets the RR menus.
			else if (MemHelpers::Contains(currentMenu, fastRRModes)) {
				// Reset loopStart and loopEnd to NULL as the user wants to do a loop with RR, or is changing some settings.
				loopStart = NULL;
				loopEnd = NULL;
			}
		}

		// Regenerate Twitch Solid Note Color for a new color
		if (D3DHooks::regenerateUserDefinedTexture) {
			Color userDefColor = Settings::ConvertHexToColor(Settings::ReturnSettingValue("SolidNoteColor"));
			//unsigned int red = userDefColor.r * 255, green = userDefColor.g * 255, blue = userDefColor.b * 255;
			//D3D::GenerateSolidTexture(pDevice, &twitchUserDefinedTexture, D3DCOLOR_ARGB(255, red, green, blue));

			ColorList customColorList(16, userDefColor); 
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
/// Convert time stored as a float of seconds, to h:m:s
/// </summary>
/// <param name="timeInSeconds"> - Float containing number of seconds elapsed.</param>
/// <returns>std::string of time in "h:m:s" format.</returns>
std::string D3DHooks::ConvertFloatTimeToStringTime(float timeInSeconds) {
	int seconds = 0, minutes = 0, hours = 0;

	seconds = (int)timeInSeconds % 60;
	minutes = (int)(timeInSeconds / 60) % 60;
	hours = timeInSeconds / 3600;

	return std::to_string(hours) + "h:" + std::to_string(minutes) + "m:" + std::to_string(seconds) + "s";
}

/// <summary>
/// Handle Twitch Toggle Message.
/// </summary>
/// <param name="currMsg"> - Mod to Trigger.</param>
/// <param name="type"> - Should We Enable or Disable.</param>
/// <returns>BOOL. If effects triggered.</returns>
bool HandleMessage(std::string currMsg, std::string type) {
	
	// Twitch wants Rainbow String mod.
	if (Contains(currMsg, "RainbowStrings")) {
		if (type == "enable")
			ERMode::RainbowEnabled = true;
		else
			ERMode::RainbowEnabled = false;
	}

	// Twitch wants Drunk Mode.
	else if (Contains(currMsg, "DrunkMode")) {
		MemHelpers::ToggleDrunkMode(type == "enable");
		Settings::ParseTwitchToggle(currMsg, type);
	}
	
	// Twitch wants Solid Note colors.
	else if (Contains(currMsg, "SolidNotes")) {
		// Don't apply any effects if we haven't even been in a song yet
		if (!ERMode::ColorsSaved) 
			return false;

		if (type == "enable") {

			// For Random Colors
			if (Contains(currMsg, "Random")) {
				static std::uniform_real_distribution<> urd(0, 9);
				currentRandomTexture = (int)urd(rng);

				ERMode::customSolidColor.clear();
				ERMode::customSolidColor.insert(ERMode::customSolidColor.begin(), randomTextureColors[currentRandomTexture].begin(), randomTextureColors[currentRandomTexture].end());

				//if(twitchUserDefinedTexture != NULL) //determine why this crashes if you send multiple in a row
				//	twitchUserDefinedTexture->Release();

				twitchUserDefinedTexture = randomTextures[currentRandomTexture];
			}

			// If colors are not random, set colors which the user defined for this reward
			else { 
				Settings::ParseSolidColorsMessage(currMsg);
				D3DHooks::regenerateUserDefinedTexture = true;
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
	while (MemHelpers::Contains(effectName, enabledEffects) && !MemHelpers::IsInSong())
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
		if (MemHelpers::Contains(effectName, enabledEffects)) // JIC
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
				if (!MemHelpers::Contains(effectName, enabledEffects)) {
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

	// Proton / Wine Check.
	// We do NOT support linux. There is some issues with the D3D pointers.
	// This check is meant so if someone does load our mods on Linux, we won't just crash. Most mods will just not work.
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
	}

	oSetVertexShader = (tSetVertexShader)MemUtil::TrampHook((PBYTE)vTable[D3DInfo::SetVertexShader_Index], (PBYTE)D3DHooks::Hook_SetVertexShader, 7); // https://docs.microsoft.com/en-us/windows/win32/api/d3d9helper/nf-d3d9helper-idirect3ddevice9-setvertexshader
	oSetPixelShader = (tSetPixelShader)MemUtil::TrampHook((PBYTE)vTable[D3DInfo::SetPixelShader_Index], (PBYTE)D3DHooks::Hook_SetPixelShader, 7); // https://docs.microsoft.com/en-us/windows/win32/api/d3d9helper/nf-d3d9helper-idirect3ddevice9-setpixelshader
	oSetStreamSource = (tSetStreamSource)MemUtil::TrampHook((PBYTE)vTable[D3DInfo::SetStreamSource_Index], (PBYTE)D3DHooks::Hook_SetStreamSource, 7); // https://docs.microsoft.com/en-us/windows/win32/api/d3d9helper/nf-d3d9helper-idirect3ddevice9-setstreamsource
	oEndScene = (tEndScene)MemUtil::TrampHook((PBYTE)vTable[D3DInfo::EndScene_Index], (PBYTE)D3DHooks::Hook_EndScene, 7); // https://docs.microsoft.com/en-us/windows/win32/api/d3d9helper/nf-d3d9helper-idirect3ddevice9-endscene
	oDrawIndexedPrimitive = (tDrawIndexedPrimitive)MemUtil::TrampHook((PBYTE)vTable[D3DInfo::DrawIndexedPrimitive_Index], (PBYTE)D3DHooks::Hook_DIP, 5); // https://docs.microsoft.com/en-us/windows/win32/api/d3d9helper/nf-d3d9helper-idirect3ddevice9-drawindexedprimitive
	oDrawPrimitive = (tDrawPrimitive)MemUtil::TrampHook((PBYTE)vTable[D3DInfo::DrawPrimitive_Index], (PBYTE)D3DHooks::Hook_DP, 7); // https://docs.microsoft.com/en-us/windows/win32/api/d3d9helper/nf-d3d9helper-idirect3ddevice9-drawprimitive
}

/// <summary>
/// Presses Enter. Normally used in a loop to skip most of the login dialog. "Fork in the toaster" method
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
void UpdateSettings() {
	Settings::UpdateSettings();
	Sleep(500);
	CustomSongTitles::LoadSettings();
	Sleep(500);
	CustomSongTitles::HookSongListsKoko();
	Sleep(500);
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

	std::cout << _RSMODS_VERSION << std::endl; // Put version info in the output log.

	bool movedToExternalDisplay = false; // User wants to move the display to a specific location on boot.
	bool skipERSleep = false; // If using RR past 100%, remove the 1.5s sleep on ER mode, to stop flickering colors.
	bool forkInToasterNewProfile = false; // If Auto Load Profile has a specified profile, and we can't find it, then this will be true.

	// Initialize Functions
	D3DHooks::debug = debug;
	Offsets::Initialize();
	*(char*)0x013aefd9 = (char)0x60; // Patches out function in Rocksmith.
	Settings::Initialize();
	UpdateSettings();
	ERMode::Initialize();
	GUI();
	Midi::InitMidi();
	Midi::tuningOffset = Settings::GetModSetting("TuningOffset");
	AudioDevices::SetupMicrophones();
	BugPrevention::PreventPnPCrash();

	if (Settings::ReturnSettingValue("FixBrokenTones") == "on")
		BugPrevention::PreventStuckTone();

	if (Settings::ReturnSettingValue("FixOculusCrash") == "on")
		BugPrevention::PreventOculusCrash();

#ifdef _FIX_STORE
	MemUtil::PatchAdr((void*)Offsets::steamApiUri, "%s://localhost:5154/api/requests/%d,%s,%s", 42); // Proxy available here: https://github.com/ffio1/SteamAPIProxy
#endif

	// AltOutputSampleRate mod.
	// We have to do this early in execution as we need to change it before the audio engine starts up.
	if (Settings::ReturnSettingValue("AltOutputSampleRate") == "on" && Settings::GetModSetting("AlternativeOutputSampleRate") != 48000) {
		std::cout << "[!] Overriding Output Sample Rate to " << Settings::GetModSetting("AlternativeOutputSampleRate") << std::endl;
		AudioDevices::output_SampleRate = Settings::GetModSetting("AlternativeOutputSampleRate");
		AudioDevices::ChangeOutputSampleRate();
	}
	
	// Look to see if RS_ASIO applied the 2 RTC input bypass.
	// If they did, then we disregard the results from our version of the mod.
	bool rs_asio_BypassTwoRTC = MemUtil::ReadPtr(Offsets::ptr_twoRTCBypass) == 0x12fe9;
	std::cout << "RS_ASIO Bypass2RTC: " << std::boolalpha << rs_asio_BypassTwoRTC << std::endl;
	if (Settings::ReturnSettingValue("BypassTwoRTCMessageBox") == "on")
		MemUtil::PatchAdr((LPVOID)Offsets::ptr_twoRTCBypass, (LPVOID)Offsets::ptr_twoRTCBypass_patch, 6);

	// Patch x86 assembly for Riff Repeater speed logic to make it linear.
	if (Settings::ReturnSettingValue("LinearRiffRepeater") == "on")
		RiffRepeater::EnableLinearSpeeds();

#ifdef _WWISE_LOGS // Only use in a debug environment. Will fill your log with spam!
	Wwise::Logging::Init();
#endif

	// Allow the user to have a small amount of time to Alt+Tab while the game continues playing the audio.
	if (Settings::ReturnSettingValue("AllowAudioInBackground") == "on")
		VolumeControl::AllowAltTabbingWithAudio();	

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
			currentMenu = MemHelpers::GetCurrentMenu(); // This loads without checking if memory is safe... This can cause crashes if used when GameLoaded is false.

			// Override the default microphone volume.
			if (Settings::ReturnSettingValue("OverrideInputVolumeEnabled") == "on" && Settings::ReturnSettingValue("OverrideInputVolumeDevice") != "" && AudioDevices::GetMicrophoneVolume(Settings::ReturnSettingValue("OverrideInputVolumeDevice")) != Settings::GetModSetting("OverrideInputVolume"))
				AudioDevices::SetMicrophoneVolume(Settings::ReturnSettingValue("OverrideInputVolumeDevice"), Settings::GetModSetting("OverrideInputVolume"));

			// User originally wanted to NOT allow audio in the background, but they changed their mind, so we have to turn it on/
			if (Settings::ReturnSettingValue("AllowAudioInBackground") == "on" && !VolumeControl::allowedAltTabbingWithAudio)
				VolumeControl::AllowAltTabbingWithAudio();

			// User originally wanted to allow audio in the background, but they changed their mind, so we have to turn it off.
			else if (Settings::ReturnSettingValue("AllowAudioInBackground") == "off" && VolumeControl::allowedAltTabbingWithAudio) {
				VolumeControl::DisableAltTabbingWithAudio();
			}

			// If the bypass for the 2 RTC dialog was set by RS_ASIO, don't change it!
			if (!rs_asio_BypassTwoRTC) { 

				// User originally had BypassTwoRTCMessageBox on, but now they want it turned off.
				if (Settings::ReturnSettingValue("BypassTwoRTCMessageBox") == "off" && *(char*)Offsets::ptr_twoRTCBypass == Offsets::ptr_twoRTCBypass_patch[0]) 
					MemUtil::PatchAdr((LPVOID)Offsets::ptr_twoRTCBypass, (LPVOID)Offsets::ptr_twoRTCBypass_original, 6);

				// User originally had BypassTwoRTCMessageBox off, but now they want it turned on.
				else if (Settings::ReturnSettingValue("BypassTwoRTCMessageBox") == "on" && *(char*)Offsets::ptr_twoRTCBypass == Offsets::ptr_twoRTCBypass_original[0]) 
					MemUtil::PatchAdr((LPVOID)Offsets::ptr_twoRTCBypass, (LPVOID)Offsets::ptr_twoRTCBypass_patch, 6);
			}

			// User wants NSP timer changed, and the time limit is not what the user set.
			if (Settings::ReturnSettingValue("UseCustomNSPTimer") == "on" && MemHelpers::GetNonStopPlayTimer() != (Settings::GetModSetting("CustomNSPTimeLimit") / 1000.0))
				MemHelpers::SetNonStopPlayTimer(Settings::GetModSetting("CustomNSPTimeLimit") / 1000.0);

			// The user originally wanted NSP timer changed, but now they disabled the mod.
			if (Settings::ReturnSettingValue("UseCustomNSPTimer") == "off" && MemHelpers::GetNonStopPlayTimer() != DefaultNSPTimeLimit)
				MemHelpers::SetNonStopPlayTimer(DefaultNSPTimeLimit);

			// User had Linear RR off, but now they want it turned on.
			if (Settings::ReturnSettingValue("LinearRiffRepeater") == "on" && !RiffRepeater::currentlyEnabled_LinearRR) 
				RiffRepeater::EnableLinearSpeeds();

			// User had Linear RR on, but now they want it turned off.
			else if (Settings::ReturnSettingValue("LinearRiffRepeater") == "off" && RiffRepeater::currentlyEnabled_LinearRR) 
				RiffRepeater::DisableLinearSpeeds();

			// Scan for MIDI devices for Automated Tuning / True-Tuning
			if (!Midi::scannedForMidiDevices && Settings::ReturnSettingValue("AutoTuneForSong") == "on") {
				Midi::scannedForMidiDevices = true;
				Midi::ReadMidiSettingsFromINI(Settings::ReturnSettingValue("ChordsMode"), Settings::GetModSetting("TuningPedal"), Settings::ReturnSettingValue("AutoTuneForSongDevice"), Settings::ReturnSettingValue("MidiInDevice"));
			}

			// Scan for MIDI In devices.
			if (!Midi::attemptedToDetachMidiInThread && Settings::ReturnSettingValue("MidiInDevice") != "") {
				Midi::attemptedToDetachMidiInThread = true;
				Midi::FindMidiInDevices(Settings::ReturnSettingValue("MidiInDevice")); // Just in-case the user has AutoTuneForSong off but MidiInDevice selected.
				std::thread(Midi::ListenToMidiInThread).detach();
			}

			// If User Is Entering / In Lesson Mode

			LessonMode = MemHelpers::Contains(currentMenu, lessonModes);

			/// Always on Mods (If the user specifies them to be on)

			// Remove Headstock (Always Off)
			if (Settings::ReturnSettingValue("RemoveHeadstockEnabled") == "on" && Settings::ReturnSettingValue("RemoveHeadstockWhen") == "startup")
				RemoveHeadstockInThisMenu = true; // In this case, the user always wants to remove the headstock. This value should never turn to false in this mode.

			// Toggle Loft (In Song / Always Off). Turn off in Lesson Mode (or the videos won't appear). Emulate effect with GreenScreenWall.
			if (LessonMode && Settings::ReturnSettingValue("ToggleLoftEnabled") == "on" && Settings::ReturnSettingValue("ToggleLoftWhen") != "manual") {
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
			if (!SkylineOff && Settings::ReturnSettingValue("RemoveSkylineEnabled") == "on" && Settings::ReturnSettingValue("ToggleSkylineWhen") == "startup")
				toggleSkyline = true;

			// Remove Lyrics (Always Off)
			if (!RemoveLyrics && Settings::ReturnSettingValue("RemoveLyricsWhen") == "startup")
				RemoveLyrics = true;

			// MIDI Auto Tuning / Auto True-Tuning (In Tuner)
			if (MemHelpers::Contains(currentMenu, preSongTuners) && Settings::ReturnSettingValue("AutoTuneForSong") == "on" && Settings::ReturnSettingValue("AutoTuneForSongWhen") == "tuner" && !Midi::alreadyAttemptedTuningInTuner && !Midi::alreadyAutomatedTuningInThisSong) {
				Midi::AttemptTuningInTuner();
				skipERSleep = true;
			}	
			/// If User Is Entering Song
			if (MemHelpers::IsInSong()) {
				GuitarSpeakPresent = false;
				AttemptedERInTuner = false;
				UseERInTuner = false;

				// We are in a song we've haven't seen in this play session. Log its Id so we can prep for the Riff Repeater > 100% mod.
				if (RiffRepeater::readyToLogSongID) {
					if (RiffRepeater::LogSongID(MemHelpers::GetSongKey()))
						RiffRepeater::readyToLogSongID = false;
				}

				// Enable riff repeater time stretching
				if (Settings::ReturnSettingValue("RRSpeedAboveOneHundred") == "on")
					RiffRepeater::EnableTimeStretch();

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

				// MIDI Auto Tuning / Auto True-Tuning (In Song)
				if (Settings::ReturnSettingValue("AutoTuneForSong") == "on" && !Midi::alreadyAutomatedTuningInThisSong && (Settings::ReturnSettingValue("AutoTuneForSongWhen") == "tuner" || (Settings::ReturnSettingValue("AutoTuneForSongWhen") == "manual" && Midi::userWantsToUseAutoTuning)))
					Midi::AutomateTuning();
					
				// Show Song Timer (In Song)
				if (!AutomatedSongTimer && Settings::ReturnSettingValue("ShowSongTimerEnabled") == "on" && Settings::ReturnSettingValue("ShowSongTimerWhen") == "automatic") {
					AutomatedSongTimer = true;
					showSongTimerOnScreen = true;
				}
				
				// Show Selected Volume (In Song)
				if (!AutomatedSelectedVolume && Settings::ReturnSettingValue("VolumeControlEnabled") == "on" && Settings::ReturnSettingValue("ShowSelectedVolumeWhen") == "song") {
					currentVolumeIndex = 1;
					AutomatedSelectedVolume = true;
				}

				// Attempt to turn on Extended Range
				if (!AttemptedERInThisSong) {
					if (!skipERSleep)
						Sleep(1500); // Tuning takes a second, or so, to get set by the game. We use this to make sure we have the right tuning numbers. Otherwise, we would never get ER mode to turn on properly.
					UseERExclusivelyInThisSong = MemHelpers::IsExtendedRangeSong();
					UseEROrColorsInThisSong = (Settings::ReturnSettingValue("ExtendedRangeEnabled") == "on" && UseERExclusivelyInThisSong || Settings::GetModSetting("CustomStringColors") == 2 || (Settings::ReturnSettingValue("SeparateNoteColors") == "on" && Settings::GetModSetting("SeparateNoteColorsMode") != 1));
					AttemptedERInThisSong = true;
				}
			}

			/// If User Is Exiting A Song / In A Menu

			else {

				/// Turn on Extended Range In Tuner
				if (MemHelpers::Contains(currentMenu, preSongTuners)) {
					if (!AttemptedERInTuner) { // The reason this is a separate if statement is so that the else statement isn't voiding the correct menu.
						if (!skipERSleep)
							Sleep(1500); // Tuning takes a second, or so, to get set by the game. We use this to make sure we have the right tuning numbers. Otherwise, we would never get ER mode to turn on properly.
						AttemptedERInTuner = true;
						UseERInTuner = MemHelpers::IsExtendedRangeTuner();
					}
				}
				else {
					AttemptedERInTuner = false;
					UseERInTuner = false;
				}

				// Turn off Looping mod
				if (Settings::ReturnSettingValue("AllowLooping") == "on") {
					if (loopStart != NULL)
						loopStart = NULL;

					if (loopEnd != NULL)
						loopEnd = NULL;
				}

				// Turn off Riff Repeater Speed above 100%
				if (!MemHelpers::Contains(currentMenu, scoreScreens) && RiffRepeater::currentlyEnabled_Above100) {
					RiffRepeater::DisableTimeStretch();
				}

				// Turn off Extended Range
				if (AttemptedERInThisSong) {
					UseERExclusivelyInThisSong = false;
					UseEROrColorsInThisSong = false;
					AttemptedERInThisSong = false;
				}
				
				// Turn off Show Song Timer (In Song)
				if (AutomatedSongTimer && Settings::ReturnSettingValue("ShowSongTimerEnabled") == "on" && Settings::ReturnSettingValue("ShowSongTimerWhen") == "automatic") {
					AutomatedSongTimer = false;
					showSongTimerOnScreen = false;
				}	

				// Turn off Show Selected Volume (In Song)
				if (AutomatedSelectedVolume && Settings::ReturnSettingValue("VolumeControl") == "on" && Settings::ReturnSettingValue("ShowSelectedVolumeWhen") == "song") {
					currentVolumeIndex = 0;
					AutomatedSelectedVolume = false;
				}

				// Turn off MIDI Auto Tuning / Auto True-Tuning
				if ((Midi::alreadyAutomatedTuningInThisSong || Midi::alreadyAttemptedTuningInTuner) && !MemHelpers::Contains(currentMenu, preSongTuners)) {
					Midi::RevertAutomatedTuning();
					Midi::alreadyAttemptedTuningInTuner = false;
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
				if (SkylineOff && Settings::ReturnSettingValue("RemoveSkylineEnabled") == "on" && Settings::ReturnSettingValue("ToggleSkylineWhen") == "song") {
					toggleSkyline = true;
					DrawSkylineInMenu = true;
				}

				// Turn on Guitar Speak
				if (!GuitarSpeakPresent && Settings::ReturnSettingValue("GuitarSpeak") == "on") { // Guitar Speak
					GuitarSpeakPresent = true;
					if (!GuitarSpeak::RunGuitarSpeak()) // If we are in a menu where we don't want to read bad values
						GuitarSpeakPresent = false;
				}

				// Disable song previews
				if (Settings::ReturnSettingValue("SongPreviews") == "on") {
					if (!VolumeControl::disabledSongPreviewAudio)
						VolumeControl::DisableSongPreviewAudio();
				}

				// User originally wanted song previews off, but now wants them on.
				else if (VolumeControl::disabledSongPreviewAudio)
					VolumeControl::EnableSongPreviewAudio();

				// Reset Headstock Cache (so we aren't running the same textures over and over again)
				if (Settings::ReturnSettingValue("RemoveHeadstockEnabled") == "on" && (!(MemHelpers::Contains(currentMenu, tuningMenus)) || currentMenu == "MissionMenu"))
					resetHeadstockCache = true;
			}
			
			/// "Other" menus. These will normally state what menus they need to be in.

			// Screenshot Scores
			if (Settings::ReturnSettingValue("ScreenShotScores") == "on" && MemHelpers::Contains(currentMenu, scoreScreens))
				TakeScreenshot();
			else
				takenScreenshotOfThisScreen = false;

			// If the current menu is not the same as the previous menu and if it's one of menus where you tune your guitar (i.e. headstock is shown), reset the cache because user may want to change the headstock style
			if (previousMenu != currentMenu && MemHelpers::Contains(currentMenu, tuningMenus)) {
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

			// Change Current Menu status to the current menu while the game is loading.
			// This is the safe version of checking the current menu.
			// It is only used while the game boots, else the game may crash.
			currentMenu = MemHelpers::GetCurrentMenu(true); 

			// Have We Loaded? Or has the user opened a new user profile?
			// This prevents the user from being locked in a loop.
			if (currentMenu == "MainMenu" || currentMenu == "PlayedRS1Select") 
				GameLoaded = true;

			// Set buffer settings if the user uses an alternative sample rate on their audio output.
			if (Settings::ReturnSettingValue("AltOutputSampleRate") == "on" && Settings::GetModSetting("AlternativeOutputSampleRate") != 48000 && *(int*)Offsets::ptr_sampleRateBuffer != 5 && *(int*)Offsets::ptr_sampleRateBuffer != 2) {
				*(int*)Offsets::ptr_sampleRateSize = 2;
				*(int*)Offsets::ptr_sampleRateBuffer = 128;
			}
				
			// Auto Load Profile. AKA "Fork in the toaster".
			if (Settings::ReturnSettingValue("ForceProfileEnabled") == "on" && !(MemHelpers::Contains(currentMenu, dontAutoEnter)) && !forkInToasterNewProfile) {
				// If the user user says "I want to always load this profile"
				if (Settings::ReturnSettingValue("ProfileToLoad") != "" && currentMenu == (std::string)"ProfileSelect") {
					selectedUser = MemHelpers::CurrentSelectedUser();
					if (selectedUser == Settings::ReturnSettingValue("ProfileToLoad")) // The profile we're looking for
						AutoEnterGame();
					else if (selectedUser == (std::string)"New profile") { // Yeah, the profile they're looking for doesn't exist :(
						std::cout << "(Auto Load) Invalid Profile Name" << std::endl;
						forkInToasterNewProfile = true;
					}
					else { // Not the profile we're looking for. Move down 1.
						PostMessage(FindWindow(NULL, L"Rocksmith 2014"), WM_KEYDOWN, VK_DOWN, 0);
						Sleep(30);
						PostMessage(FindWindow(NULL, L"Rocksmith 2014"), WM_KEYUP, VK_DOWN, 0);
					} 
				}
				// User doesn't care what profile we select, just select the first / top one.
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
	std::thread(RiffRepeaterThread).detach(); // RR Speed Above 100% Log

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
	// Init boos
	bool debugLogPresent = std::ifstream("RSMods_debug.txt").good();
	std::ofstream clearDebugLog = std::ofstream("RSMods_debug.txt");

	switch (dwReason) {
	case DLL_PROCESS_ATTACH:
		// Setup logging system
		FILE* streamRead, *streamWrite;

		// If running a debug build, give us a console.
		if (debug) 
			AllocConsole();

		// Connect stdin, stdout, and stderr to our console / log.
		freopen_s(&streamRead, "CONIN$", "r", stdin);
		freopen_s(&streamWrite, "CONOUT$", "w", stdout);
		freopen_s(&streamWrite, "CONOUT$", "w", stderr);

		// Dump console to log (mainly for debugging release build issues)
		if (debugLogPresent) { 

			// Clear log so it isn't full of junk from the last launch
			clearDebugLog.open("RSMods_debug.txt", std::ofstream::out | std::ofstream::trunc);
			clearDebugLog.close();

			// Attach log to stdout and stderr.
			// Doesn't work on debug clients as it takes over the console output.
			if (!debug) {
				FILE* debugLog;
				freopen_s(&debugLog, "RSMods_debug.txt", "w", stdout);
				freopen_s(&debugLog, "RSMods_debug.txt", "w", stderr);
			}
		}

		DisableThreadLibraryCalls(hModule); // Disables the DLL_THREAD_ATTACH and DLL_THREAD_DETACH notifications. | https://docs.microsoft.com/en-us/windows/win32/api/libloaderapi/nf-libloaderapi-disablethreadlibrarycalls
		Proxy::Init(); // Proxy all real XInput commands to the actual xinput1_3.dll.
		Initialize(); // Inject our mod code.
		return TRUE;
	case DLL_PROCESS_DETACH:
		Proxy::Shutdown(); // Kill Proxy to xinput1_3.dll

		// Shutdown ImGUI
		if (ImGuiInit)
		{
			ImGui_ImplWin32_Shutdown();
			ImGui_ImplDX9_Shutdown();
			ImGui::DestroyContext();
		}
		return TRUE;
	}
	return TRUE;
}
