#include "../stdafx.h"
#include "Exports.hpp"

void Wwise::Exports::Initialize() {
	uintptr_t baseHandle = (uintptr_t)GetModuleHandle(NULL);

	// Root
	func_Wwise_Root_IsRestoreSinkRequested = { {0x00ec5d70, baseHandle + 0x00AC54B0} };
	func_Wwise_Root_IsUsingDummySink = { {0x00ec5d60, baseHandle + 0x00AC54A0} };

	// IAkStreamMgr
	// Only has a single pointer, and is protected https://www.audiokinetic.com/library/2015.1.9_5624/?source=SDK&id=class_a_k_1_1_i_ak_stream_mgr_a85c6043c1a45f13b7df2f05729248b1f.html
	func_Wwise_IAkStreamMgr_m_pStreamMgr = { {0x01352580, baseHandle + 0x00F53580} }; // Idk

	// MemoryMgr
	func_Wwise_Memory_CheckPoolId = { {0x00E9FF70, baseHandle + 0x00A9EEE0} };
	func_Wwise_Memory_CreatePool = { {0x00E9FC80, baseHandle + 0x00A9EBD0} };
	func_Wwise_Memory_DestroyPool = { {0x00E9FE20, baseHandle + 0x00A9ED70} };
	func_Wwise_Memory_Falign = { {0x00EA01E0, baseHandle + 0x00A9F150 } };
	func_Wwise_Memory_GetBlock = { {0x00EA0230, baseHandle + 0x00A9F1A0} };
	func_Wwise_Memory_GetBlockSize = { {0x00E9FF30, baseHandle + 0x00A9EEA0} };
	func_Wwise_Memory_GetMaxPools = { {0x00E9FF60, baseHandle + 0x00A9EED0} };
	func_Wwise_Memory_GetNumPools = { {0x00E9FF50, baseHandle + 0x00A9EEC0} };
	func_Wwise_Memory_GetPoolAttributes = { {0x00E9FF10, baseHandle + 0x00A9EE80} };
	func_Wwise_Memory_GetPoolMemoryUsed = { {0x00EA01A0, baseHandle + 0x00A9F110} };
	func_Wwise_Memory_GetPoolName = { {0x00c94f80, baseHandle + 0x00472340} }; // TODO: may be worth a check, this one doesn't seem to be exported
	func_Wwise_Memory_GetPoolStats = { {0x00EA0130, baseHandle + 0x00A9F0A0} };
	func_Wwise_Memory_IsInitialized = { {0x00E9FED0, baseHandle + 0x00A9EE20} };
	func_Wwise_Memory_Malign = { {0x00EA00D0, baseHandle + 0x00A9F040} };
	func_Wwise_Memory_Malloc = { {0x00EA0050, baseHandle + 0x00A9EFC0} };
	func_Wwise_Memory_Free = { {0x00EA01E0, baseHandle + 0x00A9F150} };
	func_Wwise_Memory_ReleaseBlock = { {0x00EA0280, baseHandle + 0x00A9F1F0} };
	func_Wwise_Memory_SetMonitoring = { {0x00D3F130, baseHandle + 0x001C9BF0} };
	func_Wwise_Memory_SetPoolName = { {0x00E9FEE0, baseHandle + 0x00A9EE30} };
	func_Wwise_Memory_Term = { {0x00EA02D0, baseHandle + 0x00A9F240} };

	// Monitor
	func_Wwise_Monitor_PostCode = { {0x00A004A0, baseHandle + 0x00894B00} };

	// Motion Engine
	func_Wwise_Motion_AddPlayerMotionDevice = { {0x00ec2a00, baseHandle + 0x00AC2060} };
	func_Wwise_Motion_RegisterMotionDevice = { {0x00ec2ab0, baseHandle + 0x00AC2110} };
	func_Wwise_Motion_RemovePlayerMotionDevice = { {0x00ec2a60, baseHandle + 0x00AC20C0} };
	func_Wwise_Motion_SetPlayerListener = { {0x00ec2ac0, baseHandle + 0x00AC2120} };
	func_Wwise_Motion_SetPlayerVolume = { {0x00ec2b00, baseHandle + 0x00AC2160} };

	// Music Engine
	func_Wwise_Music_GetDefaultInitSettings = { {0x00ea16a0, baseHandle + 0x00AA0610} };
	func_Wwise_Music_GetPlayingSegmentInfo = { {0x00ea16f0, baseHandle + 0x00AA0660} };
	func_Wwise_Music_Init = { {0x00ea23e0, baseHandle + 0x00AA14A0} };
	func_Wwise_Music_Term = { {0x00ea22b0, baseHandle + 0x00AA1370} };

	// Sound Engine
	func_Wwise_Sound_AddBehaviorExtension = { {0x00ec46a0, baseHandle + 0x00AC3D60} };
	func_Wwise_Sound_CancelBankCallbackCookie = { {0x00ec2400, baseHandle + 0x00AC1A60} };
	func_Wwise_Sound_CancelEventCallback = { {0x00ec1c80, baseHandle + 0x00AC12E0} };
	func_Wwise_Sound_CancelEventCallbackCookie = { {0x00ec1c60, baseHandle + 0x00AC12C0} };
	func_Wwise_Sound_ClearBanks = { {0x00ec3140, baseHandle + 0x00AC27D0} };
	func_Wwise_Sound_ClearPreparedEvents = { {0x00ec2530, baseHandle + 0x00AC1B90} };
	func_Wwise_Sound_CloneActorMixerEffect = { {0x00ec5b70, baseHandle + 0x00AC5270} };
	func_Wwise_Sound_CloneBusEffect = { {0x00ec5b50, baseHandle + 0x00AC5250} };
	func_Wwise_Sound_DynamicDialogue_ResolveDialogueEvent_UniqueID = { {0x00ec5560, baseHandle + 0x00AC4C20} };
	func_Wwise_Sound_DynamicDialogue_ResolveDialogueEvent_Char = { {0x00ec5640, baseHandle + 0x00AC4D20} };
	func_Wwise_Sound_DynamicSequence_Break = { {0x00ec58d0, baseHandle + 0x00AC4FD0} };
	func_Wwise_Sound_DynamicSequence_Close = { {0x00ec5950, baseHandle + 0x00AC5050} };
	func_Wwise_Sound_DynamicSequence_LockPlaylist = { {0x00ec59d0, baseHandle + 0x00AC50D0} };
	func_Wwise_Sound_DynamicSequence_Open = { {0x00ec1cd0, baseHandle + 0x00AC1330} };
	func_Wwise_Sound_DynamicSequence_Pause = { {0x00ec5750, baseHandle + 0x00AC4E50} };
	func_Wwise_Sound_DynamicSequence_Play = { {0x00ec56d0, baseHandle + 0x00AC4DD0} };
	func_Wwise_Sound_DynamicSequence_Resume = { {0x00ec57d0, baseHandle + 0x00AC4ED0} };
	func_Wwise_Sound_DynamicSequence_Stop = { {0x00ec5850, baseHandle + 0x00AC4F50} };
	func_Wwise_Sound_DynamicSequence_UnlockPlaylist = { {0x00ec5a10, baseHandle + 0x00AC5110} };
	func_Wwise_Sound_ExecuteActionOnEvent_UniqueID = { {0x00ec5240, baseHandle + 0x00AC4900} };
	func_Wwise_Sound_ExecuteActionOnEvent_Char = { {0x00ec52f0, baseHandle + 0x00AC49B0} };
	func_Wwise_Sound_g_PlayingID = { {0x01351810, baseHandle + 0x00F52810} }; // Idk
	func_Wwise_Sound_GetDefaultInitSettings = { {0x00ec1120, baseHandle + 0x00AC0750 } };
	func_Wwise_Sound_GetDefaultPlatformInitSettings = { {0x00ec1180, baseHandle + 0x00AC07B0 } };
	func_Wwise_Sound_GetIDFromString = { {0x00ec2c30, baseHandle + 0x00AC22B0} };
	func_Wwise_Sound_GetPanningRule = { {0x00ec1190, baseHandle + 0x00AC07C0} };
	func_Wwise_Sound_GetSourcePlayPosition = { {0x00ec1ca0, baseHandle + 0x00AC1300} };
	func_Wwise_Sound_GetSpeakerConfiguration = { {0x00ec11a0, baseHandle + 0x00AC07D0} };
	func_Wwise_Sound_Init = { {0x00ec5b90, baseHandle + 0x00AC5290} };
	func_Wwise_Sound_IsInitialized = { {0x00ec1110, baseHandle + 0x00AC0740} };
	func_Wwise_Sound_LoadBank_BankID_MemPoolID = { {0x00ec2070, baseHandle + 0x00AC16D0} };
	func_Wwise_Sound_LoadBank_Void_UInt32_BankID = { {0x00ec2130, baseHandle + 0x00AC1790} };
	func_Wwise_Sound_LoadBank_BankID_Callback = { {0x00ec21e0, baseHandle + 0x00AC1840} };
	func_Wwise_Sound_LoadBank_Void_UInt32_Callback = { {0x00ec2240, baseHandle + 0x00AC18A0} };
	func_Wwise_Sound_LoadBank_Char_MemPoolID = { {0x00ec33a0, baseHandle + 0x00AC2A60} };
	func_Wwise_Sound_LoadBank_Char_Callback = { {0x00ec34b0, baseHandle + 0x00AC2B70} };
	func_Wwise_Sound_LoadBankUnique = { {0x00ec3510, baseHandle + 0x00AC2BD0} };
	func_Wwise_Sound_PlaySourcePlugin = { {0x00ec1b90, baseHandle + 0x00AC11F0} };
	func_Wwise_Sound_PostEvent_Char = { {0x00ec51b0, baseHandle + 0x00AC4870} };
	func_Wwise_Sound_PostEvent_UniqueID = { {0x00ec5cc0, baseHandle + 0x00AC5400} };
	func_Wwise_Sound_PostTrigger_TriggerID = { {0x00ec1610, baseHandle + 0x00AC0C70} };
	func_Wwise_Sound_PostTrigger_Char = { {0x00ec2ee0, baseHandle + 0x00AC2570} };
	func_Wwise_Sound_PrepareBank_BankID_Callback = { {0x00ec2420, baseHandle + 0x00AC1A80 } };
	func_Wwise_Sound_PrepareBank_BankID_BankContent = { {0x00ec36f0, baseHandle + 0x00AC2DB0 } };
	func_Wwise_Sound_PrepareBank_Char_Callback = { {0x00ec3800,  baseHandle + 0x00AC2EC0 } };
	func_Wwise_Sound_PrepareBank_Char_BankContent = { {0x00ec4590, baseHandle + 0x00AC3C50} };
	func_Wwise_Sound_PrepareEvent_EventID_UInt32 = { {0x00ec2480, baseHandle + 0x00AC1AE0} };
	func_Wwise_Sound_PrepareEvent_EventID_UInt32_Callback_Void = { {0x00ec2500, baseHandle + 0x00AC1B60} };
	func_Wwise_Sound_PrepareEvent_Char_UInt32 = { {0x00ec39f0, baseHandle + 0x00AC30B0 } };
	func_Wwise_Sound_PrepareEvent_Char_UInt32_Callback_Void = { {0x00ec3ce0, baseHandle + 0x00AC33A0 } };
	func_Wwise_Sound_PrepareGameSyncs_UInt32_UInt32_UInt32_Callback_Void = { {0x00ec25d0, baseHandle + 0x00AC1C30} };
	func_Wwise_Sound_PrepareGameSyncs_UInt32_UInt32_UInt32 = { {0x00ec2600, baseHandle + 0x00AC1C60 } };
	func_Wwise_Sound_PrepareGameSyncs_Char_Char_UInt32_Callback_Void = { {0x00ec3f60, baseHandle + 0x00AC3620 } };
	func_Wwise_Sound_PrepareGameSyncs_Char_Char_UInt32 = { {0x00ec41a0, baseHandle + 0x00AC3860 } };
	func_Wwise_Sound_Query_GetActiveGameObjects = { {0x00ec0790, baseHandle + 0x00ABFDC0 } };
	func_Wwise_Sound_Query_GetActiveListeners = { {0x00ec0a90, baseHandle + 0x00AC00C0 } };
	func_Wwise_Sound_Query_GetCustomPropertyValue_Int32 = { {0x00ec08c0, baseHandle + 0x00ABFEF0 } };
	func_Wwise_Sound_Query_GetCustomPropertyValue_Real32 = { {0x00ec0950, baseHandle + 0x00ABFF80 } };
	func_Wwise_Sound_Query_GetEventIDFromPlayingID = { {0x00ec0850, baseHandle + 0x00ABFE80 } };
	func_Wwise_Sound_Query_GetGameObjectAuxSendValues = { {0x00ec0d90, baseHandle + 0x00AC03C0 } };
	func_Wwise_Sound_Query_GetGameObjectDryLevelValue = { {0x00ec0e80, baseHandle + 0x00AC04B0 } };
	func_Wwise_Sound_Query_GetGameObjectFromPlayingID = { {0x00ec0870, baseHandle + 0x00ABFEA0 } };
	func_Wwise_Sound_Query_GetIsGameObjectActive = { {0x00ec07c0, baseHandle + 0x00ABFDF0 } };
	func_Wwise_Sound_Query_GetListenerPosition = { {0x00ec05b0, baseHandle + 0x00ABFBE0 } };
	func_Wwise_Sound_Query_GetListenerSpatialization = { {0x00ec0610, baseHandle + 0x00ABFC40 } };
	func_Wwise_Sound_Query_GetMaxRadius_RadiusList = { {0x00ec07f0, baseHandle + 0x00ABFE20 } };
	func_Wwise_Sound_Query_GetMaxRadius_GameObject = { {0x00ec0820, baseHandle + 0x00ABFE50 } };
	func_Wwise_Sound_Query_GetObjectObstructionAndOcclusion = { {0x00ec0f10, baseHandle + 0x00AC0540 } };
	func_Wwise_Sound_Query_GetPlayingIDsFromGameObject = { {0x00ec0890, baseHandle + 0x00ABFEC0 } };
	func_Wwise_Sound_Query_GetPosition = { {0x00ec09e0, baseHandle + 0x00AC0010 } };
	func_Wwise_Sound_Query_GetPositioningInfo = { {0x00ec0730, baseHandle + 0x00ABFD60 } };
	func_Wwise_Sound_Query_GetRTPCValue_RTPCID = { {0x00ec0b20, baseHandle + 0x00AC0150 } };
	func_Wwise_Sound_Query_GetRTPCValue_Char = { {0x00ec0c50, baseHandle + 0x00AC0280 } };
	func_Wwise_Sound_Query_GetState_StateGroupID = { {0x00ec0650, baseHandle + 0x00ABFC80 } };
	func_Wwise_Sound_Query_GetState_Char = { {0x00ec06e0, baseHandle + 0x00ABFD10 } };
	func_Wwise_Sound_Query_GetSwitch_SwitchGroupID = { {0x00ec0c90, baseHandle + 0x00AC02C0 } };
	func_Wwise_Sound_Query_GetSwitch_Char = { {0x00ec0d60, baseHandle + 0x00AC0390 } };
	func_Wwise_Sound_Query_QueryAudioObjectIDs_UniqueID = { {0x00ec0fd0, baseHandle + 0x00AC0600 } };
	func_Wwise_Sound_Query_QueryAudioObjectIDs_Char = { {0x00ec1080, baseHandle + 0x00AC06B0 } };
	func_Wwise_Sound_RegisterBusVolumeCallback = { {0x00ec1780, baseHandle + 0x00AC0DE0 } };
	func_Wwise_Sound_RegisterCodec = { {0x00ec11d0, baseHandle + 0x00AC0800 } };
	func_Wwise_Sound_RegisterGameObj = { {0x00ec1d70, baseHandle + 0x00AC13D0 } };
	func_Wwise_Sound_RegisterGlobalCallback = { {0x00ec4770, baseHandle + 0x00AC3E30} };
	func_Wwise_Sound_RegisterPlugin = { {0x00ec11c0, baseHandle + 0x00AC07F0 } };
	func_Wwise_Sound_RemoveBehavioralExtension = { {0x00ec4710, baseHandle + 0x00AC3DD0} };

	func_Wwise_Sound_RenderAudio = { {0x00ec11b0, baseHandle + 0x00AC07E0} };
	func_Wwise_Sound_ResetRTPCValue_RTPCID = { {0x00ec16a0, baseHandle + 0x00AC0D00} };
	func_Wwise_Sound_ResetRTPCValue_Char = { {0x00ec30a0, baseHandle + 0x00AC2730} };
	func_Wwise_Sound_SeekOnEvent_UniqueID_Int32 = { {0x00ec5320, baseHandle + 0x00AC49E0} };
	func_Wwise_Sound_SeekOnEvent_Char_Int32 = { {0x00ec53c0, baseHandle + 0x00AC4A80} };
	func_Wwise_Sound_SeekOnEvent_UniqueID_Float = { {0x00ec53f0, baseHandle + 0x00AC4AB0} };
	func_Wwise_Sound_SeekOnEvent_Char_Float = { {0x00ec54e0, baseHandle + 0x00AC4BA0} };
	func_Wwise_Sound_SetActiveListeners = { {0x00ec13c0, baseHandle + 0x00AC0A20} };
	func_Wwise_Sound_SetActorMixerEffect = { {0x00ec26e0, baseHandle + 0x00AC1D40} };
	func_Wwise_Sound_SetAttenuationScalingFactor = { {0x00ec1320, baseHandle + 0x00AC0980} };
	func_Wwise_Sound_SetBankLoadIOSettings = { {0x00ec1e80, baseHandle + 0x00AC14E0} };
	func_Wwise_Sound_SetBusEffect_UniqueID = { {0x00ec2690, baseHandle + 0x00AC1CF0} };
	func_Wwise_Sound_SetBusEffect_Char = { {0x00ec4320, baseHandle + 0x00AC39E0} };
	func_Wwise_Sound_SetEffectParam = { {0x00ec2730, baseHandle + 0x00AC1D90 } };
	func_Wwise_Sound_SetGameObjectAuxSendValues = { {0x00ec1720, baseHandle + 0x00AC0D80} };
	func_Wwise_Sound_SetGameObjectOutputBusVolume = { {0x00ec17b0, baseHandle + 0x00AC0E10} };
	func_Wwise_Sound_SetListenerPipeline = { {0x00ec1510, baseHandle + 0x00AC0B70} };
	func_Wwise_Sound_SetListenerPosition = { {0x00ec1400, baseHandle + 0x00AC0A60} };
	func_Wwise_Sound_SetListenerScalingFactor = { {0x00ec1370, baseHandle + 0x00AC09D0} };
	func_Wwise_Sound_SetListenerSpatialization = { {0x00ec14a0, baseHandle + 0x00AC0B00} };
	func_Wwise_Sound_SetMaxNumVoicesLimit = { {0x00ec3100, baseHandle + 0x00AC2790} };
	func_Wwise_Sound_SetMultiplePositions = { {0x00ec1240, baseHandle + 0x00AC0870} };
	func_Wwise_Sound_SetObjectObstructionAndOcclusion = { {0x00ec17f0, baseHandle + 0x00AC0E50} };
	func_Wwise_Sound_SetPanningRule = { {0x00ec1b10, baseHandle + 0x00AC1170} };
	func_Wwise_Sound_SetPosition = { {0x00ec2d10, baseHandle + 0x00AC23A0} };
	func_Wwise_Sound_SetPositionInternal = { {0x00ec11e0, baseHandle + 0x00AC0810 } };
	func_Wwise_Sound_SetRTPCValue_RTPCID = { {0x00ec1550, baseHandle + 0x00AC0BB0 } };
	func_Wwise_Sound_SetRTPCValue_Char = { {0x00ec2d70, baseHandle + 0x00AC2400 } };
	func_Wwise_Sound_SetState_StateGroupID = { {0x00ec2f30, baseHandle + 0x00AC25C0} };
	func_Wwise_Sound_SetState_Char = { {0x00ec2ff0, baseHandle + 0x00AC2680} };
	func_Wwise_Sound_SetSwitch_SwitchGroupID = { {0x00ec15d0, baseHandle + 0x00AC0C30} };
	func_Wwise_Sound_SetSwitch_Char = { {0x00ec2e20, baseHandle + 0x00AC24B0} };
	func_Wwise_Sound_SetVolumeThreshold = { {0x00ec30e0, baseHandle + 0x00AC2770} };
	func_Wwise_Sound_StartOutputCapture = { {0x00ec27a0, baseHandle + 0x00AC1E00} };
	func_Wwise_Sound_StopAll = { {0x00ec2970, baseHandle + 0x00AC1FD0} };
	func_Wwise_Sound_StopOutputCapture = { {0x00ec2850, baseHandle + 0x00AC1EB0} };
	func_Wwise_Sound_StopPlayingID = { {0x00ec29b0, baseHandle + 0x00AC2010} };
	func_Wwise_Sound_StopSourcePlugin = { {0x00ec1c00, baseHandle + 0x00AC1260} };
	func_Wwise_Sound_Term = { {0x00ec4c70, baseHandle + 0x00AC4330} };
	func_Wwise_Sound_UnloadBank_BankID_MemPoolID = { {0x00ec22c0, baseHandle + 0x00AC1920} };
	func_Wwise_Sound_UnloadBank_BankID_Callback = { {0x00ec23a0, baseHandle + 0x00AC1A00} };
	func_Wwise_Sound_UnloadBank_Char_MemPoolID = { {0x00ec35a0, baseHandle + 0x00AC2C60} };
	func_Wwise_Sound_UnloadBank_Char_Callback = { {0x00ec3620, baseHandle + 0x00AC2CE0 } };
	func_Wwise_Sound_UnloadBankUnique = { {0x00ec3680, baseHandle + 0x00AC2D40} };
	func_Wwise_Sound_UnregisterAllGameObj = { {0x00ec1e10, baseHandle + 0x00AC1470} };
	func_Wwise_Sound_UnregisterGameObj = { {0x00ec1dc0, baseHandle + 0x00AC1420} };
	func_Wwise_Sound_UnregisterGlobalCallback = { {0x00ec47a0, baseHandle + 0x00AC3E60} };

	// StreamMgr - This Section has way too many dependencies for us to really use it.
	//func_Wwise_Stream_AddLanguageChangeObserver = { {0x1fbc23a, 0x00cece20} };
	//func_Wwise_Stream_Create = { {0x1fbbf66, 0x00cecb50} };
	//func_Wwise_Stream_CreateDevice = { {0x1fbc3dc, 0x00cecfc0} };
	//func_Wwise_Stream_DestroyDevice = { {0x1fbbfe6, 0x00cecbd0} };
	//func_Wwise_Stream_FlushAllCaches = { {0x1fbc076, 0x00cecc60} };
	//func_Wwise_Stream_GetCurrentLanguage = { {0x1fbb4c3, 0x00cec170} };
	//func_Wwise_Stream_GetDefaultDeviceSettings = { {0x1fbb433, 0x00cec0e0} };
	//func_Wwise_Stream_GetDefaultSettings = { {0x1fbb423, 0x00cec0d0} };
	//func_Wwise_Stream_GetFileLocationResolver = { {0x1fbb493, 0x00cec140} };
	//func_Wwise_Stream_GetPoolID = { {0x1fbb4b3, 0x00cec160} };
	//func_Wwise_Stream_RemoveLanguageChangeObserver = { {0x1fbc056, 0x00cecc40} };
	//func_Wwise_Stream_SetCurrentLanguage = { {0x1fbc036, 0x00cecc20} };
	//func_Wwise_Stream_SetFileLocationResolver = { {0x1fbb4a3, 0x00cec150} };
}