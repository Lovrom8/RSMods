#pragma once
#include "windows.h"
#include <vector>

#include "MemUtil.hpp"

namespace Offsets {
	extern uintptr_t baseHandle; // Handle for Rocksmith

	// Toggle Loft
	extern uintptr_t ptr_loft;
	extern std::vector<unsigned int> ptr_loft_nearOffsets;
	extern std::vector<unsigned int> ptr_loft_farOffsets;

	// Extended Range
	extern uintptr_t ptr_tuning;
	extern std::vector<unsigned int> ptr_tuningOffsets;


	// Guitar Speak
	extern uintptr_t ptr_guitarSpeak;
	extern std::vector<unsigned int> ptr_guitarSpeakOffets;

	// Audio Stuff
	extern uintptr_t func_SetRTPCValue;
	extern uintptr_t func_GetRTPCValue;
	extern uintptr_t ptr_volume;
	extern std::vector<unsigned int> ptr_volumeOffsets;

	// Force Enumeration
	extern uintptr_t hookBackAddr_ForceEnumeration, hookBackAddr_Enumeration;
	extern uintptr_t func_ForceEnumeration;
	extern uintptr_t ptr_enumerateService;
	extern std::vector<unsigned int> ptr_enumerateServiceOffsets;

	// Custom Song Lists
	extern uintptr_t hookBackAddr_FakeTitles, hookBackAddr_CustomNames, hookBackAddr_missingLocalization;
	extern uintptr_t hookAddr_ModifyLocalized;
	extern uintptr_t hookAddr_ModifyCleanString;
	extern uintptr_t hookAddr_MissingLocalization;
	extern uintptr_t func_getStringFromCSV;
	extern uintptr_t func_getLocalizedString;
	extern uintptr_t func_appendString; //for reference purposes

	//D3D Stuff
	extern const char* d3dDevice_Pattern;
	extern uint32_t d3dDevice_SearchLen;
	extern char* d3dDevice_Mask;

	// DLLMain::Hook_EndScene()
	extern uintptr_t baseEnd;

	// CDLC Stuff :P
	extern uintptr_t cdlcCheckdwAdr;
	extern uintptr_t cdlcCheckSearchLen;
	extern uint8_t* cdlcCheckAdr;
	extern const char* sig_CDLCCheck;
	extern char* sig_CDLCCheckMask;
	extern const char* patch_CDLCCheck;
	extern uintptr_t patch_addedSpaces;
	extern uintptr_t patch_addedNumbers;
	extern uintptr_t patch_sprintfArg;
	extern const char* patch_ListSpaces;
	extern const char* patch_ListNumbers;
	extern const char* patch_SprintfArgs;
	void Initialize();

	// Nisc
	extern uintptr_t ptr_stringColor;

	extern uintptr_t ptr_drunkShit; //search for float 0.333333, seems like it's static

	// uintptr_t ptr_currentMenu = 0x00F5C5AC;
	// std::vector<unsigned int> ptr_currentMenuOffsets{ 0x2C, 0x30, 0x8C, 0x0 }; // Old menu check, decided it loved to not work on some builds
	extern uintptr_t ptr_currentMenu; // https://media.discordapp.net/attachments/711633334983196756/744071651498655814/unknown.png, the game uses this one, so we may as well

	extern std::vector<unsigned int> ptr_currentMenuOffsets; // But the offsets stay the same, hurray!
	extern std::vector<unsigned int> ptr_preMainMenuOffsets;

	extern uintptr_t ptr_timer;
	extern std::vector<unsigned int> ptr_timerOffsets;

	extern uintptr_t ptr_colorBlindMode;
	extern std::vector<unsigned int> ptr_colorBlindModeOffsets;
};