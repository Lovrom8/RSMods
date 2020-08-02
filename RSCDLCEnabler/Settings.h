#pragma once
#include "Settings.h"
#include "SimpleIni.h"
#include "windows.h"
#include <iostream>
#include "Structs.h"
#include <vector>

class cSettings
{
public:
	cSettings(); // Default Settings

	// Read INI
	std::vector<std::string> GetCustomSongTitles();
	void ReadKeyBinds();
	void ReadModSettings();
	void ReadStringColors();

	// Return INI Settings
	int GetKeyBind(std::string name);
	int GetModSetting(std::string name);
	std::string ReturnSettingValue(std::string name);

	// Functions
	int GetVKCodeForString(std::string vkString);
	// float GetStringColor(std::string);
	std::vector<Color> GetCustomColors(bool CB);
	void SetStringColors(int strIndex, Color c, bool CB);
	void UpdateSettings();

private:
	// INI Setting Maps
	std::map<std::string, std::string> keyBinds;
	std::map<std::string, std::string> modSettings;
	std::map<std::string, int> customSettings;

	// Misc 
	std::map<std::string, int> keyMap;

};

extern cSettings Settings;