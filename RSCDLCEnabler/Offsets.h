#include "windows.h"
#include <vector>
	
class cOffsets {
public:
	uintptr_t ptr_volume = 0x00F4E91C;
	std::vector<unsigned int> ptr_volumeOffsets{ 0x28, 0x7C0, 0x214, 0x7F4, 0xDC };

	uintptr_t ptr_loft = 0x00F5C4EC;
	std::vector<unsigned int> ptr_loftOffsets{ 0x108, 0x14, 0x28, 0x7C };

	uintptr_t ptr_timer = 0x00F5C5AC;
	std::vector<unsigned int> ptr_timerOffsets{ 0xB0, 0x538, 0x8 };

	uintptr_t ptr_enumerateService = 0xF71E10;
	std::vector<unsigned int> ptr_enumerateServiceOffsets{ 0x8, 0x4 };

	uintptr_t ptr_tuning = 0x00F5C4D8;
	std::vector<unsigned int> ptr_tuningOffsets{ 0x44, 0x4C, 0x8, 0xC, 0x30, 0x8, 0x44 };

	uintptr_t ptr_currentMenu = 0x00F5C5B4;
	std::vector<unsigned int> ptr_currentMenuOffsets{ 0x14, 0x34, 0x28, 0x8C, 0x0 };

	uintptr_t func_getStringFromCSV = 0x017B7A3E;
	uintptr_t func_getLocalizedString = 0x01395763;
	uintptr_t func_appendString = 0x01395488; //for reference purposes
	uintptr_t func_ForceEnumeration = 0x018D69E7;

	uintptr_t patch_addedSpaces = 0x01529f98;
	uintptr_t patch_addedNumbers = 0x0152a006;
	uintptr_t patch_sprintfArg = 0x0183479C;

	uintptr_t hookBackAddr_ForceEnumeration, hookBackAddr_FakeTitles, hookBackAddr_CustomNames, hookBackAddr_missingLocalization, hookBackAddr_Enumeration;
	uintptr_t hookAddr_ModifyLocalized = 0x01529F2B;
	uintptr_t hookAddr_ModifyCleanString = 0x01529F61;
	uintptr_t hookAddr_MissingLocalization = 0x01834790;

	uintptr_t ptr_stringColor = 0x135C50C;
	uintptr_t ptr_drunkShit = 0x012F4BA8; //search for float 0.333333, seems like it's static
	
	uintptr_t cdlcCheckdwAdr = 0x01377000;
	uintptr_t cdlcCheckSearchLen = 0x00DDE000;
	
	uintptr_t baseHandle;
	
	uint8_t*  cdlcCheckAdr;
	const char *sig_CDLCCheck = "\xE8\x00\x00\x00\x00\x83\xC4\x20\x88\xC3";
    char *sig_CDLCCheckMask = "x????xxxxx";
	const char* patch_CDLCCheck = "\xB3\x01";

	const char* patch_ListSpaces = "\x58\x58\x90\x90\x90";
	const char* patch_ListNumbers = "\x5A\x5A\x90\x90\x90";
	const char* patch_SprintfArgs = "\x04";

	void Initialize();
};

extern cOffsets Offsets;
