#pragma once

#include "Windows.h"

namespace Wwise::Exports {
	// Root
	inline uintptr_t func_Wwise_Root_IsRestoreSinkRequested = 0x00ec5d70; // 0x1f5bfd3;
	inline uintptr_t func_Wwise_Root_IsUsingDummySink = 0x00ec5d60; // 0x1f5bfc3;

	// IAkStreamMgr
	// Only has a single pointer, and is protected https://www.audiokinetic.com/library/2015.1.9_5624/?source=SDK&id=class_a_k_1_1_i_ak_stream_mgr_a85c6043c1a45f13b7df2f05729248b1f.html
	inline uintptr_t func_Wwise_IAkStreamMgr_m_pStreamMgr = 0x00f1c460; // 0x134f500; | Idk

	// MemoryMgr
	inline uintptr_t func_Wwise_Memory_CheckPoolId = 0x00E9FF70; // 0x1f32828
	inline uintptr_t func_Wwise_Memory_CreatePool = 0x00E9FC80; // 0x1f32519
	inline uintptr_t func_Wwise_Memory_DestroyPool = 0x00E9FE20; // 0x1f326d4;
	inline uintptr_t func_Wwise_Memory_Falign = 0x00EA01E0; // 0x1f329ee;
	inline uintptr_t func_Wwise_Memory_GetBlock = 0x00EA0230; // 0x1f32aee;
	inline uintptr_t func_Wwise_Memory_GetBlockSize = 0x00E9FF30; // 0x1f327e8;
	inline uintptr_t func_Wwise_Memory_GetMaxPools = 0x00E9FF60; // 0x1f32818;
	inline uintptr_t func_Wwise_Memory_GetNumPools = 0x00E9FF50; // 0x1f32808;
	inline uintptr_t func_Wwise_Memory_GetPoolAttributes = 0x00E9FF10; // 0x1f327c4;
	inline uintptr_t func_Wwise_Memory_GetPoolMemoryUsed = 0x00EA01A0; // 0x1f32aae;
	inline uintptr_t func_Wwise_Memory_GetPoolName = 0x00c94f80; // 0x2006a57; | TODO: may be worth a check, this one came from a different place in the exe than the rest
	inline uintptr_t func_Wwise_Memory_GetPoolStats = 0x00EA0130; // 0x1f32a3e;
	inline uintptr_t func_Wwise_Memory_IsInitialized = 0x00E9FED0; // 0x1f32784;
	inline uintptr_t func_Wwise_Memory_Malign = 0x00EA00D0; // 0x1f3298a;
	inline uintptr_t func_Wwise_Memory_Malloc = 0x00EA0050; // 0x1f3290a;
	inline uintptr_t func_Wwise_Memory_Free = 0x00EA01E0; // 0x???;
	inline uintptr_t func_Wwise_Memory_ReleaseBlock = 0x00EA0280; // 0x1f32b3e;
	inline uintptr_t func_Wwise_Memory_SetMonitoring = 0x00d3f130;
	inline uintptr_t func_Wwise_Memory_SetPoolName = 0x00E9FEE0; // 0x1f32794;
	inline uintptr_t func_Wwise_Memory_Term = 0x00EA02D0; // 0x1f32b8e;

	// Monitor
	inline uintptr_t func_Wwise_Monitor_PostCode = 0x00a004a; // 0x1f587ba;

	// Motion Engine
	inline uintptr_t func_Wwise_Motion_AddPlayerMotionDevice = 0x00ec2a00; // 0x1f58638;
	inline uintptr_t func_Wwise_Motion_RegisterMotionDevice = 0x00ec2ab0;  // 0x1f58712;
	inline uintptr_t func_Wwise_Motion_RemovePlayerMotionDevice = 0x00ec2a60; // 0x1f586ad;
	inline uintptr_t func_Wwise_Motion_SetPlayerListener = 0x00ec2ac0; // 0x1f58722;
	inline uintptr_t func_Wwise_Motion_SetPlayerVolume = 0x00ec2b00; // 0x1f5876e;

	// Music Engine
	inline uintptr_t func_Wwise_Music_GetDefaultInitSettings = 0x00ea16a0; // 0x1f340ef;
	inline uintptr_t func_Wwise_Music_GetPlayingSegmentInfo = 0x00ea16f0; // 0x1f3413f;
	inline uintptr_t func_Wwise_Music_Init = 0x00ea23e0; // 0x1f34f8c;
	inline uintptr_t func_Wwise_Music_Term = 0x00ea22b0; // 0x1f34e38;

	// Sound Engine
	inline uintptr_t func_Wwise_Sound_AddBehaviorExtension = 0x00ec46a0; // AddBehavioralExtension in the exe | 0x1f5a741;
	inline uintptr_t func_Wwise_Sound_CancelBankCallbackCookie = 0x00ec2400; // 0x1f57f0c;
	inline uintptr_t func_Wwise_Sound_CancelEventCallback = 0x00ec1c80; // 0x1f5760d;
	inline uintptr_t func_Wwise_Sound_CancelEventCallbackCookie = 0x00ec1c60; // 0x1f575ed;
	inline uintptr_t func_Wwise_Sound_ClearBanks = 0x00ec3140; // 0x1f58ecc;
	inline uintptr_t func_Wwise_Sound_ClearPreparedEvents = 0x00ec2530; // 0x1f58064;
	inline uintptr_t func_Wwise_Sound_CloneActorMixerEffect = 0x00ec5b70; // 0x1f5bd9d;
	inline uintptr_t func_Wwise_Sound_CloneBusEffect = 0x00ec5b50; // 0x1f5bd79;
	inline uintptr_t func_Wwise_Sound_DynamicDialogue_ResolveDialogueEvent_UniqueID = 0x00ec5560; // 0x1f5b68e;
	inline uintptr_t func_Wwise_Sound_DynamicDialogue_ResolveDialogueEvent_Char = 0x00ec5640; // 0x1f5b77a;
	inline uintptr_t func_Wwise_Sound_DynamicSequence_Break = 0x00ec58d0; // 0x1f5baba;
	inline uintptr_t func_Wwise_Sound_DynamicSequence_Close = 0x00ec5950; // 0x1f5bb4c;
	inline uintptr_t func_Wwise_Sound_DynamicSequence_LockPlaylist = 0x00ec59d0; // 0x1f5bbde;
	inline uintptr_t func_Wwise_Sound_DynamicSequence_Open = 0x00ec1cd0; // 0x1f57661;
	inline uintptr_t func_Wwise_Sound_DynamicSequence_Pause = 0x00ec5750; // 0x1f5b8ac;
	inline uintptr_t func_Wwise_Sound_DynamicSequence_Play = 0x00ec56d0; // 0x1f5b816;
	inline uintptr_t func_Wwise_Sound_DynamicSequence_Resume = 0x00ec57d0; // 0x1f5b93e;
	inline uintptr_t func_Wwise_Sound_DynamicSequence_Stop = 0x00ec5850; // 0x1f5b9d4;
	inline uintptr_t func_Wwise_Sound_DynamicSequence_UnlockPlaylist = 0x00ec5a10; // 0x1f5bc1e;
	inline uintptr_t func_Wwise_Sound_ExecuteActionOnEvent_UniqueID = 0x00ec5240; // 0x1f5b302;
	inline uintptr_t func_Wwise_Sound_ExecuteActionOnEvent_Char = 0x00ec52f0; // 0x1f5b3ca;
	inline uintptr_t func_Wwise_Sound_g_PlayingID = 0x00ec0890; // 0x134e790; | Idk
	inline uintptr_t func_Wwise_Sound_GetDefaultInitSettings = 0x00ec1120; // 0x1f5683d;
	inline uintptr_t func_Wwise_Sound_GetDefaultPlatformInitSettings = 0x00ec1180; // 0x1f5689d;
	inline uintptr_t func_Wwise_Sound_GetIDFromString = 0x00ec2c30; // 0x1f58956;
	inline uintptr_t func_Wwise_Sound_GetPanningRule = 0x00ec1190; // 0x1f568fd;
	inline uintptr_t func_Wwise_Sound_GetSourcePlayPosition = 0x00ec1ca0; //0x1f57631;
	inline uintptr_t func_Wwise_Sound_GetSpeakerConfiguration = 0x00ec11a0; // 0x1f5690d;
	inline uintptr_t func_Wwise_Sound_Init = 0x00ec5b90; // 0x1f5bdbd;
	inline uintptr_t func_Wwise_Sound_IsInitialized = 0x00ec1110; // 0x01f56829;
	inline uintptr_t func_Wwise_Sound_LoadBank_BankID_MemPoolID = 0x00ec2070; // 0x1f57a8f;
	inline uintptr_t func_Wwise_Sound_LoadBank_Void_UInt32_BankID = 0x00ec2130; // 0x1f57b81;
	inline uintptr_t func_Wwise_Sound_LoadBank_BankID_Callback = 0x00ec21e0; // 0x1f57c55;
	inline uintptr_t func_Wwise_Sound_LoadBank_Void_UInt32_Callback = 0x00ec2240; // 0x1f57cd0;
	inline uintptr_t func_Wwise_Sound_LoadBank_Char_MemPoolID = 0x00ec33a0; // 0x1f5916c;
	inline uintptr_t func_Wwise_Sound_LoadBank_Char_Callback = 0x00ec34b0; // 0x1f592c1;
	inline uintptr_t func_Wwise_Sound_LoadBankUnique = 0x00ec3510; // 0x1f5933c;
	inline uintptr_t func_Wwise_Sound_PlaySourcePlugin = 0x00ec1b90; // 0x1f574d5;
	inline uintptr_t func_Wwise_Sound_PostEvent_Char = 0x00ec51b0; // 0x1f5b260;
	inline uintptr_t func_Wwise_Sound_PostEvent_UniqueID = 0x00ec5cc0; // 0x1f5bf11;
	inline uintptr_t func_Wwise_Sound_PostTrigger_TriggerID = 0x00ec1610; // 0x1f56e60;
	inline uintptr_t func_Wwise_Sound_PostTrigger_Char = 0x00ec2ee0; // 0x1f58c2b;
	inline uintptr_t func_Wwise_Sound_PrepareBank_BankID_Callback = 0x00ec2420; // 0x1f57f2c;
	inline uintptr_t func_Wwise_Sound_PrepareBank_BankID_BankContent = 0x00ec36f0; // 0x1f59588;
	inline uintptr_t func_Wwise_Sound_PrepareBank_Char_Callback = 0x00ec3800; // 0x1f596d4;
	inline uintptr_t func_Wwise_Sound_PrepareBank_Char_BankContent = 0x00ec4590; // 0x1f5a651;
	inline uintptr_t func_Wwise_Sound_PrepareEvent_EventID_UInt32 = 0x00ec2480; // 0x1f57fa1;
	inline uintptr_t func_Wwise_Sound_PrepareEvent_EventID_UInt32_Callback_Void = 0x00ec2500; // 0x1f58034;
	inline uintptr_t func_Wwise_Sound_PrepareEvent_Char_UInt32 = 0x00ec39f0; // 0x1f59915;
	inline uintptr_t func_Wwise_Sound_PrepareEvent_Char_UInt32_Callback_Void = 0x00ec3ce0; // 0x1f59c73;
	inline uintptr_t func_Wwise_Sound_PrepareGameSyncs_UInt32_UInt32_UInt32_Callback_Void = 0x00ec3f60; // 0x1f58125;
	inline uintptr_t func_Wwise_Sound_PrepareGameSyncs_UInt32_UInt32_UInt32 = 0x00ec41a0; // 0x1f58155;
	inline uintptr_t func_Wwise_Sound_PrepareGameSyncs_Char_Char_UInt32_Callback_Void = 0x00ec25d0; // 0x1f59f4d;
	inline uintptr_t func_Wwise_Sound_PrepareGameSyncs_Char_Char_UInt32 = 0x00ec2600; // 0x1f5a1c0;
	inline uintptr_t func_Wwise_Sound_Query_GetActiveGameObjects = 0x00ec0790; // 0x1f55e5f;
	inline uintptr_t func_Wwise_Sound_Query_GetActiveListeners = 0x00ec0a90; // 0x1f56168;
	inline uintptr_t func_Wwise_Sound_Query_GetCustomPropertyValue_Int32 = 0x00ec08c0; // 0x1f55f95;
	inline uintptr_t func_Wwise_Sound_Query_GetCustomPropertyValue_Real32 = 0x00ec0950; // 0x1f56025;
	inline uintptr_t func_Wwise_Sound_Query_GetEventIDFromPlayingID = 0x00ec0850; // 0x1f55f25;
	inline uintptr_t func_Wwise_Sound_Query_GetGameObjectAuxSendValues = 0x00ec0d90; // 0x1f56493;
	inline uintptr_t func_Wwise_Sound_Query_GetGameObjectDryLevelValue = 0x00ec0e80; // 0x1f5658f;
	inline uintptr_t func_Wwise_Sound_Query_GetGameObjectFromPlayingID = 0x00ec0870; // 0x1f55f45;
	inline uintptr_t func_Wwise_Sound_Query_GetIsGameObjectActive = 0x00ec07c0; // 0x1f55e8f;
	inline uintptr_t func_Wwise_Sound_Query_GetListenerPosition = 0x00ec05b0; // 0x1f55c7f;
	inline uintptr_t func_Wwise_Sound_Query_GetListenerSpatialization = 0x00ec061; // 0x1f55cdf;
	inline uintptr_t func_Wwise_Sound_Query_GetMaxRadius_RadiusList = 0x00ec07f0; // 0x1f55ebf;
	inline uintptr_t func_Wwise_Sound_Query_GetMaxRadius_GameObject = 0x00ec0820; // 0x1f55eef;
	inline uintptr_t func_Wwise_Sound_Query_GetObjectObstructionAndOcclusion = 0x00ec0f10; // 0x1f56622;
	inline uintptr_t func_Wwise_Sound_Query_GetPlayingIDsFromGameObject = 0x00ec0890; // 0x1f55f65;
	inline uintptr_t func_Wwise_Sound_Query_GetPosition = 0x00ec09e0; // 0x1f560b5;
	inline uintptr_t func_Wwise_Sound_Query_GetPositioningInfo = 0x00ec0730; // 0x1f55dff;
	inline uintptr_t func_Wwise_Sound_Query_GetRTPCValue_RTPCID = 0x00ec0b20; // 0x1f561fb;
	inline uintptr_t func_Wwise_Sound_Query_GetRTPCValue_Char = 0x00ec0c50; // 0x1f5634a;
	inline uintptr_t func_Wwise_Sound_Query_GetState_StateGroupID = 0x00ec0650; // 0x1f55d1f;
	inline uintptr_t func_Wwise_Sound_Query_GetState_Char = 0x00ec06e0; // 0x1f55daf;
	inline uintptr_t func_Wwise_Sound_Query_GetSwitch_SwitchGroupID = 0x00ec0c90; // 0x1f5638a;
	inline uintptr_t func_Wwise_Sound_Query_GetSwitch_Char = 0x00ec0d60; // 0x1f56463;
	inline uintptr_t func_Wwise_Sound_Query_QueryAudioObjectIDs_UniqueID = 0x00ec0fd0; // 0x1f566e5;
	inline uintptr_t func_Wwise_Sound_Query_QueryAudioObjectIDs_Char = 0x00ec1080; // 0x1f56799;
	inline uintptr_t func_Wwise_Sound_RegisterBusVolumeCallback = 0x00ec1780; // 0x1f57018;
	inline uintptr_t func_Wwise_Sound_RegisterCodec = 0x00ec11d0; // 0x1f5693d;
	inline uintptr_t func_Wwise_Sound_RegisterGameObj = 0x00ec1d70; // 0x1f57728;
	inline uintptr_t func_Wwise_Sound_RegisterGlobalCallback = 0x00ec4770; // 0x1f5a814;
	inline uintptr_t func_Wwise_Sound_RegisterPlugin = 0x00ec11c0; // 0x1f5692d;
	inline uintptr_t func_Wwise_Sound_RemoveBehavioralExtension = 0x00ec4710; // 0x1f5a7b1;
	inline uintptr_t func_Wwise_Sound_RenderAudio = 0x00ec11b0; // 0x1f5691d;
	inline uintptr_t func_Wwise_Sound_ResetRTPCValue_RTPCID = 0x00ec16a0; // 0x1f56f0e;
	inline uintptr_t func_Wwise_Sound_ResetRTPCValue_Char = 0x00ec30a0; // 0x1f58e2c;
	inline uintptr_t func_Wwise_Sound_SeekOnEvent_UniqueID_Int32 = 0x00ec5320; // 0x1f5b3fa;
	inline uintptr_t func_Wwise_Sound_SeekOnEvent_Char_Int32 = 0x00ec53c0; // 0x1f5b4af;
	inline uintptr_t func_Wwise_Sound_SeekOnEvent_UniqueID_Float = 0x00ec53f0; // 0x1f5b4df;
	inline uintptr_t func_Wwise_Sound_SeekOnEvent_Char_Float = 0x00ec54e0; // 0x1f5b5f9;
	inline uintptr_t func_Wwise_Sound_SetActiveListeners = 0x00ec13c0; // 0x1f56b76;
	inline uintptr_t func_Wwise_Sound_SetActorMixerEffect = 0x00ec26e0; // 0x1f5825a;
	inline uintptr_t func_Wwise_Sound_SetAttenuationScalingFactor = 0x00ec1320; // 0x1f56aba;
	inline uintptr_t func_Wwise_Sound_SetBankLoadIOSettings = 0x00ec1e80; // 0x1f5785a;
	inline uintptr_t func_Wwise_Sound_SetBusEffect_UniqueID = 0x00ec2690; // 0x1f581f4;
	inline uintptr_t func_Wwise_Sound_SetBusEffect_Char = 0x00ec4320; // 0x1f5a3b1;
	inline uintptr_t func_Wwise_Sound_SetEffectParam = 0x00ec2730; // 0x1f582bc;
	inline uintptr_t func_Wwise_Sound_SetGameObjectAuxSendValues = 0x00ec1720; // 0x1f56fa9;
	inline uintptr_t func_Wwise_Sound_SetGameObjectOutputBusVolume = 0x00ec17b0; // 0x1f5704c;
	inline uintptr_t func_Wwise_Sound_SetListenerPipeline = 0x00ec1510; // 0x1f56d24;
	inline uintptr_t func_Wwise_Sound_SetListenerPosition = 0x00ec1400; // 0x1f56bc2;
	inline uintptr_t func_Wwise_Sound_SetListenerScalingFactor = 0x00ec1370; // 0x1f56b1a;
	inline uintptr_t func_Wwise_Sound_SetListenerSpatialization = 0x00ec14a0; // 0x1f56c90;
	inline uintptr_t func_Wwise_Sound_SetMaxNumVoicesLimit = 0x00ec3100; // 0x1f58e8c;
	inline uintptr_t func_Wwise_Sound_SetMultiplePositions = 0x00ec1240; // 0x1f569c8;
	inline uintptr_t func_Wwise_Sound_SetObjectObstructionAndOcclusion = 0x00ec17f0; // 0x1f57098;
	inline uintptr_t func_Wwise_Sound_SetPanningRule = 0x00ec1b10; // 0x1f5744c;
	inline uintptr_t func_Wwise_Sound_SetPosition = 0x00ec2d10; // 0x1f58a31;
	inline uintptr_t func_Wwise_Sound_SetPositionInternal = 0x00ec11e0; // 0x1f5694d;
	inline uintptr_t func_Wwise_Sound_SetRTPCValue_RTPCID = 0x00ec1550; // 0x1f56d73;
	inline uintptr_t func_Wwise_Sound_SetRTPCValue_Char = 0x00ec2d70; // 0x1f58a91;
	inline uintptr_t func_Wwise_Sound_SetState_StateGroupID = 0x00ec2f30; // 0x1f58c8b;
	inline uintptr_t func_Wwise_Sound_SetState_Char = 0x00ec2ff0; // 0x1f58d69;
	inline uintptr_t func_Wwise_Sound_SetSwitch_SwitchGroupID = 0x00ec15d0; // 0x1f56e11;
	inline uintptr_t func_Wwise_Sound_SetSwitch_Char = 0x00ec2e20; // 0x1f58b50;
	inline uintptr_t func_Wwise_Sound_SetVolumeThreshold = 0x00ec30e0; // 0x1f58e6c;
	inline uintptr_t func_Wwise_Sound_StartOutputCapture = 0x00ec27a0; // 0x1f583a4;
	inline uintptr_t func_Wwise_Sound_StopAll = 0x00ec2970; // 0x1f58590;
	inline uintptr_t func_Wwise_Sound_StopOutputCapture = 0x00ec2850; // 0x1f58463;
	inline uintptr_t func_Wwise_Sound_StopPlayingID = 0x00ec29b0; // 0x1f585d9;
	inline uintptr_t func_Wwise_Sound_StopSourcePlugin = 0x00ec1c00; // 0x1f5756c;
	inline uintptr_t func_Wwise_Sound_Term = 0x00ec4c70; // 0x1f5acc6;
	inline uintptr_t func_Wwise_Sound_UnloadBank_BankID_MemPoolID = 0x00ec22c0; // 0x1f57d6b;
	inline uintptr_t func_Wwise_Sound_UnloadBank_BankID_Callback = 0x00ec23a0; // 0x1f57e91;
	inline uintptr_t func_Wwise_Sound_UnloadBank_Char_MemPoolID = 0x00ec35a0; // 0x1f593e7;
	inline uintptr_t func_Wwise_Sound_UnloadBank_Char_Callback = 0x00ec3620; // 0x1f59482;
	inline uintptr_t func_Wwise_Sound_UnloadBankUnique = 0x00ec3680; // 0x1f594fd;
	inline uintptr_t func_Wwise_Sound_UnregisterAllGameObj = 0x00ec1e10; // 0x1f577dd;
	inline uintptr_t func_Wwise_Sound_UnregisterGameObj = 0x00ec1dc0; // 0x1f57784;
	inline uintptr_t func_Wwise_Sound_UnregisterGlobalCallback = 0x00ec47a0; // 0x1f5a844;

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