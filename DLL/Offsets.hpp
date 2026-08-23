#pragma once

#include "VersioningStruct.h"
#include <vector>
#include <cstddef>

namespace Offsets {
	extern uintptr_t baseHandle; // Beginning of Rocksmith
	inline VersioningStruct<uintptr_t> baseEnd; // End of Rocksmith

	// Loft Settings
	inline VersioningStruct<uintptr_t> ptr_loft;
	extern std::vector<unsigned int> ptr_loft_nearOffsets;
	extern std::vector<unsigned int> ptr_loft_farOffsets;

	// Current Tuning
	inline VersioningStruct<uintptr_t> ptr_tuning;
	extern std::vector<unsigned int> ptr_tuningOffsets;

	// True Tuning
	inline VersioningStruct<uintptr_t> ptr_trueTuning;
	extern std::vector<unsigned int> ptr_trueTuningOffsets;
	inline VersioningStruct<uintptr_t> ptr_disableTrueTuning;
	inline VersioningStruct<uintptr_t> ptr_disableTrueTuning_jmpBck;
	inline VersioningStruct<uintptr_t> ptr_disableTrueTuning_forceTT;
	inline VersioningStruct<uintptr_t> ptr_disableTrueTuningGate;

	// Tuning - Textual Form
	inline VersioningStruct<uintptr_t> ptr_tuningText;
	extern std::vector<unsigned int> ptr_tuningTextOffsets;

	// Current Note (Midi value: https://djip.co/w/wp-content/uploads/drupal/blog/logic-midi-note-numbers.png | 0 - 96 are used in Rocksmith).
	inline VersioningStruct<uintptr_t> ptr_guitarSpeak;
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
	inline VersioningStruct<uintptr_t> hookBackAddr_ForceEnumeration, hookBackAddr_Enumeration;
	inline VersioningStruct<uintptr_t> func_ForceEnumeration;
	inline VersioningStruct<uintptr_t> ptr_enumerateService;
	extern std::vector<unsigned int> ptr_enumerateServiceOffsets;

	// Custom Song Lists
	inline VersioningStruct<uintptr_t> hookBackAddr_FakeTitles, hookBackAddr_CustomNames, hookBackAddr_missingLocalization;
	inline VersioningStruct<uintptr_t> hookAddr_ModifyLocalized;
	inline VersioningStruct<uintptr_t> hookAddr_ModifyCleanString;
	inline VersioningStruct<uintptr_t> hookAddr_MissingLocalization;
	inline VersioningStruct<uintptr_t> func_ecxAddress;
	inline VersioningStruct<uintptr_t> func_getStringFromCSV;
	inline VersioningStruct<uintptr_t> func_getLocalizedString;
	inline VersioningStruct<uintptr_t> func_appendString; //for reference purposes
	inline VersioningStruct<uintptr_t> patch_addedSpaces;
	inline VersioningStruct<uintptr_t> patch_addedNumbers;
	inline VersioningStruct<uintptr_t> patch_sprintfArg;
	extern const char* patch_ListSpaces;
	extern const char* patch_ListNumbers;
	extern const char* patch_SprintfArgs;

	// Disable Controllers
	inline VersioningStruct<uintptr_t> hookAddr_DirectInput8, hookBackAddr_DirectInput8;
	inline VersioningStruct<uintptr_t> xinputModule;
	inline VersioningStruct<uintptr_t> xinputEnable;

	//D3D Stuff
	extern const char* d3dDevice_Pattern;
	extern uint32_t d3dDevice_SearchLen;
	extern const char* d3dDevice_Mask;

	// Multiplayer
	inline VersioningStruct<uintptr_t> ptr_multiplayer;
	extern std::vector<unsigned int> ptr_multiplayerOffsets;

	void Initialize();

	// Current Menu
	// extern uintptr_t ptr_currentMenu;
	// extern std::vector<unsigned int> ptr_currentMenuOffsets; // Old menu check, decided it loved to not work on some builds
	inline VersioningStruct<uintptr_t> ptr_currentMenu; // https://media.discordapp.net/attachments/711633334983196756/744071651498655814/unknown.png, the game uses this one, so we may as well
	extern std::vector<unsigned int> ptr_currentMenuOffsets; // But the offsets stay the same, hurray!
	extern std::vector<unsigned int> ptr_preMainMenuOffsets;

	// Timer
	inline VersioningStruct<uintptr_t> ptr_timer;
	extern std::vector<unsigned int> ptr_timerBaseOffsets;
	inline VersioningStruct<uintptr_t> ptr_timerRare;
	extern std::vector<unsigned int> ptr_timerRareOffsets;

	// Grey Out Note Timer
	inline VersioningStruct<uintptr_t> ptr_greyOutNoteTimer;
	extern std::vector<unsigned int> ptr_greyOutNoteTimerOffsets;

	// Song Speed (Riff Repeater Speed! Not Scroll Speed)
	inline VersioningStruct<uintptr_t> ptr_previewName;
	extern std::vector<unsigned int> ptr_previewNameOffsets;
	inline VersioningStruct<uintptr_t> ptr_timeStretchCalculations;
	inline VersioningStruct<uintptr_t> ptr_timeStretchCalculationsJmpBck;

	// Selected Profile Name
	inline VersioningStruct<uintptr_t> ptr_selectedProfileName;
	extern std::vector<unsigned int> ptr_selectedProfileNameOffsets;

	// Two RTC Bypass
	inline VersioningStruct<uintptr_t> ptr_twoRTCBypass;
	extern const char* ptr_twoRTCBypass_patch_call;
	extern const char* ptr_twoRTCBypass_patch_test;
	extern const char* ptr_twoRTCBypass_patch_jz;
	extern const char* ptr_twoRTCBypass_patch_mov;
	extern const char* ptr_twoRTCBypass_patch_lea;
	extern const char* ptr_twoRTCBypass_origina;
	extern const char* ptr_twoRTCBypass_original;

	// Non-stop Play Pre-Song Timer.
	// STATIC. This does not need any offsets since the address is hard-coded into the executable.
	inline VersioningStruct<uintptr_t> ptr_NonStopPlayPreSongTimer;

	// Colorblind Mode
	inline VersioningStruct<uintptr_t> ptr_colorBlindMode;
	extern std::vector<unsigned int> ptr_colorBlindModeOffsets;

	// Twitch Mods
	inline VersioningStruct<uintptr_t> ptr_currentNoteStreak;
	extern std::vector<unsigned int> ptr_currentNoteStreakLASOffsets;
	extern std::vector<unsigned int> ptr_currentNoteStreakSAOffsets;
	inline VersioningStruct<uintptr_t> ptr_wavyNotesHook;
	inline VersioningStruct<uintptr_t> ptr_wavyNotesJmpBck;

	// Wwise Logging
	inline VersioningStruct<uintptr_t> ptr_Wwise_Log_PostEventHook;
	inline VersioningStruct<uintptr_t> ptr_Wwise_Log_PostEventHookJmpBck;
	inline VersioningStruct<uintptr_t> ptr_Wwise_Log_SetRTPCValueHook;
	inline VersioningStruct<uintptr_t> ptr_Wwise_Log_SetRTPCValueHookJmpBck;
	inline VersioningStruct<uintptr_t> ptr_Wwise_Log_SeekOnEventHook;
	inline VersioningStruct<uintptr_t> ptr_Wwise_Log_SeekOnEventHookJmpBck;
	inline VersioningStruct<uintptr_t> ptr_Wwise_Log_SetBusEffect;
	inline VersioningStruct<uintptr_t> ptr_Wwise_Log_SetBusEffectJmpBck;
	inline VersioningStruct<uintptr_t> ptr_Wwise_Log_CloneBusEffect;
	inline VersioningStruct<uintptr_t> ptr_Wwise_Log_CloneBusEffectJmpBck;

	// Bug Prevention
	inline VersioningStruct<uintptr_t> ptr_OculusCrashJmp;
	inline VersioningStruct<uintptr_t> ptr_StuckToneJmp;
	inline VersioningStruct<uintptr_t> ptr_PnpJmp_1;
	inline VersioningStruct<uintptr_t> ptr_PnpJmp_2;
	inline VersioningStruct<uintptr_t> ptr_Password_LimitCharacters_Clipboard;
	inline VersioningStruct<uintptr_t> ptr_Password_LimitCharacters;
	inline VersioningStruct<uintptr_t> ptr_AdvancedDisplayCrash;
	inline VersioningStruct<uintptr_t> ptr_AdvancedDisplayCrashJmpBck;
	inline VersioningStruct<uintptr_t> ptr_PortAudioInCrash; 
	inline VersioningStruct<uintptr_t> ptr_AdditionalAudioDevicesCrash;
	inline VersioningStruct<uintptr_t> ptr_ModdedPtrCrashFix;

	// Audio In Background
	inline VersioningStruct<uintptr_t> ptr_IsWindowInFocus;
	inline VersioningStruct<uintptr_t> ptr_WindowNotInFocusValue;

	// Lua Hijack
		// Gamebryo Game Engine
	inline VersioningStruct<uintptr_t> ptr_luaopen_BehaviorAPI;
	inline VersioningStruct<uintptr_t> ptr_luaopen_ecr;
	inline VersioningStruct<uintptr_t> ptr_luaopen_ecrInput;
	inline VersioningStruct<uintptr_t> ptr_luaopen_egmAnimation;
		// Rocksmith
	inline VersioningStruct<uintptr_t> ptr_luaopen_AlphaGame;
	inline VersioningStruct<uintptr_t> ptr_luaopen_DetectionBindings;
	inline VersioningStruct<uintptr_t> ptr_luaopen_GameBindings;
	inline VersioningStruct<uintptr_t> ptr_luaopen_GuitarBindings;
	inline VersioningStruct<uintptr_t> ptr_luaopen_Guitarcade;
	inline VersioningStruct<uintptr_t> ptr_luaopen_RSAudioBindings;
	inline VersioningStruct<uintptr_t> ptr_luaopen_UIBindings;
	inline VersioningStruct<uintptr_t> ptr_luaopen_VenueBindings;
		// Misc
	inline VersioningStruct<uintptr_t> ptr_luaopen_epgmGFx;
	inline VersioningStruct<uintptr_t> ptr_luaopen_epgmWwise;

	// Adjust sample rate requirements
	inline VersioningStruct<uintptr_t> ptr_sampleRateRequirementAudioOutput;
	inline VersioningStruct<uintptr_t> ptr_sampleRateRequirementAudioOutput_JmpBck;
	inline VersioningStruct<uintptr_t> ptr_sampleRateDivZeroCrash;
	inline VersioningStruct<uintptr_t> ptr_sampleRateDivZeroCrash_JmpBck;
	inline VersioningStruct<uintptr_t> ptr_sampleRateSize;
	inline VersioningStruct<uintptr_t> ptr_sampleRateBuffer;

	// Adjust sample rate requirements (audio input / RTC / mic capture device open path)
	// RVAs sourced from L0FKA's RS_ASIO fork (build ebf44968-dirty, 2026-08-20), which patches
	// these same constants when its own GameSampleRate ini setting is enabled - confirmed live via
	// that fork's RS_ASIO-log.txt against the currently-installed game build. Only verified for
	// RemasteredSeptember2022; LPDecember2024 equivalents are unknown and left 0, so
	// AudioDevices::ChangeInputSampleRate() no-ops on that build until they're found.
	extern std::vector<VersioningStruct<uintptr_t>> ptr_sampleRateRequirementAudioInputSites;
	inline VersioningStruct<uintptr_t> ptr_sampleRateRequirementAudioInputDouble;

	// Misc Mods
	inline VersioningStruct<uintptr_t> ptr_stringColor;
	inline VersioningStruct<uintptr_t> ptr_drunkShit; //search for float 0.333333, seems like it's static
	inline VersioningStruct<uintptr_t> ptr_scrollSpeedMultiplier;
	inline VersioningStruct<uintptr_t> patch_SongPreviewWwiseEvent;
	inline VersioningStruct<uintptr_t> steamApiUri;

	inline volatile double* ref_scrollSpeedMultiplier;

	// Objects
	inline VersioningStruct<uintptr_t> ptr_rootObject;
	extern std::vector<unsigned int> ptr_rootObjectOffsets;

	// Faster dynamic density / scroll speed change
	inline VersioningStruct<uintptr_t> patch_scrollSpeedLTTarget; // Less than target
	inline VersioningStruct<uintptr_t> patch_scrollSpeedGTTarget; // Greater than target
	extern const char* patch_scrollSpeedChange;

	// Calibration meter volume sample count
	inline VersioningStruct<uintptr_t> ptr_calibrationSampleCountClamp;
	inline VersioningStruct<uintptr_t> ptr_calibrationSampleCountClampJmpBck;

	// Runtime data.
	inline uintptr_t runtimeVersionStructValue;

	extern std::vector<unsigned int> ptr_noteDataOffsets;
	extern std::vector<unsigned int> ptr_scoreAttackNoteDataOffsets;

	inline VersioningStruct<uintptr_t> ptr_noteData;
	inline VersioningStruct<uintptr_t> ptr_scoreAttackNoteData;
};