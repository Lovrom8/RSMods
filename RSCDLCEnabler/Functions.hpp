#pragma once
#include "windows.h"
#include "Lib/DirectX/d3d9.h"
#include "Lib/DirectX/d3dx9.h"

#pragma comment(lib, "d3d9.lib")
#pragma comment(lib, "d3dx9.lib")


/*----------------------- FORCE ENUMERATION -----------------------*/

typedef void(__thiscall* _tForceEnumeration)(byte* rs_dlc_service_flags); //maybe in the future
_tForceEnumeration forceEnumeration;


void __fastcall tForceEnumeration(byte* rs_dlc_service_flags) {
	return forceEnumeration(rs_dlc_service_flags);
}

/*-------------------- AUDIO KINETIC STUFF ------------------------*/

// Get documentation here: https://www.audiokinetic.com/library/2015.1.9_5624/?source=SDK&id=namespace_a_k_1_1_sound_engine.html

// IAKStreamMgr
// Only has a single pointer, and is protected https://www.audiokinetic.com/library/2015.1.9_5624/?source=SDK&id=class_a_k_1_1_i_ak_stream_mgr_a85c6043c1a45f13b7df2f05729248b1f.html
// End IAKStreamMgr
typedef bool(__cdecl* tIsRestoreSinkRequested)(void);
typedef bool(__cdecl* tIsUsingDummySink)(void);
// MemoryMgr
typedef AKRESULT(__cdecl* tMemory_CheckPoolId)(AkMemPoolId in_poolId);
typedef AKRESULT(__cdecl* tMemory_CreatePool)(void* in_pMemAddress, AkUInt32 in_uMemSize, AkUInt32 in_uBlockSize, AkUInt32 in_eAttributes, AkUInt32 in_uBlockAlign);
typedef AKRESULT(__cdecl* tMemory_DestroyPool)(AkMemPoolId in_poolId);
typedef AKRESULT(__cdecl* tMemory_Falign)(AkMemPoolId in_poolId, void* in_pMemAddress);
typedef void*(__cdecl* tMemory_GetBlock)(AkMemPoolId in_poolId);
typedef AkUInt32(__cdecl* tMemory_GetBlockSize)(AkMemPoolId in_poolId);
typedef AkInt32(__cdecl* tMemory_GetMaxPools)(void);
typedef AkInt32(__cdecl* tMemory_GetNumPools)(void);
typedef AkMemPoolAttributes(__cdecl* tMemory_GetPoolAttributes)(AkMemPoolId in_poolId);
typedef void(__cdecl* tMemory_GetPoolMemoryUsed)(AkMemPoolId in_poolId, PoolMemInfo* out_memInfo);
typedef AkOSChar*(__cdecl* tMemory_GetPoolName)(AkMemPoolId in_poolId);
typedef AKRESULT(__cdecl* tMemory_GetPoolStats)(AkMemPoolId in_poolId, PoolStats* out_stats);
typedef bool(__cdecl* tMemory_IsInitialized)(void);
typedef void*(__cdecl* tMemory_Malign)(AkMemPoolId in_poolId, size_t in_uSize, AkUInt32 in_uAlignment);
typedef void*(__cdecl* tMemory_Malloc)(AkMemPoolId in_poolId, size_t in_uSize);
typedef AKRESULT(__cdecl* tMemory_ReleaseBlock)(AkMemPoolId in_poolId, void* in_pMemAddress);
typedef AKRESULT(__cdecl* tMemory_SetMonitoring)(AkMemPoolId in_poolId, bool in_bDoMonitor);
typedef AKRESULT(__cdecl* tMemory_SetPoolName)(AkMemPoolId in_poolId, const char* in_pszPoolName);
typedef void(__cdecl* tMemory_Term)(void);
// End MemoryMgr
// Monitor
typedef AKRESULT(__cdecl* tMonitor_PostCode)(ErrorCode in_eError, ErrorLevel in_eErrorLevel);
// End Monitor
// Motion Engine
typedef AKRESULT(__cdecl* tMotion_AddPlayerMotionDevice)(AkUInt8 in_iPlayerID, AkUInt32 in_iCompanyID, AkUInt32 in_iDeviceID, void* in_pDevice);
typedef void(__cdecl* tMotion_RegisterMotionDevice)(AkUInt32 in_ulCompanyID, AkUInt32 in_ulPluginID, AkCreatePluginCallback in_pCreateFunc);
typedef void(__cdecl* tMotion_RemovePlayerMotionDevice)(AkUInt8 in_iPlayerID, AkUInt32 in_iCompanyID, AkUInt32 in_iDeviceID);
typedef void(__cdecl* tMotion_SetPlayerListener)(AkUInt8 in_iPlayerID, AkUInt8 in_iListener);
typedef void(__cdecl* tMotion_SetPlayerVolume)(AkUInt8 in_iPlayerID, AkReal32 in_fVolume);
// End Motion Engine
// Music Engine
typedef void(__cdecl* tMusic_GetDefaultInitSettings)(AkMusicSettings* out_settings);
typedef AKRESULT(__cdecl* tMusic_GetPlayingSegmentInfo)(AkPlayingID in_playingID, AkSegmentInfo* out_segmentInfo, bool in_bExtrapolate);
typedef AKRESULT(__cdecl* tMusic_Init)(AkMusicSettings* in_pSettings);
typedef void(__cdecl* tMusic_Term)(void);
// End Music Engine
// Sound Engine
typedef void(__cdecl* tCancelBankCallbackCookie)(void* in_pCookie);
typedef void(__cdecl* tCancelEventCallback)(AkPlayingID in_playingID);
typedef void(__cdecl* tCancelEventCallbackCookie)(void* in_pCookie);
typedef AKRESULT(__cdecl* tClearBanks)(void);
typedef AKRESULT(__cdecl* tClearPreparedEvents)(void);
// Dynamic Dialogue
typedef AkUniqueID(__cdecl* tDynamicDialogue_ResolveDialogueEvent_UniqueID)(AkUniqueID in_eventID, AkArgumentValueID* in_aArgumentValues, AkUInt32 in_uNumArguments, AkPlayingID in_idSequence);
typedef AkUniqueID(__cdecl* tDynamicDialogue_ResolveDialogueEvent_Char)(const char* in_pszEventName, const char** in_aArgumentValueNames, AkUInt32 in_uNumArguments, AkPlayingID in_idSequence);
// End Dynamic Dialogue
// Dynamic Sequence
typedef AKRESULT(__cdecl* tDynamicSequence_Break)(AkPlayingID in_playingID);
typedef AKRESULT(__cdecl* tDynamicSequence_Close)(AkPlayingID in_playingID);
// LockPlaylist (Dependancies)
typedef AkPlayingID(__cdecl* tDynamicSequence_Open)(AkGameObjectID in_gameObjectID, AkUInt32 in_uFlags, AkCallbackFunc in_pfnCallback, void* in_pCookie, DynamicSequenceType in_eDynamicSequenceType);
typedef AKRESULT(__cdecl* tDynamicSequence_Pause)(AkPlayingID in_playingID, AkTimeMs in_uTransitionDuration, AkCurveInterpolation in_eFadeCurve);
typedef AKRESULT(__cdecl* tDynamicSequence_Play)(AkPlayingID in_playingID, AkTimeMs in_uTransitionDuration, AkCurveInterpolation in_eFadeCurve);
typedef AKRESULT(__cdecl* tDynamicSequence_Resume)(AkPlayingID in_playingID, AkTimeMs in_uTransitionDuration, AkCurveInterpolation in_eFadeCurve);
typedef AKRESULT(__cdecl* tDynamicSequence_Stop)(AkPlayingID in_playingID, AkTimeMs in_uTransitionDuration, AkCurveInterpolation in_eFadeCurve);
// UnlockPlaylist (Dependancies)
// End Dynamic Sequence
typedef AKRESULT(__cdecl* tExecuteActionOnEvent_UniqueID)(AkUniqueID in_eventID, AkActionOnEventType in_ActionType, AkGameObjectID in_gameObjectID, AkTimeMs in_uTransitionDuration, AkCurveInterpolation in_eFadeCurve, AkPlayingID in_PlayingID);
typedef AKRESULT(__cdecl* tExecuteActionOnEvent_Char)(const char* in_pszEventName, AkActionOnEventType in_ActionType, AkGameObjectID in_gameObjectID, AkTimeMs in_uTransitionDuration, AkCurveInterpolation in_eFadeCurve, AkPlayingID in_PlayingID);
typedef void(__cdecl* tGetDefaultInitSettings)(AkCommSettings& out_settings);
// GetDefaultPlatformInitSettings (We only run on Windows, so we don't need this)
typedef AkUInt32(__cdecl* tGetIDFromString)(const char* in_pszString);
// GetPanningRule (Dependancies)
typedef AKRESULT(__cdecl* tGetSourcePlayPosition)(AkPlayingID in_PlayingID, AkTimeMs* out_puPosition, bool in_bExtrapolate);
typedef AkUInt32(__cdecl* tGetSpeakerConfiguration)(void);
// Init (This is called before we can even push different settings to it)
typedef bool(__cdecl* tIsInitialized)(void);
typedef AKRESULT(__cdecl* tLoadBank_BankID_MemPoolID)(AkBankID in_bankID, AkMemPoolId in_memPoolId);
typedef AKRESULT(__cdecl* tLoadBank_Void_UInt32_BankID)(const void* in_plnMemoryBankPtr, AkUInt32 in_ulnMemoryBankSize, AkBankID* out_bankID);
typedef AKRESULT(__cdecl* tLoadBank_BankID_CallBack)(AkBankID in_BankID, AkBankCallbackFunc in_pfnBankCallback, void* in_pCookie, AkMemPoolId in_memPoolId);
typedef AKRESULT(__cdecl* tLoadBank_Void_UInt32_CallBack)(const void* in_plnMemoryBankPtr, AkUInt32 in_ulnMemoryBankSize, AkBankCallbackFunc in_pfnBankCallback, void* in_pCookie, AkBankID* out_bankID);
typedef AKRESULT(__cdecl* tLoadBank_Char_MemPoolID)(const char* in_pszString, AkMemPoolId in_memPoolId, AkBankID* out_bankID);
typedef AKRESULT(__cdecl* tLoadBank_Char_CallBack)(const char* in_pszString, AkBankCallbackFunc in_pfnBankCallback, void* in_pCookie, AkMemPoolId in_memPoolId, AkBankID* out_bankId);
typedef AkPlayingID(__cdecl* tPostEvent_Char)(const char* in_pszEventName, AkGameObjectID in_gameObjectID, AkUInt32 in_uFlags, AkCallbackFunc in_pfnCallback, void* in_pCookie, AkUInt32 in_cExternals, AkExternalSourceInfo* in_pExternalSources, AkPlayingID in_PlayingID);
typedef AkPlayingID(__cdecl* tPostEvent_UniqueID)(AkUniqueID in_eventID, AkGameObjectID in_gameObjectID, AkUInt32 in_uFlags, AkCallbackFunc in_pfnCallback, void* in_pCookie, AkUInt32 in_cExternals, AkExternalSourceInfo* in_pExternalSources, AkPlayingID in_PlayingID);
typedef AKRESULT(__cdecl* tPostTrigger_TriggerID)(AkTriggerID in_triggerID, AkGameObjectID in_gameObjectID);
typedef AKRESULT(__cdecl* tPostTrigger_Char)(char* in_szTriggerName, AkGameObjectID in_gameObjectID);
typedef AKRESULT(__cdecl* tPrepareBank_BankID_Callback)(PreparationType in_PreparationType, AkBankID in_bankID, AkBankCallbackFunc in_pfnBankCallback, void* in_pCookie, AkBankContent in_uFlags);
typedef AKRESULT(__cdecl* tPrepareBank_BankID_BankContent)(PreparationType in_PreparationType, AkBankID in_bankID, AkBankContent in_uFlags);
typedef AKRESULT(__cdecl* tPrepareBank_Char_CallBack)(PreparationType in_PreparationType, const char* in_pszString, AkBankCallbackFunc in_pfnBankCallback, void* in_pCookie, AkBankContent in_uFlags);
typedef AKRESULT(__cdecl* tPrepareBank_Char_BankContent)(PreparationType in_PreparationType, const char* in_pszString, AkBankContent in_uFlags);
typedef AKRESULT(__cdecl* tPrepareEvent_EventID_UInt32)(PreparationType in_PreparationType, AkUniqueID* in_pEventID, AkUInt32 in_uNumEvent);
typedef AKRESULT(__cdecl* tPrepareEvent_EventID_UInt32_Callback_Void)(PreparationType in_PreparationType, AkUniqueID* in_pEventID, AkUInt32 in_uNumEvent, AkBankCallbackFunc in_pfnBankCallback, void* in_pCookie);
typedef AKRESULT(__cdecl* tPrepareEvent_Char_UInt32)(PreparationType in_PreparationType, const char** in_ppszString, AkUInt32 in_uNumEvent);
typedef AKRESULT(__cdecl* tPrepareEvent_Char_UInt32_Callback_Void)(PreparationType in_PreparationType, const char** in_ppszString, AkUInt32 in_uNumEvent, AkBankCallbackFunc in_pfnBankCallback, void* in_pCookie);
typedef AKRESULT(__cdecl* tPrepareGameSyncs_UInt32_UInt32_UInt32_Callback_Void)(PreparationType in_PreparationType, AkGroupType in_eGameSyncType, AkUInt32 in_GroupID, AkUInt32* in_paGameSyncID, AkUInt32 in_uNumGameSyncs, AkBankCallbackFunc in_pfnBankCallback, void* in_pCookie);
typedef AKRESULT(__cdecl* tPrepareGameSyncs_UInt32_UInt32_UInt32)(PreparationType in_PreparationType, AkGroupType in_eGameSyncType, AkUInt32 in_GroupID, AkUInt32* in_paGameSyncID, AkUInt32 in_uNumGameSyncs);
typedef AKRESULT(__cdecl* tPrepareGameSyncs_Char_Char_UInt32_Callback_Void)(PreparationType in_PreparationType, AkGroupType in_eGameSyncType, const char* in_pszGroupName, const char** in_ppszGameSyncName, AkUInt32 in_uNumGameSyncs, AkBankCallbackFunc in_pfnBankCallback, void* in_pCookie);
typedef AKRESULT(__cdecl* tPrepareGameSyncs_Char_Char_UInt32)(PreparationType in_PreparationType, AkGroupType in_eGameSyncType, const char* in_pszGroupName, const char** in_ppszGameSyncName, AkUInt32 in_uNumGameSyncs);
// Query
// GetActiveGameObjects (Dependancies & Requires AkArray)
typedef AKRESULT(__cdecl* tQuery_GetActiveListeners)(AkGameObjectID in_GameObjectID, AkUInt32* out_ruListenerMask);
typedef AKRESULT(__cdecl* tQuery_GetCustomPropertyValue_Int32)(AkUniqueID in_ObjectID, AkUInt32 in_uPropID, AkInt32* out_iValue);
typedef AKRESULT(__cdecl* tQuery_GetCustomPropertyValue_Real32)(AkUniqueID in_ObjectID, AkUInt32 in_uPropID, AkReal32* out_fValue);
typedef AkUniqueID(__cdecl* tQuery_GetEventIDFromPlayingID)(AkPlayingID in_playingID);
typedef AKRESULT(__cdecl* tQuery_GetGameObjectAuxSendValues)(AkGameObjectID in_gameObjectID, AkAuxSendValue* out_paAuxSendValues, AkUInt32 &io_ruNumSendValues);
typedef AKRESULT(__cdecl* tQuery_GetGameObjectDryLevelValue)(AkGameObjectID in_gameObjectID, AkReal32* out_rfControlValue);
typedef AkGameObjectID(__cdecl* tQuery_GetGameObjectFromPlayingID)(AkPlayingID in_playingID);
typedef bool(__cdecl* tQuery_GetIsGameObjectActive)(AkGameObjectID in_GameObjId);
typedef AKRESULT(__cdecl* tQuery_GetListenerPosition)(AkUInt32 in_uIndex, AkListenerPosition* out_rPosition);
// GetMaxRadius(AkArray) (Dependancies & Requires AkArray)
typedef AKRESULT(__cdecl* tQuery_GetMaxRadius)(AkGameObjectID in_GameObjId);
typedef AKRESULT(__cdecl* tQuery_GetObjectObstructionAndOcclusion)(AkGameObjectID in_ObjectID, AkUInt32 in_uListener, AkReal32* out_rfObstructionLevel, AkReal32* out_rfOcclusionLevel);
typedef AKRESULT(__cdecl* tQuery_GetPlayingIDsFromGameObject)(AkGameObjectID in_GameObjId, AkUInt32* io_ruNumIds, AkPlayingID* out_aPlayingIDs);
typedef AKRESULT(__cdecl* tQuery_GetPosition)(AkGameObjectID in_GameObjectID, AkSoundPosition* out_rPosition);
typedef AKRESULT(__cdecl* tQuery_GetPositioningInfo)(AkUniqueID in_ObjectID, AkPositioningInfo* out_rPositioningInfo);
typedef AKRESULT(__cdecl* tQuery_GetRTPCValue_Char)(const char* in_pszRtpcName, AkGameObjectID in_gameObjectID, AkRtpcValue* out_rValue, RTPCValue_type* io_rValueType);
typedef AKRESULT(__cdecl* tQuery_GetRTPCValue_RTPCID)(AkRtpcID in_rtpcID, AkGameObjectID in_gameObjectID, AkRtpcValue* out_rValue, RTPCValue_type* io_rValueType);
typedef AKRESULT(__cdecl* tQuery_GetState_StateGroupID)(AkStateGroupID in_stateGroup, AkStateID* out_rState);
typedef AKRESULT(__cdecl* tQuery_GetState_Char)(const char* in_pstrStateGroupName, AkStateID* out_rState);
typedef AKRESULT(__cdecl* tQuery_GetSwitch_SwitchGroupID)(AkSwitchGroupID in_switchGroup, AkGameObjectID in_gameObjectID, AkSwitchStateID* out_rSwitchState);
typedef AKRESULT(__cdecl* tQuery_GetSwitch_Char)(const char* in_pstrSwitchGroupName, AkGameObjectID in_GameObj, AkSwitchStateID* out_rSwitchState);
typedef AKRESULT(__cdecl* tQuery_QueryAudioObjectIDs_UniqueID)(AkUniqueID in_eventID, AkUInt32* io_ruNumItems, AkObjectInfo* out_aObjectInfos);
typedef AKRESULT(__cdecl* tQuery_QueryAudioObjectIDs_Char)(const char* in_pszEventName, AkUInt32* io_ruNumItems, AkObjectInfo* out_aObjectInfos);
// End Query
typedef AKRESULT(__cdecl* tRegisterCodec)(AkUInt32 in_ulCompanyID, AkUInt32 in_ulCodecID, AkCreateFileSourceCallback in_pFileCreateFunc, AkCreateBankSourceCallback in_pBankCreateFunc);
typedef AKRESULT(__cdecl* tRegisterGlobalCallback)(AkGlobalCallbackFunc in_pCallback);
typedef AKRESULT(__cdecl* tRegisterPlugin)(AkPluginType in_eType, AkUInt32 in_ulCompanyID, AkUInt32 in_ulPluginID, AkCreatePluginCallback in_pCreateFunc, AkCreateParamCallback in_pCreateParamFunc);
typedef AKRESULT(__cdecl* tRenderAudio)(void);
typedef AKRESULT(__cdecl* tSetActiveListeners)(AkGameObjectID in_gameObjectID, AkUInt32 in_uiListenerMask);
typedef AKRESULT(__cdecl* tSetActorMixerEffect)(AkUniqueID in_audioNodeID, AkUInt32 in_uFXIndex, AkUniqueID in_shareSetID);
typedef AKRESULT(__cdecl* tSetAttenuationScalingFactor)(AkGameObjectID in_GameObjectID, AkReal32 in_fAttenuationScalingFactor);
typedef AKRESULT(__cdecl* tSetBankLoadIOSettings)(AkReal32 in_fThroughput, AkPriority in_priority);
typedef AKRESULT(__cdecl* tSetBusEffect_UniqueID)(AkUniqueID in_audioNodeID, AkUInt32 in_uFXIndex, AkUniqueID in_shareSetID);
typedef AKRESULT(__cdecl* tSetBusEffect_Char)(const char* in_pszBusName, AkUInt32 in_uFXIndex, AkUniqueID in_shareSetID);
typedef AKRESULT(__cdecl* tSetGameObjectAuxSendValues)(AkGameObjectID in_gameObjectID, AkAuxSendValue* in_aAuxSendValues, AkUInt32 in_uNumSendValues);
typedef AKRESULT(__cdecl* tSetGameObjectOutputBusVolume)(AkGameObjectID in_gameObjectID, AkReal32 in_fControlValue);
typedef AKRESULT(__cdecl* tSetListenerPipeline)(AkUInt32 in_uIndex, bool in_bAudio, bool in_bMotion);
typedef AKRESULT(__cdecl* tSetListenerPosition)(const AkListenerPosition* in_rPosition, AkUInt32 in_uiIndex);
typedef AKRESULT(__cdecl* tSetListenerScalingFactor)(AkUInt32 in_uiIndex, AkReal32 in_fAttenuationScalingFactor);
// SetListenerSpatialization (Dependancies)
typedef AKRESULT(__cdecl* tSetMaxNumVoicesLimit)(AkUInt16 in_maxNumberVoices);
typedef AKRESULT(__cdecl* tSetMultiplePositions)(AkGameObjectID in_GameObjectID, const AkSoundPosition* in_pPositions, AkUInt16 in_NumPositions, MultiPositionType in_eMultiPositionType);
typedef AKRESULT(__cdecl* tSetObjectObstructionAndOcclusion)(AkGameObjectID in_ObjectID, AkUInt32 in_uListener, AkReal32 in_fObstructionLevel, AkReal32 in_fOcclusionLevel);
// SetPanningRule (Dependancies)
typedef AKRESULT(__cdecl* tSetPosition)(AkGameObjectID in_GameObjectID, const AkSoundPosition* in_Position);
typedef AKRESULT(__cdecl* tSetRTPCValue_RTPCID)(AkRtpcID in_rtpcID, AkRtpcValue in_value, AkGameObjectID in_gameObjectID, AkTimeMs in_uValueChangeDuration, AkCurveInterpolation in_eFadeCruve, bool in_bBypassInternalValueInterpolation);
typedef AKRESULT(__cdecl* tSetRTPCValue_Char) (const char* in_pszRtpcName, AkRtpcValue in_value, AkGameObjectID in_gameObjectID, AkTimeMs in_uValueChangeDuration, AkCurveInterpolation in_eFadeCurve);
typedef AKRESULT(__cdecl* tSetState_StateGroupID)(AkStateGroupID in_stateGroup, AkStateID in_state);
typedef AKRESULT(__cdecl* tSetState_Char)(const char* in_pszStateGroup, const char* in_pszState);
typedef AKRESULT(__cdecl* tSetSwitch_SwitchGroupID)(AkSwitchGroupID in_switchGroup, AkSwitchStateID in_switchState, AkGameObjectID in_gameObjectID);
typedef AKRESULT(__cdecl* tSetSwitch_Char)(const char* in_pszSwitchGroup, const char* in_pszSwitchState, AkGameObjectID in_gameObjectID);
typedef AKRESULT(__cdecl* tSetVolumeThreshold)(AkReal32 in_fVolumeThresholdDB);
typedef AKRESULT(__cdecl* tStartOutputCapture)(const AkOSChar* in_CaptureFileName);
typedef void(__cdecl* tStopAll)(AkGameObjectID in_gameObjectID);
typedef AKRESULT(__cdecl* tStopOutputCapture)(void);
typedef AKRESULT(__cdecl* tStopPlayingID)(AkPlayingID in_playingID, AkTimeMs in_uTransitionDuration, AkCurveInterpolation in_eFadeCurve);
typedef AKRESULT(__cdecl* tTerm)(void);
typedef AKRESULT(__cdecl* tUnloadBank_BankID_MemPoolID)(AkBankID in_bankID, const void* in_pInMemoryBankPtr, AkMemPoolId* out_pMemPoolId);
typedef AKRESULT(__cdecl* tUnloadBank_BankID_Callback)(AkBankID in_bankID, const void* in_pInMemoryBankPtr, AkBankCallbackFunc in_pfnBankCallback, void* in_pCookie);
typedef AKRESULT(__cdecl* tUnloadBank_Char_MemPoolID)(const char* in_pszString, const void* in_pInMemoryBankPtr, AkMemPoolId* out_pMemPoolId);
typedef AKRESULT(__cdecl* tUnloadBank_Char_Callback)(const char* in_pszString, const void* in_pInMemoryBankPtr, AkBankCallbackFunc in_pfnBankCallback, void* in_pCookie);
typedef AKRESULT(__cdecl* tUnregisterAllGameObj)(void);
typedef AKRESULT(__cdecl* tUnregisterGameObj)(AkGameObjectID in_gameObjectID);
typedef AKRESULT(__cdecl* tUnregisterGlobalCallback)(AkGlobalCallbackFunc in_pCallback);
// End Sound Engine
// StreamMgr
// This Section has way too many dependancies for us to really use it.
// End StreamMgr
// End WWise Documentation

// Rocksmith Custom Wwise Functions | These will use vague types since that's all I can get out of Ghidra
// AddBehavioralExtension (Custom | Can't be added because a param requires a function)
typedef AkUInt32(__cdecl* tCloneActorMixerEffect)(AkUInt32 param_1, AkUInt32 param_2, AkUInt32 param_3);
typedef AkUInt32(__cdecl* tCloneBusEffect)(AkUInt32 param_1, AkUInt32 param_2, AkUInt32 param_3);
typedef AKRESULT(__cdecl* tLoadBankUnique)(const char* in_pszString, AkBankCallbackFunc in_pfnBankCallback, void* in_pCookie, AkMemPoolId in_memPoolId, AkBankID* out_bankId);
typedef AkUInt32(__cdecl* tPlaySourcePlugin)(AkUInt32 param_1, AkUInt32 param_2, AkUInt32 param_3);
// RegisterBusVolumeCallback (Custom | Can't be added because a param requires a function)
typedef AKRESULT(__cdecl* tRegisterGameObj)(AkGameObjectID in_gameObjectID);
typedef AKRESULT(__cdecl* tResetRTPCValue_RTPCID)(AkRtpcID in_rtpcID, UINT param_2, long param_3, AkCurveInterpolation in_curveInterpolation);
typedef AKRESULT(__cdecl* tResetRTPCValue_Char)(const char* in_pszRtpcName, UINT param_2, long param_3, AkCurveInterpolation in_curveInterpolation);
typedef AKRESULT(__cdecl* tSeekOnEvent_UniqueID_Int32)(AkUniqueID in_eventID, AkGameObjectID in_gameObjectID, AkTimeMs in_iPosition, bool in_bSeekToNearestMarker);
typedef AKRESULT(__cdecl* tSeekOnEvent_Char_Int32)(const char* in_pszEventName, AkGameObjectID in_gameObjectID, AkTimeMs in_iPosition, bool in_bSeekToNearestMarker);
typedef AKRESULT(__cdecl* tSeekOnEvent_UniqueID_Float)(AkUniqueID in_eventID, AkGameObjectID in_gameObjectID, AkReal32 in_iPosition, bool in_bSeekToNearestMarker);
typedef AKRESULT(__cdecl* tSeekOnEvent_Char_Float)(const char* in_pszEventName, AkGameObjectID in_gameObjectID, AkReal32 in_iPosition, bool in_bSeekToNearestMarker);
typedef AKRESULT(__cdecl* tSetEffectParam)(AkUInt32 param_1, short param_2, void* in_pCookie);
typedef AKRESULT(__cdecl* tSetPositionInternal)(AkGameObjectID in_GameObjectID, AkSoundPosition* in_soundPosition);
typedef AKRESULT(__cdecl* tStopSourcePlugin)(AkUInt32 param_1, AkUInt32 param_2, AkUInt32 param_3);
typedef AKRESULT(__cdecl* tUnloadBankUnique)(const char* in_pszString, AkBankCallbackFunc in_pfnBankCallback, void* in_pCookie);
// End Rocksmith Custom Wwise Functions

// TODO: Merge Function with Type-Variable in DLLMain::InitEngineFunctions()

// Types to Variables List

// Root Functions
tIsRestoreSinkRequested Wwise_Root_IsRestoreSinkRequested;
tIsUsingDummySink Wwise_Root_IsUsingDummySink;

// MemoryMgr
tMemory_CheckPoolId Wwise_Memory_CheckPoolId;
tMemory_CreatePool Wwise_Memory_CreatePool;
tMemory_DestroyPool Wwise_Memory_DestroyPool;
tMemory_Falign Wwise_Memory_Falign;
tMemory_GetBlock Wwise_Memory_GetBlock;
tMemory_GetBlockSize Wwise_Memory_GetBlockSize;
tMemory_GetMaxPools Wwise_Memory_GetMaxPools;
tMemory_GetNumPools Wwise_Memory_GetNumPools;
tMemory_GetPoolAttributes Wwise_Memory_GetPoolAttributes;
tMemory_GetPoolMemoryUsed Wwise_Memory_GetPoolMemoryUsed;
tMemory_GetPoolName Wwise_Memory_GetPoolName;
tMemory_GetPoolStats Wwise_Memory_GetPoolStats;
tMemory_IsInitialized Wwise_Memory_IsInitialized;
tMemory_Malign Wwise_Memory_Malign;
tMemory_Malloc Wwise_Memory_Malloc;
tMemory_ReleaseBlock Wwise_Memory_ReleaseBlock;
tMemory_SetMonitoring Wwise_Memory_SetMonitoring;
tMemory_SetPoolName Wwise_Memory_SetPoolName;
tMemory_Term Wwise_Memory_Term;

// Monitor
tMonitor_PostCode Wwise_Monitor_PostCode;

// Motion Engine
tMotion_AddPlayerMotionDevice Wwise_Motion_AddPlayerMotionDevice;
tMotion_RegisterMotionDevice Wwise_Motion_RegisterMotionDevice;
tMotion_RemovePlayerMotionDevice Wwise_Motion_RemovePlayerMotionDevice;
tMotion_SetPlayerListener Wwise_Motion_SetPlayerListener;
tMotion_SetPlayerVolume Wwise_Motion_SetPlayerVolume;

// Music Engine
tMusic_GetDefaultInitSettings Wwise_Music_GetDefaultInitSettings;
tMusic_GetPlayingSegmentInfo Wwise_Music_GetPlayingSegmentInfo;
tMusic_Init Wwise_Music_Init;
tMusic_Term Wwise_Music_Term;

// Sound Engine
tCancelBankCallbackCookie Wwise_Sound_CancelBankCallbackCookie;
tCancelEventCallback Wwise_Sound_CancelEventCallback;
tCancelEventCallbackCookie Wwise_Sound_CancelEventCallbackCookie;
tClearBanks Wwise_Sound_ClearBanks;
tClearPreparedEvents Wwise_Sound_ClearPreparedEvents;

	// Dynamic Dialogue
tDynamicDialogue_ResolveDialogueEvent_UniqueID Wwise_Sound_DynamicDialogue_ResolveDialogueEvent_UniqueID;
tDynamicDialogue_ResolveDialogueEvent_Char Wwise_Sound_DynamicDialogue_ResolveDialogueEvent_Char;

	// Dynamic Sequence
tDynamicSequence_Break Wwise_Sound_DynamicSequence_Break;
tDynamicSequence_Close Wwise_Sound_DynamicSequence_Close;
tDynamicSequence_Open Wwise_Sound_DynamicSequence_Open;
tDynamicSequence_Pause Wwise_Sound_DynamicSequence_Pause;
tDynamicSequence_Play Wwise_Sound_DynamicSequence_Play;
tDynamicSequence_Resume Wwise_Sound_DynamicSequence_Resume;
tDynamicSequence_Stop Wwise_Sound_DynamicSequence_Stop;
	// End Dynamic Sequence

tExecuteActionOnEvent_UniqueID Wwise_Sound_ExecuteActionOnEvent_UniqueID;
tExecuteActionOnEvent_Char Wwise_Sound_ExecuteActionOnEvent_Char;
tGetDefaultInitSettings Wwise_Sound_GetDefaultInitSettings;
tGetIDFromString Wwise_Sound_GetIDFromString;
tGetSourcePlayPosition Wwise_Sound_GetSourcePlayPosition;
tGetSpeakerConfiguration Wwise_Sound_GetSpeakerConfiguration;
tIsInitialized Wwise_Sound_IsInitialized;
tLoadBank_BankID_MemPoolID Wwise_Sound_LoadBank_BankID_MemPoolID;
tLoadBank_Void_UInt32_BankID Wwise_Sound_LoadBank_Void_UInt32_BankID;
tLoadBank_BankID_CallBack Wwise_Sound_LoadBank_BankID_CallBack;
tLoadBank_Void_UInt32_CallBack Wwise_Sound_LoadBank_Void_UInt32_CallBack;
tLoadBank_Char_MemPoolID Wwise_Sound_LoadBank_Char_MemPoolID;
tLoadBank_Char_CallBack Wwise_Sound_LoadBank_Char_CallBack;
tPostEvent_Char Wwise_Sound_PostEvent_Char;
tPostEvent_UniqueID Wwise_Sound_PostEvent_UniqueID;
tPostTrigger_TriggerID Wwise_Sound_PostTrigger_TriggerID;
tPostTrigger_Char Wwise_Sound_PostTrigger_Char;
tPrepareBank_BankID_Callback Wwise_Sound_PrepareBank_BankID_Callback;
tPrepareBank_BankID_BankContent Wwise_Sound_PrepareBank_BankID_BankContent;
tPrepareBank_Char_CallBack Wwise_Sound_PrepareBank_Char_CallBack;
tPrepareBank_Char_BankContent Wwise_Sound_PrepareBank_Char_BankContent;
tPrepareEvent_EventID_UInt32 Wwise_Sound_PrepareEvent_EventID_UInt32;
tPrepareEvent_EventID_UInt32_Callback_Void Wwise_Sound_PrepareEvent_EventID_UInt32_Callback_Void;
tPrepareEvent_Char_UInt32 Wwise_Sound_PrepareEvent_Char_UInt32;
tPrepareEvent_Char_UInt32_Callback_Void Wwise_Sound_PrepareEvent_Char_UInt32_Callback_Void;
tPrepareGameSyncs_UInt32_UInt32_UInt32_Callback_Void Wwise_Sound_PrepareGameSyncs_UInt32_UInt32_UInt32_Callback_Void;
tPrepareGameSyncs_UInt32_UInt32_UInt32 Wwise_Sound_PrepareGameSyncs_UInt32_UInt32_UInt32;
tPrepareGameSyncs_Char_Char_UInt32_Callback_Void Wwise_Sound_PrepareGameSyncs_Char_Char_UInt32_Callback_Void;
tPrepareGameSyncs_Char_Char_UInt32 Wwise_Sound_PrepareGameSyncs_Char_Char_UInt32;

	// Query
tQuery_GetActiveListeners Wwise_Sound_Query_GetActiveListeners;
tQuery_GetCustomPropertyValue_Int32 Wwise_Sound_Query_GetCustomPropertyValue_Int32;
tQuery_GetCustomPropertyValue_Real32 Wwise_Sound_Query_GetCustomPropertyValue_Real32;
tQuery_GetEventIDFromPlayingID Wwise_Sound_Query_GetEventIDFromPlayingID;
tQuery_GetGameObjectAuxSendValues Wwise_Sound_Query_GetGameObjectAuxSendValues;
tQuery_GetGameObjectDryLevelValue Wwise_Sound_Query_GetGameObjectDryLevelValue;
tQuery_GetGameObjectFromPlayingID Wwise_Sound_Query_GetGameObjectFromPlayingID;
tQuery_GetIsGameObjectActive Wwise_Sound_Query_GetIsGameObjectActive;
tQuery_GetListenerPosition Wwise_Sound_Query_GetListenerPosition;
tQuery_GetMaxRadius Wwise_Sound_Query_GetMaxRadius;
tQuery_GetObjectObstructionAndOcclusion Wwise_Sound_Query_GetObjectObstructionAndOcclusion;
tQuery_GetPlayingIDsFromGameObject Wwise_Sound_Query_GetPlayingIDsFromGameObject;
tQuery_GetPosition Wwise_Sound_Query_GetPosition;
tQuery_GetPositioningInfo Wwise_Sound_Query_GetPositioningInfo;
tQuery_GetRTPCValue_Char Wwise_Sound_Query_GetRTPCValue_Char;
tQuery_GetRTPCValue_RTPCID Wwise_Sound_Query_GetRTPCValue_RTPCID;
tQuery_GetState_StateGroupID Wwise_Sound_Query_GetState_StateGroupID;
tQuery_GetState_Char Wwise_Sound_Query_GetState_Char;
tQuery_GetSwitch_SwitchGroupID Wwise_Sound_Query_GetSwitch_SwitchGroupID;
tQuery_GetSwitch_Char Wwise_Sound_Query_GetSwitch_Char;
tQuery_QueryAudioObjectIDs_UniqueID Wwise_Sound_Query_QueryAudioObjectIDs_UniqueID;
tQuery_QueryAudioObjectIDs_Char Wwise_Sound_Query_QueryAudioObjectIDs_Char;
	// End Query

tRegisterCodec Wwise_Sound_RegisterCodec;
tRegisterGlobalCallback Wwise_Sound_RegisterGlobalCallback;
tRegisterPlugin Wwise_Sound_RegisterPlugin;
tRenderAudio Wwise_Sound_RenderAudio;
tSetActiveListeners Wwise_Sound_SetActiveListeners;
tSetActorMixerEffect Wwise_Sound_SetActorMixerEffect;
tSetAttenuationScalingFactor Wwise_Sound_SetAttenuationScalingFactor;
tSetBankLoadIOSettings Wwise_Sound_SetBankLoadIOSettings;
tSetBusEffect_UniqueID Wwise_Sound_SetBusEffect_UniqueID;
tSetBusEffect_Char Wwise_Sound_SetBusEffect_Char;
tSetGameObjectAuxSendValues Wwise_Sound_SetGameObjectAuxSendValues;
tSetGameObjectOutputBusVolume Wwise_Sound_SetGameObjectOutputBusVolume;
tSetListenerPipeline Wwise_Sound_SetListenerPipeline;
tSetListenerPosition Wwise_Sound_SetListenerPosition;
tSetListenerScalingFactor Wwise_Sound_SetListenerScalingFactor;
tSetMaxNumVoicesLimit Wwise_Sound_SetMaxNumVoicesLimit;
tSetMultiplePositions Wwise_Sound_SetMultiplePositions;
tSetObjectObstructionAndOcclusion Wwise_Sound_SetObjectObstructionAndOcclusion;
tSetPosition Wwise_Sound_SetPosition;
tSetRTPCValue_RTPCID Wwise_Sound_SetRTPCValue_RTPCID;
tSetRTPCValue_Char Wwise_Sound_SetRTPCValue_Char;
tSetState_StateGroupID Wwise_Sound_SetState_StateGroupID;
tSetState_Char Wwise_Sound_SetState_Char;
tSetSwitch_SwitchGroupID Wwise_Sound_SetSwitch_SwitchGroupID;
tSetSwitch_Char Wwise_Sound_SetSwitch_Char;
tSetVolumeThreshold Wwise_Sound_SetVolumeThreshold;
tStartOutputCapture Wwise_Sound_StartOutputCapture;
tStopAll Wwise_Sound_StopAll;
tStopOutputCapture Wwise_Sound_StopOutputCapture;
tStopPlayingID Wwise_Sound_StopPlayingID;
tTerm Wwise_Sound_Term;
tUnloadBank_BankID_MemPoolID Wwise_Sound_UnloadBank_BankID_MemPoolID;
tUnloadBank_BankID_Callback Wwise_Sound_UnloadBank_BankID_Callback;
tUnloadBank_Char_MemPoolID Wwise_Sound_UnloadBank_Char_MemPoolID;
tUnloadBank_Char_Callback Wwise_Sound_UnloadBank_Char_Callback;
tUnregisterAllGameObj Wwise_Sound_UnregisterAllGameObj;
tUnregisterGameObj Wwise_Sound_UnregisterGameObj;
tUnregisterGlobalCallback Wwise_Sound_UnregisterGlobalCallback;

// Rocksmith Custom Wwise Functions
tCloneActorMixerEffect Wwise_Sound_CloneActorMixerEffect;
tCloneBusEffect Wwise_Sound_CloneBusEffect;
tLoadBankUnique Wwise_Sound_LoadBankUnique;
tPlaySourcePlugin Wwise_Sound_PlaySourcePlugin;
tRegisterGameObj Wwise_Sound_RegisterGameObj;
tResetRTPCValue_RTPCID Wwise_Sound_ResetRTPCValue_RTPCID;
tResetRTPCValue_Char Wwise_Sound_ResetRTPCValue_Char;
tSeekOnEvent_UniqueID_Int32 Wwise_Sound_SeekOnEvent_UniqueID_Int32;
tSeekOnEvent_UniqueID_Float Wwise_Sound_SeekOnEvent_UniqueID_Float;
tSeekOnEvent_Char_Int32 Wwise_Sound_SeekOnEvent_Char_Int32;
tSeekOnEvent_Char_Float Wwise_Sound_SeekOnEvent_Char_Float;
tSetEffectParam Wwise_Sound_SetEffectParam;
tSetPositionInternal Wwise_Sound_SetPositionInternal;
tStopSourcePlugin Wwise_Sound_StopSourcePlugin;
tUnloadBankUnique Wwise_Sound_UnloadBankUnique;

// End Types to Variables List
