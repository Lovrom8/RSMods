#include "Offsets.hpp"

void Offsets::Initialize() {
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
	uintptr_t ptr_tuning = 0x00F5C4FC;
	std::vector<unsigned int> ptr_tuningOffsets{ 0x10, 0x8, 0x44 };
	
	// True Tuning
	uintptr_t ptr_trueTuning = 0x00F5C4FC;
	std::vector<unsigned int> ptr_trueTuningOffsets{ 0x10, 0x4, 0x418, 0xB4, 0xF18 };

	// Tuning - Textual Form
	uintptr_t ptr_tuningText = 0x00F5C5AC;
	std::vector<unsigned int> ptr_tuningTextOffsets{ 0x28, 0x44, 0x0 };

	// Current Note (Midi value: https://djip.co/w/wp-content/uploads/drupal/blog/logic-midi-note-numbers.png | 0 - 96 are used in Rocksmith).
	uintptr_t ptr_guitarSpeak = 0x00F5C4FC;
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
	uintptr_t patch_addedSpaces = 0x01529f98;
	uintptr_t patch_addedNumbers = 0x0152a006;
	uintptr_t patch_sprintfArg = 0x0183479C;
	const char* patch_ListSpaces = "\x58\x58\x90\x90\x90";
	const char* patch_ListNumbers = "\x5A\x5A\x90\x90\x90";
	const char* patch_SprintfArgs = "\x04";

	// Disable Controllers
	uintptr_t hookAddr_DirectInput8 = 0x01c5961a;
	uintptr_t hookBackAddr_DirectInput8 = 0x01c5961f;

	// D3D Stuff
	const char* d3dDevice_Pattern = "\xC7\x06\x00\x00\x00\x00\x89\x86\x00\x00\x00\x00\x89\x86";
	uint32_t d3dDevice_SearchLen = 0x128000;
	char* d3dDevice_Mask = "xx????xx????xx";

	void Initialize();

	// Current Menu
	// uintptr_t ptr_currentMenu = 0x00F5C5AC;
	// std::vector<unsigned int> ptr_currentMenuOffsets{ 0x2C, 0x30, 0x8C, 0x0 }; // Old menu check, decided it loved to not work on some builds
	uintptr_t ptr_currentMenu = 0x0135c5ac; // https://media.discordapp.net/attachments/711633334983196756/744071651498655814/unknown.png, the game uses this one, so we may as well
	std::vector<unsigned int> ptr_currentMenuOffsets{ 0x28, 0x8C, 0x0 }; // But the offsets stay the same, hurray!
	std::vector<unsigned int> ptr_preMainMenuOffsets{ 0x28, 0x8C };

	// Timer
	uintptr_t ptr_timer = 0x00F5C4FC;
	//std::vector<unsigned int> ptr_timerOffsets{ 0xB0, 0x538, 0x8 }; // Single Player Only Value
	std::vector<unsigned int> ptr_timerOffsets{ 0x34, 0x1C, 0x3C, 0x1F4, 0x98 }; // Singleplayer & Multiplayer

	// Song Speed (Riff Repeater Speed! Not Scroll Speed)
	uintptr_t ptr_songSpeed = 0x00F5C4CC;
	std::vector<unsigned int> ptr_songSpeedOffsets{ 0x10, 0x28, 0x38, 0x18, 0x4, 0x1C, 0x28 };

	// Selected Profile Name
	uintptr_t ptr_selectedProfileName = 0x00F5C5AC;
	std::vector<unsigned int> ptr_selectedProfileNameOffsets{ 0x18, 0x3C, 0x28, 0x1FC };

	// Colorblind Mode
	uintptr_t ptr_colorBlindMode = 0x00F5C50C;
	std::vector<unsigned int> ptr_colorBlindModeOffsets{ 0x14, 0x24, 0x348 };

	// Twitch Mods
	uintptr_t ptr_currentNoteStreak = 0x00F5C5AC;
	std::vector<unsigned int> ptr_currentNoteStreakLASOffsets{ 0xB0, 0x18, 0x4, 0x84, 0x34 };
	std::vector<unsigned int> ptr_currentNoteStreakSAOffsets{ 0xB0, 0x18, 0x4, 0x4C, 0x3C };

	// These are based on the RH Guitar values. Nothing else has been added.
	uintptr_t ptr_stringLocation = 0x00F69D6C;
	std::vector<unsigned int> ptr_stringLocationPurpleOffsets = { 0x30, 0x84, 0x8, 0x24, 0xD8, 0xE8, 0x78 };

	uintptr_t ptr_noteLocation = 0x00F5C53C; // Note these are only work on the 3rd, 4th, and 5th frets... I don't know how these are the only ones that stayed constant between launches.
	std::vector<unsigned int> ptr_noteLocationPurpleThirdOffsets = { 0x10, 0xC, 0x30, 0x4, 0x10, 0x558 };
	std::vector<unsigned int> ptr_noteLocationPurpleFourthOffsets = { 0x10, 0x10, 0x4, 0xC, 0x0, 0x10, 0x724 };
	std::vector<unsigned int> ptr_noteLocationPurpleFifthOffsets = { 0x10, 0x10, 0x4, 0x1C, 0x0, 0x10, 0xABC };

	// Wwise Logging
	uintptr_t ptr_Wwise_Log_PostEventHook = 0x01f5b2d6;
	uintptr_t ptr_Wwise_Log_PostEventHookJmpBck = 0x01f5b2d6 + 5;
	uintptr_t ptr_Wwise_Log_SetRTPCValueHook = 0x01f58a9d;
	uintptr_t ptr_Wwise_Log_SetRTPCValueHookJmpBck = 0x01f58a9d + 5;


	// Audio In Background
	uintptr_t ptr_IsWindowInFocus = 0x0124ea78; // Is the window currently in focus? 0 - Out of focus, 1 - In focus.
	uintptr_t ptr_WindowNotInFocusValue = 0x01f5bfa9; // When the user alt+tabs, the IsWindowInFocus changes to this value.

	// Lua Hijack
		// Gamebryo Game Engine
	uintptr_t ptr_luaopen_BehaviorAPI = 0x019a4664; // https://cdn.discordapp.com/attachments/711634485388771439/759602588588179486/screencapture-file-C-Emergent-Gamebryo-LightSpeed-3-2-Source-Documentation-HTML-documentation-htm-20.png
	uintptr_t ptr_luaopen_ecr = 0x01c5ffb5; // https://cdn.discordapp.com/attachments/711634485388771439/759602220294602852/unknown.png
	uintptr_t ptr_luaopen_ecrInput = 0x019d7f58; // https://cdn.discordapp.com/attachments/711634485388771439/759602433490944000/screencapture-file-C-Emergent-Gamebryo-LightSpeed-3-2-Source-Documentation-HTML-documentation-htm-20.png
	uintptr_t ptr_luaopen_egmAnimation = 0x01c6d459; // https://cdn.discordapp.com/attachments/711634485388771439/759602087557726248/unknown.png
		// Rocksmith
	uintptr_t ptr_luaopen_AlphaGame = 0x0178e7c7;
	uintptr_t ptr_luaopen_DetectionBindings = 0x0176be6f;
	uintptr_t ptr_luaopen_GameBindings = 0x0176d7fb;
	uintptr_t ptr_luaopen_GuitarBindings = 0x0176eaef;
	uintptr_t ptr_luaopen_Guitarcade = 0x01779cbf;
	uintptr_t ptr_luaopen_RSAudioBindings = 0x0178261b;
	uintptr_t ptr_luaopen_UIBindings = 0x018c2595;
	uintptr_t ptr_luaopen_VenueBindings = 0x018c3d08;
		// Misc
	uintptr_t ptr_luaopen_epgmGFx = 0x01c75fcc; // Scaleform x Flash
	uintptr_t ptr_luaopen_epgmWwise = 0x01cb197d; // Wwise (Not really needed since we have access to all the raw functions, but better safe than sorry)

	// Misc Mods
	uintptr_t ptr_stringColor = 0x135C50C;
	uintptr_t ptr_drunkShit = 0x012F4BA8; //search for float 0.333333, seems like it's static
	//uintptr_t ptr_scrollSpeedMultiplier = 0x0118DF40; // Thank mr. Koko, it's static
	uintptr_t ptr_scrollSpeedMultiplier = 0x0118DF40;
	volatile double& ref_scrollSpeedMultiplier = *((volatile double *)ptr_scrollSpeedMultiplier);
	uintptr_t patch_SongPreviewWwiseEvent = 0x011AE6E0;

	// Objects
	uintptr_t ptr_rootObject = 0x00F5C508;
	std::vector<unsigned int> ptr_rootObjectOffsets{ 0xAC, 0x20, 0x24, 0x24, 0x0 };

	// Faster dynamic density / scroll speed change
	uintptr_t patch_scrollSpeedLTTarget = 0x01406278; // Less than target
	uintptr_t patch_scrollSpeedGTTarget = 0x01406242; // Greater than target
	const char* patch_scrollSpeedChange = "\xD9\xE8\x90";
}


