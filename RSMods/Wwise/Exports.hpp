#pragma once

#include "Windows.h"

namespace Wwise::Exports {
	// Root
	inline uintptr_t func_Wwise_Root_IsRestoreSinkRequested = 0x1f5bfd3;
	inline uintptr_t func_Wwise_Root_IsUsingDummySink = 0x1f5bfc3;

	// IAkStreamMgr
	// Only has a single pointer, and is protected https://www.audiokinetic.com/library/2015.1.9_5624/?source=SDK&id=class_a_k_1_1_i_ak_stream_mgr_a85c6043c1a45f13b7df2f05729248b1f.html
	inline uintptr_t func_Wwise_IAkStreamMgr_m_pStreamMgr = 0x134f500;

	// MemoryMgr
	inline uintptr_t func_Wwise_Memory_CheckPoolId = 0x1f32828;
	inline uintptr_t func_Wwise_Memory_CreatePool = 0x1f32519;
	inline uintptr_t func_Wwise_Memory_DestroyPool = 0x1f326d4;
	inline uintptr_t func_Wwise_Memory_Falign = 0x1f329ee;
	inline uintptr_t func_Wwise_Memory_GetBlock = 0x1f32aee;
	inline uintptr_t func_Wwise_Memory_GetBlockSize = 0x1f327e8;
	inline uintptr_t func_Wwise_Memory_GetMaxPools = 0x1f32818;
	inline uintptr_t func_Wwise_Memory_GetNumPools = 0x1f32808;
	inline uintptr_t func_Wwise_Memory_GetPoolAttributes = 0x1f327c4;
	inline uintptr_t func_Wwise_Memory_GetPoolMemoryUsed = 0x1f32aae;
	inline uintptr_t func_Wwise_Memory_GetPoolName = 0x2006a57;
	inline uintptr_t func_Wwise_Memory_GetPoolStats = 0x1f32a3e;
	inline uintptr_t func_Wwise_Memory_IsInitialized = 0x1f32784;
	inline uintptr_t func_Wwise_Memory_Malign = 0x1f3298a;
	inline uintptr_t func_Wwise_Memory_Malloc = 0x1f3290a;
	inline uintptr_t func_Wwise_Memory_ReleaseBlock = 0x1f32b3e;
	inline uintptr_t func_Wwise_Memory_SetMonitoring = 0x167e4f8;
	inline uintptr_t func_Wwise_Memory_SetPoolName = 0x1f32794;
	inline uintptr_t func_Wwise_Memory_Term = 0x1f32b8e;

	// Monitor
	inline uintptr_t func_Wwise_Monitor_PostCode = 0x1f587ba;

	// Motion Engine
	inline uintptr_t func_Wwise_Motion_AddPlayerMotionDevice = 0x1f58638;
	inline uintptr_t func_Wwise_Motion_RegisterMotionDevice = 0x1f58712;
	inline uintptr_t func_Wwise_Motion_RemovePlayerMotionDevice = 0x1f586ad;
	inline uintptr_t func_Wwise_Motion_SetPlayerListener = 0x1f58722;
	inline uintptr_t func_Wwise_Motion_SetPlayerVolume = 0x1f5876e;

	// Music Engine
	inline uintptr_t func_Wwise_Music_GetDefaultInitSettings = 0x1f340ef;
	inline uintptr_t func_Wwise_Music_GetPlayingSegmentInfo = 0x1f3413f;
	inline uintptr_t func_Wwise_Music_Init = 0x1f34f8c;
	inline uintptr_t func_Wwise_Music_Term = 0x1f34e38;

	// Sound Engine
	inline uintptr_t func_Wwise_Sound_AddBehaviorExtension = 0x1f5a741;
	inline uintptr_t func_Wwise_Sound_CancelBankCallbackCookie = 0x1f57f0c;
	inline uintptr_t func_Wwise_Sound_CancelEventCallback = 0x1f5760d;
	inline uintptr_t func_Wwise_Sound_CancelEventCallbackCookie = 0x1f575ed;
	inline uintptr_t func_Wwise_Sound_ClearBanks = 0x1f58ecc;
	inline uintptr_t func_Wwise_Sound_ClearPreparedEvents = 0x1f58064;
	inline uintptr_t func_Wwise_Sound_CloneActorMixerEffect = 0x1f5bd9d;
	inline uintptr_t func_Wwise_Sound_CloneBusEffect = 0x1f5bd79;
	inline uintptr_t func_Wwise_Sound_DynamicDialogue_ResolveDialogueEvent_UniqueID = 0x1f5b68e;
	inline uintptr_t func_Wwise_Sound_DynamicDialogue_ResolveDialogueEvent_Char = 0x1f5b77a;
	inline uintptr_t func_Wwise_Sound_DynamicSequence_Break = 0x1f5baba;
	inline uintptr_t func_Wwise_Sound_DynamicSequence_Close = 0x1f5bb4c;
	inline uintptr_t func_Wwise_Sound_DynamicSequence_LockPlaylist = 0x1f5bbde;
	inline uintptr_t func_Wwise_Sound_DynamicSequence_Open = 0x1f57661;
	inline uintptr_t func_Wwise_Sound_DynamicSequence_Pause = 0x1f5b8ac;
	inline uintptr_t func_Wwise_Sound_DynamicSequence_Play = 0x1f5b816;
	inline uintptr_t func_Wwise_Sound_DynamicSequence_Resume = 0x1f5b93e;
	inline uintptr_t func_Wwise_Sound_DynamicSequence_Stop = 0x1f5b9d4;
	inline uintptr_t func_Wwise_Sound_DynamicSequence_UnlockPlaylist = 0x1f5bc1e;
	inline uintptr_t func_Wwise_Sound_ExecuteActionOnEvent_UniqueID = 0x1f5b302;
	inline uintptr_t func_Wwise_Sound_ExecuteActionOnEvent_Char = 0x1f5b3ca;
	inline uintptr_t func_Wwise_Sound_g_PlayingID = 0x134e790;
	inline uintptr_t func_Wwise_Sound_GetDefaultInitSettings = 0x1f5683d;
	inline uintptr_t func_Wwise_Sound_GetDefaultPlatformInitSettings = 0x1f5689d;
	inline uintptr_t func_Wwise_Sound_GetIDFromString = 0x1f58956;
	inline uintptr_t func_Wwise_Sound_GetPanningRule = 0x1f568fd;
	inline uintptr_t func_Wwise_Sound_GetSourcePlayPosition = 0x1f57631;
	inline uintptr_t func_Wwise_Sound_GetSpeakerConfiguration = 0x1f5690d;
	inline uintptr_t func_Wwise_Sound_Init = 0x1f5bdbd;
	inline uintptr_t func_Wwise_Sound_IsInitialized = 0x01f56829;
	inline uintptr_t func_Wwise_Sound_LoadBank_BankID_MemPoolID = 0x1f57a8f;
	inline uintptr_t func_Wwise_Sound_LoadBank_Void_UInt32_BankID = 0x1f57b81;
	inline uintptr_t func_Wwise_Sound_LoadBank_BankID_Callback = 0x1f57c55;
	inline uintptr_t func_Wwise_Sound_LoadBank_Void_UInt32_Callback = 0x1f57cd0;
	inline uintptr_t func_Wwise_Sound_LoadBank_Char_MemPoolID = 0x1f5916c;
	inline uintptr_t func_Wwise_Sound_LoadBank_Char_Callback = 0x1f592c1;
	inline uintptr_t func_Wwise_Sound_LoadBankUnique = 0x1f5933c;
	inline uintptr_t func_Wwise_Sound_PlaySourcePlugin = 0x1f574d5;
	inline uintptr_t func_Wwise_Sound_PostEvent_Char = 0x1f5b260;
	inline uintptr_t func_Wwise_Sound_PostEvent_UniqueID = 0x1f5bf11;
	inline uintptr_t func_Wwise_Sound_PostTrigger_TriggerID = 0x1f56e60;
	inline uintptr_t func_Wwise_Sound_PostTrigger_Char = 0x1f58c2b;
	inline uintptr_t func_Wwise_Sound_PrepareBank_BankID_Callback = 0x1f57f2c;
	inline uintptr_t func_Wwise_Sound_PrepareBank_BankID_BankContent = 0x1f59588;
	inline uintptr_t func_Wwise_Sound_PrepareBank_Char_Callback = 0x1f596d4;
	inline uintptr_t func_Wwise_Sound_PrepareBank_Char_BankContent = 0x1f5a651;
	inline uintptr_t func_Wwise_Sound_PrepareEvent_EventID_UInt32 = 0x1f57fa1;
	inline uintptr_t func_Wwise_Sound_PrepareEvent_EventID_UInt32_Callback_Void = 0x1f58034;
	inline uintptr_t func_Wwise_Sound_PrepareEvent_Char_UInt32 = 0x1f59915;
	inline uintptr_t func_Wwise_Sound_PrepareEvent_Char_UInt32_Callback_Void = 0x1f59c73;
	inline uintptr_t func_Wwise_Sound_PrepareGameSyncs_UInt32_UInt32_UInt32_Callback_Void = 0x1f58125;
	inline uintptr_t func_Wwise_Sound_PrepareGameSyncs_UInt32_UInt32_UInt32 = 0x1f58155;
	inline uintptr_t func_Wwise_Sound_PrepareGameSyncs_Char_Char_UInt32_Callback_Void = 0x1f59f4d;
	inline uintptr_t func_Wwise_Sound_PrepareGameSyncs_Char_Char_UInt32 = 0x1f5a1c0;
	inline uintptr_t func_Wwise_Sound_Query_GetActiveGameObjects = 0x1f55e5f;
	inline uintptr_t func_Wwise_Sound_Query_GetActiveListeners = 0x1f56168;
	inline uintptr_t func_Wwise_Sound_Query_GetCustomPropertyValue_Int32 = 0x1f55f95;
	inline uintptr_t func_Wwise_Sound_Query_GetCustomPropertyValue_Real32 = 0x1f56025;
	inline uintptr_t func_Wwise_Sound_Query_GetEventIDFromPlayingID = 0x1f55f25;
	inline uintptr_t func_Wwise_Sound_Query_GetGameObjectAuxSendValues = 0x1f56493;
	inline uintptr_t func_Wwise_Sound_Query_GetGameObjectDryLevelValue = 0x1f5658f;
	inline uintptr_t func_Wwise_Sound_Query_GetGameObjectFromPlayingID = 0x1f55f45;
	inline uintptr_t func_Wwise_Sound_Query_GetIsGameObjectActive = 0x1f55e8f;
	inline uintptr_t func_Wwise_Sound_Query_GetListenerPosition = 0x1f55c7f;
	inline uintptr_t func_Wwise_Sound_Query_GetListenerSpatialization = 0x1f55cdf;
	inline uintptr_t func_Wwise_Sound_Query_GetMaxRadius_RadiusList = 0x1f55ebf;
	inline uintptr_t func_Wwise_Sound_Query_GetMaxRadius_GameObject = 0x1f55eef;
	inline uintptr_t func_Wwise_Sound_Query_GetObjectObstructionAndOcclusion = 0x1f56622;
	inline uintptr_t func_Wwise_Sound_Query_GetPlayingIDsFromGameObject = 0x1f55f65;
	inline uintptr_t func_Wwise_Sound_Query_GetPosition = 0x1f560b5;
	inline uintptr_t func_Wwise_Sound_Query_GetPositioningInfo = 0x1f55dff;
	inline uintptr_t func_Wwise_Sound_Query_GetRTPCValue_RTPCID = 0x1f561fb;
	inline uintptr_t func_Wwise_Sound_Query_GetRTPCValue_Char = 0x1f5634a;
	inline uintptr_t func_Wwise_Sound_Query_GetState_StateGroupID = 0x1f55d1f;
	inline uintptr_t func_Wwise_Sound_Query_GetState_Char = 0x1f55daf;
	inline uintptr_t func_Wwise_Sound_Query_GetSwitch_SwitchGroupID = 0x1f5638a;
	inline uintptr_t func_Wwise_Sound_Query_GetSwitch_Char = 0x1f56463;
	inline uintptr_t func_Wwise_Sound_Query_QueryAudioObjectIDs_UniqueID = 0x1f566e5;
	inline uintptr_t func_Wwise_Sound_Query_QueryAudioObjectIDs_Char = 0x1f56799;
	inline uintptr_t func_Wwise_Sound_RegisterBusVolumeCallback = 0x1f57018;
	inline uintptr_t func_Wwise_Sound_RegisterCodec = 0x1f5693d;
	inline uintptr_t func_Wwise_Sound_RegisterGameObj = 0x1f57728;
	inline uintptr_t func_Wwise_Sound_RegisterGlobalCallback = 0x1f5a814;
	inline uintptr_t func_Wwise_Sound_RegisterPlugin = 0x1f5692d;
	inline uintptr_t func_Wwise_Sound_RemoveBehavioralExtension = 0x1f5a7b1;
	inline uintptr_t func_Wwise_Sound_RenderAudio = 0x1f5691d;
	inline uintptr_t func_Wwise_Sound_ResetRTPCValue_RTPCID = 0x1f56f0e;
	inline uintptr_t func_Wwise_Sound_ResetRTPCValue_Char = 0x1f58e2c;
	inline uintptr_t func_Wwise_Sound_SeekOnEvent_UniqueID_Int32 = 0x1f5b3fa;
	inline uintptr_t func_Wwise_Sound_SeekOnEvent_Char_Int32 = 0x1f5b4af;
	inline uintptr_t func_Wwise_Sound_SeekOnEvent_UniqueID_Float = 0x1f5b4df;
	inline uintptr_t func_Wwise_Sound_SeekOnEvent_Char_Float = 0x1f5b5f9;
	inline uintptr_t func_Wwise_Sound_SetActiveListeners = 0x1f56b76;
	inline uintptr_t func_Wwise_Sound_SetActorMixerEffect = 0x1f5825a;
	inline uintptr_t func_Wwise_Sound_SetAttenuationScalingFactor = 0x1f56aba;
	inline uintptr_t func_Wwise_Sound_SetBankLoadIOSettings = 0x1f5785a;
	inline uintptr_t func_Wwise_Sound_SetBusEffect_UniqueID = 0x1f581f4;
	inline uintptr_t func_Wwise_Sound_SetBusEffect_Char = 0x1f5a3b1;
	inline uintptr_t func_Wwise_Sound_SetEffectParam = 0x1f582bc;
	inline uintptr_t func_Wwise_Sound_SetGameObjectAuxSendValues = 0x1f56fa9;
	inline uintptr_t func_Wwise_Sound_SetGameObjectOutputBusVolume = 0x1f5704c;
	inline uintptr_t func_Wwise_Sound_SetListenerPipeline = 0x1f56d24;
	inline uintptr_t func_Wwise_Sound_SetListenerPosition = 0x1f56bc2;
	inline uintptr_t func_Wwise_Sound_SetListenerScalingFactor = 0x1f56b1a;
	inline uintptr_t func_Wwise_Sound_SetListenerSpatialization = 0x1f56c90;
	inline uintptr_t func_Wwise_Sound_SetMaxNumVoicesLimit = 0x1f58e8c;
	inline uintptr_t func_Wwise_Sound_SetMultiplePositions = 0x1f569c8;
	inline uintptr_t func_Wwise_Sound_SetObjectObstructionAndOcclusion = 0x1f57098;
	inline uintptr_t func_Wwise_Sound_SetPanningRule = 0x1f5744c;
	inline uintptr_t func_Wwise_Sound_SetPosition = 0x1f58a31;
	inline uintptr_t func_Wwise_Sound_SetPositionInternal = 0x1f5694d;
	inline uintptr_t func_Wwise_Sound_SetRTPCValue_RTPCID = 0x1f56d73;
	inline uintptr_t func_Wwise_Sound_SetRTPCValue_Char = 0x1f58a91;
	inline uintptr_t func_Wwise_Sound_SetState_StateGroupID = 0x1f58c8b;
	inline uintptr_t func_Wwise_Sound_SetState_Char = 0x1f58d69;
	inline uintptr_t func_Wwise_Sound_SetSwitch_SwitchGroupID = 0x1f56e11;
	inline uintptr_t func_Wwise_Sound_SetSwitch_Char = 0x1f58b50;
	inline uintptr_t func_Wwise_Sound_SetVolumeThreshold = 0x1f58e6c;
	inline uintptr_t func_Wwise_Sound_StartOutputCapture = 0x1f583a4;
	inline uintptr_t func_Wwise_Sound_StopAll = 0x1f58590;
	inline uintptr_t func_Wwise_Sound_StopOutputCapture = 0x1f58463;
	inline uintptr_t func_Wwise_Sound_StopPlayingID = 0x1f585d9;
	inline uintptr_t func_Wwise_Sound_StopSourcePlugin = 0x1f5756c;
	inline uintptr_t func_Wwise_Sound_Term = 0x1f5acc6;
	inline uintptr_t func_Wwise_Sound_UnloadBank_BankID_MemPoolID = 0x1f57d6b;
	inline uintptr_t func_Wwise_Sound_UnloadBank_BankID_Callback = 0x1f57e91;
	inline uintptr_t func_Wwise_Sound_UnloadBank_Char_MemPoolID = 0x1f593e7;
	inline uintptr_t func_Wwise_Sound_UnloadBank_Char_Callback = 0x1f59482;
	inline uintptr_t func_Wwise_Sound_UnloadBankUnique = 0x1f594fd;
	inline uintptr_t func_Wwise_Sound_UnregisterAllGameObj = 0x1f577dd;
	inline uintptr_t func_Wwise_Sound_UnregisterGameObj = 0x1f57784;
	inline uintptr_t func_Wwise_Sound_UnregisterGlobalCallback = 0x1f5a844;

	// StreamMgr - This Section has way too many dependancies for us to really use it.
	//inline uintptr_t func_Wwise_Stream_AddLanguageChangeObserver = 0x1fbc23a;
	//inline uintptr_t func_Wwise_Stream_Create = 0x1fbbf66;
	//inline uintptr_t func_Wwise_Stream_CreateDevice = 0x1fbc3dc;
	//inline uintptr_t func_Wwise_Stream_DestroyDevice = 0x1fbbfe6;
	//inline uintptr_t func_Wwise_Stream_FlushAllCaches = 0x1fbc076;
	//inline uintptr_t func_Wwise_Stream_GetCurrentLanguage = 0x1fbb4c3;
	//inline uintptr_t func_Wwise_Stream_GetDefaultDeviceSettings = 0x1fbb433;
	//inline uintptr_t func_Wwise_Stream_GetDefaultSettings = 0x1fbb423;
	//inline uintptr_t func_Wwise_Stream_GetFileLocationResolver = 0x1fbb493;
	//inline uintptr_t func_Wwise_Stream_GetPoolID = 0x1fbb4b3;
	//inline uintptr_t func_Wwise_Stream_RemoveLanguageChangeObserver = 0x1fbc056;
	//inline uintptr_t func_Wwise_Stream_SetCurrentLanguage = 0x1fbc036;
	//inline uintptr_t func_Wwise_Stream_SetFileLocationResolver = 0x1fbb4a3;
}