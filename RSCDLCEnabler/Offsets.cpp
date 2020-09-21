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
	uintptr_t func_Wwise_Sound_AddBehaviorExtension = 0x1b5a741;
	uintptr_t func_Wwise_Sound_CancelBankCallbackCookie = 0x1b57f0c;
	uintptr_t func_Wwise_Sound_CancelEventCallback = 0x1b5760d;
	uintptr_t func_Wwise_Sound_CancelEventCallbackCookie = 0x1b575ed;
	uintptr_t func_Wwise_Sound_ClearBanks = 0x1b58ecc;
	uintptr_t func_Wwise_Sound_ClearPreparedEvents = 0x1b58064;
	uintptr_t func_Wwise_Sound_CloneActorMixerEffect = 0x1b5bd9d;
	uintptr_t func_Wwise_Sound_CloneBusEffect = 0x1b5bd79;
	uintptr_t func_Wwise_Sound_DynamicDialogue_ResolveDialogueEvent_UniqueID = 0x1b5b68e;
	uintptr_t func_Wwise_Sound_DynamicDialogue_ResolveDialogueEvent_Char = 0x1b5b77a;
	uintptr_t func_Wwise_Sound_DynamicSequence_Break = 0x1b5baba;
	uintptr_t func_Wwise_Sound_DynamicSequence_Close = 0x1b5bb4c;
	uintptr_t func_Wwise_Sound_DynamicSequence_LockPlaylist = 0x1b5bbde;
	uintptr_t func_Wwise_Sound_DynamicSequence_Open = 0x1b57661;
	uintptr_t func_Wwise_Sound_DynamicSequence_Pause = 0x1b5b8ac;
	uintptr_t func_Wwise_Sound_DynamicSequence_Play = 0x1b5b816;
	uintptr_t func_Wwise_Sound_DynamicSequence_Resume = 0x1b5b93e;
	uintptr_t func_Wwise_Sound_DynamicSequence_Stop = 0x1b5b9d4;
	uintptr_t func_Wwise_Sound_DynamicSequence_UnlockPlaylist = 0x1b5bc1e;
	uintptr_t func_Wwise_Sound_ExecuteActionOnEvent_UniqueID = 0x1b5b302;
	uintptr_t func_Wwise_Sound_ExecuteActionOnEvent_Char = 0x1b5b3ca;
	uintptr_t func_Wwise_Sound_g_PlayingID = 0xf4e790;
	uintptr_t func_Wwise_Sound_GetDefaultInitSettings = 0x1b5683d;
	uintptr_t func_Wwise_Sound_GetDefaultPlatformInitSettings = 0x1b5689d;
	uintptr_t func_Wwise_Sound_GetIDFromString = 0x1b58956;
	uintptr_t func_Wwise_Sound_GetPanningRule = 0x1b568fd;
	uintptr_t func_Wwise_Sound_GetSourcePlayPosition = 0x1b57631;
	uintptr_t func_Wwise_Sound_GetSpeakerConfiguration = 0x1b5690d;
	uintptr_t func_Wwise_Sound_Init = 0x1b5bdbd;
	uintptr_t func_Wwise_Sound_IsInitialized = 0x01f56829;
	uintptr_t func_Wwise_Sound_LoadBank_BankID_MemPoolID = 0x1b57a8f;
	uintptr_t func_Wwise_Sound_LoadBank_Void_UInt32_BankID = 0x1b57b81;
	uintptr_t func_Wwise_Sound_LoadBank_BankID_CallBack = 0x1b57c55;
	uintptr_t func_Wwise_Sound_LoadBank_Void_UInt32_CallBack = 0x1b57cd0;
	uintptr_t func_Wwise_Sound_LoadBank_Char_MemPoolID = 0x1b5916c;
	uintptr_t func_Wwise_Sound_LoadBank_Char_CallBack = 0x1b592c1;
	uintptr_t func_Wwise_Sound_LoadBankUnique = 0x1b5933c;
	uintptr_t func_Wwise_Sound_PlaySourcePlugin = 0x1b574d5;
	uintptr_t func_Wwise_Sound_PostEvent_Char = 0x1b5b260;
	uintptr_t func_Wwise_Sound_PostEvent_UniqueID = 0x1b5bf11;
	uintptr_t func_Wwise_Sound_PostTrigger_TriggerID = 0x1b56e60;
	uintptr_t func_Wwise_Sound_PostTrigger_Char = 0x1b58c2b;
	uintptr_t func_Wwise_Sound_PrepareBank_BankID_Callback = 0x1b57f2c;
	uintptr_t func_Wwise_Sound_PrepareBank_BankID_BankContent = 0x1b59588;
	uintptr_t func_Wwise_Sound_PrepareBank_Char_CallBack = 0x1b596d4;
	uintptr_t func_Wwise_Sound_PrepareBank_Char_BankContent = 0x1b5a651;
	uintptr_t func_Wwise_Sound_PrepareEvent_EventID_UInt32 = 0x1b57fa1;
	uintptr_t func_Wwise_Sound_PrepareEvent_EventID_UInt32_Callback_Void = 0x1b58034;
	uintptr_t func_Wwise_Sound_PrepareEvent_Char_UInt32 = 0x1b59915;
	uintptr_t func_Wwise_Sound_PrepareEvent_Char_UInt32_Callback_Void = 0x1b59c73;
	uintptr_t func_Wwise_Sound_PrepareGameSyncs_UInt32_UInt32_UInt32_Callback_Void = 0x1b58125;
	uintptr_t func_Wwise_Sound_PrepareGameSyncs_UInt32_UInt32_UInt32 = 0x1b58155;
	uintptr_t func_Wwise_Sound_PrepareGameSyncs_Char_Char_UInt32_Callback_Void = 0x1b59f4d;
	uintptr_t func_Wwise_Sound_PrepareGameSyncs_Char_Char_UInt32 = 0x1b5a1c0;
	uintptr_t func_Wwise_Sound_Query_GetActiveGameObjects = 0x1b55e5f;
	uintptr_t func_Wwise_Sound_Query_GetActiveListeners = 0x1b56168;
	uintptr_t func_Wwise_Sound_Query_GetCustomPropertyValue_Int32 = 0x1b55f95;
	uintptr_t func_Wwise_Sound_Query_GetCustomPropertyValue_Real32 = 0x1b56025;
	uintptr_t func_Wwise_Sound_Query_GetEventIDFromPlayingID = 0x1b55f25;
	uintptr_t func_Wwise_Sound_Query_GetGameObjectAuxSendValues = 0x1b56493;
	uintptr_t func_Wwise_Sound_Query_GetGameObjectDryLevelValue = 0x1b5658f;
	uintptr_t func_Wwise_Sound_Query_GetGameObjectFromPlayingID = 0x1b55f45;
	uintptr_t func_Wwise_Sound_Query_GetIsGameObjectActive = 0x1b55e8f;
	uintptr_t func_Wwise_Sound_Query_GetListenerPosition = 0x1b55c7f;
	uintptr_t func_Wwise_Sound_Query_GetListenerSpatialization = 0x1b55cdf;
	uintptr_t func_Wwise_Sound_Query_GetMaxRadius_Array = 0x1b55ebf;
	uintptr_t func_Wwise_Sound_Query_GetMaxRadius_GameObject = 0x1b55eef;
	uintptr_t func_Wwise_Sound_Query_GetObjectObstructionAndOcclusion = 0x1b56622;
	uintptr_t func_Wwise_Sound_Query_GetPlayingIDsFromGameObject = 0x1b55f65;
	uintptr_t func_Wwise_Sound_Query_GetPosition = 0x1b560b5;
	uintptr_t func_Wwise_Sound_Query_GetPositioningInfo = 0x1b55dff;
	uintptr_t func_Wwise_Sound_Query_GetRTPCValue_RTPCID = 0x1b561fb;
	uintptr_t func_Wwise_Sound_Query_GetRTPCValue_Char = 0x1b5634a;
	uintptr_t func_Wwise_Sound_Query_GetState_StateGroupID = 0x1b55d1f;
	uintptr_t func_Wwise_Sound_Query_GetState_Char = 0x1b55daf;
	uintptr_t func_Wwise_Sound_Query_GetSwitch_SwitchGroupID = 0x1b5638a;
	uintptr_t func_Wwise_Sound_Query_GetSwitch_Char = 0x1b56463;
	uintptr_t func_Wwise_Sound_Query_QueryAudioObjectIDs_UniqueID = 0x1b566e5;
	uintptr_t func_Wwise_Sound_Query_QueryAudioObjectIDs_Char = 0x1b56799;
	uintptr_t func_Wwise_Sound_RegisterBusVolumeCallback = 0x1b57018;
	uintptr_t func_Wwise_Sound_RegisterCodec = 0x1b5693d;
	uintptr_t func_Wwise_Sound_RegisterGameObj = 0x1b57728;
	uintptr_t func_Wwise_Sound_RegisterGlobalCallback = 0x1b5a814;
	uintptr_t func_Wwise_Sound_RegisterPlugin = 0x1b5692d;
	uintptr_t func_Wwise_Sound_RemoveBehavioralExtension = 0x1b5a7b1;
	uintptr_t func_Wwise_Sound_RenderAudio = 0x1b5691d;
	uintptr_t func_Wwise_Sound_ResetRTPCValue_RTPCID = 0x1b56f0e;
	uintptr_t func_Wwise_Sound_ResetRTPCValue_Char = 0x1b58e2c;
	uintptr_t func_Wwise_Sound_SeekOnEvent_UniqueID_Int32 = 0x1b5b3fa;
	uintptr_t func_Wwise_Sound_SeekOnEvent_Char_Int32 = 0x1b5b4af;
	uintptr_t func_Wwise_Sound_SeekOnEvent_UniqueID_Float = 0x1b5b4df;
	uintptr_t func_Wwise_Sound_SeekOnEvent_Char_Float = 0x1b5b5f9;
	uintptr_t func_Wwise_Sound_SetActiveListeners = 0x1b56b76;
	uintptr_t func_Wwise_Sound_SetActorMixerEffect = 0x1b5825a;
	uintptr_t func_Wwise_Sound_SetAttenuationScalingFactor = 0x1b56aba;
	uintptr_t func_Wwise_Sound_SetBankLoadIOSettings = 0x1b5785a;
	uintptr_t func_Wwise_Sound_SetBusEffect_UniqueID = 0x1b581f4;
	uintptr_t func_Wwise_Sound_SetBusEffect_Char = 0x1b5a3b1;
	uintptr_t func_Wwise_Sound_SetEffectParam = 0x1b582bc;
	uintptr_t func_Wwise_Sound_SetGameObjectAuxSendValues = 0x1b56fa9;
	uintptr_t func_Wwise_Sound_SetGameObjectOutputBusVolume = 0x1b5704c;
	uintptr_t func_Wwise_Sound_SetListenerPipeline = 0x1b56d24;
	uintptr_t func_Wwise_Sound_SetListenerPosition = 0x1b56bc2;
	uintptr_t func_Wwise_Sound_SetListenerScalingFactor = 0x1b56b1a;
	uintptr_t func_Wwise_Sound_SetListenerSpatialization = 0x1b56c90;
	uintptr_t func_Wwise_Sound_SetMaxNumVoicesLimit = 0x1b58e8c;
	uintptr_t func_Wwise_Sound_SetMultiplePositions = 0x1b569c8;
	uintptr_t func_Wwise_Sound_SetObjectObstructionAndOcclusion = 0x1b57098;
	uintptr_t func_Wwise_Sound_SetPanningRule = 0x1b5744c;
	uintptr_t func_Wwise_Sound_SetPosition = 0x1b58a31;
	uintptr_t func_Wwise_Sound_SetPositionInternal = 0x1b5694d;
	uintptr_t func_Wwise_Sound_SetRTPCValue_RTPCID = 0x1b56d73;
	uintptr_t func_Wwise_Sound_SetRTPCValue_Char = 0x1b58a91;
	uintptr_t func_Wwise_Sound_SetState_StateGroupID = 0x1b58c8b;
	uintptr_t func_Wwise_Sound_SetState_Char = 0x1b58d69;
	uintptr_t func_Wwise_Sound_SetSwitch_SwitchGroupID = 0x1b56e11;
	uintptr_t func_Wwise_Sound_SetSwitch_Char = 0x1b58b50;
	uintptr_t func_Wwise_Sound_SetVolumeThreshold = 0x1b58e6c;
	uintptr_t func_Wwise_Sound_StartOutputCapture = 0x1b583a4;
	uintptr_t func_Wwise_Sound_StopAll = 0x1b58590;
	uintptr_t func_Wwise_Sound_StopOutputCapture = 0x1b58463;
	uintptr_t func_Wwise_Sound_StopPlayingID = 0x1b585d9;
	uintptr_t func_Wwise_Sound_StopSourcePlugin = 0x1b5756c;
	uintptr_t func_Wwise_Sound_Term = 0x1b5acc6;
	uintptr_t func_Wwise_Sound_UnloadBank_BankID_MemPoolID = 0x1b57d6b;
	uintptr_t func_Wwise_Sound_UnloadBank_BankID_Callback = 0x1b57e91;
	uintptr_t func_Wwise_Sound_UnloadBank_Char_MemPoolID = 0x1b593e7;
	uintptr_t func_Wwise_Sound_UnloadBank_Char_Callback = 0x1b59482;
	uintptr_t func_Wwise_Sound_UnloadBankUnique = 0x1b594fd;
	uintptr_t func_Wwise_Sound_UnregisterAllGameObj = 0x1b577dd;
	uintptr_t func_Wwise_Sound_UnregisterGameObj = 0x1b57784;
	uintptr_t func_Wwise_Sound_UnregisterGlobalCallback = 0x1b5a844;
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

	// End Wwise Hijack

	// Misc Mods
	uintptr_t ptr_stringColor = 0x135C50C;
	uintptr_t ptr_drunkShit = 0x012F4BA8; //search for float 0.333333, seems like it's static
}


