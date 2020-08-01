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
	cSettings();
	std::vector<std::string> GetCustomSongTitles();
	int GetKeyBind(std::string name);
	void ReadKeyBinds();
	void ReadModSettings();
	int GetModSetting(std::string name);
	int GetVKCodeForString(std::string vkString);
	std::string ReturnSettingValue(std::string name);
	void ReadStringColors();
	float GetStringColor(std::string);
	std::vector<Color> GetCustomColors(bool CB);
	void SetStringColors(int strIndex, Color c, bool CB);
	void UpdateSettings();
private:
	std::map<std::string, std::string> keyBinds;
	std::map<std::string, int> customSettings;
	std::map<std::string, int> guitarSpeakNotes;
	std::map<std::string, int> keyMap;
	std::map<std::string, float> stringColors;
};

extern cSettings Settings;