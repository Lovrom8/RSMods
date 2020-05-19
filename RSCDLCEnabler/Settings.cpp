#include "Settings.h"
#include "INIReader.h"
#include "windows.h"

cSettings Settings;

cSettings::cSettings()
{
	cSettings::keyBinds = { 
	{"VolumeUp",  'O'},
	{"VolumeDown", 'I'},
	{"CustomSongListTitles", 'K'},
	{"ToggleLoft", 'T'},
	{"ForceEnumeration", 'F'}
	};
}


char cSettings::GetKeyBind(std::string name) {
	return keyBinds[name];
}

void cSettings::ReadKeyBinds() {
	INIReader reader("RSMods.ini");
	if (reader.ParseError() != 0)
		return;

	//cSettings::keyBinds["VolumeUp"] = reader.Get("Keybinds", "VolumeUp", "O")[0];
	cSettings::keyBinds = {
	{ "VolumeUp", reader.Get("Keybinds", "VolumeUp", "O")[0] },
	{ "VolumeDown",  reader.Get("Keybinds", "VolumeDown", "I")[0] },
	{ "CustomSongListTitles", reader.Get("Keybinds", "CustomTitlesBetter", "K")[0] },
	{ "ToggleLoft", reader.Get("Keybinds", "ToggleLoft", "T")[0] },
	{ "ForceEnumeration", reader.Get("Keybinds", "ForceEnumeration", "F")[0]}
	};
}

int cSettings::GetModSetting(std::string name) {
	return customSettings[name];
}

void cSettings::ReadModSettings() {
	INIReader reader("RSMods.ini");
	if (reader.ParseError() != 0)
		return;

	cSettings::customSettings = {
		{"ExtendedRangeMode", reader.GetInteger("Mod Settings", "ExtendedRangeModeAt", -5)}
	};
}

std::vector<std::string> cSettings::GetCustomSongTitles() {
	INIReader reader("RSMods.ini");
	std::vector<std::string> retList(6);

	if (reader.ParseError() != 0) {
		//do sth
		return retList;
	}

	for (int i = 0; i < 6; i++)
		retList[i] = reader.Get("SongListTitles", "SongListTitle_" + std::to_string(i + 1), "SONG LIST");
	
	return retList;
}

