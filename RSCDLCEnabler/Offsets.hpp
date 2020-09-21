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

	// Current Note (Midi value: https://djip.co/w/wp-content/uploads/drupal/blog/logic-midi-note-numbers.png | 0 - 96 are used in Rocksmith).
	extern uintptr_t ptr_guitarSpeak;
	extern std::vector<unsigned int> ptr_guitarSpeakOffets;

	// Wwise Hijack
		// Root
	extern uintptr_t func_Wwise_Root_IsRestoreSinkRequested;
	extern uintptr_t func_Wwise_Root_IsUsingDummySink;
	// IAkStreamMgr
	extern uintptr_t func_Wwise_IAkStreamMgr_m_pStreamMgr;
	// MemoryMgr
	extern uintptr_t func_Wwise_Memory_CheckPoolId;
	extern uintptr_t func_Wwise_Memory_CreatePool;
	extern uintptr_t func_Wwise_Memory_DestroyPool;
	extern uintptr_t func_Wwise_Memory_Falign;
	extern uintptr_t func_Wwise_Memory_GetBlock;
	extern uintptr_t func_Wwise_Memory_GetBlockSize;
	extern uintptr_t func_Wwise_Memory_GetMaxPools;
	extern uintptr_t func_Wwise_Memory_GetNumPools;
	extern uintptr_t func_Wwise_Memory_GetPoolAttributes;
	extern uintptr_t func_Wwise_Memory_GetPoolMemoryUsed;
	extern uintptr_t func_Wwise_Memory_GetPoolName;
	extern uintptr_t func_Wwise_Memory_GetPoolStats;
	extern uintptr_t func_Wwise_Memory_IsInitialized;
	extern uintptr_t func_Wwise_Memory_Malign;
	extern uintptr_t func_Wwise_Memory_Malloc;
	extern uintptr_t func_Wwise_Memory_ReleaseBlock;
	extern uintptr_t func_Wwise_Memory_SetMonitoring;
	extern uintptr_t func_Wwise_Memory_SetPoolName;
	extern uintptr_t func_Wwise_Memory_Term;
	// Monitor
	extern uintptr_t func_Wwise_Monitor_PostCode;
	// Motion Engine
	extern uintptr_t func_Wwise_Motion_AddPlayerMotionDevice;
	extern uintptr_t func_Wwise_Motion_RegisterMotionDevice;
	extern uintptr_t func_Wwise_Motion_RemovePlayerMotionDevice;
	extern uintptr_t func_Wwise_Motion_SetPlayerListener;
	extern uintptr_t func_Wwise_Motion_SetPlayerVolume;
	// Music Engine
	extern uintptr_t func_Wwise_Music_GetDefaultInitSettings;
	extern uintptr_t func_Wwise_Music_GetPlayingSegmentInfo;
	extern uintptr_t func_Wwise_Music_Init;
	extern uintptr_t func_Wwise_Music_Term;
	// Sound Engine
	extern uintptr_t func_Wwise_Sound_AddBehaviorExtension;
	extern uintptr_t func_Wwise_Sound_CancelBankCallbackCookie;
	extern uintptr_t func_Wwise_Sound_CancelEventCallback;
	extern uintptr_t func_Wwise_Sound_CancelEventCallbackCookie;
	extern uintptr_t func_Wwise_Sound_ClearBanks;
	extern uintptr_t func_Wwise_Sound_ClearPreparedEvents;
	extern uintptr_t func_Wwise_Sound_CloneActorMixerEffect;
	extern uintptr_t func_Wwise_Sound_CloneBusEffect;
	extern uintptr_t func_Wwise_Sound_DynamicDialogue_ResolveDialogueEvent_UniqueID;
	extern uintptr_t func_Wwise_Sound_DynamicDialogue_ResolveDialogueEvent_Char;
	extern uintptr_t func_Wwise_Sound_DynamicSequence_Break;
	extern uintptr_t func_Wwise_Sound_DynamicSequence_Close;
	extern uintptr_t func_Wwise_Sound_DynamicSequence_LockPlaylist;
	extern uintptr_t func_Wwise_Sound_DynamicSequence_Open;
	extern uintptr_t func_Wwise_Sound_DynamicSequence_Pause;
	extern uintptr_t func_Wwise_Sound_DynamicSequence_Play;
	extern uintptr_t func_Wwise_Sound_DynamicSequence_Resume;
	extern uintptr_t func_Wwise_Sound_DynamicSequence_Stop;
	extern uintptr_t func_Wwise_Sound_DynamicSequence_UnlockPlaylist;
	extern uintptr_t func_Wwise_Sound_ExecuteActionOnEvent_UniqueID;
	extern uintptr_t func_Wwise_Sound_ExecuteActionOnEvent_Char;
	extern uintptr_t func_Wwise_Sound_g_PlayingID;
	extern uintptr_t func_Wwise_Sound_GetDefaultInitSettings;
	extern uintptr_t func_Wwise_Sound_GetDefaultPlatformInitSettings;
	extern uintptr_t func_Wwise_Sound_GetIDFromString;
	extern uintptr_t func_Wwise_Sound_GetPanningRule;
	extern uintptr_t func_Wwise_Sound_GetSourcePlayPosition;
	extern uintptr_t func_Wwise_Sound_GetSpeakerConfiguration;
	extern uintptr_t func_Wwise_Sound_Init;
	extern uintptr_t func_Wwise_Sound_IsInitialized;
	extern uintptr_t func_Wwise_Sound_LoadBank_BankID_MemPoolID;
	extern uintptr_t func_Wwise_Sound_LoadBank_Void_UInt32_BankID;
	extern uintptr_t func_Wwise_Sound_LoadBank_BankID_CallBack;
	extern uintptr_t func_Wwise_Sound_LoadBank_Void_UInt32_CallBack;
	extern uintptr_t func_Wwise_Sound_LoadBank_Char_MemPoolID;
	extern uintptr_t func_Wwise_Sound_LoadBank_Char_CallBack;
	extern uintptr_t func_Wwise_Sound_LoadBankUnique;
	extern uintptr_t func_Wwise_Sound_PlaySourcePlugin;
	extern uintptr_t func_Wwise_Sound_PostEvent_Char;
	extern uintptr_t func_Wwise_Sound_PostEvent_UniqueID;
	extern uintptr_t func_Wwise_Sound_PostTrigger_TriggerID;
	extern uintptr_t func_Wwise_Sound_PostTrigger_Char;
	extern uintptr_t func_Wwise_Sound_PrepareBank_BankID_Callback;
	extern uintptr_t func_Wwise_Sound_PrepareBank_BankID_BankContent;
	extern uintptr_t func_Wwise_Sound_PrepareBank_Char_CallBack;
	extern uintptr_t func_Wwise_Sound_PrepareBank_Char_BankContent;
	extern uintptr_t func_Wwise_Sound_PrepareEvent_EventID_UInt32;
	extern uintptr_t func_Wwise_Sound_PrepareEvent_EventID_UInt32_Callback_Void;
	extern uintptr_t func_Wwise_Sound_PrepareEvent_Char_UInt32;
	extern uintptr_t func_Wwise_Sound_PrepareEvent_Char_UInt32_Callback_Void;
	extern uintptr_t func_Wwise_Sound_PrepareGameSyncs_UInt32_UInt32_UInt32_Callback_Void;
	extern uintptr_t func_Wwise_Sound_PrepareGameSyncs_UInt32_UInt32_UInt32;
	extern uintptr_t func_Wwise_Sound_PrepareGameSyncs_Char_Char_UInt32_Callback_Void;
	extern uintptr_t func_Wwise_Sound_PrepareGameSyncs_Char_Char_UInt32;
	extern uintptr_t func_Wwise_Sound_Query_GetActiveGameObjects;
	extern uintptr_t func_Wwise_Sound_Query_GetActiveListeners;
	extern uintptr_t func_Wwise_Sound_Query_GetCustomPropertyValue_Int32;
	extern uintptr_t func_Wwise_Sound_Query_GetCustomPropertyValue_Real32;
	extern uintptr_t func_Wwise_Sound_Query_GetEventIDFromPlayingID;
	extern uintptr_t func_Wwise_Sound_Query_GetGameObjectAuxSendValues;
	extern uintptr_t func_Wwise_Sound_Query_GetGameObjectDryLevelValue;
	extern uintptr_t func_Wwise_Sound_Query_GetGameObjectFromPlayingID;
	extern uintptr_t func_Wwise_Sound_Query_GetIsGameObjectActive;
	extern uintptr_t func_Wwise_Sound_Query_GetListenerPosition;
	extern uintptr_t func_Wwise_Sound_Query_GetListenerSpatialization;
	extern uintptr_t func_Wwise_Sound_Query_GetMaxRadius_Array;
	extern uintptr_t func_Wwise_Sound_Query_GetMaxRadius_GameObject;
	extern uintptr_t func_Wwise_Sound_Query_GetObjectObstructionAndOcclusion;
	extern uintptr_t func_Wwise_Sound_Query_GetPlayingIDsFromGameObject;
	extern uintptr_t func_Wwise_Sound_Query_GetPosition;
	extern uintptr_t func_Wwise_Sound_Query_GetPositioningInfo;
	extern uintptr_t func_Wwise_Sound_Query_GetRTPCValue_RTPCID;
	extern uintptr_t func_Wwise_Sound_Query_GetRTPCValue_Char;
	extern uintptr_t func_Wwise_Sound_Query_GetState_StateGroupID;
	extern uintptr_t func_Wwise_Sound_Query_GetState_Char;
	extern uintptr_t func_Wwise_Sound_Query_GetSwitch_SwitchGroupID;
	extern uintptr_t func_Wwise_Sound_Query_GetSwitch_Char;
	extern uintptr_t func_Wwise_Sound_Query_QueryAudioObjectIDs_UniqueID;
	extern uintptr_t func_Wwise_Sound_Query_QueryAudioObjectIDs_Char;
	extern uintptr_t func_Wwise_Sound_RegisterBusVolumeCallback;
	extern uintptr_t func_Wwise_Sound_RegisterCodec;
	extern uintptr_t func_Wwise_Sound_RegisterGameObj;
	extern uintptr_t func_Wwise_Sound_RegisterGlobalCallback;
	extern uintptr_t func_Wwise_Sound_RegisterPlugin;
	extern uintptr_t func_Wwise_Sound_RemoveBehavioralExtension;
	extern uintptr_t func_Wwise_Sound_RenderAudio;
	extern uintptr_t func_Wwise_Sound_ResetRTPCValue_RTPCID;
	extern uintptr_t func_Wwise_Sound_ResetRTPCValue_Char;
	extern uintptr_t func_Wwise_Sound_SeekOnEvent_UniqueID_Int32;
	extern uintptr_t func_Wwise_Sound_SeekOnEvent_Char_Int32;
	extern uintptr_t func_Wwise_Sound_SeekOnEvent_UniqueID_Float;
	extern uintptr_t func_Wwise_Sound_SeekOnEvent_Char_Float;
	extern uintptr_t func_Wwise_Sound_SetActiveListeners;
	extern uintptr_t func_Wwise_Sound_SetActorMixerEffect;
	extern uintptr_t func_Wwise_Sound_SetAttenuationScalingFactor;
	extern uintptr_t func_Wwise_Sound_SetBankLoadIOSettings;
	extern uintptr_t func_Wwise_Sound_SetBusEffect_UniqueID;
	extern uintptr_t func_Wwise_Sound_SetBusEffect_Char;
	extern uintptr_t func_Wwise_Sound_SetEffectParam;
	extern uintptr_t func_Wwise_Sound_SetGameObjectAuxSendValues;
	extern uintptr_t func_Wwise_Sound_SetGameObjectOutputBusVolume;
	extern uintptr_t func_Wwise_Sound_SetListenerPipeline;
	extern uintptr_t func_Wwise_Sound_SetListenerPosition;
	extern uintptr_t func_Wwise_Sound_SetListenerScalingFactor;
	extern uintptr_t func_Wwise_Sound_SetListenerSpatialization;
	extern uintptr_t func_Wwise_Sound_SetMaxNumVoicesLimit;
	extern uintptr_t func_Wwise_Sound_SetMultiplePositions;
	extern uintptr_t func_Wwise_Sound_SetObjectObstructionAndOcclusion;
	extern uintptr_t func_Wwise_Sound_SetPanningRule;
	extern uintptr_t func_Wwise_Sound_SetPosition;
	extern uintptr_t func_Wwise_Sound_SetPositionInternal;
	extern uintptr_t func_Wwise_Sound_SetRTPCValue_RTPCID;
	extern uintptr_t func_Wwise_Sound_SetRTPCValue_Char;
	extern uintptr_t func_Wwise_Sound_SetState_StateGroupID;
	extern uintptr_t func_Wwise_Sound_SetState_Char;
	extern uintptr_t func_Wwise_Sound_SetSwitch_SwitchGroupID;
	extern uintptr_t func_Wwise_Sound_SetSwitch_Char;
	extern uintptr_t func_Wwise_Sound_SetVolumeThreshold;
	extern uintptr_t func_Wwise_Sound_StartOutputCapture;
	extern uintptr_t func_Wwise_Sound_StopAll;
	extern uintptr_t func_Wwise_Sound_StopOutputCapture;
	extern uintptr_t func_Wwise_Sound_StopPlayingID;
	extern uintptr_t func_Wwise_Sound_StopSourcePlugin;
	extern uintptr_t func_Wwise_Sound_Term;
	extern uintptr_t func_Wwise_Sound_UnloadBank_BankID_MemPoolID;
	extern uintptr_t func_Wwise_Sound_UnloadBank_BankID_Callback;
	extern uintptr_t func_Wwise_Sound_UnloadBank_Char_MemPoolID;
	extern uintptr_t func_Wwise_Sound_UnloadBank_Char_Callback;
	extern uintptr_t func_Wwise_Sound_UnloadBankUnique;
	extern uintptr_t func_Wwise_Sound_UnregisterAllGameObj;
	extern uintptr_t func_Wwise_Sound_UnregisterGameObj;
	extern uintptr_t func_Wwise_Sound_UnregisterGlobalCallback;
	// StreamMgr
	extern uintptr_t func_Wwise_Stream_AddLanguageChangeObserver;
	extern uintptr_t func_Wwise_Stream_Create;
	extern uintptr_t func_Wwise_Stream_CreateDevice;
	extern uintptr_t func_Wwise_Stream_DestroyDevice;
	extern uintptr_t func_Wwise_Stream_FlushAllCaches;
	extern uintptr_t func_Wwise_Stream_GetCurrentLanguage;
	extern uintptr_t func_Wwise_Stream_GetDefaultDeviceSettings;
	extern uintptr_t func_Wwise_Stream_GetDefaultSettings;
	extern uintptr_t func_Wwise_Stream_GetFileLocationResolver;
	extern uintptr_t func_Wwise_Stream_GetPoolID;
	extern uintptr_t func_Wwise_Stream_RemoveLanguageChangeObserver;
	extern uintptr_t func_Wwise_Stream_SetCurrentLanguage;
	extern uintptr_t func_Wwise_Stream_SetFileLocationResolver;
	// End Wwise Hijack

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

	//D3D Stuff
	extern const char* d3dDevice_Pattern;
	extern uint32_t d3dDevice_SearchLen;
	extern char* d3dDevice_Mask;

	// cDLC Stuff :P
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

	// Current Menu
	// extern uintptr_t ptr_currentMenu;
	// extern std::vector<unsigned int> ptr_currentMenuOffsets; // Old menu check, decided it loved to not work on some builds
	extern uintptr_t ptr_currentMenu; // https://media.discordapp.net/attachments/711633334983196756/744071651498655814/unknown.png, the game uses this one, so we may as well
	extern std::vector<unsigned int> ptr_currentMenuOffsets; // But the offsets stay the same, hurray!
	extern std::vector<unsigned int> ptr_preMainMenuOffsets;

	// Timer
	extern uintptr_t ptr_timer;
	extern std::vector<unsigned int> ptr_timerOffsets;

	// Colorblind Mode
	extern uintptr_t ptr_colorBlindMode;
	extern std::vector<unsigned int> ptr_colorBlindModeOffsets;

	// Misc Mods
	extern uintptr_t ptr_stringColor;
	extern uintptr_t ptr_drunkShit; //search for float 0.333333, seems like it's static
};