#pragma once
#include <vector>
#include <map>
#include <string>

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
	std::string ReturnToggleValue(std::string name);

private:
	std::map<std::string, std::string> keyBinds;
	std::map<std::string, int> customSettings;
	std::map<std::string, int> keyMap;
};

extern cSettings Settings;