#include "stdafx.h"
#include "Settings.hpp"

#include <mutex>
#include <shared_mutex>
#include <string>

// The Settings maps/vectors are read from the D3D render thread and the Twitch/CrowdControl
// threads while writes are drained on the main thread. std::map is not safe for concurrent
// read/write, and the getters used to read via operator[] (which inserts on a miss), so even
// two concurrent "reads" mutated the map and raced. Every accessor now takes a shared lock and
// every mutator a unique lock on this one mutex, and reads use non-mutating lookups. Composite
// getters call the *Unlocked helpers (never another public accessor) so they never re-lock. The
// Read* reload functions take the lock only after LoadFile, so disk IO stays outside it.
//
// TODO(C++20): writes are rare and already serialized on the main thread, so this is a
// single-writer/many-reader case. Publishing the state as an immutable snapshot behind a
// std::atomic<std::shared_ptr<const SettingsData>> would make reads lock-free and never block on a reload.
namespace {
	std::shared_mutex g_settingsMutex;

	std::string ModSettingUnlocked(const std::string& name) {
		auto it = Settings::modSettings.find(name);
		return it != Settings::modSettings.end() ? it->second : std::string();
	}

	int CustomSettingUnlocked(const std::string& name) {
		auto it = Settings::customSettings.find(name);
		return it != Settings::customSettings.end() ? it->second : 0;
	}

	unsigned int VKCodeUnlocked(const std::string& vkString) {
		auto it = Settings::keyMap.find(vkString);
		return it != Settings::keyMap.end() ? it->second : 0u;
	}
}

/// <summary>
/// Load Default Settings.
/// Used if the user has the DLL but no INI.
/// </summary>
void Settings::Initialize()
{
	std::unique_lock lock(g_settingsMutex);

	modSettings = {
		{Setting::Key::CustomSongListTitles, "K"},
		{Setting::Key::ToggleLoft,          "T"},
		{Setting::Key::ShowSongTimer,        "S"},
		{Setting::Key::ForceReEnumeration,   "F"},
		{Setting::Key::RainbowStrings,       "V"},
		{Setting::Key::RainbowNotes,         "N"},
		{Setting::Key::RemoveLyrics,         "L"},
		{Setting::Key::RRSpeed,              "R"},
		{Setting::Key::MenuToggle,           "M"},
		{Setting::Key::TuningOffset,         "O"},
		{Setting::Key::ToggleExtendedRange,  "E"},
		{Setting::Key::LoopStart,            "Y"},
		{Setting::Key::LoopEnd,              "U"},
		{Setting::Key::Rewind,               "Z"},

		{Setting::Key::MasterVolume,         "5"},
		{Setting::Key::SongVolume,           "6"},
		{Setting::Key::Player1Volume,        "7"},
		{Setting::Key::Player2Volume,        "8"},
		{Setting::Key::MicrophoneVolume,     "9"},
		{Setting::Key::VoiceOverVolume,      "0"},
		{Setting::Key::SFXVolume,            "S"},
		{Setting::Key::DisplayMixer,         "P"},
		{Setting::Key::MutePlayer1,              "X"},
		{Setting::Key::MutePlayer2,              "C"},
		{Setting::Key::ChangedSelectedVolume,    "B"},

		{Setting::ForceReEnumerationEnabled, "off"},

		{Setting::ToggleLoftEnabled, "off"},
		{Setting::VolumeControlEnabled, "off"},
		{Setting::ShowSongTimerEnabled, "off"},
		{Setting::RainbowStringsEnabled, "off"},
		{Setting::ExtendedRangeEnabled, "off"},
		{Setting::ExtendedRangeDropTuning, "off"},
		{Setting::ExtendedRangeFixBassTuning, "off"},
		{Setting::SeparateNoteColors, "off"},
		{Setting::DiscoModeEnabled, "off"},
		{Setting::RemoveHeadstockEnabled, "off"},
		{Setting::RemoveSkylineEnabled, "off"},
		{Setting::GreenScreenWallEnabled, "off"},
		{Setting::ForceProfileEnabled, "off"},
		{Setting::FretlessModeEnabled, "off"},
		{Setting::RemoveInlaysEnabled, "off"},
		{Setting::ToggleLoftWhen, "manual"},
		{Setting::ToggleSkylineWhen, "song"},
		{Setting::RemoveLaneMarkersEnabled, "off"},
		{Setting::RemoveLyricsEnabled, "off"},
		{Setting::RemoveLyricsWhen, "manual"},
		{Setting::GuitarSpeak, "off"},
		{Setting::GuitarSpeakWhileTuning, "off"},
		{Setting::RemoveHeadstockWhen, "song"},
		{Setting::ScreenShotScores, "off"},
		{Setting::RRSpeedAboveOneHundred, "off"},
		{Setting::AutoTuneForSong, "off"},
		{Setting::AutoTuneForSongDevice, ""},
		{Setting::MidiInDevice, ""},
		{Setting::AutoTuneForSongWhen, "manual"},
		{Setting::AutoTuneForSoftwareSemitoneSettings, ""},
		{Setting::AutoTuneForSoftwareSemitoneTriggers, ""},
		{Setting::AutoTuneForSoftwareTrueTuningSettings, ""},
		{Setting::AutoTuneForSoftwareTrueTuningTriggers, ""},
		{Setting::ChordsMode, "off"},
		{Setting::ShowCurrentNoteOnScreen, "off"},
		{Setting::OnScreenFont, "Arial"},
		{Setting::ProfileToLoad, ""},
		{Setting::ShowSongTimerWhen, "manual"},
		{Setting::SecondaryMonitor, "off"},
		{Setting::SongPreviews, "off"},
		{Setting::OverrideInputVolumeEnabled, "off"},
		{Setting::OverrideInputVolumeDevice, ""},
		{Setting::AllowAudioInBackground, "off"},
		{Setting::BypassTwoRTCMessageBox, "off"},
		{Setting::LinearRiffRepeater, "off"},
		{Setting::AltOutputSampleRate, "off"},
		{Setting::AllowLooping, "off"},
		{Setting::AllowRewind, "off"},
		{Setting::FixOculusCrash, "off"},
		{Setting::FixBrokenTones, "off"},
		{Setting::UseCustomNSPTimer, "off"},
		{Setting::DisplayCurrentAccuracy, "off"},
		{Setting::PreventMidSongPause, "off"},
		{Setting::RemoveFingerprints, "off"},
		{Setting::Ultrawide, "off"},
	};

	customSettings = {
		{Setting::ExtendedRangeMode, -5},
		{Setting::CheckForNewSongsInterval, 5000},
		{Setting::RRSpeedInterval, 2},
		{Setting::TuningPedal, 0},
		{Setting::TuningOffset, 0},
		{Setting::VolumeControlInterval, 5},
		{Setting::SecondaryMonitorXPosition, 0},
		{Setting::SecondaryMonitorYPosition, 0},
		{Setting::SeparateNoteColorsMode, 0},
		{Setting::OverrideInputVolume, 17},
		{Setting::CustomStringColors, 0},
		{Setting::AlternativeOutputSampleRate, 48000},
		{Setting::LoopingLeadUp, 0},
		{Setting::RewindBy, 5000},
		{Setting::RewindLeadup, 2000},
		{Setting::CustomNSPTimeLimit, 10000},
		{Setting::OnScreenFontSize, 24},

		{Setting::GuitarSpeakDelete, 0},
		{Setting::GuitarSpeakSpace, 0},
		{Setting::GuitarSpeakEnter, 0},
		{Setting::GuitarSpeakTab, 0},
		{Setting::GuitarSpeakPageUp, 0},
		{Setting::GuitarSpeakPageDown, 0},
		{Setting::GuitarSpeakUpArrow, 0},
		{Setting::GuitarSpeakDownArrow, 0},
		{Setting::GuitarSpeakEscape, 0},
		{Setting::GuitarSpeakClose, 0},
		{Setting::GuitarSpeakOBracket, 0},
		{Setting::GuitarSpeakCBracket, 0},
		{Setting::GuitarSpeakTildea, 0},
		{Setting::GuitarSpeakForSlash, 0},
		{Setting::GuitarSpeakAlt, 0}
	};

	twitchSettings = {
		{Setting::Twitch::RainbowStrings, "off"},
		{Setting::Twitch::RemoveNotes, "off"},
		{Setting::Twitch::TransparentNotes, "off"},
		{Setting::Twitch::SolidNotes, "off"},
		{Setting::Twitch::DrunkMode, "off"},
		{Setting::Twitch::FYourFC, "off"},
	};
}


// Read INI

/// <summary>
/// Parse INI for SongLists
/// </summary>
/// <returns>List of Custom Songlist Names</returns>
std::vector<std::string> Settings::GetCustomSongTitles() {
	std::vector<std::string> retList(20);
	CSimpleIniA reader;
	if (reader.LoadFile("RSMods.ini") < 0)
		return retList;

	for (int i = 0; i < 20; i++) {
		std::string songListName = "SongListTitle_" + std::to_string(i + 1);
		retList[i] = reader.GetValue("SongListTitles", songListName.c_str(), "SONG LIST");
	}

	return retList;
}

/// <summary>
/// Parse Mod / Volume Keybind Toggles
/// </summary>
void Settings::ReadKeyBinds() {
	CSimpleIniA reader;
	if (reader.LoadFile("RSMods.ini") < 0) {
		LOG_ERROR("Error reading saved settings" << std::endl);
		return;
	}

	std::unique_lock lock(g_settingsMutex);

	modSettings = {
		{ Setting::Key::ToggleLoft,         reader.GetValue("Keybinds", Setting::Key::ToggleLoft,         "T") },
		{ Setting::Key::CustomSongListTitles, reader.GetValue("Keybinds", Setting::Key::CustomSongListTitles, "K") },
		{ Setting::Key::ShowSongTimer,       reader.GetValue("Keybinds", Setting::Key::ShowSongTimer,       "N") },
		{ Setting::Key::ForceReEnumeration,  reader.GetValue("Keybinds", Setting::Key::ForceReEnumeration,  "F") },
		{ Setting::Key::MenuToggle,          reader.GetValue("Keybinds", Setting::Key::MenuToggle,          "M") },
		{ Setting::Key::RainbowStrings,      reader.GetValue("Keybinds", Setting::Key::RainbowStrings,      "V") },
		{ Setting::Key::RainbowNotes,        reader.GetValue("Keybinds", Setting::Key::RainbowNotes,        "N") },
		{ Setting::Key::RemoveLyrics,        reader.GetValue("Keybinds", Setting::Key::RemoveLyrics,        "L") },
		{ Setting::Key::RRSpeed,             reader.GetValue("Keybinds", Setting::Key::RRSpeed,             "R") },
		{ Setting::Key::TuningOffset,        reader.GetValue("Keybinds", Setting::Key::TuningOffset,        "O") },
		{ Setting::Key::ToggleExtendedRange, reader.GetValue("Keybinds", Setting::Key::ToggleExtendedRange, "E") },
		{ Setting::Key::LoopStart,           reader.GetValue("Keybinds", Setting::Key::LoopStart,           "Y") },
		{ Setting::Key::LoopEnd,             reader.GetValue("Keybinds", Setting::Key::LoopEnd,             "U") },
		{ Setting::Key::Rewind,              reader.GetValue("Keybinds", Setting::Key::Rewind,              "Z") },

		{ Setting::Key::MasterVolume,        reader.GetValue("Audio Keybindings", Setting::Key::MasterVolume,        "5") },
		{ Setting::Key::SongVolume,          reader.GetValue("Audio Keybindings", Setting::Key::SongVolume,          "6") },
		{ Setting::Key::Player1Volume,       reader.GetValue("Audio Keybindings", Setting::Key::Player1Volume,       "7") },
		{ Setting::Key::Player2Volume,       reader.GetValue("Audio Keybindings", Setting::Key::Player2Volume,       "8") },
		{ Setting::Key::MicrophoneVolume,    reader.GetValue("Audio Keybindings", Setting::Key::MicrophoneVolume,    "9") },
		{ Setting::Key::VoiceOverVolume,     reader.GetValue("Audio Keybindings", Setting::Key::VoiceOverVolume,     "0") },
		{ Setting::Key::SFXVolume,           reader.GetValue("Audio Keybindings", Setting::Key::SFXVolume,           "S") },
		{ Setting::Key::DisplayMixer,        reader.GetValue("Audio Keybindings", Setting::Key::DisplayMixer,        "P") },
		{ Setting::Key::MutePlayer1,             reader.GetValue("Audio Keybindings", Setting::Key::MutePlayer1,             "X") },
		{ Setting::Key::MutePlayer2,             reader.GetValue("Audio Keybindings", Setting::Key::MutePlayer2,             "C") },
		{ Setting::Key::ChangedSelectedVolume,   reader.GetValue("Audio Keybindings", Setting::Key::ChangedSelectedVolume,   "B") },
	};
}

/// <summary>
/// Parse Settings For Mods
/// </summary>
void Settings::ReadModSettings() {
	CSimpleIniA reader;
	if (reader.LoadFile("RSMods.ini") < 0) {
		LOG_ERROR("Error reading saved settings" << std::endl);
		return;
	}

	// Augments the modSettings that ReadKeyBinds already populated, so it adds keys rather
	// than replacing the map wholesale.
	std::unique_lock lock(g_settingsMutex);

	customSettings = {
		{Setting::ExtendedRangeMode, reader.GetLongValue("Mod Settings", "ExtendedRangeModeAt", -5)},
		{Setting::CheckForNewSongsInterval, reader.GetLongValue("Mod Settings", "CheckForNewSongsInterval", 5000)},
		{Setting::RRSpeedInterval, reader.GetLongValue("Mod Settings", "RRSpeedInterval", 2)},
		{Setting::TuningPedal, reader.GetLongValue("Mod Settings", "TuningPedal", 0)},
		{Setting::TuningOffset, reader.GetLongValue("Mod Settings", "TuningOffset", 0)},
		{Setting::VolumeControlInterval, reader.GetLongValue("Mod Settings", "VolumeControlInterval", 5)},
		{Setting::SecondaryMonitorXPosition, reader.GetLongValue("Mod Settings", "SecondaryMonitorXPosition", 0)},
		{Setting::SecondaryMonitorYPosition, reader.GetLongValue("Mod Settings", "SecondaryMonitorYPosition", 0)},
		{Setting::SeparateNoteColorsMode, reader.GetLongValue("Mod Settings", "SeparateNoteColorsMode", 0)}, // 0 = same as strings, 1 = default, 2 = custom
		{Setting::CustomStringColors, reader.GetLongValue("Toggle Switches", "CustomStringColors", 0)}, //0 = default, 1 = Zag, 2 = custom colors
		{Setting::OverrideInputVolume, reader.GetLongValue("Mod Settings", "OverrideInputVolume", 17)}, // 17 is what Rocksmith calls default.
		{Setting::AlternativeOutputSampleRate, reader.GetLongValue("Mod Settings", "AlternativeOutputSampleRate", 48000)},
		{Setting::LoopingLeadUp, reader.GetLongValue("Mod Settings", "LoopingLeadUp", 0)},
		{Setting::RewindBy, reader.GetLongValue("Mod Settings", "RewindBy", 5000)},
		{Setting::RewindLeadup, reader.GetLongValue("Mod Settings", "RewindLeadup", 2000)},
		{Setting::CustomNSPTimeLimit, reader.GetLongValue("Mod Settings", "CustomNSPTimeLimit", 10000)},
		{Setting::OnScreenFontSize, reader.GetLongValue("Mod Settings", "OnScreenFontSize", 24)},

		{Setting::GuitarSpeakDelete, reader.GetLongValue("Guitar Speak", "GuitarSpeakDeleteWhen", 0)},
		{Setting::GuitarSpeakSpace, reader.GetLongValue("Guitar Speak", "GuitarSpeakSpaceWhen", 0)},
		{Setting::GuitarSpeakEnter, reader.GetLongValue("Guitar Speak", "GuitarSpeakEnterWhen", 0)},
		{Setting::GuitarSpeakTab, reader.GetLongValue("Guitar Speak", "GuitarSpeakTabWhen", 0)},
		{Setting::GuitarSpeakPageUp, reader.GetLongValue("Guitar Speak", "GuitarSpeakPGUPWhen", 0)},
		{Setting::GuitarSpeakPageDown, reader.GetLongValue("Guitar Speak", "GuitarSpeakPGDNWhen", 0)},
		{Setting::GuitarSpeakUpArrow, reader.GetLongValue("Guitar Speak", "GuitarSpeakUPWhen", 0)},
		{Setting::GuitarSpeakDownArrow, reader.GetLongValue("Guitar Speak", "GuitarSpeakDNWhen", 0)},
		{Setting::GuitarSpeakEscape, reader.GetLongValue("Guitar Speak", "GuitarSpeakESCWhen", 0)},
		{Setting::GuitarSpeakClose, reader.GetLongValue("Guitar Speak", "GuitarSpeakCloseWhen", 0)},
		{Setting::GuitarSpeakOBracket, reader.GetLongValue("Guitar Speak", "GuitarSpeakOBracketWhen", 0)},
		{Setting::GuitarSpeakCBracket, reader.GetLongValue("Guitar Speak", "GuitarSpeakCBracketWhen", 0)},
		{Setting::GuitarSpeakTildea, reader.GetLongValue("Guitar Speak", "GuitarSpeakTildeaWhen", 0)},
		{Setting::GuitarSpeakForSlash, reader.GetLongValue("Guitar Speak", "GuitarSpeakForSlashWhen", 0)},
		{Setting::GuitarSpeakAlt, reader.GetLongValue("Guitar Speak", "GuitarSpeakAltWhen", 0)},
	};

	modSettings[Setting::ToggleLoftEnabled] = reader.GetValue("Toggle Switches", "ToggleLoft", "off");
	modSettings[Setting::VolumeControlEnabled] = reader.GetValue("Toggle Switches", "VolumeControl", "off");
	modSettings[Setting::ShowSongTimerEnabled] = reader.GetValue("Toggle Switches", "ShowSongTimer", "off");
	modSettings[Setting::ForceReEnumerationEnabled] = reader.GetValue("Toggle Switches", "ForceReEnumeration", "off");
	modSettings[Setting::RainbowStringsEnabled] = reader.GetValue("Toggle Switches", "RainbowStrings", "off");
	modSettings[Setting::RainbowNotesEnabled] = reader.GetValue("Toggle Switches", "RainbowNotes", "off");
	modSettings[Setting::ExtendedRangeEnabled] = reader.GetValue("Toggle Switches", "ExtendedRange", "off");
	modSettings[Setting::ExtendedRangeDropTuning] = reader.GetValue("Toggle Switches", "ExtendedRangeDropTuning", "off");
	modSettings[Setting::ExtendedRangeFixBassTuning] = reader.GetValue("Toggle Switches", "ExtendedRangeFixBassTuning", "off");
	modSettings[Setting::SeparateNoteColors] = reader.GetValue("Toggle Switches", "SeparateNoteColors", "off");
	modSettings[Setting::DiscoModeEnabled] = reader.GetValue("Toggle Switches", "DiscoMode", "off");
	modSettings[Setting::RemoveHeadstockEnabled] = reader.GetValue("Toggle Switches", "Headstock", "off");
	modSettings[Setting::RemoveSkylineEnabled] = reader.GetValue("Toggle Switches", "Skyline", "off");
	modSettings[Setting::Ultrawide] = reader.GetValue("Toggle Switches", "Ultrawide", "off");
	modSettings[Setting::GreenScreenWallEnabled] = reader.GetValue("Toggle Switches", "GreenScreenWall", "off");
	modSettings[Setting::ForceProfileEnabled] = reader.GetValue("Toggle Switches", "ForceProfileLoad", "off");
	modSettings[Setting::FretlessModeEnabled] = reader.GetValue("Toggle Switches", "Fretless", "off");
	modSettings[Setting::RemoveInlaysEnabled] = reader.GetValue("Toggle Switches", "Inlays", "off");
	modSettings[Setting::ToggleLoftWhen] = reader.GetValue("Toggle Switches", "ToggleLoftWhen", "manual");
	modSettings[Setting::ToggleSkylineWhen] = reader.GetValue("Toggle Switches", "ToggleSkylineWhen", "song");
	modSettings[Setting::RemoveLaneMarkersEnabled] = reader.GetValue("Toggle Switches", "LaneMarkers", "off");
	modSettings[Setting::RemoveLyricsEnabled] = reader.GetValue("Toggle Switches", "Lyrics", "off");
	modSettings[Setting::RemoveLyricsWhen] = reader.GetValue("Toggle Switches", "RemoveLyricsWhen", "manual");
	modSettings[Setting::GuitarSpeak] = reader.GetValue("Toggle Switches", "GuitarSpeak", "off");
	modSettings[Setting::GuitarSpeakWhileTuning] = reader.GetValue("Guitar Speak", "GuitarSpeakWhileTuning", "off");
	modSettings[Setting::RemoveHeadstockWhen] = reader.GetValue("Toggle Switches", "RemoveHeadstockWhen", "song");
	modSettings[Setting::ScreenShotScores] = reader.GetValue("Toggle Switches", "ScreenShotScores", "off");
	modSettings[Setting::RRSpeedAboveOneHundred] = reader.GetValue("Toggle Switches", "RRSpeedAboveOneHundred", "off");
	modSettings[Setting::AutoTuneForSong] = reader.GetValue("Toggle Switches", "AutoTuneForSong", "off");
	modSettings[Setting::AutoTuneForSongDevice] = reader.GetValue("Toggle Switches", "AutoTuneForSongDevice", "");
	modSettings[Setting::MidiInDevice] = reader.GetValue("Toggle Switches", "MidiInDevice", "");
	modSettings[Setting::AutoTuneForSongWhen] = reader.GetValue("Toggle Switches", "AutoTuneForSongWhen", "manual");
	modSettings[Setting::AutoTuneForSoftwareSemitoneSettings] = reader.GetValue("Toggle Switches", "AutoTuneForSoftwareSemitoneSettings", "");
	modSettings[Setting::AutoTuneForSoftwareSemitoneTriggers] = reader.GetValue("Toggle Switches", "AutoTuneForSoftwareSemitoneTriggers", "");
	modSettings[Setting::AutoTuneForSoftwareTrueTuningSettings] = reader.GetValue("Toggle Switches", "AutoTuneForSoftwareTrueTuningSettings", "");
	modSettings[Setting::AutoTuneForSoftwareTrueTuningTriggers] = reader.GetValue("Toggle Switches", "AutoTuneForSoftwareTrueTuningTriggers", "");
	modSettings[Setting::ChordsMode] = reader.GetValue("Toggle Switches", "ChordsMode", "off");
	modSettings[Setting::ShowCurrentNoteOnScreen] = reader.GetValue("Toggle Switches", "ShowCurrentNoteOnScreen", "off");
	modSettings[Setting::OnScreenFont] = reader.GetValue("Toggle Switches", "OnScreenFont", "Arial");
	modSettings[Setting::ProfileToLoad] = reader.GetValue("Toggle Switches", "ProfileToLoad", "");
	modSettings[Setting::CustomHighwayColors] = reader.GetValue("Highway Colors", "CustomHighwayColors", "off");
	modSettings[Setting::ShowSongTimerWhen] = reader.GetValue("Toggle Switches", "ShowSongTimerWhen", "manual");
	modSettings[Setting::SecondaryMonitor] = reader.GetValue("Toggle Switches", "SecondaryMonitor", "off");
	modSettings[Setting::SongPreviews] = reader.GetValue("Toggle Switches", "SongPreviews", "off");
	modSettings[Setting::OverrideInputVolumeEnabled] = reader.GetValue("Toggle Switches", "OverrideInputVolumeEnabled", "off");
	modSettings[Setting::OverrideInputVolumeDevice] = reader.GetValue("Toggle Switches", "OverrideInputVolumeDevice", "");
	modSettings[Setting::AllowAudioInBackground] = reader.GetValue("Toggle Switches", "AllowAudioInBackground", "off");
	modSettings[Setting::BypassTwoRTCMessageBox] = reader.GetValue("Toggle Switches", "BypassTwoRTCMessageBox", "off");
	modSettings[Setting::LinearRiffRepeater] = reader.GetValue("Toggle Switches", "LinearRiffRepeater", "off");
	modSettings[Setting::AltOutputSampleRate] = reader.GetValue("Toggle Switches", "AltOutputSampleRate", "off");
	modSettings[Setting::AllowLooping] = reader.GetValue("Toggle Switches", "AllowLooping", "off");
	modSettings[Setting::AllowRewind] = reader.GetValue("Toggle Switches", "AllowRewind", "off");
	modSettings[Setting::FixOculusCrash] = reader.GetValue("Toggle Switches", "FixOculusCrash", "off");
	modSettings[Setting::FixBrokenTones] = reader.GetValue("Toggle Switches", "FixBrokenTones", "off");
	modSettings[Setting::UseCustomNSPTimer] = reader.GetValue("Toggle Switches", "UseCustomNSPTimer", "off");
	modSettings[Setting::DisplayCurrentAccuracy] = reader.GetValue("Toggle Switches", "DisplayCurrentAccuracy", "off");
	modSettings[Setting::PreventMidSongPause] = reader.GetValue("Toggle Switches", "PreventMidSongPause", "off");
	modSettings[Setting::RemoveFingerprints] = reader.GetValue("Toggle Switches", "RemoveFingerprints", "off");
}

/// <summary>
/// Parse String Colors From INI -> Color List
/// </summary>
void Settings::ReadStringColors() {
	CSimpleIniA reader;
	if (reader.LoadFile("RSMods.ini") < 0)
		return;

	std::unique_lock lock(g_settingsMutex);

	customStringColorsNormal.clear();
	customStringColorsCB.clear();
	customNoteColorsNormal.clear();
	customNoteColorsCB.clear();

	for (int stringIdx = 0; stringIdx < 6; stringIdx++) {
		std::string strKey = "";
		std::string val;

		// Read string colors (normal)
		strKey = "string" + std::to_string(stringIdx) + "_N";
		val = reader.GetValue("String Colors", strKey.c_str(), defaultStrColors[stringIdx].c_str());
		customStringColorsNormal.push_back(ConvertHexToColor(val));

		// Read string colors (colorblind)
		strKey = "string" + std::to_string(stringIdx) + "_CB";
		val = reader.GetValue("String Colors", strKey.c_str(), defaultStrColorsCB[stringIdx].c_str());
		customStringColorsCB.push_back(ConvertHexToColor(val));

		// Read note colors (normal)
		strKey = "note" + std::to_string(stringIdx) + "_N";
		val = reader.GetValue("String Colors", strKey.c_str(), defaultStrColors[stringIdx].c_str());
		customNoteColorsNormal.push_back(ConvertHexToColor(val));

		// Read note colors (colorblind)
		strKey = "note" + std::to_string(stringIdx) + "_CB";
		val = reader.GetValue("String Colors", strKey.c_str(), defaultStrColorsCB[stringIdx].c_str());
		customNoteColorsCB.push_back(ConvertHexToColor(val));
	}

	// Set the default colors for deactivated notes.
	for (int stringIdx = 6; stringIdx < 8; stringIdx++) {
		customStringColorsNormal.push_back(ConvertHexToColor(defaultStrColors[stringIdx]));
		customStringColorsCB.push_back(ConvertHexToColor(defaultStrColorsCB[stringIdx]));
		customNoteColorsNormal.push_back(ConvertHexToColor(defaultStrColors[stringIdx]));
		customNoteColorsCB.push_back(ConvertHexToColor(defaultStrColorsCB[stringIdx]));
	}
}

/// <summary>
/// Parse Noteway Colors From INI
/// </summary>
void Settings::ReadNotewayColors() {
	CSimpleIniA reader;
	if (reader.LoadFile("RSMods.ini") < 0) {
		LOG_ERROR("Error reading saved settings" << std::endl);
		return;
	}

	std::unique_lock lock(g_settingsMutex);

	notewayColors = {
			{ "CustomHighwayNumbered", reader.GetValue("Highway Colors", "CustomHighwayNumbered", "") },
			{ "CustomHighwayUnNumbered", reader.GetValue("Highway Colors", "CustomHighwayUnNumbered", "") },
			{ "CustomHighwayGutter", reader.GetValue("Highway Colors", "CustomHighwayGutter", "") },
			{ "CustomFretNubmers", reader.GetValue("Highway Colors", "CustomFretNubmers", "") },
	};
}

/// <summary>
/// Turn ExtendedRangeEnabled off / on, when ToggleExtendedRangeKey is pressed.
/// </summary>
void Settings::ToggleExtendedRangeMode()
{
	std::unique_lock lock(g_settingsMutex);
	modSettings[Setting::ExtendedRangeEnabled] = (ModSettingUnlocked(Setting::ExtendedRangeEnabled) == "on") ? "off" : "on";
}


/// <summary>
/// Read Keybind From INI
/// </summary>
/// <param name="name"> - std::map[key]</param>
/// <returns>Virtual Key | uint</returns>
unsigned int Settings::GetKeyBind(const std::string& name) {
	std::shared_lock lock(g_settingsMutex);
	return VKCodeUnlocked(ModSettingUnlocked(name));
}

/// <summary>
/// Read Mod Setting (internally called customSettings)
/// </summary>
/// <param name="name"> - std::map[key]</param>
/// <returns>Int for mod setting</returns>
int Settings::GetModSetting(const std::string& name) {
	std::shared_lock lock(g_settingsMutex);
	return CustomSettingUnlocked(name);
}

/// <summary>
/// Read Mod Toggle On / Off (internally modSettings)
/// </summary>
/// <param name="name"> - std::map[key]</param>
/// <returns>Value of mod toggle</returns>
std::string Settings::ReturnSettingValue(const std::string& name) {
	std::shared_lock lock(g_settingsMutex);
	return ModSettingUnlocked(name);
}

/// <summary>
/// True when a mod toggle is set to "on". The single home for the on/off convention.
/// </summary>
bool Settings::IsOn(const std::string& name) {
	std::shared_lock lock(g_settingsMutex);
	return ModSettingUnlocked(name) == "on";
}

/// <summary>
/// True only when a mod toggle is literally "off". Deliberately not !IsOn: an unset or
/// unrecognized value is neither on nor off, so this stays a faithful swap for == "off".
/// </summary>
bool Settings::IsOff(const std::string& name) {
	std::shared_lock lock(g_settingsMutex);
	return ModSettingUnlocked(name) == "off";
}

/// <summary>
/// Parse a raw "...When" INI value into its enum. Unrecognized / empty -> When::Unknown.
/// </summary>
Settings::When Settings::ParseWhen(std::string_view value) {
	if (value == "manual")    return When::Manual;
	if (value == "startup")   return When::Startup;
	if (value == "song")      return When::Song;
	if (value == "tuner")     return When::Tuner;
	if (value == "automatic") return When::Automatic;
	return When::Unknown;
}

/// <summary>
/// Read a "...When" setting by name and return it parsed. See Settings::When.
/// </summary>
Settings::When Settings::GetWhen(const std::string& name) {
	std::shared_lock lock(g_settingsMutex);
	return ParseWhen(ModSettingUnlocked(name));
}

template <typename T>
T GetEnumSetting(const std::string& name) {
	static_assert(std::is_enum_v<T>);

	std::shared_lock lock(g_settingsMutex);
	return static_cast<T>(CustomSettingUnlocked(name));
}

/// <summary>
/// Read the CustomStringColors mode. The stored int maps 1:1 onto the enum, so an
/// out-of-range value stays out-of-range and falls through consumers' default cases.
/// </summary>
Settings::StringColorMode Settings::GetStringColorMode() {
	return GetEnumSetting<StringColorMode>(Setting::CustomStringColors);
}
	
/// <summary>
/// Read the SeparateNoteColorsMode. Like StringColorMode, the stored int maps 1:1 onto
/// the enum, so out-of-range values are preserved for consumers' default handling.
/// </summary>
Settings::NoteColorMode Settings::GetNoteColorMode() {
	return GetEnumSetting<NoteColorMode>(Setting::SeparateNoteColorsMode);
}

/// <summary>
/// Get Virtual Key code for input string
/// </summary>
/// <param name="vkString"> - std::map[key]</param>
/// <returns></returns>
int Settings::GetVKCodeForString(const std::string& vkString) {
	std::shared_lock lock(g_settingsMutex);
	return VKCodeUnlocked(vkString);
}

/// <summary>
/// Is the twitch effect on
/// </summary>
/// <param name="name"> - std::map[key]</param>
bool Settings::IsTwitchSettingEnabled(const std::string& name) {
	std::shared_lock lock(g_settingsMutex);
	auto it = twitchSettings.find(name);
	return it != twitchSettings.end() && it->second == "on";
}

/// <summary>
/// Read Noteway Color
/// </summary>
/// <param name="name"> - std::map[key]</param>
/// <returns>HEX color</returns>
std::string Settings::ReturnNotewayColor(const std::string& name) {
	std::shared_lock lock(g_settingsMutex);
	auto it = notewayColors.find(name);
	return it != notewayColors.end() ? it->second : std::string();
}

/// <summary>
/// Split input into list of strings, based on spaces
/// </summary>
/// <param name="input"> - Input string</param>
/// <returns>List of strings taken from input, that were separated by spaces.</returns>
std::vector<std::string> Settings::SplitByWhitespace(const std::string& input) {
	std::regex re("\\s+");
	std::sregex_token_iterator first{ input.begin(), input.end(), re, -1 };
	std::sregex_token_iterator last;

	return { first, last };
}

/// <summary>
/// Change mod setting to new value
/// </summary>
/// <param name="name"> - std::map[key]</param>
/// <param name="newValue"> - new setting value</param>
void Settings::UpdateModSetting(const std::string& name, const std::string_view& newValue) {
	std::unique_lock lock(g_settingsMutex);
	modSettings[name] = newValue;
}

/// <summary>
/// Change custom setting to new value
/// </summary>
/// <param name="name"> - std::map[key]</param>
/// <param name="newValue"> - new setting value</param>
void Settings::UpdateCustomSetting(const std::string& name, int newValue) {
	std::unique_lock lock(g_settingsMutex);
	customSettings[name] = newValue;
}

/// <summary>
/// Change mod setting to new value
/// </summary>
/// <param name="name"> - std::map[key]</param>
/// <param name="newValue"> - new setting value</param>
void Settings::UpdateTwitchSetting(const std::string& name, const std::string_view& newValue) {
	std::unique_lock lock(g_settingsMutex);
	twitchSettings[name] = newValue;
}

/// <summary>
/// Trigger single setting update.
/// </summary>
/// <param name="updateMessage"> - Format: update (custom|mod) name newValue</param>
void Settings::ParseSettingUpdate(const std::string& updateMessage) {
	auto msgParts = SplitByWhitespace(updateMessage);

	if (msgParts.size() < 4)
		return;

	std::string type = msgParts[1];
	std::string entry = msgParts[2];
	std::string value = msgParts[3];

	if (type == "custom") {
		int val = stoi(value);

		UpdateCustomSetting(entry, val);
	}
	else
		UpdateModSetting(entry, value);
}

/// <summary>
/// Read twitch message and edit effect
/// </summary>
/// <param name="twitchMsg"> - twitch message with mod</param>
/// <param name="toggleType"> - "enable" / "disable"</param>
void Settings::ParseTwitchToggle(const std::string& twitchMsg, const std::string_view& toggleType) {
	auto msgParts = SplitByWhitespace(twitchMsg);

	if (msgParts.size() < 2)
		return;

	std::string effectName = msgParts[1];

	std::unique_lock lock(g_settingsMutex);
	twitchSettings[effectName] = toggleType == "enable" ? "on" : "off";
}

/// <summary>
/// Twitch: Solid note color
/// </summary>
/// <param name="twitchMsg"> - twitch message with new color</param>
void Settings::ParseSolidColorsMessage(const std::string& twitchMsg) {
	auto msgParts = SplitByWhitespace(twitchMsg);

	if (msgParts.size() < 3)
		return;

	UpdateModSetting(Setting::SolidNoteColor, msgParts[2]);
}

/// <summary>
/// Get color list of strings
/// </summary>
/// <param name="CB"> - colorblind or not</param>
/// <returns>List of all string colors</returns>
std::vector<RSColor> Settings::GetStringColors(bool CB) {
	std::shared_lock lock(g_settingsMutex);
	if (CB)
		return customStringColorsCB;
	else
		return customStringColorsNormal;
}

/// <summary>
/// Get color list of notes
/// </summary>
/// <param name="CB"> - colorblind or not</param>
/// <returns>List of all note colors</returns>
std::vector<RSColor> Settings::GetNoteColors(bool CB) {
	std::shared_lock lock(g_settingsMutex);
	if (CB)
		return customNoteColorsCB;
	else
		return customNoteColorsNormal;
}


/// <summary>
/// Change string color in color list
/// </summary>
/// <param name="strIndex"> - string number (zero-indexed)</param>
/// <param name="c"> - new color</param>
/// <param name="CB"> - colorblind or not</param>
void Settings::SetStringColors(int strIndex, RSColor c, bool CB) {
	std::unique_lock lock(g_settingsMutex);
	if (CB)
		customStringColorsCB[strIndex] = c;
	else
		customStringColorsNormal[strIndex] = c;
}

/// <summary>
/// Change note color in color list
/// </summary>
/// <param name="strIndex"> - string number (zero-indexed)</param>
/// <param name="c"> - new color</param>
/// <param name="CB"> - colorblind or not</param>
void Settings::SetNoteColors(int strIndex, RSColor c, bool CB) {
	std::unique_lock lock(g_settingsMutex);
	if (CB)
		customNoteColorsCB[strIndex] = c;
	else
		customNoteColorsNormal[strIndex] = c;
}

/// <summary>
/// Re-Parse INI
/// </summary>
void Settings::UpdateSettings() {
	ReadKeyBinds();
	ReadModSettings();
	ReadStringColors();
	ReadNotewayColors();

	async_UpdateMidiSettings = true;
	D3DHooks::RecreateTextures = true;
}

/// <summary>
/// Convert HEX -> Color struct
/// </summary>
/// <param name="hexStr"> - String of hex, without #</param>
/// <returns>Color struct</returns>
RSColor Settings::ConvertHexToColor(const std::string& hexStr) {
	int r, g, b;
	if (sscanf_s(hexStr.c_str(), "%02x%02x%02x", &r, &g, &b) != EOF) {
		RSColor c((float)r / 255, (float)g / 255, (float)b / 255);

		return c;
	}
	else {
		RSColor nullColor(0.0f, 0.0f, 0.0f);
		return nullColor;
	}
}
