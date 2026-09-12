#pragma once

#include <atomic>
#include <functional>
#include <map>
#include <string>
#include <string_view>
#include <vector>

#include "RSColor.h"

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
		inline constexpr char CustomStringColors[]     = "CustomStringColors";
		inline constexpr char SeparateNoteColors[]     = "SeparateNoteColors";
		inline constexpr char SeparateNoteColorsMode[] = "SeparateNoteColorsMode";
		inline constexpr char ExtendedRangeMode[]          = "ExtendedRangeMode";
		inline constexpr char ExtendedRangeDropTuning[]    = "ExtendedRangeDropTuning";
		inline constexpr char ExtendedRangeFixBassTuning[] = "ExtendedRangeFixBassTuning";

		// Gameplay / Highway toggles (D3D texture mods)
		inline constexpr char DiscoModeEnabled[]         = "DiscoModeEnabled";
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

		// Standalone toggles
		inline constexpr char AllowAudioInBackground[]  = "AllowAudioInBackground";
		inline constexpr char BypassTwoRTCMessageBox[] = "BypassTwoRTCMessageBox";
		inline constexpr char GuitarSpeak[]             = "GuitarSpeak";
		inline constexpr char ScreenShotScores[]        = "ScreenShotScores";
		inline constexpr char SongPreviews[]            = "SongPreviews";

		// Non-Stop Play Timer
		inline constexpr char UseCustomNSPTimer[]      = "UseCustomNSPTimer";
		inline constexpr char CustomNSPTimeLimit[]     = "CustomNSPTimeLimit";

		// Solid Notes (user-defined hex color; lives in modSettings, set by the Twitch/CC SolidNotes effect)
		inline constexpr char SolidNoteColor[]         = "SolidNoteColor";

		// Ultrawide support
		inline constexpr char Ultrawide[]               = "Ultrawide";

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

		// Keybind names. These live in the modSettings map (GetModSetting / GetKeyBind)
		// alongside the feature-flag and numeric settings above.
		namespace Key {
			inline constexpr char CustomSongListTitles[] = "CustomSongListTitles";
			inline constexpr char ToggleLoft[]           = "ToggleLoftKey";
			inline constexpr char ShowSongTimer[]        = "ShowSongTimerKey";
			inline constexpr char ForceReEnumeration[]   = "ForceReEnumerationKey";
			inline constexpr char RainbowStrings[]       = "RainbowStringsKey";
			inline constexpr char RainbowNotes[]         = "RainbowNotesKey";
			inline constexpr char RemoveLyrics[]         = "RemoveLyricsKey";
			inline constexpr char RRSpeed[]              = "RRSpeedKey";
			inline constexpr char MenuToggle[]           = "MenuToggleKey";
			inline constexpr char TuningOffset[]         = "TuningOffsetKey";
			inline constexpr char ToggleExtendedRange[]  = "ToggleExtendedRangeKey";
			inline constexpr char LoopStart[]            = "LoopStartKey";
			inline constexpr char LoopEnd[]              = "LoopEndKey";
			inline constexpr char Rewind[]               = "RewindKey";

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

		// Twitch / Crowd Control effect toggles. These live in the separate twitchSettings map
		// (IsTwitchSettingEnabled / UpdateTwitchSetting), NOT modSettings, so they get their own scope.
		namespace Twitch {
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
	void ReadKeyBinds();
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

	enum class When { Unknown, Manual, Startup, Song, Tuner, Automatic };
	When ParseWhen(std::string_view value);
	When GetWhen(const std::string& name);

	enum class StringColorMode { Default = 0, Zag = 1, Custom = 2, Test = 3 };
	StringColorMode GetStringColorMode();

	enum class NoteColorMode { SameAsStrings = 0, Default = 1, Custom = 2 };
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
	void ParseTwitchToggle(const std::string& twitchMsg, const std::string_view& toggleType);
	void ParseSolidColorsMessage(const std::string& twitchMsg);
	bool IsTwitchSettingEnabled(const std::string& name);
	std::vector<std::string> SplitByWhitespace(const std::string& input);

	inline std::map<std::string, std::string, std::less<>> keyBinds;
	inline std::map<std::string, std::string, std::less<>> modSettings;
	inline std::map<std::string, int, std::less<>> customSettings;
	inline std::map<std::string, std::string, std::less<>> twitchSettings;
	inline std::map<std::string, std::string, std::less<>> notewayColors;

	// Misc 
	inline std::map<std::string, unsigned int, std::less<>> keyMap = {
		{ "VK_LBUTTON" , 0x01 },
		{ "VK_RBUTTON" , 0x02 },
		{ "VK_CANCEL" , 0x03 },
		{ "VK_MBUTTON" , 0x04 },
		{ "VK_XBUTTON1" , 0x05 },
		{ "VK_XBUTTON2" , 0x06 },
		{ "VK_BACK" , 0x08 },
		{ "VK_TAB" , 0x09 },
		{ "VK_CLEAR" , 0x0C },
		{ "VK_RETURN" , 0x0D },
		{ "VK_SHIFT" , 0x10 },
		{ "VK_CONTROL" , 0x11 },
		{ "VK_MENU" , 0x12 },
		{ "VK_PAUSE" , 0x13 },
		{ "VK_CAPITAL" , 0x14 },
		{ "VK_KANA" , 0x15 },
		{ "VK_HANGUEL" , 0x15 },
		{ "VK_HANGUL" , 0x15 },
		{ "VK_IME_ON" , 0x16 },
		{ "VK_JUNJA" , 0x17 },
		{ "VK_FINAL" , 0x18 },
		{ "VK_HANJA" , 0x19 },
		{ "VK_KANJI" , 0x19 },
		{ "VK_IME_OFF" , 0x1A },
		{ "VK_ESCAPE" , 0x1B },
		{ "VK_CONVERT" , 0x1C },
		{ "VK_NONCONVERT" , 0x1D },
		{ "VK_ACCEPT" , 0x1E },
		{ "VK_MODECHANGE" , 0x1F },
		{ "VK_SPACE" , 0x20 },
		{ "VK_PRIOR" , 0x21 },
		{ "VK_NEXT" , 0x22 },
		{ "VK_END" , 0x23 },
		{ "VK_HOME" , 0x24 },
		{ "VK_LEFT" , 0x25 },
		{ "VK_UP" , 0x26 },
		{ "VK_RIGHT" , 0x27 },
		{ "VK_DOWN" , 0x28 },
		{ "VK_SELECT" , 0x29 },
		{ "VK_PRINT" , 0x2A },
		{ "VK_EXECUTE" , 0x2B },
		{ "VK_SNAPSHOT" , 0x2C },
		{ "VK_INSERT" , 0x2D },
		{ "VK_DELETE" , 0x2E },
		{ "VK_HELP" , 0x2F },
		{ "0" , 0x30 },
		{ "1" , 0x31 },
		{ "2" , 0x32 },
		{ "3" , 0x33 },
		{ "4" , 0x34 },
		{ "5" , 0x35 },
		{ "6" , 0x36 },
		{ "7" , 0x37 },
		{ "8" , 0x38 },
		{ "9" , 0x39 },
		{ "A" , 0x41 },
		{ "B" , 0x42 },
		{ "C" , 0x43 },
		{ "D" , 0x44 },
		{ "E" , 0x45 },
		{ "F" , 0x46 },
		{ "G" , 0x47 },
		{ "H" , 0x48 },
		{ "I" , 0x49 },
		{ "J" , 0x4A },
		{ "K" , 0x4B },
		{ "L" , 0x4C },
		{ "M" , 0x4D },
		{ "N" , 0x4E },
		{ "O" , 0x4F },
		{ "P" , 0x50 },
		{ "Q" , 0x51 },
		{ "R" , 0x52 },
		{ "S" , 0x53 },
		{ "T" , 0x54 },
		{ "U" , 0x55 },
		{ "V" , 0x56 },
		{ "W" , 0x57 },
		{ "X" , 0x58 },
		{ "Y" , 0x59 },
		{ "Z" , 0x5A },
		{ "VK_LWIN" , 0x5B },
		{ "VK_RWIN" , 0x5C },
		{ "VK_APPS" , 0x5D },
		{ "VK_SLEEP" , 0x5F },
		{ "VK_NUMPAD0" , 0x60 },
		{ "VK_NUMPAD1" , 0x61 },
		{ "VK_NUMPAD2" , 0x62 },
		{ "VK_NUMPAD3" , 0x63 },
		{ "VK_NUMPAD4" , 0x64 },
		{ "VK_NUMPAD5" , 0x65 },
		{ "VK_NUMPAD6" , 0x66 },
		{ "VK_NUMPAD7" , 0x67 },
		{ "VK_NUMPAD8" , 0x68 },
		{ "VK_NUMPAD9" , 0x69 },
		{ "VK_MULTIPLY" , 0x6A },
		{ "VK_ADD" , 0x6B },
		{ "VK_SEPARATOR" , 0x6C },
		{ "VK_SUBTRACT" , 0x6D },
		{ "VK_DECIMAL" , 0x6E },
		{ "VK_DIVIDE" , 0x6F },
		{ "VK_F1" , 0x70 },
		{ "VK_F2" , 0x71 },
		{ "VK_F3" , 0x72 },
		{ "VK_F4" , 0x73 },
		{ "VK_F5" , 0x74 },
		{ "VK_F6" , 0x75 },
		{ "VK_F7" , 0x76 },
		{ "VK_F8" , 0x77 },
		{ "VK_F9" , 0x78 },
		{ "VK_F10" , 0x79 },
		{ "VK_F11" , 0x7A },
		{ "VK_F12" , 0x7B },
		{ "VK_F13" , 0x7C },
		{ "VK_F14" , 0x7D },
		{ "VK_F15" , 0x7E },
		{ "VK_F16" , 0x7F },
		{ "VK_F17" , 0x80 },
		{ "VK_F18" , 0x81 },
		{ "VK_F19" , 0x82 },
		{ "VK_F20" , 0x83 },
		{ "VK_F21" , 0x84 },
		{ "VK_F22" , 0x85 },
		{ "VK_F23" , 0x86 },
		{ "VK_F24" , 0x87 },
		{ "VK_NUMLOCK" , 0x90 },
		{ "VK_SCROLL" , 0x91 },
		{ "VK_LSHIFT" , 0xA0 },
		{ "VK_RSHIFT" , 0xA1 },
		{ "VK_LCONTROL" , 0xA2 },
		{ "VK_RCONTROL" , 0xA3 },
		{ "VK_LMENU" , 0xA4 },
		{ "VK_RMENU" , 0xA5 },
		{ "VK_BROWSER_BACK" , 0xA6 },
		{ "VK_BROWSER_FORWARD" , 0xA7 },
		{ "VK_BROWSER_REFRESH" , 0xA8 },
		{ "VK_BROWSER_STOP" , 0xA9 },
		{ "VK_BROWSER_SEARCH" , 0xAA },
		{ "VK_BROWSER_FAVORITES" , 0xAB },
		{ "VK_BROWSER_HOME" , 0xAC },
		{ "VK_VOLUME_MUTE" , 0xAD },
		{ "VK_VOLUME_DOWN" , 0xAE },
		{ "VK_VOLUME_UP" , 0xAF },
		{ "VK_MEDIA_NEXT_TRACK" , 0xB0 },
		{ "VK_MEDIA_PREV_TRACK" , 0xB1 },
		{ "VK_MEDIA_STOP" , 0xB2 },
		{ "VK_MEDIA_PLAY_PAUSE" , 0xB3 },
		{ "VK_LAUNCH_MAIL" , 0xB4 },
		{ "VK_LAUNCH_MEDIA_SELECT" , 0xB5 },
		{ "VK_LAUNCH_APP1" , 0xB6 },
		{ "VK_LAUNCH_APP2" , 0xB7 },
		{ "VK_OEM_1" , 0xBA },
		{ "VK_OEM_PLUS" , 0xBB },
		{ "VK_OEM_COMMA" , 0xBC },
		{ "VK_OEM_MINUS" , 0xBD },
		{ "VK_OEM_PERIOD" , 0xBE },
		{ "VK_OEM_2" , 0xBF },
		{ "VK_OEM_3" , 0xC0 },
		{ "VK_OEM_4" , 0xDB },
		{ "VK_OEM_5" , 0xDC },
		{ "VK_OEM_6" , 0xDD },
		{ "VK_OEM_7" , 0xDE },
		{ "VK_OEM_8" , 0xDF },
		{ "VK_OEM_102" , 0xE2 },
		{ "VK_PROCESSKEY" , 0xE5 },
		{ "VK_PACKET" , 0xE7 },
		{ "VK_ATTN" , 0xF6 },
		{ "VK_CRSEL" , 0xF7 },
		{ "VK_EXSEL" , 0xF8 },
		{ "VK_EREOF" , 0xF9 },
		{ "VK_PLAY" , 0xFA },
		{ "VK_ZOOM" , 0xFB },
		{ "VK_NONAME" , 0xFC },
		{ "VK_PA1" , 0xFD },
		{ "VK_OEM_CLEAR" , 0xFE }
	};
	
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
