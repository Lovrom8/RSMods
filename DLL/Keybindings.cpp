#include "stdafx.h"
#include "Keybindings.hpp"
#include "Mods/DropPedal/DropPedalInput.hpp"

namespace Keybindings {
	std::map<std::string, ModCommand, std::less<>> keyUpCommands;
	std::map<std::string, ModCommand, std::less<>> keyDownCommands;

	void HandleTuningOffset()
	{
		bool isCtrlPressed = GetAsyncKeyState(VK_CONTROL) & 0x8000;

		Midi::tuningOffset += isCtrlPressed ? -1 : 1;
		Midi::tuningOffset = std::clamp(Midi::tuningOffset, -3, 12);

		LOG_INFO("Triggered Mod Setting: Tuning Offset is now set to " << Midi::tuningOffset << std::endl);
	}

	void HandleRewind()
	{
		// SongTimer is stored in seconds, while RewindBy is stored in milliseconds.
		// We need milliseconds to send to Wwise, so change SongTimer to milliseconds, then subtract the Rewind value.
		auto seekTo = static_cast<AkTimeMs>((SongTimer::SongTimer() * 1000) - Settings::GetModSetting("RewindBy") - Settings::GetModSetting("RewindLeadup"));

		// RewindBy is greater than the amount of time we've been in the song.
		// Reset seekTo to 0 to prevent seeking to a negative time.
		if (seekTo < 0) seekTo = 0;

		// Send event to Wwise to rewind the song.
		// Or more accurately, move to the seek time since Wwise doesn't have a rewind function.
		Wwise::SoundEngine::SeekOnEvent(std::string("Play_" + GameState::GetSongKey()).c_str(), 0x1234, seekTo, false);

		// Tell Rocksmith to make all notes before the section we want the user to play to be greyed out.
		// While this isn't absolutely necessary, it is best to have this run just in case.
		// Our seek time needs to be stored as milliseconds when sending to Wwise, but we need to have it in seconds when setting the GreyNoteTimer.
		AkTimeMs greyNoteTimerMs = seekTo - Settings::GetModSetting("RewindLeadup");
		SongTimer::SetGreyNoteTimer(greyNoteTimerMs / 1000.f);

		LOG_INFO("(REWIND) Seeked to " << seekTo << "ms." << std::endl);
	}

	void SetLoopStartingPoint()
	{
		if (GetAsyncKeyState(VK_CONTROL) & 0x8000) {
			loopStart = loopEnd = NULL;
		}
		else {
			loopStart = SongTimer::SongTimer();
			if (loopEnd <= loopStart) {
				loopEnd = NULL;
			}
		}
	}

	void SetLoopEndingPoint()
	{
		if (GetAsyncKeyState(VK_CONTROL) & 0x8000) {
			loopEnd = NULL;
		}
		else {
			loopEnd = SongTimer::SongTimer();

			// If end point of the loop comes at the same time as, or before, the start of the loop, reset it to 0.
			if (loopEnd <= loopStart) {
				loopEnd = NULL;
			}
		}
	}

	void HandleMutePlayer1()
	{
		if (VolumeControl::player1Muted)
		{
			VolumeControl::UnmutePlayer();
		}
		else
		{
			VolumeControl::MutePlayer();
		}

		GameOverlay::displayCurrentVolume = true;
		GameOverlay::displayVolumeStartTime = std::chrono::steady_clock::now();
		GameOverlay::currentVolumeIndex = 2;
	}

	void HandleMutePlayer2()
	{
		if (VolumeControl::player2Muted)
		{
			VolumeControl::UnmutePlayer(true);
		}
		else
		{
			VolumeControl::MutePlayer(true);
		}

		GameOverlay::displayCurrentVolume = true;
		GameOverlay::displayVolumeStartTime = std::chrono::steady_clock::now();
		GameOverlay::currentVolumeIndex = 3;
	}

	void HandleRRSpeed()
	{
		float realSongSpeed = RiffRepeater::GetSpeed(true);
		bool isCtrlPressed = GetAsyncKeyState(VK_CONTROL) & 0x8000;
		auto interval = (float)Settings::GetModSetting("RRSpeedInterval");
		realSongSpeed += isCtrlPressed ? -interval : interval;

		// Set limits to the speed
		// Cap at 400. Plugin only goes down to 25. 10000 / 25 = 400.
		// Cap at 25. Plugin only goes up to 400. 10000 / 400 = 25.
		realSongSpeed = std::clamp(realSongSpeed, 25.f, 400.f);

		// Save new speed, and save it to a file (for streamers to use as a custom on-screen overlay)
		RiffRepeater::SetSpeed(realSongSpeed, true);
		RiffRepeater::EnableTimeStretch();
		RiffRepeater::saveNewRRSpeedToFile = true;

		LOG_INFO("Triggered Mod: Song Speed set to " << realSongSpeed << "%" << std::endl);
	}

	struct VolumeControlInfo
	{
		std::string keyBindName;
		std::string volumeChannel;
		int overlayIndex;
	};


	void HandleVolumeKeyPress(int keyPressed)
	{
		if (Settings::ReturnSettingValue("VolumeControlEnabled") != "on") {
			return;
		}

		const std::vector<VolumeControlInfo> controls = {
			{"MasterVolumeKey",     "Master_Volume", 0},
			{"SongVolumeKey",       "Mixer_Music",   1},
			{"Player1VolumeKey",    "Mixer_Player1", 2},
			{"Player2VolumeKey",    "Mixer_Player2", 3},
			{"MicrophoneVolumeKey", "Mixer_Mic",     4},
			{"VoiceOverVolumeKey",  "Mixer_VO",      5},
			{"SFXVolumeKey",        "Mixer_SFX",     6}
		};

		for (const auto& control : controls) {
			if (keyPressed == Settings::GetKeyBind(control.keyBindName)) {
				if (GetAsyncKeyState(VK_CONTROL) & 0x8000) {
					VolumeControl::DecreaseVolume(Settings::GetModSetting("VolumeControlInterval"), control.volumeChannel);
				}
				else {
					VolumeControl::IncreaseVolume(Settings::GetModSetting("VolumeControlInterval"), control.volumeChannel);
				}

				GameOverlay::displayCurrentVolume = true;
				GameOverlay::displayVolumeStartTime = std::chrono::steady_clock::now();
				GameOverlay::currentVolumeIndex = control.overlayIndex;

				break;
			}
		}
	}

	void HandleKeyUp(WPARAM keyPressed)
	{
		if (!GameState::GameLoaded) return; // Game must not be on the startup videos or it will crash
		DispatchCommand(keyPressed, keyUpCommands);

		// Control + A. Force us to read the Settings from the INI again, to renew our cached values.
		if (keyPressed == 0x41 && (GetAsyncKeyState(VK_CONTROL) & 0x8000)) {
			Settings::UpdateSettings();
			LOG_INFO("Triggered Setting Update" << std::endl);
		}

		// Auto Tuning via MIDI mod. 
		// Checks if we are in a tuning menu, and the user tried to skip tuning.
		if (Settings::ReturnSettingValue("AutoTuneForSongWhen") == "manual" && GameState::Menus::IsInTuningMenus() && keyPressed == VK_DELETE) {
			Midi::userWantsToUseAutoTuning = true;
		}

		if (D3DHooks::debug && keyPressed == VK_BACK) {
			Menu::menuEnabled = !Menu::menuEnabled;
		}
	}

	void HandleKeyDown(WPARAM keyPressed)
	{
		if (!GameState::GameLoaded) return;
		DispatchCommand(keyPressed, keyDownCommands);

		HandleVolumeKeyPress(keyPressed);
	}

	void UpdateSettingsOnGUIChange(LPARAM lParam)
	{
		auto pcds = (COPYDATASTRUCT*)lParam;
		if (pcds->dwData == 1)
		{
			std::string currMsg = (char*)pcds->lpData;
			LOG_INFO(currMsg << std::endl);

			if (Contains(currMsg, "update")) {
				if (Contains(currMsg, "all"))
					Settings::UpdateSettings();
				else
					Settings::ParseSettingUpdate(currMsg);
			}
			else if (Contains(currMsg, "WwiseEvent")) {
				auto msgParts = Settings::SplitByWhitespace(currMsg);

				if (msgParts.size() == 2)
					VoiceOverControl::PlayVoiceOver(msgParts[1]);
			}
			else if (Contains(currMsg, "TurboSpeed")) // Deprecated - use CrowdControl
			{
				if (Contains(currMsg, "enable"))
					RiffRepeater::EnableTimeStretch();
				else
					RiffRepeater::DisableTimeStretch();
			}
			else if (Contains(currMsg, "enable"))
				Twitch::effectQueue.push_back(currMsg);
			else if (Contains(currMsg, "Reconnect"))
				CrowdControl::StartServerLoop();
		}
	}

	void DispatchCommand(WPARAM keyPressed, const std::map<std::string, ModCommand, std::less<>>& commands)
	{
		for (const auto& [key, value] : commands) {

			if (keyPressed == Settings::GetKeyBind(key)) {
				if (!value.condition()) return;

				value.action();

				if (!value.logMessage.empty()) {
					LOG_INFO("Triggered: " << value.logMessage << std::endl);
				}

				return;
			}
		}
	}

	void InitializeCommands()
	{
		keyUpCommands["ToggleLoftKey"] =
		{
			[] { return Settings::ReturnSettingValue("ToggleLoftEnabled") == "on"; },
			[] { Loft::ToggleLoft(); },
			"Toggle Loft"
		};
		keyUpCommands["ShowSongTimerKey"] =
		{
			[] { return Settings::ReturnSettingValue("ShowSongTimerEnabled") == "on"; },
			[] { D3DHooks::showSongTimerOnScreen = !D3DHooks::showSongTimerOnScreen; },
			"Show Song Timer"
		};
		keyUpCommands["ForceReEnumerationKey"] =
		{
			[] { return Settings::ReturnSettingValue("ForceReEnumerationEnabled") == "manual"; },
			[] { Enumeration::ForceEnumeration(); },
			"Force Enumeration"
		};
		keyUpCommands["RainbowStringsKey"] =
		{
			[] { return Settings::ReturnSettingValue("RainbowStringsEnabled") == "on"; },
			[] { ERMode::ToggleRainbowMode();
				 if (!ERMode::RainbowEnabled) ERMode::ResetAllStrings(); },
			"Rainbow Strings"
		};
		keyUpCommands["TuningOffsetKey"] =
		{
			[] { return Settings::ReturnSettingValue("AutoTuneForSong") == "on"; },
			HandleTuningOffset,
			""
		};
		keyUpCommands["RewindKey"] =
		{
			[] { return Settings::ReturnSettingValue("AllowRewind") == "on" && GameState::Menus::IsInLASPlayingModes(); },
			HandleRewind,
			""
		};
		keyDownCommands["LoopStartKey"] =
		{
			[] { return Settings::ReturnSettingValue("AllowLooping") == "on" && GameState::Menus::IsInModesWithAllowedFastRiffRepeater(); },
			SetLoopStartingPoint,
			"Loop Start Point Set"
		};
		keyDownCommands["LoopEndKey"] =
		{
			[] { return Settings::ReturnSettingValue("AllowLooping") == "on" && GameState::Menus::IsInModesWithAllowedFastRiffRepeater(); },
			SetLoopEndingPoint,
			"Loop End Point Set"
		};
		keyUpCommands["RemoveLyricsKey"] =
		{
			[] { return Settings::ReturnSettingValue("RemoveLyricsWhen") == "manual"; },
			[] { D3DHooks::RemoveLyrics = !D3DHooks::RemoveLyrics; },
			"Remove Lyrics"
		};
		keyUpCommands["ToggleExtendedRangeKey"] =
		{
			[] { return true; },
			[] { ERMode::UseERExclusivelyInThisSong = !ERMode::UseERExclusivelyInThisSong; GameState::ToggleCB(ERMode::UseERExclusivelyInThisSong); },
			"Toggle Extended Range"
		};
		keyUpCommands["DropPedalPitchDownKey"] =
		{
			[] { return true; },
			[] { DropPedalInput::AdjustTarget(-1); },
			""
		};
		keyUpCommands["DropPedalPitchUpKey"] =
		{
			[] { return true; },
			[] { DropPedalInput::AdjustTarget(1); },
			""
		};
		keyUpCommands["DropPedalToggleKey"] =
		{
			[] { return true; },
			DropPedalInput::ToggleEnabled,
			""
		};
		keyUpCommands["DropPedalBaseTuningDownKey"] =
		{
			[] { return true; },
			[] { DropPedalInput::AdjustBaseTuning(-1); },
			""
		};
		keyUpCommands["DropPedalBaseTuningUpKey"] =
		{
			[] { return true; },
			[] { DropPedalInput::AdjustBaseTuning(1); },
			""
		};
		keyUpCommands["MutePlayer1Key"] =
		{
			[] { return true; },
			[] { HandleMutePlayer1(); },
			"Mute Player 1"
		};
		keyUpCommands["MutePlayer2Key"] =
		{
			[] { return true; },
			[] { HandleMutePlayer2(); },
			"Mute Player 2"
		};
		keyUpCommands["DisplayMixerKey"] =
		{
			[] { return true; },
			[] { GameOverlay::displayMixer = false; }, // Hide the mixer if it is not actively being pressed
			""
		};
		keyUpCommands["ChangedSelectedVolumeKey"] =
		{
			[] { return Settings::ReturnSettingValue("VolumeControlEnabled") == "on"; },
			[] {
					GameOverlay::currentVolumeIndex++;
					if (GameOverlay::currentVolumeIndex > (GameOverlay::mixerInternalNames.size() - 1)) // There are only so many values we know we can edit, so loop back.
						GameOverlay::currentVolumeIndex = 0;},
			""
		};

		keyDownCommands["RRSpeedKey"] =
		{
			[] { return Settings::ReturnSettingValue("RRSpeedAboveOneHundred") == "on" && GameState::Menus::IsInModesWithAllowedFastRiffRepeater() && RiffRepeater::loggedCurrentSongID; },
			HandleRRSpeed,
			""
		};
		keyDownCommands["DisplayMixerKey"] =
		{
			[] { return Settings::ReturnSettingValue("VolumeControlEnabled") == "on"; },
			[] { GameOverlay::displayMixer = true; },
			"Display Mixer"
		};
	}
}
