#include "stdafx.h"
#include "Offsets.hpp"

void Offsets::Initialize() {
	baseHandle = (uintptr_t)GetModuleHandle(NULL);

	baseEnd = { {0x04F80000, baseHandle + MemUtil::GetTextSectionLength()  } };			// End of Rocksmith /* or close enough */
	ptr_loft = { {0x00F5F56C, 0x00F6056C} };											// Memory | Scan for a float, rounded (default), of 10000. Grab all of them. Then in chunks set them to 1. Once the loft turns off, narrow down to the specific one. Pointer map to far offsets.
	ptr_tuning = { {0x00F5F62C, 0x00F6062C} };											// Memory | Interpolated from Loft. +0x1000.
	ptr_trueTuning = { {0x00F5F57C, 0x00F6057C} };										// Memory | Copied from timer
	func_tuningReferenceBuilder = { {0x004DCCB0, 0x0} };								// Code | 55 8b ec 83 e4 f8 83 ec 10 83 7d 08 00 53 57 74 ? db 45 08 (we want the start of the function). Converts an arrangement cent offset to the note-detection reference, 440 * 2^(cents / 1200), and stamps it at [detection + 0x135C]. LP address is unresolved: the same mask on LP matched NoteDetection RMS code at 0x004DD93F, which is the wrong function.
	ptr_tuningText = { {0x00F5F62C, 0x00F6062C} };										// Memory | Copied from loft
	ptr_guitarSpeak = { {0x00F5F57C, 0x00F6057C} };										// Memory | Copied from timer
	func_ForceEnumeration = { {0x008c9cb0, baseHandle + 0x004C9310 } };					// Code | c6 86 dc 00 00 00 01 38 5e 05 (we want addresss of the start of the function)
	ptr_enumerateService = { { 0xF74E90 } };											// Memory |
	hookAddr_ModifyLocalized = { {0x005511EB, baseHandle + 0x001524FB } };				// Code | 8b 45 e0 8b ? ? ? ? ? 50 6a 01 51 8d 4d 80 (8b ? near the MOV + DWORD is what we want)
	hookAddr_ModifyCleanString = { {0x0055120F, baseHandle + 0x0015251F } };			// Code | 89 45 ac 89 55 a8 8d 45 80 39 4d 94 (we want 8d 45 80).
	hookAddr_MissingLocalization = { {0x00832647, baseHandle + 0x00432917 } };			// Code | Only xref for #%d#%s - 8b 54 24 18 8b 4c 24 14 50 8b 42 2c 50 68 ? ? ? ? (68 is the byte we want)
	func_ecxAddress = { {0x135FBFC, baseHandle + 0x00F60BFC } };						// Code | Addr comes from hookAddr_ModifyLocalized hook place.
	func_getStringFromCSV = { {0x007bf420, baseHandle + 0x003C0150 } };		// Code | 83 c0 04 89 44 24 18 8b 00 85 c9 0f ? ? ? ? ? 80 38 24 (we want address of the start of the function)
	func_getLocalizedString = { {0x004079d0, baseHandle + 0x007920 } };		// Code | 81 fb 00 00 00 40 (we want address of the start of the function)
	func_appendString = { {0x00407710, baseHandle + 0x00007660 } };			// Code | 81 fb ff ff ff 3f 0f ? ? ? ? ? (we want address of the  start of the function)
	patch_addedSpaces = { {0x00551234, baseHandle + 0x00152544 } };			// Code | 8d 45 b0 8d 55 a8 8d 4d c0 89 45 c0 89 4d c4 8b c6 39 55 ac (function call right before these bytes)
	patch_addedNumbers = { {0x00551275, baseHandle + 0x00152585 } };		// Code | 8b d6 3b d7 74 19 8d 45 c0 39 45 c4 (function call right before these bytes)
	patch_sprintfArg = { {0x00832653, baseHandle + 0x00432923 } };			// Code | 83 c4 0c 8b 44 24 18 8b 48 4c 2b 48 48 b8 ab aa aa 2a (we want byte 0c)
	hookAddr_DirectInput8 = { {0x00C019EC, baseHandle + 0x00800B0C } };		// Code | 56 6a 00 8d b7 80 00 00 00 56 (we want 6 bytes after)
	hookBackAddr_DirectInput8 = { {0x00C019F1, baseHandle + 0x00800B11 } };	// Code | 56 6a 00 8d b7 80 00 00 00 56 (we want 11 bytes after)
	xinputModule = { {0x01360e98, baseHandle + 0x00F61E98 } };				// Code | 56 6a 00 8d b7 80 00 00 00 56 68 ? ? ? ? 68 00 08 00 00 50 (we want the static variable set at the end)
	xinputEnable = { {0x01360eac, baseHandle + 0x00F61EAC } };				// Code | 56 6a 00 8d b7 80 00 00 00 56 68 ? ? ? ? 68 00 08 00 00 50 (we want the static variable set before the get keystroke - XInputGetDSoundAudioDeviceGuids)
	ptr_multiplayer = { {0x00F5F57C, 0x00F6057C} };							// Memory | Copied from timer
	ptr_currentMenu = { {0x135F62C, baseHandle + 0x00F6062C} };				// Memory | Look for menu names with offset matching ptr_currentMenuOffsets; probably easiest way is by x-refing FECalibrationMeter and finding the place where it dereferences the pointer at the beginning of the function
	ptr_timer = { {0x00F5F62C, 0x00F6062C} };								// Memory | Start song, keep narrowing down with song time increasing. Eventually do pointer map with ptr_timerBaseOffsets.
	ptr_timerRare = { {0x00F5F54C, 0x00F6054C} };							// Memory | Get ptr_timer, then subtract 0x30 from base address.
	ptr_greyOutNoteTimer = { {0x00F5F62C, 0x00F6062C} };					// Memory | Copeied from loft
	ptr_previewName = { {0x00F5F514, 0x00F60514} };							// Memory | Look for "Play_", add all results, then go scrolling through songs till you find an entry changing. Then pointer map with ptr_previewNameOffsets.
	ptr_timeStretchCalculations = { {0x00409406, baseHandle + 0x009366 } };				// Code | dd 05 ? ? ? ? d8 d1 df e0 dd 05 ? ? ? ? (we want first byte)
	ptr_timeStretchCalculationsJmpBck = { {0x00409441, baseHandle + 0x000093A1 } };		// Code | dd 05 ? ? ? ? d8 d1 df e0 dd 05 ? ? ? ? (0x3B bytes after first byte in mask)
	ptr_selectedProfileName = { {0x00F5F62C, 0x00F6062C} };					// Memory | Copied from Tuning.
	ptr_twoRTCBypass = { {0x007C0C5F, baseHandle + 0x003C1A2F} };			// Code | dc a3 90 01 00 00 dc ? ? ? ? ? df e0 f6 c4 41 (0x28 bytes after first byte in mask)
	ptr_NonStopPlayPreSongTimer = { {0x1224410, baseHandle + 0x00E25CA8} };				// Static memory | 00 00 00 40 e1 fa 25 40
	ptr_colorBlindMode = { {0x00F5F58C, 0x00F6058C} };						// Memory | Offset interpolated from others (old value +0x1000)
	ptr_currentNoteStreak = { {0x00F5F62C, 0x00F6062C} };					// Memory | Copied from Tuning.
	ptr_wavyNotesHook = { {0x007AE0AD, baseHandle + 0x003AF13D } };						// Code | 80 7c 24 0c 00 8b 45 0c d9 00 d9 5c 24 24 d9 40 04 (we want bytes d9 00)
	ptr_wavyNotesJmpBck = { {0x007AE0D6, baseHandle + 0x003AF166 } };					// Code | 80 7c 24 0c 00 8b 45 0c d9 00 d9 5c 24 24 d9 40 04 (0x29 bytes after the first byte in mask)
	ptr_OculusCrashJmp = { {0x00E7CFEF, baseHandle + 0x00A7C04F } };					// Code | 8b 95 ec fd ff ff 8b 02 8b 95 e8 fd ff ff 8b 34 90 8b 55 08 (opcode before this)
	ptr_StuckToneJmp = { {0x007D1BCA, baseHandle + 0x003D263A} };						// Code | 80 bc 37 82 00 00 00 01 (opcode after mask)
	ptr_PnpJmp_1 = { {0x00E7CFEF, baseHandle + 0x00A7C04F} };							// Code | 8b 95 ec fd ff ff 8b 02 8b 95 e8 fd ff ff 8b 34 90 (opcode before this)
	ptr_PnpJmp_2 = { {0x00E7D220, baseHandle + 0x00A7C280 } };							// Code | 89 85 dc fd ff ff 3b 83 34 04 00 00 (opcode after this)
	ptr_Password_LimitCharacters_Clipboard = { {0x005CF24E, baseHandle + 0x001D03CE} };	// Code | 8d 55 b4 52 8d 45 c8 50 8b ce (opcode before this)
	ptr_Password_LimitCharacters = { {0x005CF318, baseHandle + 0x001D0498} };			// Code | 8b 43 10 8d 7b 10 8b cb 39 7b 14 (opcode before this)
	ptr_AdvancedDisplayCrash = { {0x0091FB73, baseHandle + 0x0051E343} };				// Code | 8a 51 04 57 8b 7e 0c 88 10 2b 01 33 d2 (first opcode)
	ptr_AdvancedDisplayCrashJmpBck = { {0x0091FB7A, baseHandle + 0x0051E34A} };			// Code | 8a 51 04 57 8b 7e 0c 88 10 2b 01 33 d2 (fourth opcode)
	ptr_PortAudioInCrash = { {0x00C43AF5, baseHandle + 0x00842C25} };		// Code | 8b 85 c8 fe ff ff 80 b8 df 1b 00 00 00 (two opcodes before this - cmp)
	ptr_AdditionalAudioDevicesCrash = { { 0x00E7CF70, baseHandle + 0x00A7BFD0 } };   // Code 8b 8d c4 fd ff ff 8b 1c 81
	ptr_ModdedPtrCrashFix = { {0x0001C640, 0x0001C8A0} };		// Code | 89 46 1c 89 46 2c 89 7e 28 (MOV for ForceSuccess)
	ptr_IsWindowInFocus = { {0x1251A78, baseHandle + 0x00E52A78} };						// Static Memory | 00 00 00 80 01 00 00 00 04 00 00 00 01 00 00 00 (second variable)
	ptr_WindowNotInFocusValue = { {0xEC5D46, baseHandle + 0x00AC5496} };    // Code | c6 05 78 2a 02 01 00 (we want to change that 00 to an 01).

	// Lua Hijack
	// Gamebryo Game Engine
	ptr_luaopen_BehaviorAPI = { {0x019a4664, baseHandle + 0x005865D0 } };					// Code | Export
	ptr_luaopen_ecr = { {0x01c5ffb5, baseHandle + 0x00807300 } };							// Code | Export
	ptr_luaopen_ecrInput = { {0x019d7f58, baseHandle + 0x005B7F50 } };						// Code | Export
	ptr_luaopen_egmAnimation = { {0x01c6d459, baseHandle + 0x00814060 } };					// Code | Export
	// Rocksmith
	ptr_luaopen_AlphaGame = { {0x0178e7c7, baseHandle + 0x0039A870 } };						// Code | Export
	ptr_luaopen_DetectionBindings = { {0x0176be6f, baseHandle + 0x0037B160 } };				// Code | Export
	ptr_luaopen_GameBindings = { {0x0176d7fb, baseHandle + 0x0037CF30 } };					// Code | Export
	ptr_luaopen_GuitarBindings = { {0x0176eaef, baseHandle + 0x0037DEF0 } };				// Code | Export
	ptr_luaopen_Guitarcade = { {0x01779cbf, baseHandle + 0x00388CD0 } };					// Code | Export
	ptr_luaopen_RSAudioBindings = { {0x0178261b, baseHandle + 0x00390A70} };				// Code | Export
	ptr_luaopen_UIBindings = { {0x018c2595, baseHandle + 0x004B6D80 } };					// Code | Export
	ptr_luaopen_VenueBindings = { {0x018c3d08, baseHandle + 0x004B7F00} };					// Code | Export
	// Misc
	ptr_luaopen_epgmGFx = { {0x01c75fcc, baseHandle + 0x0081CCC0 } };						// Code | Export | Scaleform x Flash
	ptr_luaopen_epgmWwise = { {0x01cb197d, baseHandle + 0x00854FF0 } };						// Code | Export | Wwise

	// Sample Rate mod
	ptr_sampleRateRequirementAudioOutput = { {0x00EC6508, baseHandle + 0x00AC5CB8 } };			// Code | 48 b8 80 bb 00 00 (second opcode)
	ptr_sampleRateRequirementAudioOutput_JmpBck = { {0x00EC650D, baseHandle + 0x00AC5CBD } };	// Code | 48 b8 80 bb 00 00 (opcode after this)
	ptr_sampleRateDivZeroCrash = { {0x00F13896, baseHandle + 0x00B13D26 } };					// Code | f7 f3 c1 ee 10 89 4d c4 89 5d f0 39 45 f8 (first opcode)
	ptr_sampleRateDivZeroCrash_JmpBck = { {0x00F1389B, baseHandle + 0x00B13D2B } };				// Code | 89 4d c4 89 5d f0 39 45 f8 (first opcode)
	ptr_sampleRateSize = { {0x135198C, baseHandle + 0x00F5298C } };								// Static Memory | 00 40 1c 46 02 00 00 00 00 00 00 00 (second variable)
	ptr_sampleRateBuffer = { {0x1251A9C, baseHandle + 0x00E52A9C } };							// Static Memory | 80 bb 00 00 80 00 00 00 (second variable)

	ptr_stringColor = { {0x135F58C, baseHandle + 0x00F6058C } };								// Static Memory | 56 e8 ? ? ? ? 83 c4 04 89 46 14 (look at variable set at end of function)
	ptr_drunkShit = { {0x12F7C20, baseHandle + 0x00EF8C20 } };									// Static Memory | ab aa aa 3e (first variable)

	ptr_scrollSpeedMultiplier = { {0x1190F40, baseHandle + 0x00D90EF0 } };						// Static Memory | 01 db 0f 49 40 db 0f 49 40 00 00 00 00 00 00 14 40 (second variable)
	Offsets::ref_scrollSpeedMultiplier = ((volatile double*)ptr_scrollSpeedMultiplier.Get());
	patch_SongPreviewWwiseEvent = { {0x011B1818, baseHandle + 0x00DB3088 } };					// Static Memory | "Play_%s_Preview"
	steamApiUri = { {0x011DC908, baseHandle + 0x00DDE1D8 } };									// Static Memory | "%s://api.steampowered.com ..."

	ptr_rootObject = { {0x00F5F588, 0x00F60588} };											// Memory | "head_left"

	patch_scrollSpeedLTTarget = { {0x0046A319, baseHandle + 0x006A7E9 } };					// Code | d9 43 1c d8 89 c0 00 00 00 de e9 d9 5d e0 (first opcode)
	patch_scrollSpeedGTTarget = { {0x0046A2E9, baseHandle + 0x006A7B9 } };					// Code | d9 43 1c d8 89 c0 00 00 00 d8 43 20 (first opcode)

	ptr_noteData = { {0x00F5F62C, 0x00F6062C} };
	ptr_scoreAttackNoteData = { {0x00F5F62C, 0x00F6062C} };
}

namespace Offsets { // Addresses for pre-2021 patch are in the comments
	uintptr_t baseHandle; // Beginning of Rocksmith

	// Loft Settings
	std::vector<unsigned int> ptr_loft_nearOffsets{ 0x108, 0x14, 0x28, 0x7C };
	std::vector<unsigned int> ptr_loft_farOffsets{ 0x108, 0x14, 0x28, 0x80 };

	// Current Tuning
	// uintptr_t ptr_tuning = 0x00F5C4FC; // Works in most cases EXCEPT disconnected mode.
	// std::vector<unsigned int> ptr_tuningOffsets{ 0x10, 0x8, 0x44 }; // Works in most cases EXCEPT disconnected mode.
	std::vector<unsigned int> ptr_tuningOffsets{ 0x68, 0x10, 0x28, 0x38, 0x78, 0x110, 0x0 };

	// True Tuning
	std::vector<unsigned int> ptr_trueTuningOffsets{ 0x10, 0x4, 0x135C };

	// Tuning - Textual Form
	std::vector<unsigned int> ptr_tuningTextOffsets{ 0x28, 0x44, 0x0 };

	// Current Note (Midi value: https://djip.co/w/wp-content/uploads/drupal/blog/logic-midi-note-numbers.png | 0 - 96 are used in Rocksmith).
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
	std::vector<unsigned int> ptr_enumerateServiceOffsets{ 0x8, 0x4 };

	// Custom Song Lists
	const char* patch_ListSpaces = "\x58\x58\x90\x90\x90";
	const char* patch_ListNumbers = "\x5A\x5A\x90\x90\x90";
	const char* patch_SprintfArgs = "\x04";

	// D3D Stuff
	const char* d3dDevice_Pattern = "\xC7\x06\x00\x00\x00\x00\x89\x86\x00\x00\x00\x00\x89\x86";
	uint32_t d3dDevice_SearchLen = 0x128000;
	char* d3dDevice_Mask = "xx????xx????xx";

	// Multiplayer
	std::vector<unsigned int> ptr_multiplayerOffsets{ 0x24, 0x28, 0x14, 0x90, 0xC };

	void Initialize();

	// Current Menu
	// uintptr_t ptr_currentMenu = 0x00F5C5AC;
	// std::vector<unsigned int> ptr_currentMenuOffsets{ 0x2C, 0x30, 0x8C, 0x0 }; // Old menu check, decided it loved to not work on some builds
	std::vector<unsigned int> ptr_currentMenuOffsets{ 0x28, 0x8C, 0x0 }; // But the offsets stay the same, hurray!
	std::vector<unsigned int> ptr_preMainMenuOffsets{ 0x28, 0x8C };

	// Timer
	std::vector<unsigned int> ptr_timerBaseOffsets{ 0xB0, 0x538, 0x8 };
	std::vector<unsigned int> ptr_timerRareOffsets{ 0x20, 0x28, 0x0, 0x24, 0xC, 0x3B4 };

	// Grey Out Note Timer
	std::vector<unsigned int> ptr_greyOutNoteTimerOffsets{ 0x68, 0x10, 0x2C, 0x28, 0x3DC };

	// Song Speed (Riff Repeater Speed! Not Scroll Speed)
	std::vector<unsigned int> ptr_previewNameOffsets{ 0xBC, 0x0 };

	// Selected Profile Name
	std::vector<unsigned int> ptr_selectedProfileNameOffsets{ 0x18, 0x3C, 0x28, 0x1FC };

	// Two RTC Bypass
	/*
	Previous ver:

	uintptr_t ptr_twoRTCBypass = 0x017b9518;
	const char* ptr_twoRTCBypass_patch = "\xE9\x2F\x01\x00\x00\x90";
	const char* ptr_twoRTCBypass_original = "\x8B\xB5\x8C\xFF\xFF\xFF";*/

	const char* ptr_twoRTCBypass_patch_call = "\x90\x90\x90\x90\x90";
	const char* ptr_twoRTCBypass_patch_test = "\x90\x90";
	const char* ptr_twoRTCBypass_patch_jz = "\x90\x90";
	const char* ptr_twoRTCBypass_patch_mov = "\x90\x90\x90\x90\x90";
	const char* ptr_twoRTCBypass_patch_lea = "\x90\x90\x90\x90\x90\x90";
	const char* ptr_twoRTCBypass_original = "\x8B\x75\x8C";

	// Colorblind Mode
	std::vector<unsigned int> ptr_colorBlindModeOffsets{ 0x14, 0x24, 0x348 };

	// Twitch Mods
	std::vector<unsigned int> ptr_currentNoteStreakLASOffsets{ 0xB0, 0x18, 0x4, 0x84, 0x34 };
	std::vector<unsigned int> ptr_currentNoteStreakSAOffsets{ 0xB0, 0x18, 0x4, 0x4C, 0x3C };

	// Objects
	 // 0x00F5C508
	std::vector<unsigned int> ptr_rootObjectOffsets{ 0xAC, 0x20, 0x24, 0x24, 0x0 };

	// Faster dynamic density / scroll speed change
	const char* patch_scrollSpeedChange = "\xD9\xE8\x90";

	std::vector<unsigned int> ptr_noteDataOffsets{ 0xB0, 0x18, 0x4, 0x84, 0x0 };
	std::vector<unsigned int> ptr_scoreAttackNoteDataOffsets{ 0xB0, 0x18, 0x4, 0x4C, 0x0 };
}


