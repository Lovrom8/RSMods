#pragma once

#include <atomic>
#include <functional>
#include <map>
#include <string>
#include <string_view>
#include <vector>

#include "RSColor.h"
#include "Framework/SettingsStore.hpp"

namespace Settings {
	// Mods reference these (through ModContext) instead of raw string literals, 
	// so a typo becomes a compile error rather than a silently-missing setting.
	// NOTE: a setting's on-disk INI key can differ from its in-code key (e.g. "ToggleLoft" 
	// on disk vs "ToggleLoftEnabled" in code), so the reader's INI-name argument stays
	// its own literal and is not one of these constants.
	namespace Setting {
		// Riff Repeater
		inline constexpr char AllowRewind[]             = "AllowRewind";
		inline constexpr char AllowLooping[]            = "AllowLooping";
		inline constexpr char RewindBy[]                = "RewindBy";
		inline constexpr char RewindLeadup[]            = "RewindLeadup";
		inline constexpr char RRSpeedInterval[]         = "RRSpeedInterval";
		inline constexpr char RRSpeedAboveOneHundred[]  = "RRSpeedAboveOneHundred";
		inline constexpr char LinearRiffRepeater[]      = "LinearRiffRepeater";

		// Loft
		inline constexpr char ToggleLoftEnabled[]       = "ToggleLoftEnabled";
		inline constexpr char ToggleLoftWhen[]          = "ToggleLoftWhen";

		// Remove Skyline
		inline constexpr char RemoveSkylineEnabled[]    = "RemoveSkylineEnabled";
		inline constexpr char ToggleSkylineWhen[]       = "ToggleSkylineWhen";

		// Remove Headstock
		inline constexpr char RemoveHeadstockEnabled[]  = "RemoveHeadstockEnabled";
		inline constexpr char RemoveHeadstockWhen[]     = "RemoveHeadstockWhen";

		// Show Song Timer
		inline constexpr char ShowSongTimerEnabled[]    = "ShowSongTimerEnabled";
		inline constexpr char ShowSongTimerWhen[]       = "ShowSongTimerWhen";

		// Enumeration
		inline constexpr char ForceReEnumerationEnabled[] = "ForceReEnumerationEnabled";
		inline constexpr char CheckForNewSongsInterval[]  = "CheckForNewSongsInterval";

		// Remove Lyrics
		inline constexpr char RemoveLyricsEnabled[]     = "RemoveLyrics";
		inline constexpr char RemoveLyricsWhen[]        = "RemoveLyricsWhen";

		// In-game settings window
		inline constexpr char SettingsMenuEnabled[]     = "SettingsMenuEnabled";

		// Volume
		inline constexpr char VolumeControlEnabled[]    = "VolumeControlEnabled";
		inline constexpr char VolumeControlInterval[]   = "VolumeControlInterval";

		// MIDI
		inline constexpr char AutoTuneForSong[]         = "AutoTuneForSong";
		inline constexpr char AutoTuneForSongWhen[]     = "AutoTuneForSongWhen";
		inline constexpr char AutoTuneForSongDevice[]   = "AutoTuneForSongDevice";
		inline constexpr char MidiInDevice[]            = "MidiInDevice";
		inline constexpr char ChordsMode[]              = "ChordsMode";
		inline constexpr char TuningPedal[]             = "TuningPedal";
		inline constexpr char TuningOffset[]			= "TuningOffset";

		// AutoTune for Software (MIDI)
		inline constexpr char AutoTuneForSoftwareSemitoneSettings[]   = "AutoTuneForSoftwareSemitoneSettings";
		inline constexpr char AutoTuneForSoftwareSemitoneTriggers[]   = "AutoTuneForSoftwareSemitoneTriggers";
		inline constexpr char AutoTuneForSoftwareTrueTuningSettings[] = "AutoTuneForSoftwareTrueTuningSettings";
		inline constexpr char AutoTuneForSoftwareTrueTuningTriggers[] = "AutoTuneForSoftwareTrueTuningTriggers";

		// Alternative Sample Rate
		inline constexpr char AltOutputSampleRate[]         = "AltOutputSampleRate";
		inline constexpr char AlternativeOutputSampleRate[] = "AlternativeOutputSampleRate";

		// Auto Load Profile
		inline constexpr char ForceProfileEnabled[]    = "ForceProfileEnabled";
		inline constexpr char ProfileToLoad[]          = "ProfileToLoad";

		// Extended Range
		inline constexpr char RainbowStringsEnabled[]  = "RainbowStringsEnabled";
		inline constexpr char RainbowNotesEnabled[]    = "RainbowNotesEnabled";
		inline constexpr char ExtendedRangeEnabled[]   = "ExtendedRangeEnabled";
		inline constexpr char SeparateNoteColors[]     = "SeparateNoteColors";
		inline constexpr char ExtendedRangeMode[]          = "ExtendedRangeMode";
		inline constexpr char ExtendedRangeDropTuning[]    = "ExtendedRangeDropTuning";
		inline constexpr char ExtendedRangeFixBassTuning[] = "ExtendedRangeFixBassTuning";

		// Gameplay / Highway toggles (D3D texture mods)
		inline constexpr char RemoveFingerprints[]       = "RemoveFingerprints";
		inline constexpr char CustomHighwayColors[]      = "CustomHighwayColors";
		inline constexpr char GreenScreenWallEnabled[]   = "GreenScreenWallEnabled";
		inline constexpr char FretlessModeEnabled[]      = "FretlessModeEnabled";
		inline constexpr char RemoveInlaysEnabled[]      = "RemoveInlaysEnabled";
		inline constexpr char RemoveLaneMarkersEnabled[] = "RemoveLaneMarkersEnabled";

		// Launch On External Monitor
		inline constexpr char SecondaryMonitor[]          = "SecondaryMonitor";
		inline constexpr char SecondaryMonitorXPosition[] = "SecondaryMonitorXPosition";
		inline constexpr char SecondaryMonitorYPosition[] = "SecondaryMonitorYPosition";

		// Microphone Volume Override
		inline constexpr char OverrideInputVolume[]        = "OverrideInputVolume";
		inline constexpr char OverrideInputVolumeEnabled[] = "OverrideInputVolumeEnabled";
		inline constexpr char OverrideInputVolumeDevice[]  = "OverrideInputVolumeDevice";

		// On-Screen Display / Overlay
		inline constexpr char OnScreenFont[]            = "OnScreenFont";
		inline constexpr char OnScreenFontSize[]        = "OnScreenFontSize";
		inline constexpr char ShowCurrentNoteOnScreen[] = "ShowCurrentNoteOnScreen";
		inline constexpr char DisplayCurrentAccuracy[]  = "DisplayCurrentAccuracy";
		inline constexpr char LoopingLeadUp[]           = "LoopingLeadUp";

		// Guitar Speak
		inline constexpr char GuitarSpeakWhileTuning[] = "GuitarSpeakWhileTuning";
		inline constexpr char GuitarSpeakDelete[]      = "GuitarSpeakDelete";
		inline constexpr char GuitarSpeakSpace[]       = "GuitarSpeakSpace";
		inline constexpr char GuitarSpeakEnter[]       = "GuitarSpeakEnter";
		inline constexpr char GuitarSpeakTab[]         = "GuitarSpeakTab";
		inline constexpr char GuitarSpeakPageUp[]      = "GuitarSpeakPageUp";
		inline constexpr char GuitarSpeakPageDown[]    = "GuitarSpeakPageDown";
		inline constexpr char GuitarSpeakUpArrow[]     = "GuitarSpeakUpArrow";
		inline constexpr char GuitarSpeakDownArrow[]   = "GuitarSpeakDownArrow";
		inline constexpr char GuitarSpeakEscape[]      = "GuitarSpeakEscape";
		inline constexpr char GuitarSpeakClose[]       = "GuitarSpeakClose";
		inline constexpr char GuitarSpeakOBracket[]    = "GuitarSpeakOBracket";
		inline constexpr char GuitarSpeakCBracket[]    = "GuitarSpeakCBracket";
		inline constexpr char GuitarSpeakTildea[]      = "GuitarSpeakTildea";
		inline constexpr char GuitarSpeakForSlash[]    = "GuitarSpeakForSlash";
		inline constexpr char GuitarSpeakAlt[]         = "GuitarSpeakAlt";

		// Fixes / misc toggles
		inline constexpr char FixBrokenTones[]      = "FixBrokenTones";
		inline constexpr char FixOculusCrash[]      = "FixOculusCrash";
		inline constexpr char PreventMidSongPause[] = "PreventMidSongPause";
		inline constexpr char BackupProfile[]           = "BackupProfile"; // Shared with the GUI, which also backs up when it opens.
		inline constexpr char FastProfileLoadAndSave[]  = "FastProfileLoadAndSave";
		inline constexpr char UltrafastLoad[]           = "UltrafastLoad";

		// Standalone toggles
		inline constexpr char AllowAudioInBackground[]  = "AllowAudioInBackground";
		inline constexpr char BypassTwoRTCMessageBox[] = "BypassTwoRTCMessageBox";
		inline constexpr char GuitarSpeak[]             = "GuitarSpeak";
		inline constexpr char ScreenShotScores[]        = "ScreenShotScores";
		inline constexpr char SongPreviews[]            = "SongPreviews";

		// Non-Stop Play Timer
		inline constexpr char UseCustomNSPTimer[]      = "UseCustomNSPTimer";
		inline constexpr char CustomNSPTimeLimit[]     = "CustomNSPTimeLimit";

		// Wwise RTPC channel names passed to SetRTPCValue / GetRTPCValue.
		namespace Channel {
			inline constexpr char Master[]    = "Master_Volume";
			inline constexpr char Music[]     = "Mixer_Music";
			inline constexpr char Player1[]   = "Mixer_Player1";
			inline constexpr char Player2[]   = "Mixer_Player2";
			inline constexpr char Mic[]       = "Mixer_Mic";
			inline constexpr char VoiceOver[] = "Mixer_VO";
			inline constexpr char SFX[]       = "Mixer_SFX";
		}

		// Keybind names. These are ordinary mod settings (GetModSetting / GetKeyBind)
		// alongside the feature-flag and numeric settings above. Each is declared, with its
		// default, by the mod that binds it (SettingDef::KeyBind).
		namespace Key {
			inline constexpr char ToggleLoft[]           = "ToggleLoftKey";
			inline constexpr char ShowSongTimer[]        = "ShowSongTimerKey";
			inline constexpr char ForceReEnumeration[]   = "ForceReEnumerationKey";
			inline constexpr char RainbowStrings[]       = "RainbowStringsKey";
			inline constexpr char RainbowNotes[]         = "RainbowNotesKey";
			inline constexpr char RemoveLyrics[]         = "RemoveLyricsKey";
			inline constexpr char RRSpeed[]              = "RRSpeedKey";
			inline constexpr char TuningOffset[]         = "TuningOffsetKey";
			inline constexpr char ToggleExtendedRange[]  = "ToggleExtendedRangeKey";
			inline constexpr char LoopStart[]            = "LoopStartKey";
			inline constexpr char LoopEnd[]              = "LoopEndKey";
			inline constexpr char Rewind[]               = "RewindKey";
			inline constexpr char ToggleSettingsMenu[]   = "ToggleSettingsMenuKey";

			inline constexpr char MasterVolume[]         = "MasterVolumeKey";
			inline constexpr char SongVolume[]           = "SongVolumeKey";
			inline constexpr char Player1Volume[]        = "Player1VolumeKey";
			inline constexpr char Player2Volume[]        = "Player2VolumeKey";
			inline constexpr char MicrophoneVolume[]     = "MicrophoneVolumeKey";
			inline constexpr char VoiceOverVolume[]      = "VoiceOverVolumeKey";
			inline constexpr char SFXVolume[]            = "SFXVolumeKey";
			inline constexpr char DisplayMixer[]         = "DisplayMixerKey";
			inline constexpr char MutePlayer1[]              = "MutePlayer1Key";
			inline constexpr char MutePlayer2[]              = "MutePlayer2Key";
			inline constexpr char ChangedSelectedVolume[]    = "ChangedSelectedVolumeKey";
		}

		// Twitch / Crowd Control effect state. These live in the separate twitchSettings map
		// (IsTwitchSettingEnabled / TwitchSettingValue / UpdateTwitchSetting), NOT the mod settings, so an INI reload keeps them.
		namespace Twitch {
			inline constexpr char SolidNoteColor[]   = "SolidNoteColor"; // Hex, or "random"; set by the SolidNotes effect.
			inline constexpr char RainbowStrings[]   = "RainbowStrings";
			inline constexpr char RemoveNotes[]      = "RemoveNotes";
			inline constexpr char TransparentNotes[] = "TransparentNotes";
			inline constexpr char SolidNotes[]       = "SolidNotes";
			inline constexpr char DrunkMode[]        = "DrunkMode";
			inline constexpr char FYourFC[]          = "FYourFC";
		}
	}

	void Initialize(); // Default Settings

	// Read INI
	std::vector<std::string> GetCustomSongTitles();
	void ReadModSettings();
	void ReadStringColors();
	void ReadNotewayColors();

	void ToggleExtendedRangeMode();

	// Return INI Settings
	unsigned int GetKeyBind(const std::string& name);
	int GetModSetting(const std::string& name);
	std::string ReturnSettingValue(const std::string& name);
	bool IsOn(const std::string& name);
	bool IsOff(const std::string& name);
	std::string ReturnNotewayColor(const std::string& name);

	When GetWhen(const std::string& name);
	StringColorMode GetStringColorMode();
	NoteColorMode GetNoteColorMode();

	int GetVKCodeForString(const std::string& vkString);
	std::vector<RSColor> GetStringColors(bool CB);
	std::vector<RSColor> GetNoteColors(bool CB);
	void SetStringColors(int strIndex, RSColor c, bool CB);
	void SetNoteColors(int strIndex, RSColor c, bool CB);
	void UpdateSettings();
	void UpdateModSetting(const std::string& name, const std::string_view& newValue);
	void UpdateCustomSetting(const std::string& name, int newValue);
	void UpdateTwitchSetting(const std::string& name, const std::string_view& newValue);
	void ParseSettingUpdate(const std::string& updateMessage);
	bool IsTwitchSettingEnabled(const std::string& name);
	std::string TwitchSettingValue(const std::string& name);
	std::vector<std::string> SplitByWhitespace(const std::string& input);

	inline std::map<std::string, std::string, std::less<>> keyBinds;
	inline std::map<std::string, std::string, std::less<>> twitchSettings;
	inline std::map<std::string, std::string, std::less<>> notewayColors;

	// Misc 
	
	RSColor ConvertHexToColor(const std::string& hexStr);

	inline std::vector<RSColor> customStringColorsNormal;
	inline std::vector<RSColor> customStringColorsCB;
	inline std::vector<RSColor> customNoteColorsNormal;
	inline std::vector<RSColor> customNoteColorsCB;

	inline std::atomic_bool async_UpdateMidiSettings = false;

	const std::vector<std::string> defaultStrColors = {
		"FF4F5A", "E2C102", "1DACF9", "FF9216", "3FCC0C", "C825ED", "0ABCB9", "909090"
	};

	const std::vector<std::string> defaultStrColorsCB = {
		"C12A2A", "A3F400", "1DACF9", "DB7F41", "00C68E", "7648A8", "493647", "4C4C4C"
	};
};
