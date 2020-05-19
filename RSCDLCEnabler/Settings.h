#include <vector>
#include <map>
#include <string>

class cSettings
{
public:
	cSettings();
	std::vector<std::string> GetCustomSongTitles();
	char GetKeyBind(std::string name);
	void ReadKeyBinds();
	void ReadModSettings();
	int GetModSetting(std::string name);

private:
	std::map<std::string, char> keyBinds;
	std::map<std::string, int> customSettings;
};

extern cSettings Settings;