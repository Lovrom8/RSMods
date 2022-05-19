#include "Settings.hpp"
#include <regex>

/// <summary>
/// Load Default Settings.
/// Used if the user has the DLL but no INI.
/// </summary>
void Settings::Initialize()
{
	modSettings = {

		{"CustomSongListTitles", "K"},
		{"ToggleLoftKey", "T"},
		{"ShowSongTimerKey", "S"},
		{"ForceReEnumerationKey", "F"},
		{"RainbowStringsKey", "V"},
		{"RainbowNotesKey", "N"},
		{"RemoveLyricsKey", "L"},
		{"RRSpeedKey", "R"},
		{"MenuToggleKey", "M"},
		{"TuningOffsetKey", "O"},
		{"ToggleExtendedRangeKey", "E"},
		{"LoopStartKey", "Y"},
		{"LoopEndKey", "U"},
		{"RewindKey", "Z"},

		{"MasterVolumeKey", "5"},
		{"SongVolumeKey", "6"},
		{"Player1VolumeKey", "7"},
		{"Player2VolumeKey", "8"},
		{"MicrophoneVolumeKey", "9"},
		{"VoiceOverVolumeKey", "0"},
		{"SFXVolumeKey", "S"},
		{"DisplayMixerKey", "P"},
		{"MutePlayer1Key", "X"},
		{"MutePlayer2Key", "C"},

		{"ForceReEnumerationEnabled", "automatic"},

		{"ToggleLoftEnabled", "off"},
		{"VolumeControlEnabled", "off"},
		{"ShowSongTimerEnabled", "on"},
		{"ForceReEnumerationEnabled", "off"},
		{"RainbowStringsEnabled", "off"},
		{"ExtendedRangeEnabled", "on"},
		{"ExtendedRangeDropTuning", "off"},
		{"ExtendedRangeFixBassTuning", "off"},
		{"SeparateNoteColors", "off"},
		{"DiscoModeEnabled", "off"},
		{"RemoveHeadstockEnabled", "off"},
		{"RemoveSkylineEnabled", "off"},
		{"GreenScreenWallEnabled", "off"},
		{"ForceProfileEnabled", "off"},
		{"FretlessModeEnabled", "off"},
		{"RemoveInlaysEnabled", "off"},
		{"ToggleLoftWhen", "manual"},
		{"ToggleSkylineWhen", "song"},
		{"RemoveLaneMarkersEnabled", "off"},
		{"RemoveLyrics", "off"},
		{"RemoveLyricsWhen", "manual"},
		{"GuitarSpeak", "off"},
		{"GuitarSpeakWhileTuning", "off"},
		{"RemoveHeadstockWhen", "song"},
		{"ScreenShotScores", "off"},
		{"RRSpeedAboveOneHundred", "off"},
		{"AutoTuneForSong", "off"},
		{"AutoTuneForSongDevice", ""},
		{"MidiInDevice", ""},
		{"AutoTuneForSongWhen", "manual"},
		{"AutoTuneForSoftwareSemitoneSettings", ""},
		{"AutoTuneForSoftwareSemitoneTriggers", ""},
		{"AutoTuneForSoftwareTrueTuningSettings", ""},
		{"AutoTuneForSoftwareTrueTuningTriggers", ""},
		{"ChordsMode", "off"},
		{"ShowCurrentNoteOnScreen", "off"},
		{"OnScreenFont", "Arial"},
		{"ProfileToLoad", ""},
		{"ShowSongTimerWhen", "manual"},
		{"ShowSelectedVolumeWhen", "manual"},
		{"SecondaryMonitor", "off"},
		{"SongPreviews", "off"},
		{"OverrideInputVolumeEnabled", "off"},
		{"OverrideInputVolumeDevice", ""},
		{"AllowAudioInBackground", "off"},
		{"BypassTwoRTCMessageBox", "off"},
		{"LinearRiffRepeater", "off"},
		{"AltOutputSampleRate", "off"},
		{"AllowLooping", "off"},
		{"AllowRewind", "off"},
		{"FixOculusCrash", "off"},
		{"FixBrokenTones", "off"},
		{"UseCustomNSPTimer", "off"}
	};

	customSettings = {
		{"ExtendedRangeMode", -5},
		{"CheckForNewSongsInterval", 5000},
		{"RRSpeedInterval", 0},
		{"TuningPedal", 0},
		{"TuningOffset", 0},
		{"VolumeControlInterval", 5},
		{"SecondaryMonitorXPosition", 0},
		{"SecondaryMonitorYPosition", 0},
		{"SeparateNoteColorsMode", 0},
		{"OverrideInputVolume", 17},
		{"CustomStringColors", 0},
		{"AlternativeOutputSampleRate", 48000},
		{"LoopingLeadUp", 0},
		{"RewindBy", 0},
		{"CustomNSPTimeLimit", 10000},

		{"GuitarSpeakDelete", 0},
		{"GuitarSpeakSpace", 0},
		{"GuitarSpeakEnter", 0},
		{"GuitarSpeakTab", 0},
		{"GuitarSpeakPageUp", 0},
		{"GuitarSpeakPageDown", 0},
		{"GuitarSpeakUpArrow", 0},
		{"GuitarSpeakDownArrow", 0},
		{"GuitarSpeakEscape", 0},
		{"GuitarSpeakClose", 0},
		{"GuitarSpeakOBracket", 0},
		{"GuitarSpeakCBracket", 0},
		{"GuitarSpeakTildea", 0},
		{"GuitarSpeakForSlash", 0},
		{"GuitarSpeakAlt", 0}
	};

	twitchSettings = {
		{"RainbowStrings", "off"},
		{"RemoveNotes", "off"},
		{"TransparentNotes", "off"},
		{"SolidNotes", "off"},
		{"DrunkMode", "off"},
		{"FYourFC", "off"},
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
	_LOG_INIT;

	_LOG_SETLEVEL(LogLevel::Error);

	CSimpleIniA reader;
	if (reader.LoadFile("RSMods.ini") < 0) {
		_LOG("Error reading saved settings" << std::endl);
		return;
	}

	//_LOG(reader.GetValue("Keybinds", "ToggleLoftKey", "T") << std::endl);
	modSettings = {
		// Mods
			{ "ToggleLoftKey", reader.GetValue("Keybinds", "ToggleLoftKey", "T") },
			{ "CustomSongListTitles", reader.GetValue("Keybinds", "CustomSongListTitles", "K")},
			{ "ShowSongTimerKey", reader.GetValue("Keybinds", "ShowSongTimerKey", "N")},
			{ "ForceReEnumerationKey", reader.GetValue("Keybinds", "ForceReEnumerationKey", "F")},
			{ "MenuToggleKey", reader.GetValue("Keybinds", "MenuToggleKey", "M")},
			{ "RainbowStringsKey", reader.GetValue("Keybinds", "RainbowStringsKey", "V")},
			{ "RainbowNotesKey", reader.GetValue("Keybinds", "RainbowNotesKey", "N")},
			{ "RemoveLyricsKey", reader.GetValue("Keybinds", "RemoveLyricsKey", "L")},
			{ "RRSpeedKey", reader.GetValue("Keybinds", "RRSpeedKey", "R")},
			{ "TuningOffsetKey", reader.GetValue("Keybinds", "TuningOffsetKey", "O")},
			{ "ToggleExtendedRangeKey", reader.GetValue("Keybinds", "ToggleExtendedRangeKey", "E")},
			{ "LoopStartKey", reader.GetValue("Keybinds", "LoopStartKey", "Y")},
			{ "LoopEndKey", reader.GetValue("Keybinds", "LoopEndKey", "U")},
			{ "RewindKey", reader.GetValue("Keybinds", "RewindKey", "Z")},

			{ "MasterVolumeKey", reader.GetValue("Audio Keybindings", "MasterVolumeKey", "5") },
			{ "SongVolumeKey", reader.GetValue("Audio Keybindings", "SongVolumeKey", "6") },
			{ "Player1VolumeKey", reader.GetValue("Audio Keybindings", "Player1VolumeKey", "7") },
			{ "Player2VolumeKey", reader.GetValue("Audio Keybindings", "Player2VolumeKey", "8") },
			{ "MicrophoneVolumeKey", reader.GetValue("Audio Keybindings", "MicrophoneVolumeKey", "9") },
			{ "VoiceOverVolumeKey", reader.GetValue("Audio Keybindings", "VoiceOverVolumeKey", "0") },
			{ "SFXVolumeKey", reader.GetValue("Audio Keybindings", "SFXVolumeKey", "S") },
			{ "DisplayMixerKey", reader.GetValue("Audio Keybindings", "DisplayMixerKey", "P") },
			{ "MutePlayer1Key", reader.GetValue("Audio Keybindings", "MutePlayer1Key", "X")},
			{ "MutePlayer2Key", reader.GetValue("Audio Keybindings", "MutePlayer2Key", "C")}
	};
	// _LOG("Read " << modSettings["ToggleLoftKey"] << std::endl);
}

/// <summary>
/// Parse Settings For Mods
/// </summary>
void Settings::ReadModSettings() {
	_LOG_INIT;

	_LOG_SETLEVEL(LogLevel::Error);

	CSimpleIniA reader;
	if (reader.LoadFile("RSMods.ini") < 0) {
		_LOG("Error reading saved settings" << std::endl);
		return;
	}

	customSettings = {
		{"ExtendedRangeMode", reader.GetLongValue("Mod Settings", "ExtendedRangeModeAt", -5)},
		{"CheckForNewSongsInterval", reader.GetLongValue("Mod Settings", "CheckForNewSongsInterval", 5000)},
		{"RRSpeedInterval", reader.GetLongValue("Mod Settings", "RRSpeedInterval", 0)},
		{"TuningPedal", reader.GetLongValue("Mod Settings", "TuningPedal", 0)},
		{"TuningOffset", reader.GetLongValue("Mod Settings", "TuningOffset", 0)},
		{"VolumeControlInterval", reader.GetLongValue("Mod Settings", "VolumeControlInterval", 5)},
		{"SecondaryMonitorXPosition", reader.GetLongValue("Mod Settings", "SecondaryMonitorXPosition", 0)},
		{"SecondaryMonitorYPosition", reader.GetLongValue("Mod Settings", "SecondaryMonitorYPosition", 0)},
		{"SeparateNoteColorsMode", reader.GetLongValue("Mod Settings", "SeparateNoteColorsMode", 0)}, // 0 = same as strings, 1 = default, 2 = custom
		{"CustomStringColors", reader.GetLongValue("Toggle Switches", "CustomStringColors", 0)}, //0 = default, 1 = Zag, 2 = custom colors
		{"OverrideInputVolume", reader.GetLongValue("Mod Settings", "OverrideInputVolume", 17)}, // 17 is what Rocksmith calls default.
		{"AlternativeOutputSampleRate", reader.GetLongValue("Mod Settings", "AlternativeOutputSampleRate", 48000)},
		{"LoopingLeadUp", reader.GetLongValue("Mod Settings", "LoopingLeadUp", 0)},
		{"RewindBy", reader.GetLongValue("Mod Settings", "RewindBy", 0)},
		{"CustomNSPTimeLimit", reader.GetLongValue("Mod Settings", "CustomNSPTimeLimit", 10000)},

		{"GuitarSpeakDelete", reader.GetLongValue("Guitar Speak", "GuitarSpeakDeleteWhen", 0)},
		{"GuitarSpeakSpace", reader.GetLongValue("Guitar Speak", "GuitarSpeakSpaceWhen", 0)},
		{"GuitarSpeakEnter", reader.GetLongValue("Guitar Speak", "GuitarSpeakEnterWhen", 0)},
		{"GuitarSpeakTab", reader.GetLongValue("Guitar Speak", "GuitarSpeakTabWhen", 0)},
		{"GuitarSpeakPageUp", reader.GetLongValue("Guitar Speak", "GuitarSpeakPGUPWhen", 0)},
		{"GuitarSpeakPageDown", reader.GetLongValue("Guitar Speak", "GuitarSpeakPGDNWhen", 0)},
		{"GuitarSpeakUpArrow", reader.GetLongValue("Guitar Speak", "GuitarSpeakUPWhen", 0)},
		{"GuitarSpeakDownArrow", reader.GetLongValue("Guitar Speak", "GuitarSpeakDNWhen", 0)},
		{"GuitarSpeakEscape", reader.GetLongValue("Guitar Speak", "GuitarSpeakESCWhen", 0)},
		{"GuitarSpeakClose", reader.GetLongValue("Guitar Speak", "GuitarSpeakCloseWhen", 0)},
		{"GuitarSpeakOBracket", reader.GetLongValue("Guitar Speak", "GuitarSpeakOBracketWhen", 0)},
		{"GuitarSpeakCBracket", reader.GetLongValue("Guitar Speak", "GuitarSpeakCBracketWhen", 0)},
		{"GuitarSpeakTildea", reader.GetLongValue("Guitar Speak", "GuitarSpeakTildeaWhen", 0)},
		{"GuitarSpeakForSlash", reader.GetLongValue("Guitar Speak", "GuitarSpeakForSlashWhen", 0)},
		{"GuitarSpeakAlt", reader.GetLongValue("Guitar Speak", "GuitarSpeakAltWhen", 0)},
	};

	// Mods Enabled / Disabled
	modSettings["ToggleLoftEnabled"] = reader.GetValue("Toggle Switches", "ToggleLoft", "on");
	modSettings["VolumeControlEnabled"] = reader.GetValue("Toggle Switches", "VolumeControl", "off");
	modSettings["ShowSongTimerEnabled"] = reader.GetValue("Toggle Switches", "ShowSongTimer", "off");
	modSettings["ForceReEnumerationEnabled"] = reader.GetValue("Toggle Switches", "ForceReEnumeration", "automatic");
	modSettings["RainbowStringsEnabled"] = reader.GetValue("Toggle Switches", "RainbowStrings", "off");
	modSettings["RainbowNotesEnabled"] = reader.GetValue("Toggle Switches", "RainbowNotes", "off");
	modSettings["ExtendedRangeEnabled"] = reader.GetValue("Toggle Switches", "ExtendedRange", "off");
	modSettings["ExtendedRangeDropTuning"] = reader.GetValue("Toggle Switches", "ExtendedRangeDropTuning", "off");
	modSettings["ExtendedRangeFixBassTuning"] = reader.GetValue("Toggle Switches", "ExtendedRangeFixBassTuning", "off");
	modSettings["SeparateNoteColors"] = reader.GetValue("Toggle Switches", "SeparateNoteColors", "off");
	modSettings["DiscoModeEnabled"] = reader.GetValue("Toggle Switches", "DiscoMode", "off");
	modSettings["RemoveHeadstockEnabled"] = reader.GetValue("Toggle Switches", "Headstock", "off");
	modSettings["RemoveSkylineEnabled"] = reader.GetValue("Toggle Switches", "Skyline", "off");
	modSettings["GreenScreenWallEnabled"] = reader.GetValue("Toggle Switches", "GreenScreenWall", "off");
	modSettings["ForceProfileEnabled"] = reader.GetValue("Toggle Switches", "ForceProfileLoad", "off");
	modSettings["FretlessModeEnabled"] = reader.GetValue("Toggle Switches", "Fretless", "off");
	modSettings["RemoveInlaysEnabled"] = reader.GetValue("Toggle Switches", "Inlays", "off");
	modSettings["ToggleLoftWhen"] = reader.GetValue("Toggle Switches", "ToggleLoftWhen", "manual");
	modSettings["ToggleSkylineWhen"] = reader.GetValue("Toggle Switches", "ToggleSkylineWhen", "song");
	modSettings["RemoveLaneMarkersEnabled"] = reader.GetValue("Toggle Switches", "LaneMarkers", "off");
	modSettings["RemoveLyrics"] = reader.GetValue("Toggle Switches", "Lyrics", "off");
	modSettings["RemoveLyricsWhen"] = reader.GetValue("Toggle Switches", "RemoveLyricsWhen", "manual");
	modSettings["GuitarSpeak"] = reader.GetValue("Toggle Switches", "GuitarSpeak", "off");
	modSettings["GuitarSpeakWhileTuning"] = reader.GetValue("Guitar Speak", "GuitarSpeakWhileTuning", "off");
	modSettings["RemoveHeadstockWhen"] = reader.GetValue("Toggle Switches", "RemoveHeadstockWhen", "song");
	modSettings["ScreenShotScores"] = reader.GetValue("Toggle Switches", "ScreenShotScores", "off");
	modSettings["RRSpeedAboveOneHundred"] = reader.GetValue("Toggle Switches", "RRSpeedAboveOneHundred", "off");
	modSettings["AutoTuneForSong"] = reader.GetValue("Toggle Switches", "AutoTuneForSong", "off");
	modSettings["AutoTuneForSongDevice"] = reader.GetValue("Toggle Switches", "AutoTuneForSongDevice", "");
	modSettings["MidiInDevice"] = reader.GetValue("Toggle Switches", "MidiInDevice", "");
	modSettings["AutoTuneForSongWhen"] = reader.GetValue("Toggle Switches", "AutoTuneForSongWhen", "manual");
	modSettings["AutoTuneForSoftwareSemitoneSettings"] = reader.GetValue("Toggle Switches", "AutoTuneForSoftwareSemitoneSettings", "");
	modSettings["AutoTuneForSoftwareSemitoneTriggers"] = reader.GetValue("Toggle Switches", "AutoTuneForSoftwareSemitoneTriggers", "");
	modSettings["AutoTuneForSoftwareTrueTuningSettings"] = reader.GetValue("Toggle Switches", "AutoTuneForSoftwareTrueTuningSettings", "");
	modSettings["AutoTuneForSoftwareTrueTuningTriggers"] = reader.GetValue("Toggle Switches", "AutoTuneForSoftwareTrueTuningTriggers", "");
	modSettings["ChordsMode"] = reader.GetValue("Toggle Switches", "ChordsMode", "off");
	modSettings["ShowCurrentNoteOnScreen"] = reader.GetValue("Toggle Switches", "ShowCurrentNoteOnScreen", "off");
	modSettings["OnScreenFont"] = reader.GetValue("Toggle Switches", "OnScreenFont", "Arial");
	modSettings["ProfileToLoad"] = reader.GetValue("Toggle Switches", "ProfileToLoad", "");
	modSettings["CustomHighwayColors"] = reader.GetValue("Highway Colors", "CustomHighwayColors", "");
	modSettings["ShowSongTimerWhen"] = reader.GetValue("Toggle Switches", "ShowSongTimerWhen", "manual");
	modSettings["ShowSelectedVolumeWhen"] = reader.GetValue("Toggle Switches", "ShowSelectedVolumeWhen", "manual");
	modSettings["SecondaryMonitor"] = reader.GetValue("Toggle Switches", "SecondaryMonitor", "off");
	modSettings["SongPreviews"] = reader.GetValue("Toggle Switches", "SongPreviews", "off");
	modSettings["OverrideInputVolumeEnabled"] = reader.GetValue("Toggle Switches", "OverrideInputVolumeEnabled", "off");
	modSettings["OverrideInputVolumeDevice"] = reader.GetValue("Toggle Switches", "OverrideInputVolumeDevice", "");
	modSettings["AllowAudioInBackground"] = reader.GetValue("Toggle Switches", "AllowAudioInBackground", "off");
	modSettings["BypassTwoRTCMessageBox"] = reader.GetValue("Toggle Switches", "BypassTwoRTCMessageBox", "off");
	modSettings["LinearRiffRepeater"] = reader.GetValue("Toggle Switches", "LinearRiffRepeater", "off");
	modSettings["AltOutputSampleRate"] = reader.GetValue("Toggle Switches", "AltOutputSampleRate", "off");
	modSettings["AllowLooping"] = reader.GetValue("Toggle Switches", "AllowLooping", "off");
	modSettings["AllowRewind"] = reader.GetValue("Toggle Switches", "AllowRewind", "off");
	modSettings["FixOculusCrash"] = reader.GetValue("Toggle Switches", "FixOculusCrash", "off");
	modSettings["FixBrokenTones"] = reader.GetValue("Toggle Switches", "FixBrokenTones", "off");
	modSettings["UseCustomNSPTimer"] = reader.GetValue("Toggle Switches", "UseCustomNSPTimer", "off");
}

/// <summary>
/// Parse String Colors From INI -> Color List
/// </summary>
void Settings::ReadStringColors() {
	CSimpleIniA reader;
	if (reader.LoadFile("RSMods.ini") < 0)
		return;

	// Clear the previous string colors
	customStringColorsNormal.clear();
	customStringColorsCB.clear();
	customNoteColorsNormal.clear();
	customNoteColorsCB.clear();

	// Loop throught the strings to make the code easier to read.
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
	_LOG_INIT;

	_LOG_SETLEVEL(LogLevel::Error);

	CSimpleIniA reader;
	if (reader.LoadFile("RSMods.ini") < 0) {
		_LOG("Error reading saved settings" << std::endl);
		return;
	}

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
	modSettings["ExtendedRangeEnabled"] = (modSettings["ExtendedRangeEnabled"] == "on") ? "off" : "on";
}


/// <summary>
/// Read Keybind From INI
/// </summary>
/// <param name="name"> - std::map[key]</param>
/// <returns>Virtual Key | uint</returns>
unsigned int Settings::GetKeyBind(std::string name) {
	return GetVKCodeForString(modSettings[name]);
}

/// <summary>
/// Read Mod Setting (internally called customSettings)
/// </summary>
/// <param name="name"> - std::map[key]</param>
/// <returns>Int for mod setting</returns>
int Settings::GetModSetting(std::string name) {
	return customSettings[name];
}

/// <summary>
/// Read Mod Toggle On / Off (internally modSettings)
/// </summary>
/// <param name="name"> - std::map[key]</param>
/// <returns>Value of mod toggle</returns>
std::string Settings::ReturnSettingValue(std::string name) {
	return modSettings[name];
}

/// <summary>
/// Get Virtual Key code for input string
/// </summary>
/// <param name="vkString"> - std::map[key]</param>
/// <returns></returns>
int Settings::GetVKCodeForString(std::string vkString) {
	return keyMap[vkString];
}

/// <summary>
/// Is the twitch effect on
/// </summary>
/// <param name="name"> - std::map[key]</param>
bool Settings::IsTwitchSettingEnabled(std::string name) {
	if (twitchSettings.count(name) == 0) // JIC
		return false;

	return twitchSettings[name] == "on";
}

/// <summary>
/// Read Noteway Color
/// </summary>
/// <param name="name"> - std::map[key]</param>
/// <returns>HEX color</returns>
std::string Settings::ReturnNotewayColor(std::string name) {
	return notewayColors[name];
}

/// <summary>
/// Split input into list of strings, based on spaces
/// </summary>
/// <param name="input"> - Input string</param>
/// <returns>List of strings taken from input, that were seperated by spaces.</returns>
std::vector<std::string> Settings::SplitByWhitespace(const std::string& input) {
	std::regex re("\\s+");
	std::sregex_token_iterator first{ input.begin(), input.end(), re, -1 }, last;
	return { first, last };
}

/// <summary>
/// Change mod setting to new value
/// </summary>
/// <param name="name"> - std::map[key]</param>
/// <param name="newValue"> - new setting value</param>
void Settings::UpdateModSetting(std::string name, std::string newValue) {
	modSettings[name] = newValue;
}

/// <summary>
/// Change custom setting to new value
/// </summary>
/// <param name="name"> - std::map[key]</param>
/// <param name="newValue"> - new setting value</param>
void Settings::UpdateCustomSetting(std::string name, int newValue) {
	customSettings[name] = newValue;
}

/// <summary>
/// Change mod setting to new value
/// </summary>
/// <param name="name"> - std::map[key]</param>
/// <param name="newValue"> - new setting value</param>
void Settings::UpdateTwitchSetting(std::string name, std::string newValue) {
	twitchSettings[name] = newValue;
}

/// <summary>
/// Trigger single setting update.
/// </summary>
/// <param name="updateMessage"> - Format: update (custom|mod) name newValue</param>
void Settings::ParseSettingUpdate(std::string updateMessage) {
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
void Settings::ParseTwitchToggle(std::string twitchMsg, std::string toggleType) {
	auto msgParts = SplitByWhitespace(twitchMsg);

	if (msgParts.size() < 2)
		return;

	std::string effectName = msgParts[1];

	twitchSettings[effectName] = toggleType == "enable" ? "on" : "off";
}

/// <summary>
/// Twitch: Solid note color
/// </summary>
/// <param name="twitchMsg"> - twitch message with new color</param>
void Settings::ParseSolidColorsMessage(std::string twitchMsg) {
	auto msgParts = SplitByWhitespace(twitchMsg);

	if (msgParts.size() < 3)
		return;

	UpdateModSetting("SolidNoteColor", msgParts[2]);
}

/*
float cSettings::GetStringColor(std::string string) {
	return stringColors[string];
}
*/

/// <summary>
/// Get color list of strings
/// </summary>
/// <param name="CB"> - colorblind or not</param>
/// <returns>List of all string colors</returns>
std::vector<Color> Settings::GetStringColors(bool CB) {
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
std::vector<Color> Settings::GetNoteColors(bool CB) {
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
void Settings::SetStringColors(int strIndex, Color c, bool CB) {
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
void Settings::SetNoteColors(int strIndex, Color c, bool CB) {
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


// Misc

/// <summary>
/// Convert HEX -> Color struct
/// </summary>
/// <param name="hexStr"> - String of hex, without #</param>
/// <returns>Color struct</returns>
Color Settings::ConvertHexToColor(std::string hexStr) {
	int r, g, b;
	if (sscanf_s(hexStr.c_str(), "%02x%02x%02x", &r, &g, &b) != EOF) {
		Color c((float)r / 255, (float)g / 255, (float)b / 255);

		return c;
	}
	else {
		Color nullColor(0.0f, 0.0f, 0.0f);
		return nullColor;
	}
}