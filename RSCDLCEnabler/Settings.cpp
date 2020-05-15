#include "Settings.h"
#include "INIReader.h"

Settings::Settings()
{
	Settings::keyBinds = { 
	{"VolumeUp",  'O'},
	{"VolumeDown", 'I'},
	{"CustomTitlesSimple", 'L'},
	{"CustomTitlesBetter", 'K'},
	{"ToggleLoft", 'T'}
	};
}


char Settings::GetKeyBind(std::string name) {
	return keyBinds[name];
}

void Settings::ReadKeyBinds() {
	INIReader reader("RSMods.ini");
	if (reader.ParseError() != 0)
		return;

	//Settings::keyBinds["VolumeUp"] = reader.Get("Keybinds", "VolumeUp", "O")[0];
	Settings::keyBinds = {
	{ "VolumeUp", reader.Get("Keybinds", "VolumeUp", "O")[0] },
	{ "VolumeDown",  reader.Get("Keybinds", "VolumeDown", "I")[0] },
	{ "CustomTitlesSimple", reader.Get("Keybinds", "CustomTitlesSimple", "L")[0]},
	{ "CustomTitlesBetter", reader.Get("Keybinds", "CustomTitlesBetter", "K")[0] },
	{ "ToggleLoft", reader.Get("Keybinds", "ToggleLoft", "T")[0] }
	};
}

Settings::~Settings()
{
}


std::vector<std::string> Settings::GetCustomSongTitles() {
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