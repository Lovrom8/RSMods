#pragma once
#include "windows.h"
#include <vector>

#include "MemUtil.hpp"

namespace Offsets {
	extern uintptr_t baseHandle; // Beginning of Rocksmith
	extern uintptr_t baseEnd; // End of Rocksmith

	// Loft Settings
	extern uintptr_t ptr_loft;
	extern std::vector<unsigned int> ptr_loft_nearOffsets;
	extern std::vector<unsigned int> ptr_loft_farOffsets;

	// Current Tuning
	extern uintptr_t ptr_tuning;
	extern std::vector<unsigned int> ptr_tuningOffsets;

	// True Tuning
	extern uintptr_t ptr_trueTuning;
	extern std::vector<unsigned int> ptr_trueTuningOffsets;

	// Tuning - Textual Form
	extern uintptr_t ptr_tuningText;
	extern std::vector<unsigned int> ptr_tuningTextOffsets;

	// Current Note (Midi value: https://djip.co/w/wp-content/uploads/drupal/blog/logic-midi-note-numbers.png | 0 - 96 are used in Rocksmith).
	extern uintptr_t ptr_guitarSpeak;
	extern std::vector<unsigned int> ptr_guitarSpeakOffets;

	// Removed do to access to Wwise calls.
	//// Mixer Volumes
	//extern uintptr_t ptr_songVolume;
	//extern std::vector<unsigned int> ptr_songVolumeOffsets;
	//extern uintptr_t ptr_mixerVolumeBase;
	//extern std::vector<unsigned int> ptr_playerOneGuitarOffsets;
	//extern std::vector<unsigned int> ptr_playerOneBassOffsets;
	//extern std::vector<unsigned int> ptr_playerTwoGuitarOffsets;
	//extern std::vector<unsigned int> ptr_playerTwoBassOffsets;
	//extern std::vector<unsigned int> ptr_micOffsets;
	//extern std::vector<unsigned int> ptr_voOffsets;
	//extern std::vector<unsigned int> ptr_sfxOffsets;

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
	extern uintptr_t patch_addedSpaces;
	extern uintptr_t patch_addedNumbers;
	extern uintptr_t patch_sprintfArg;
	extern const char* patch_ListSpaces;
	extern const char* patch_ListNumbers;
	extern const char* patch_SprintfArgs;

	// Disable Controllers
	extern uintptr_t hookAddr_DirectInput8, hookBackAddr_DirectInput8;

	//D3D Stuff
	extern const char* d3dDevice_Pattern;
	extern uint32_t d3dDevice_SearchLen;
	extern char* d3dDevice_Mask;

	// Multiplayer
	extern uintptr_t ptr_multiplayer;
	extern std::vector<unsigned int> ptr_multiplayerOffsets;

	void Initialize();

	// Current Menu
	// extern uintptr_t ptr_currentMenu;
	// extern std::vector<unsigned int> ptr_currentMenuOffsets; // Old menu check, decided it loved to not work on some builds
	extern uintptr_t ptr_currentMenu; // https://media.discordapp.net/attachments/711633334983196756/744071651498655814/unknown.png, the game uses this one, so we may as well
	extern std::vector<unsigned int> ptr_currentMenuOffsets; // But the offsets stay the same, hurray!
	extern std::vector<unsigned int> ptr_preMainMenuOffsets;

	// Timer
	extern uintptr_t ptr_timer;
	extern std::vector<unsigned int> ptr_timerOffsets;

	// Grey Out Note Timer
	extern uintptr_t ptr_greyOutNoteTimer;
	extern std::vector<unsigned int> ptr_greyOutNoteTimerOffsets;

	// Song Speed (Riff Repeater Speed! Not Scroll Speed)
	extern uintptr_t ptr_previewName;
	extern std::vector<unsigned int> ptr_previewNameOffsets;
	extern uintptr_t ptr_timeStretchCalculations;
	extern uintptr_t ptr_timeStretchCalculationsJmpBck;

	// Selected Profile Name
	extern uintptr_t ptr_selectedProfileName;
	extern std::vector<unsigned int> ptr_selectedProfileNameOffsets;

	// Two RTC Bypass
	extern uintptr_t ptr_twoRTCBypass;
	extern const char* ptr_twoRTCBypass_patch;
	extern const char* ptr_twoRTCBypass_original;

	// Colorblind Mode
	extern uintptr_t ptr_colorBlindMode;
	extern std::vector<unsigned int> ptr_colorBlindModeOffsets;

	// Twitch Mods
	extern uintptr_t ptr_currentNoteStreak;
	extern std::vector<unsigned int> ptr_currentNoteStreakLASOffsets;
	extern std::vector<unsigned int> ptr_currentNoteStreakSAOffsets;

	// Wwise Logging
	extern uintptr_t ptr_Wwise_Log_PostEventHook;
	extern uintptr_t ptr_Wwise_Log_PostEventHookJmpBck;
	extern uintptr_t ptr_Wwise_Log_SetRTPCValueHook;
	extern uintptr_t ptr_Wwise_Log_SetRTPCValueHookJmpBck;
	extern uintptr_t ptr_Wwise_Log_SeekOnEventHook;
	extern uintptr_t ptr_Wwise_Log_SeekOnEventHookJmpBck;
	extern uintptr_t ptr_Wwise_Log_SetBusEffect;
	extern uintptr_t ptr_Wwise_Log_SetBusEffectJmpBck;
	extern uintptr_t ptr_Wwise_Log_CloneBusEffect;
	extern uintptr_t ptr_Wwise_Log_CloneBusEffectJmpBck;

	// Bug Prevention
	extern uintptr_t ptr_OculusCrashJmp;
	extern uintptr_t ptr_StuckToneJmp;

	// Audio In Background
	extern uintptr_t ptr_IsWindowInFocus;
	extern uintptr_t ptr_WindowNotInFocusValue;

	// Lua Hijack
		// Gamebryo Game Engine
	extern uintptr_t ptr_luaopen_BehaviorAPI;
	extern uintptr_t ptr_luaopen_ecr;
	extern uintptr_t ptr_luaopen_ecrInput;
	extern uintptr_t ptr_luaopen_egmAnimation;
		// Rocksmith
	extern uintptr_t ptr_luaopen_AlphaGame;
	extern uintptr_t ptr_luaopen_DetectionBindings;
	extern uintptr_t ptr_luaopen_GameBindings;
	extern uintptr_t ptr_luaopen_GuitarBindings;
	extern uintptr_t ptr_luaopen_Guitarcade;
	extern uintptr_t ptr_luaopen_RSAudioBindings;
	extern uintptr_t ptr_luaopen_UIBindings;
	extern uintptr_t ptr_luaopen_VenueBindings;
		// Misc
	extern uintptr_t ptr_luaopen_epgmGFx;
	extern uintptr_t ptr_luaopen_epgmWwise;

	// Adjust sample rate requirements
	extern uintptr_t ptr_sampleRateRequirementAudioOutput;
	extern uintptr_t ptr_sampleRateRequirementAudioOutput_JmpBck;
	extern uintptr_t ptr_sampleRateDivZeroCrash;
	extern uintptr_t ptr_sampleRateDivZeroCrash_JmpBck;
	extern uintptr_t ptr_sampleRateSize;
	extern uintptr_t ptr_sampleRateBuffer;

	// Misc Mods
	extern uintptr_t ptr_stringColor;
	extern uintptr_t ptr_drunkShit; //search for float 0.333333, seems like it's static
	extern uintptr_t ptr_scrollSpeedMultiplier;
	extern uintptr_t patch_SongPreviewWwiseEvent;
	extern uintptr_t steamApiUri;

	extern volatile double& ref_scrollSpeedMultiplier;

	// Objects
	extern uintptr_t ptr_rootObject;
	extern std::vector<unsigned int> ptr_rootObjectOffsets;

	// Faster dynamic density / scroll speed change
	extern uintptr_t patch_scrollSpeedLTTarget; // Less than target
	extern uintptr_t patch_scrollSpeedGTTarget; // Greater than target
	extern const char* patch_scrollSpeedChange;
};