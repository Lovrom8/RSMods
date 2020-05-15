#include <vector>
#include <map>
#include <string>

#pragma once
class Settings
{
public:
	Settings();
	~Settings();
	std::vector<std::string> GetCustomSongTitles();
	char GetKeyBind(std::string name);
	void ReadKeyBinds();

private:
	std::map<std::string, char> keyBinds;
};

