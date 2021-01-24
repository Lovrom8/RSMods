#include "Settings.hpp"
#include <regex>

void Settings::Initialize()
{
	modSettings = {  // Default values incase the INI doesn't load

		{"CustomSongListTitles", "K"},
		{"ToggleLoftKey", "T"},
		{"ShowSongTimerKey", "S"},
		{"ForceReEnumerationKey", "F"},
		{"RainbowStringsKey", "V"},
		{"RainbowNotesKey", "N"},
		{"RemoveLyricsKey", "L"},
		{"RRSpeedKey", "R"},
		{"MenuToggleKey", "M"},

		{"MasterVolumeKey", "5"},
		{"SongVolumeKey", "6"},
		{"Player1VolumeKey", "7"},
		{"Player2VolumeKey", "8"},
		{"MicrophoneVolumeKey", "9"},
		{"VoiceOverVolumeKey", "0"},
		{"SFXVolumeKey", "S"},
		{"ChangedSelectedVolumeKey", "P"},

		{"ForceReEnumerationEnabled", "automatic"},

		{"ToggleLoftEnabled", "off"},
		{"VolumeControlEnabled", "off"},
		{"ShowSongTimerEnabled", "on"},
		{"ForceReEnumerationEnabled", "off"},
		{"RainbowStringsEnabled", "off"},
		{"ExtendedRangeEnabled", "on"},
		{"ExtendedRangeDropTuning", "off"},
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
		{"ChordsMode", "off"},
		{"ShowCurrentNoteOnScreen", "off"},
		{"OnScreenFont", "Arial"},
		{"ProfileToLoad", ""},
		{"ShowSongTimerWhen", "manual"},
		{"ShowSelectedVolumeWhen", "manual"},
		{"SecondaryMonitor", "off"},
	};

	customSettings = {
		{"ExtendedRangeMode", -5},
		{"CheckForNewSongsInterval", 5000},
		{"RRSpeedInterval", 0},
		{"TuningPedal", 0},
		{"VolumeControlInterval", 5},
		{"SecondaryMonitorPosition", 0},
		{"CustomStringColors", 0},
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

std::vector<std::string> Settings::GetCustomSongTitles() {
	std::vector<std::string> retList(6);
	CSimpleIniA reader;
	if (reader.LoadFile("RSMods.ini") < 0)
		return retList;

	for (int i = 0; i < 6; i++) {
		std::string songListName = "SongListTitle_" + std::to_string(i + 1);
		retList[i] = reader.GetValue("SongListTitles", songListName.c_str(), "SONG LIST");
		//std::cout << "Song List #" << (i+1) << ": " << retList[i] << std::endl;
	}

	return retList;
}

void Settings::ReadKeyBinds() {
	CSimpleIniA reader;
	if (reader.LoadFile("RSMods.ini") < 0) {
		std::cout << "Error reading saved settings" << std::endl;
		return;
	}

	//std::cout << reader.GetValue("Keybinds", "ToggleLoftKey", "T") << std::endl;
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

			{ "MasterVolumeKey", reader.GetValue("Audio Keybindings", "MasterVolumeKey", "5") },
			{ "SongVolumeKey", reader.GetValue("Audio Keybindings", "SongVolumeKey", "6") },
			{ "Player1VolumeKey", reader.GetValue("Audio Keybindings", "Player1VolumeKey", "7") },
			{ "Player2VolumeKey", reader.GetValue("Audio Keybindings", "Player2VolumeKey", "8") },
			{ "MicrophoneVolumeKey", reader.GetValue("Audio Keybindings", "MicrophoneVolumeKey", "9") },
			{ "VoiceOverVolumeKey", reader.GetValue("Audio Keybindings", "VoiceOverVolumeKey", "0") },
			{ "SFXVolumeKey", reader.GetValue("Audio Keybindings", "SFXVolumeKey", "S") },
			{ "ChangedSelectedVolumeKey", reader.GetValue("Audio Keybindings", "ChangedSelectedVolumeKey", "P") },
	};
	//std::cout << "Read " << modSettings["ToggleLoftKey"] << std::endl;
}

void Settings::ReadModSettings() {
	CSimpleIniA reader;
	if (reader.LoadFile("RSMods.ini") < 0) {
		std::cout << "Error reading saved settings" << std::endl;
		return;
	}

	customSettings = {
		{"ExtendedRangeMode", reader.GetLongValue("Mod Settings", "ExtendedRangeModeAt", -5)},
		{"CheckForNewSongsInterval", reader.GetLongValue("Mod Settings", "CheckForNewSongsInterval", 5000)},
		{"RRSpeedInterval", reader.GetLongValue("Mod Settings", "RRSpeedInterval", 0)},
		{"TuningPedal", reader.GetLongValue("Mod Settings", "TuningPedal", 0)},
		{"VolumeControlInterval", reader.GetLongValue("Mod Settings", "VolumeControlInterval", 5)},
		{"SecondaryMonitorPosition", reader.GetLongValue("Mod Settings", "SecondaryMonitorPosition", 0)},
		{"CustomStringColors", reader.GetLongValue("Toggle Switches", "CustomStringColors", 0)}, //0 = default, 1 = Zag, 2 = custom colors
		{"GuitarSpeakDelete", reader.GetLongValue("Guitar Speak", "GuitarSpeakDeleteWhen", 0)},
		{"GuitarSpeakSpace", reader.GetLongValue("Guitar Speak", "GuitarSpeakSpaceWhen", 0)},
		{"GuitarSpeakEnter", reader.GetLongValue("Guitar Speak", "GuitarSpeakEnterWhen", 0)},
		{"GuitarSpeakTab", reader.GetLongValue("Guitar Speak", "GuitarSpeakTabWhen", 0)},
		{"GuitarSpeakPageUp", reader.GetLongValue("Guitar Speak", "GuitarSpeakPGUPWhen", 0)},
		{"GuitarSpeakPageDown", reader.GetLongValue("Guitar Speak", "GuitarSpeakPGDNWhen", 0)},
		{"GuitarSpeakUpArrow", reader.GetLongValue("Guitar Speak", "GuitarSpeakUPWhen", 0)},
		{"GuitarSpeakDownArrow", reader.GetLongValue("Guitar Speak", "GuitarSpeanDNWhen", 0)},
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
	modSettings["ChordsMode"] = reader.GetValue("Toggle Switches", "ChordsMode", "off");
	modSettings["ShowCurrentNoteOnScreen"] = reader.GetValue("Toggle Switches", "ShowCurrentNoteOnScreen", "off");
	modSettings["OnScreenFont"] = reader.GetValue("Toggle Switches", "OnScreenFont", "Arial");
	modSettings["ProfileToLoad"] = reader.GetValue("Toggle Switches", "ProfileToLoad", "");
	modSettings["CustomHighwayColors"] = reader.GetValue("Highway Colors", "CustomHighwayColors", "");
	modSettings["ShowSongTimerWhen"] = reader.GetValue("Toggle Switches", "ShowSongTimerWhen", "manual");
	modSettings["ShowSelectedVolumeWhen"] = reader.GetValue("Toggle Switches", "ShowSelectedVolumeWhen", "manual");
	modSettings["SecondaryMonitor"] = reader.GetValue("Toggle Switches", "SecondaryMonitor", "off");
}

void Settings::ReadStringColors() {
	CSimpleIniA reader;
	if (reader.LoadFile("RSMods.ini") < 0)
		return;

	customStringColorsNormal.clear();
	customStringColorsCB.clear();

	for (int stringIdx = 0; stringIdx < 6; stringIdx++) {
		std::string strKey = "";
		std::string val;

		strKey = "string" + std::to_string(stringIdx) + "_N";
		val = reader.GetValue("String Colors", strKey.c_str(), defaultStrColors[stringIdx].c_str());

		customStringColorsNormal.push_back(ConvertHexToColor(val));

		strKey = "string" + std::to_string(stringIdx) + "_CB";
		val = reader.GetValue("String Colors", strKey.c_str(), defaultStrColorsCB[stringIdx].c_str());

		customStringColorsCB.push_back(ConvertHexToColor(val));
	}

	for (int stringIdx = 6; stringIdx < 8; stringIdx++) {
		customStringColorsNormal.push_back(ConvertHexToColor(defaultStrColors[stringIdx]));
		customStringColorsCB.push_back(ConvertHexToColor(defaultStrColorsCB[stringIdx]));
	}
}

void Settings::ReadNotewayColors() {
	CSimpleIniA reader;
	if (reader.LoadFile("RSMods.ini") < 0) {
		std::cout << "Error reading saved settings" << std::endl;
		return;
	}

	notewayColors = {
			{ "CustomHighwayNumbered", reader.GetValue("Highway Colors", "CustomHighwayNumbered", "") },
			{ "CustomHighwayUnNumbered", reader.GetValue("Highway Colors", "CustomHighwayUnNumbered", "") },
			{ "CustomHighwayGutter", reader.GetValue("Highway Colors", "CustomHighwayGutter", "") },
			{ "CustomFretNubmers", reader.GetValue("Highway Colors", "CustomFretNubmers", "") },
	};
}


// Return INI Settings

unsigned int Settings::GetKeyBind(std::string name) {
	return GetVKCodeForString(modSettings[name]);
}

int Settings::GetModSetting(std::string name) {
	return customSettings[name];
}

std::string Settings::ReturnSettingValue(std::string name) {
	return modSettings[name];
}

bool Settings::IsTwitchSettingEnabled(std::string name) {
	if (twitchSettings.count(name) == 0) // JIC
		return false;

	return twitchSettings[name] == "on";
}

std::string Settings::ReturnNotewayColor(std::string name) {
	return notewayColors[name];
}

// Misc Functions

std::vector<std::string> Settings::SplitByWhitespace(const std::string& input) {
	std::regex re("\\s+");
	std::sregex_token_iterator first{ input.begin(), input.end(), re, -1 }, last;
	return { first, last };
}

void Settings::UpdateModSetting(std::string name, std::string newValue) {
	modSettings[name] = newValue;
}

void Settings::UpdateCustomSetting(std::string name, int newValue) {
	customSettings[name] = newValue;
}


void Settings::UpdateTwitchSetting(std::string name, std::string newValue) {
	twitchSettings[name] = newValue;
}

void Settings::ParseSettingUpdate(std::string updateMessage) {
	auto msgParts = SplitByWhitespace(updateMessage); // Format: update (custom|mod) <entryName> <value>

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

void Settings::ParseTwitchToggle(std::string twitchMsg, std::string toggleType) {
	auto msgParts = SplitByWhitespace(twitchMsg);

	if (msgParts.size() < 2)
		return;

	std::string effectName = msgParts[1];

	twitchSettings[effectName] = toggleType == "enable" ? "on" : "off";
}

void Settings::ParseSolidColorsMessage(std::string twitchMsg) {
	auto msgParts = SplitByWhitespace(twitchMsg);

	if (msgParts.size() < 3)
		return;

	UpdateModSetting("SolidNoteColor", msgParts[2]);
}

int Settings::GetVKCodeForString(std::string vkString) {
	return keyMap[vkString];
}

/*
float cSettings::GetStringColor(std::string string) {
	return stringColors[string];
}
*/

std::vector<Color> Settings::GetCustomColors(bool CB) {
	if (CB)
		return customStringColorsCB;
	else
		return customStringColorsNormal;
}

void Settings::SetStringColors(int strIndex, Color c, bool CB) {
	if (CB)
		customStringColorsCB[strIndex] = c;
	else
		customStringColorsNormal[strIndex] = c;
}

void Settings::UpdateSettings() {
	ReadKeyBinds();
	ReadModSettings();
	ReadStringColors();
	ReadNotewayColors();
}


// Misc

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