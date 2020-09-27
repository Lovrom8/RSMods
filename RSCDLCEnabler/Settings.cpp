#include "Settings.hpp"
#include <regex>

void Settings::Initialize()
{
	modSettings = {  // Default values incase the INI doesn't load
		{"AddVolumeKey",  "O"},
		{"DecreaseVolumeKey", "I"},
		{"ChangedSelectedVolumeKey", "P"},
		{"CustomSongListTitles", "K"},
		{"ToggleLoftKey", "T"},
		{"ShowSongTimerKey", "S"},
		{"ForceReEnumerationKey", "F"},
		{"RainbowStringsKey", "R"},
		{"RemoveLyricsKey", "L"},
		{"MenuToggleKey", "M"},
		{"ForceReEnumerationEnabled", "automatic"},
		{"ToggleLoftEnabled", "on"},
		{"AddVolumeEnabled", "off"},
		{"DecreaseVolumeEnabled", "off"},
		{"ShowSongTimerEnabled", "on"},
		{"ForceReEnumerationEnabled", "on"},
		{"RainbowStringsEnabled", "off"},
		{"ExtendedRangeEnabled", "on"},
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
	};

	customSettings = {
		{"ExtendedRangeMode", -5 },
		{"CheckForNewSongsInterval", 5000},
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
		{"GuitarSpeakForSlash", 0}
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
			{ "AddVolumeKey", reader.GetValue("Keybinds", "AddVolumeKey", "O") },
			{ "DecreaseVolumeKey",  reader.GetValue("Keybinds", "DecreaseVolumeKey", "I") },
			{ "ChangedSelectedVolumeKey", reader.GetValue("Keybinds", "ChangedSelectedVolumeKey", "P") },
			{ "CustomSongListTitles", reader.GetValue("Keybinds", "CustomSongListTitles", "K")},
			{ "ShowSongTimerKey", reader.GetValue("Keybinds", "ShowSongTimerKey", "N")},
			{ "ForceReEnumerationKey", reader.GetValue("Keybinds", "ForceReEnumerationKey", "F")},
			{ "MenuToggleKey", reader.GetValue("Keybinds", "MenuToggleKey", "M")},
			{ "RainbowStringsKey", reader.GetValue("Keybinds", "RainbowStringsKey", "V")},
			{ "RemoveLyricsKey", reader.GetValue("Keybinds", "RemoveLyricsKey", "L")},
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
	};

	// Mods Enabled / Disabled
	modSettings["ToggleLoftEnabled"] = reader.GetValue("Toggle Switches", "ToggleLoft", "on");
	modSettings["AddVolumeEnabled"] = reader.GetValue("Toggle Switches", "AddVolume", "off");
	modSettings["DecreaseVolumeEnabled"] = reader.GetValue("Toggle Switches", "DecreaseVolume", "off");
	modSettings["ShowSongTimerEnabled"] = reader.GetValue("Toggle Switches", "ShowSongTimer", "on");
	modSettings["ForceReEnumerationEnabled"] = reader.GetValue("Toggle Switches", "ForceReEnumeration", "automatic");
	modSettings["RainbowStringsEnabled"] = reader.GetValue("Toggle Switches", "RainbowStrings", "off");
	modSettings["ExtendedRangeEnabled"] = reader.GetValue("Toggle Switches", "ExtendedRange", "on");
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

void Settings::ParseTwitchToggle(std::string twitchMsg) {
	auto msgParts = SplitByWhitespace(twitchMsg);

	if (msgParts.size() < 2)
		return;

	std::string toggleType = msgParts[0];
	std::string effectName = msgParts[1];
	
	twitchSettings[effectName] = toggleType == "enable" ? "on" : "off";
}

void Settings::ParseSolidColorsMessage(std::string twitchMsg) {
	auto msgParts = SplitByWhitespace(twitchMsg);

	if (msgParts.size() != 3)
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