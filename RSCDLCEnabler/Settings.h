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
	
	Color ConvertHexToColor(std::string hexStr);

	std::vector<Color> customStringColorsNormal;
	std::vector<Color> customStringColorsCB;

	std::vector<std::string> defaultStrColors = {
		"FF0010", "FFC700", "00A9FF", "FF7100", "43FF00", "BE00FF", "0ABCB9", "909090"
	};

	std::vector<std::string> defaultStrColorsCB = {
		"FF0000", "B1FF00", "00A9FF", "FF5800", "00FFA4", "6A00FF", "493647", "4C4C4C"
	};

};

extern cSettings Settings;