#include "Offsets.hpp"

void Offsets::Initialize() {
	cdlcCheckAdr = MemUtil::FindPattern<uint8_t*>(cdlcCheckdwAdr, cdlcCheckSearchLen, (uint8_t*)sig_CDLCCheck, sig_CDLCCheckMask);
	baseHandle = (uintptr_t)GetModuleHandle(NULL);
}

namespace Offsets {
	uintptr_t baseHandle; // Beginning of Rocksmith
	uintptr_t baseEnd = 0x04F80000; // End of Rocksmith

	// Loft Settings
	uintptr_t ptr_loft = 0x00F5C4EC;
	std::vector<unsigned int> ptr_loft_nearOffsets{ 0x108, 0x14, 0x28, 0x7C };
	std::vector<unsigned int> ptr_loft_farOffsets{ 0x108, 0x14, 0x28, 0x80 };

	// Current Tuning
	uintptr_t ptr_tuning = 0x00F5C4D8;
	std::vector<unsigned int> ptr_tuningOffsets{ 0x44, 0x4C, 0x8, 0xC, 0x30, 0x8, 0x44 };

	// Current Note (Midi value: https://djip.co/w/wp-content/uploads/drupal/blog/logic-midi-note-numbers.png | 0 - 96 are used in Rocksmith).
	uintptr_t ptr_guitarSpeak = 0x135C4FC;
	std::vector<unsigned int> ptr_guitarSpeakOffets{ 0x10, 0x4, 0x5FC };

	// Removed do to access to Wwise calls.
	//// Mixer Volumes - the game uses 0x08 as the last offset for those mixer related things, so that could help with reducing pointerscan results - but I forgot about that, so only one of those uses 0x08 :P 
	//// If you don't need the old values, you can always just use SetRTPCValue them to whatever and they will stay in floats pointed to by these pointers 
	//uintptr_t ptr_songVolume = 0x00F4E91C;
	//std::vector<unsigned int> ptr_songVolumeOffsets{ 0x28, 0x7C0, 0x214, 0x7F4, 0xDC };
	//uintptr_t ptr_mixerVolumeBase = 0x00F4E71C;
	//std::vector<unsigned int> ptr_voOffsets = { 0xF50, 0xB44, 0x75C, 0xA5C, 0x74 };
	//std::vector<unsigned int> ptr_sfxOffsets = { 0xE6C, 0x234, 0xEDC, 0x0, 0x0, 0x74 };

	//uintptr_t ptr_playerVolumeBase = 0x00F4E738;
	//std::vector<unsigned int> ptr_playerOneGuitarOffsets = { 0xA4, 0x30, 0x4, 0x4, 0x68, 0x54 }; // Not set to the correct value in mic mode!
	//std::vector<unsigned int> ptr_playerOneBassOffsets = { 0x1C, 0x28, 0x3C, 0x0, 0x68, 0x28, 0x88 }; // Not updated - not sure how these work since there's same object for Player1/Player2 guitar and bass - maybe it even uses the same variable when you are in bass mode 
	//std::vector<unsigned int> ptr_playerTwoGuitarOffsets = { 0xBC, 0xB4, 0x4, 0x4, 0xBC, 0x8 }; // Will only have the correct value if you are in MP mode, otherwise 
	//std::vector<unsigned int> ptr_playerTwoBassOffsets = { 0x18, 0x18, 0x14, 0x68, 0xC, 0x28, 0x88 }; // Not updated

	//std::vector<unsigned int> ptr_micOffsets = { 0x18, 0x18, 0x14, 0x64, 0xC, 0x28, 0x88 }; // Not updated - this one is an oddball 

	// Force Enumeration
	uintptr_t hookBackAddr_ForceEnumeration, hookBackAddr_Enumeration;
	uintptr_t func_ForceEnumeration = 0x018D69E7;
	uintptr_t ptr_enumerateService = 0xF71E10;
	std::vector<unsigned int> ptr_enumerateServiceOffsets{ 0x8, 0x4 };
	
	// Custom Song Lists
	uintptr_t hookBackAddr_FakeTitles, hookBackAddr_CustomNames, hookBackAddr_missingLocalization;
	uintptr_t hookAddr_ModifyLocalized = 0x01529F2B;
	uintptr_t hookAddr_ModifyCleanString = 0x01529F61;
	uintptr_t hookAddr_MissingLocalization = 0x01834790;
	uintptr_t func_getStringFromCSV = 0x017B7A3E;
	uintptr_t func_getLocalizedString = 0x01395763;
	uintptr_t func_appendString = 0x01395488; //for reference purposes

	// D3D Stuff
	const char* d3dDevice_Pattern = "\xC7\x06\x00\x00\x00\x00\x89\x86\x00\x00\x00\x00\x89\x86";
	uint32_t d3dDevice_SearchLen = 0x128000;
	char* d3dDevice_Mask = "xx????xx????xx";

	// cDLC Stuff :P
	uintptr_t cdlcCheckdwAdr = 0x01377000;
	uintptr_t cdlcCheckSearchLen = 0x00DDE000;
	uint8_t* cdlcCheckAdr;
	const char* sig_CDLCCheck = "\xE8\x00\x00\x00\x00\x83\xC4\x20\x88\xC3";
	char* sig_CDLCCheckMask = "x????xxxxx";
	const char* patch_CDLCCheck = "\xB3\x01";
	uintptr_t patch_addedSpaces = 0x01529f98;
	uintptr_t patch_addedNumbers = 0x0152a006;
	uintptr_t patch_sprintfArg = 0x0183479C;
	const char* patch_ListSpaces = "\x58\x58\x90\x90\x90";
	const char* patch_ListNumbers = "\x5A\x5A\x90\x90\x90";
	const char* patch_SprintfArgs = "\x04";
	void Initialize();

	// Current Menu
	// uintptr_t ptr_currentMenu = 0x00F5C5AC;
	// std::vector<unsigned int> ptr_currentMenuOffsets{ 0x2C, 0x30, 0x8C, 0x0 }; // Old menu check, decided it loved to not work on some builds
	uintptr_t ptr_currentMenu = 0x0135c5ac; // https://media.discordapp.net/attachments/711633334983196756/744071651498655814/unknown.png, the game uses this one, so we may as well
	std::vector<unsigned int> ptr_currentMenuOffsets{ 0x28, 0x8C, 0x0 }; // But the offsets stay the same, hurray!
	std::vector<unsigned int> ptr_preMainMenuOffsets{ 0x28, 0x8C };

	// Timer
	uintptr_t ptr_timer = 0x00F5C5AC;
	std::vector<unsigned int> ptr_timerOffsets{ 0xB0, 0x538, 0x8 };

	// Colorblind Mode
	uintptr_t ptr_colorBlindMode = 0x00F5C50C;
	std::vector<unsigned int> ptr_colorBlindModeOffsets{ 0x14, 0x24, 0x348 };

	// Wwise Hijack

		// Root
	uintptr_t func_Wwise_Root_IsRestoreSinkRequested = 0x1b5bfd3;
	uintptr_t func_Wwise_Root_IsUsingDummySink = 0x1b5bfc3;
		// IAkStreamMgr
	uintptr_t func_Wwise_IAkStreamMgr_m_pStreamMgr = 0xf4f500;
		// MemoryMgr
	uintptr_t func_Wwise_Memory_CheckPoolId = 0x1b32828;
	uintptr_t func_Wwise_Memory_CreatePool = 0x1b32519;
	uintptr_t func_Wwise_Memory_DestroyPool = 0x1b326d4;
	uintptr_t func_Wwise_Memory_Falign = 0x1b329ee;
	uintptr_t func_Wwise_Memory_GetBlock = 0x1b32aee;
	uintptr_t func_Wwise_Memory_GetBlockSize = 0x1b327e8;
	uintptr_t func_Wwise_Memory_GetMaxPools = 0x1b32818;
	uintptr_t func_Wwise_Memory_GetNumPools = 0x1b32808;
	uintptr_t func_Wwise_Memory_GetPoolAttributes = 0x1b327c4;
	uintptr_t func_Wwise_Memory_GetPoolMemoryUsed = 0x1b32aae;
	uintptr_t func_Wwise_Memory_GetPoolName = 0x1c06a57;
	uintptr_t func_Wwise_Memory_GetPoolStats = 0x1b32a3e;
	uintptr_t func_Wwise_Memory_IsInitialized = 0x1b32784;
	uintptr_t func_Wwise_Memory_Malign = 0x1b3298a;
	uintptr_t func_Wwise_Memory_Malloc = 0x1b3290a;
	uintptr_t func_Wwise_Memory_ReleaseBlock = 0x1b32b3e;
	uintptr_t func_Wwise_Memory_SetMonitoring = 0x127e4f3;
	uintptr_t func_Wwise_Memory_SetPoolName = 0x1b32794;
	uintptr_t func_Wwise_Memory_Term = 0x1b32b8e;
		// Monitor
	uintptr_t func_Wwise_Monitor_PostCode = 0x1b587ba;
		// Motion Engine
	uintptr_t func_Wwise_Motion_AddPlayerMotionDevice = 0x1b58638;
	uintptr_t func_Wwise_Motion_RegisterMotionDevice = 0x1b58712;
	uintptr_t func_Wwise_Motion_RemovePlayerMotionDevice = 0x1b586ad;
	uintptr_t func_Wwise_Motion_SetPlayerListener = 0x1b58722;
	uintptr_t func_Wwise_Motion_SetPlayerVolume = 0x1b5876e;
		// Music Engine
	uintptr_t func_Wwise_Music_GetDefaultInitSettings = 0x1b340ef;
	uintptr_t func_Wwise_Music_GetPlayingSegmentInfo = 0x1b3413f;
	uintptr_t func_Wwise_Music_Init = 0x1b34f8c;
	uintptr_t func_Wwise_Music_Term = 0x1b34e38;
		// Sound Engine

		// StreamMgr
	uintptr_t func_Wwise_Stream_AddLanguageChangeObserver = 0x1bbc23a;
	uintptr_t func_Wwise_Stream_Create = 0x1bbbf66;
	uintptr_t func_Wwise_Stream_CreateDevice = 0x1bbc3dc;
	uintptr_t func_Wwise_Stream_DestroyDevice = 0x1bbbfe6;
	uintptr_t func_Wwise_Stream_FlushAllCaches = 0x1bbc076;
	uintptr_t func_Wwise_Stream_GetCurrentLanguage = 0x1bbb4c3;
	uintptr_t func_Wwise_Stream_GetDefaultDeviceSettings = 0x1bbb433;
	uintptr_t func_Wwise_Stream_GetDefaultSettings = 0x1bbb423;
	uintptr_t func_Wwise_Stream_GetFileLocationResolver = 0x1bbb493;
	uintptr_t func_Wwise_Stream_GetPoolID = 0x1bbb4b3;
	uintptr_t func_Wwise_Stream_RemoveLanguageChangeObserver = 0x1bbc056;
	uintptr_t func_Wwise_Stream_SetCurrentLanguage = 0x1bbc036;
	uintptr_t func_Wwise_Stream_SetFileLocationResolver = 0x1bbb4a3;



	uintptr_t func_SetRTPCValue = 0x1F58A91; // Set Game Volume
	uintptr_t func_GetRTPCValue = 0x1F5634A; // Get Game Volume
	uintptr_t func_ClearBanks = 0x1F58ECC; // Close All Sound Objects & Stops Noteway
	uintptr_t func_UnloadBank = 0x1B57D6B; // Unload Bank


	// Misc Mods
	uintptr_t ptr_stringColor = 0x135C50C;
	uintptr_t ptr_drunkShit = 0x012F4BA8; //search for float 0.333333, seems like it's static
}


