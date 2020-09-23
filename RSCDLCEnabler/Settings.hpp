#pragma once
#include "Settings.hpp"
#include "Lib/Ini/SimpleIni.h"
#include "windows.h"
#include <iostream>
#include "Structs.hpp"
#include <vector>

namespace Settings {
	void Initialize(); // Default Settings

	// Read INI
	std::vector<std::string> GetCustomSongTitles();
	void ReadKeyBinds();
	void ReadModSettings();
	void ReadStringColors();

	// Return INI Settings
	unsigned int GetKeyBind(std::string name);
	int GetModSetting(std::string name);
	std::string ReturnSettingValue(std::string name);

	// Functions
	int GetVKCodeForString(std::string vkString);
	// float GetStringColor(std::string);
	std::vector<Color> GetCustomColors(bool CB);
	void SetStringColors(int strIndex, Color c, bool CB);
	void UpdateSettings();
	void UpdateModSetting(std::string name, std::string newValue);
	void UpdateCustomSetting(std::string name, int newValue);
	void ParseSettingUpdate(std::string updateMessage);
	void ParseTwitchToggle(std::string twitchMsg);
	bool IsTwitchSettingEnabled(std::string name);

	// INI Setting Maps
	inline std::map<std::string, std::string> keyBinds;
	inline std::map<std::string, std::string> modSettings;
	inline std::map<std::string, int> customSettings;
	inline std::map<std::string, std::string> twitchSettings;

	// Misc 
	inline std::map<std::string, int> keyMap = { // Talk about taking the easy way out ;)
		{ "VK_LBUTTON" , 0x01 },
		{ "VK_RBUTTON" , 0x02 },
		{ "VK_CANCEL" , 0x03 },
		{ "VK_MBUTTON" , 0x04 },
		{ "VK_XBUTTON1" , 0x05 },
		{ "VK_XBUTTON2" , 0x06 },
		{ "VK_BACK" , 0x08 },
		{ "VK_TAB" , 0x09 },
		{ "VK_CLEAR" , 0x0C },
		{ "VK_RETURN" , 0x0D },
		{ "VK_SHIFT" , 0x10 },
		{ "VK_CONTROL" , 0x11 },
		{ "VK_MENU" , 0x12 },
		{ "VK_PAUSE" , 0x13 },
		{ "VK_CAPITAL" , 0x14 },
		{ "VK_KANA" , 0x15 },
		{ "VK_HANGUEL" , 0x15 },
		{ "VK_HANGUL" , 0x15 },
		{ "VK_IME_ON" , 0x16 },
		{ "VK_JUNJA" , 0x17 },
		{ "VK_FINAL" , 0x18 },
		{ "VK_HANJA" , 0x19 },
		{ "VK_KANJI" , 0x19 },
		{ "VK_IME_OFF" , 0x1A },
		{ "VK_ESCAPE" , 0x1B },
		{ "VK_CONVERT" , 0x1C },
		{ "VK_NONCONVERT" , 0x1D },
		{ "VK_ACCEPT" , 0x1E },
		{ "VK_MODECHANGE" , 0x1F },
		{ "VK_SPACE" , 0x20 },
		{ "VK_PRIOR" , 0x21 },
		{ "VK_NEXT" , 0x22 },
		{ "VK_END" , 0x23 },
		{ "VK_HOME" , 0x24 },
		{ "VK_LEFT" , 0x25 },
		{ "VK_UP" , 0x26 },
		{ "VK_RIGHT" , 0x27 },
		{ "VK_DOWN" , 0x28 },
		{ "VK_SELECT" , 0x29 },
		{ "VK_PRINT" , 0x2A },
		{ "VK_EXECUTE" , 0x2B },
		{ "VK_SNAPSHOT" , 0x2C },
		{ "VK_INSERT" , 0x2D },
		{ "VK_DELETE" , 0x2E },
		{ "VK_HELP" , 0x2F },
		{ "0" , 0x30 },
		{ "1" , 0x31 },
		{ "2" , 0x32 },
		{ "3" , 0x33 },
		{ "4" , 0x34 },
		{ "5" , 0x35 },
		{ "6" , 0x36 },
		{ "7" , 0x37 },
		{ "8" , 0x38 },
		{ "9" , 0x39 },
		{ "A" , 0x41 },
		{ "B" , 0x42 },
		{ "C" , 0x43 },
		{ "D" , 0x44 },
		{ "E" , 0x45 },
		{ "F" , 0x46 },
		{ "G" , 0x47 },
		{ "H" , 0x48 },
		{ "I" , 0x49 },
		{ "J" , 0x4A },
		{ "K" , 0x4B },
		{ "L" , 0x4C },
		{ "M" , 0x4D },
		{ "N" , 0x4E },
		{ "O" , 0x4F },
		{ "P" , 0x50 },
		{ "Q" , 0x51 },
		{ "R" , 0x52 },
		{ "S" , 0x53 },
		{ "T" , 0x54 },
		{ "U" , 0x55 },
		{ "V" , 0x56 },
		{ "W" , 0x57 },
		{ "X" , 0x58 },
		{ "Y" , 0x59 },
		{ "Z" , 0x5A },
		{ "VK_LWIN" , 0x5B },
		{ "VK_RWIN" , 0x5C },
		{ "VK_APPS" , 0x5D },
		{ "VK_SLEEP" , 0x5F },
		{ "VK_NUMPAD0" , 0x60 },
		{ "VK_NUMPAD1" , 0x61 },
		{ "VK_NUMPAD2" , 0x62 },
		{ "VK_NUMPAD3" , 0x63 },
		{ "VK_NUMPAD4" , 0x64 },
		{ "VK_NUMPAD5" , 0x65 },
		{ "VK_NUMPAD6" , 0x66 },
		{ "VK_NUMPAD7" , 0x67 },
		{ "VK_NUMPAD8" , 0x68 },
		{ "VK_NUMPAD9" , 0x69 },
		{ "VK_MULTIPLY" , 0x6A },
		{ "VK_ADD" , 0x6B },
		{ "VK_SEPARATOR" , 0x6C },
		{ "VK_SUBTRACT" , 0x6D },
		{ "VK_DECIMAL" , 0x6E },
		{ "VK_DIVIDE" , 0x6F },
		{ "VK_F1" , 0x70 },
		{ "VK_F2" , 0x71 },
		{ "VK_F3" , 0x72 },
		{ "VK_F4" , 0x73 },
		{ "VK_F5" , 0x74 },
		{ "VK_F6" , 0x75 },
		{ "VK_F7" , 0x76 },
		{ "VK_F8" , 0x77 },
		{ "VK_F9" , 0x78 },
		{ "VK_F10" , 0x79 },
		{ "VK_F11" , 0x7A },
		{ "VK_F12" , 0x7B },
		{ "VK_F13" , 0x7C },
		{ "VK_F14" , 0x7D },
		{ "VK_F15" , 0x7E },
		{ "VK_F16" , 0x7F },
		{ "VK_F17" , 0x80 },
		{ "VK_F18" , 0x81 },
		{ "VK_F19" , 0x82 },
		{ "VK_F20" , 0x83 },
		{ "VK_F21" , 0x84 },
		{ "VK_F22" , 0x85 },
		{ "VK_F23" , 0x86 },
		{ "VK_F24" , 0x87 },
		{ "VK_NUMLOCK" , 0x90 },
		{ "VK_SCROLL" , 0x91 },
		{ "VK_LSHIFT" , 0xA0 },
		{ "VK_RSHIFT" , 0xA1 },
		{ "VK_LCONTROL" , 0xA2 },
		{ "VK_RCONTROL" , 0xA3 },
		{ "VK_LMENU" , 0xA4 },
		{ "VK_RMENU" , 0xA5 },
		{ "VK_BROWSER_BACK" , 0xA6 },
		{ "VK_BROWSER_FORWARD" , 0xA7 },
		{ "VK_BROWSER_REFRESH" , 0xA8 },
		{ "VK_BROWSER_STOP" , 0xA9 },
		{ "VK_BROWSER_SEARCH" , 0xAA },
		{ "VK_BROWSER_FAVORITES" , 0xAB },
		{ "VK_BROWSER_HOME" , 0xAC },
		{ "VK_VOLUME_MUTE" , 0xAD },
		{ "VK_VOLUME_DOWN" , 0xAE },
		{ "VK_VOLUME_UP" , 0xAF },
		{ "VK_MEDIA_NEXT_TRACK" , 0xB0 },
		{ "VK_MEDIA_PREV_TRACK" , 0xB1 },
		{ "VK_MEDIA_STOP" , 0xB2 },
		{ "VK_MEDIA_PLAY_PAUSE" , 0xB3 },
		{ "VK_LAUNCH_MAIL" , 0xB4 },
		{ "VK_LAUNCH_MEDIA_SELECT" , 0xB5 },
		{ "VK_LAUNCH_APP1" , 0xB6 },
		{ "VK_LAUNCH_APP2" , 0xB7 },
		{ "VK_OEM_1" , 0xBA },
		{ "VK_OEM_PLUS" , 0xBB },
		{ "VK_OEM_COMMA" , 0xBC },
		{ "VK_OEM_MINUS" , 0xBD },
		{ "VK_OEM_PERIOD" , 0xBE },
		{ "VK_OEM_2" , 0xBF },
		{ "VK_OEM_3" , 0xC0 },
		{ "VK_OEM_4" , 0xDB },
		{ "VK_OEM_5" , 0xDC },
		{ "VK_OEM_6" , 0xDD },
		{ "VK_OEM_7" , 0xDE },
		{ "VK_OEM_8" , 0xDF },
		{ "VK_OEM_102" , 0xE2 },
		{ "VK_PROCESSKEY" , 0xE5 },
		{ "VK_PACKET" , 0xE7 },
		{ "VK_ATTN" , 0xF6 },
		{ "VK_CRSEL" , 0xF7 },
		{ "VK_EXSEL" , 0xF8 },
		{ "VK_EREOF" , 0xF9 },
		{ "VK_PLAY" , 0xFA },
		{ "VK_ZOOM" , 0xFB },
		{ "VK_NONAME" , 0xFC },
		{ "VK_PA1" , 0xFD },
		{ "VK_OEM_CLEAR" , 0xFE }
	};
	
	Color ConvertHexToColor(std::string hexStr);

	inline std::vector<Color> customStringColorsNormal;
	inline std::vector<Color> customStringColorsCB;

	inline std::vector<std::string> defaultStrColors = {
		"FF4F5A", "E2C102", "1DACF9", "FF9216", "3FCC0C", "C825ED", "0ABCB9", "909090"
	};

	inline std::vector<std::string> defaultStrColorsCB = {
		"C12A2A", "A3F400", "1DACF9", "DB7F41", "00C68E", "7648A8", "493647", "4C4C4C"
	};
};