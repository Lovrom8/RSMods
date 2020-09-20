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
typedef bool(__cdecl* IsRestoreSinkRequested)(void);
typedef bool(__cdecl* IsUsingDummySink)(void);
// MemoryMgr
typedef AKRESULT(__cdecl* tMemory_CheckPoolId)(AkMemPoolId in_poolId);
typedef AKRESULT(__cdecl* tMemory_CreatePool)(void* in_pMemAddress, AkUInt32 in_uMemSize, AkUInt32 in_uBlockSize, AkUInt32 in_eAttributes, AkUInt32 in_uBlockAlign);
typedef AKRESULT(__cdecl* tMemory_DestroyPool)(AkMemPoolId in_poolId);
typedef AKRESULT(__cdecl* tMemory_Falign)(AkMemPoolId in_poolId, void* in_pMemAddress);
typedef void*(__cdecl* tMemory_GetBlock)(AkMemPoolId in_poolId);
typedef AkUInt32(__cdecl* tMemory_GetBlockSize)(AkMemPoolId in_poolId);
typedef AkInt32(__cdecl* tMemory_GetMaxPools)(void);
typedef AkInt32(__cdecl* tMemory_GetNumPools)(void);
	// GetPoolAttributes (Unknown)
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
	// AddBehavioralExtension (Custom)
typedef void(__cdecl* tCancelBankCallbackCookie)(void* in_pCookie);
typedef void(__cdecl* tCancelEventCallback)(AkPlayingID in_playingID);
typedef void(__cdecl* tCancelEventCallbackCookie)(void* in_pCookie);
typedef AKRESULT(__cdecl* tClearBanks)(void);
typedef AKRESULT(__cdecl* tClearPreparedEvents)(void);
	// CloneActorMixerEffect (Custom)
	// CloneBusEffect (Custom)
// Dynamic Dialogue
typedef AkUniqueID(__cdecl* tDynamicDialogue_ResolveDialogueEvent_UniqueID)(AkUniqueID in_eventID, AkArgumentValueID* in_aArgumentValues, AkUInt32 in_uNumArguments, AkPlayingID in_idSequence);
typedef AkUniqueID(__cdecl* tDynamicDialogue_ResolveDialogueEvent_Char)(const char* in_pszEventName, const char** in_aArgumentValueNames, AkUInt32 in_uNumArguments, AkPlayingID in_idSequence);
// End Dynamic Dialogue
// Dynamic Sequence
typedef AKRESULT(__cdecl* tDynamicSequence_Break)(AkPlayingID in_playingID);
typedef AKRESULT(__cdecl* tDynamicSequence_Close)(AkPlayingID in_playingID);
	// LockPlaylist (Personal || A lot of Dependancies)
typedef AkPlayingID(__cdecl* tDynamicSequence_Open)(AkGameObjectID in_gameObjectID, AkUInt32 in_uFlags, AkCallbackFunc in_pfnCallback, void* in_pCookie, DynamicSequenceType in_eDynamicSequenceType);
typedef AKRESULT(__cdecl* tDynamicSequence_Pause)(AkPlayingID in_playingID, AkTimeMs in_uTransitionDuration, AkCurveInterpolation in_eFadeCurve);
typedef AKRESULT(__cdecl* tDynamicSequence_Play)(AkPlayingID in_playingID, AkTimeMs in_uTransitionDuration, AkCurveInterpolation in_eFadeCurve);
typedef AKRESULT(__cdecl* tDynamicSequence_Resume)(AkPlayingID in_playingID, AkTimeMs in_uTransitionDuration, AkCurveInterpolation in_eFadeCurve);
typedef AKRESULT(__cdecl* tDynamicSequence_Stop)(AkPlayingID in_playingID, AkTimeMs in_uTransitionDuration, AkCurveInterpolation in_eFadeCurve);
	// UnlockPlaylist (Personal || A lot of Dependancies)
// End Dynamic Sequence
typedef AKRESULT(__cdecl* tExecuteActionOnEvent_UniqueID)(AkUniqueID in_eventID, AkActionOnEventType in_ActionType, AkGameObjectID in_gameObjectID, AkTimeMs in_uTransitionDuration, AkCurveInterpolation in_eFadeCurve, AkPlayingID in_PlayingID);
typedef AKRESULT(__cdecl* tExecuteActionOnEvent_Char)(const char* in_pszEventName, AkActionOnEventType in_ActionType, AkGameObjectID in_gameObjectID, AkTimeMs in_uTransitionDuration, AkCurveInterpolation in_eFadeCurve, AkPlayingID in_PlayingID);
typedef void(__cdecl* tGetDefaultInitSettings)(AkCommSettings& out_settings);
	// GetDefaultPlatformInitSettings (Personal || We only run on Windows, so we don't need this)
typedef AkUInt32(__cdecl* tGetIDFromString)(const char* in_pszString);
	// GetPanningRule (Personal || A lot of Dependancies)
typedef AKRESULT(__cdecl* tGetSourcePlayPosition)(AkPlayingID in_PlayingID, AkTimeMs* out_puPosition, bool in_bExtrapolate);
typedef AkUInt32(__cdecl* tGetSpeakerConfiguration)(void);
	// Init (Personal || This is called before we can even push different settings to it)
typedef bool(__cdecl* tIsInitialized)(void);
// Load Banks
typedef AKRESULT(__cdecl* tLoadBank_BankID_MemPoolID)(AkBankID in_bankID, AkMemPoolId in_memPoolId);
typedef AKRESULT(__cdecl* tLoadBank_Void_UInt32_BankID)(const void* in_plnMemoryBankPtr, AkUInt32 in_ulnMemoryBankSize, AkBankID* out_bankID);
typedef AKRESULT(__cdecl* tLoadBank_BankID_CallBack)(AkBankID in_BankID, AkBankCallbackFunc in_pfnBankCallback, void* in_pCookie, AkMemPoolId in_memPoolId);
typedef AKRESULT(__cdecl* tLoadBank_Void_UInt32_CallBack)(const void* in_plnMemoryBankPtr, AkUInt32 in_ulnMemoryBankSize, AkBankCallbackFunc in_pfnBankCallback, void* in_pCookie, AkBankID* out_bankID);
typedef AKRESULT(__cdecl* tLoadBank_Char_MemPoolID)(const char* in_pszString, AkMemPoolId in_memPoolId, AkBankID* out_bankID);
typedef AKRESULT(__cdecl* tLoadBank_Char_CallBack)(const char* in_pszString, AkBankCallbackFunc in_pfnBankCallback, void* in_pCookie, AkMemPoolId in_memPoolId, AkBankID* out_bankId);
	// LoadBankUnique (Custom)
// End Load Banks
	// PlaySourcePlugin (Custom)
typedef AkPlayingID(__cdecl* tPostEvent_Char)(const char* in_pszEventName, AkGameObjectID in_gameObjectID, AkUInt32 in_uFlags, AkCallbackFunc in_pfnCallback, void* in_pCookie, AkUInt32 in_cExternals, AkExternalSourceInfo* in_pExternalSources, AkPlayingID in_PlayingID);
typedef AkPlayingID(__cdecl* tPostEven_UniqueID)(AkUniqueID in_eventID, AkGameObjectID in_gameObjectID, AkUInt32 in_uFlags, AkCallbackFunc in_pfnCallback, void* in_pCookie, AkUInt32 in_cExternals, AkExternalSourceInfo* in_pExternalSources, AkPlayingID in_PlayingID);
typedef AKRESULT(__cdecl* tPostTrigger_TriggerID)(AkTriggerID in_triggerID, AkGameObjectID in_gameObjectID);
typedef AKRESULT(__cdecl* tPostTrigger_Char)(char* in_szTriggerName, AkGameObjectID in_gameObjectID);
// Prepare Bank
typedef AKRESULT(__cdecl* tPrepareBank_BankID_Callback)(PreparationType in_PreparationType, AkBankID in_bankID, AkBankCallbackFunc in_pfnBankCallback, void* in_pCookie, AkBankContent in_uFlags);
typedef AKRESULT(__cdecl* tPrepareBank_BankID_BankContent)(PreparationType in_PreparationType, AkBankID in_bankID, AkBankContent in_uFlags);
typedef AKRESULT(__cdecl* tPrepareBank_Char_CallBack)(PreparationType in_PreparationType, const char* in_pszString, AkBankCallbackFunc in_pfnBankCallback, void* in_pCookie, AkBankContent in_uFlags);
typedef AKRESULT(__cdecl* tPrepareBank_Char_BankContent)(PreparationType in_PreparationType, const char* in_pszString, AkBankContent in_uFlags);
// End Prepare Bank
// Prepare Event
typedef AKRESULT(__cdecl* tPrepareEvent_EventID_UInt32)(PreparationType in_PreparationType, AkUniqueID* in_pEventID, AkUInt32 in_uNumEvent);
typedef AKRESULT(__cdecl* tPrepareEvent_EventID_UInt32_Callback_Void)(PreparationType in_PreparationType, AkUniqueID* in_pEventID, AkUInt32 in_uNumEvent, AkBankCallbackFunc in_pfnBankCallback, void* in_pCookie);
typedef AKRESULT(__cdecl* tPrepareEvent_Char_UInt32)(PreparationType in_PreparationType, const char** in_ppszString, AkUInt32 in_uNumEvent);
typedef AKRESULT(__cdecl* tPrepareEvent_Char_UInt32_Callback_Void)(PreparationType in_PreparationType, const char** in_ppszString, AkUInt32 in_uNumEvent, AkBankCallbackFunc in_pfnBankCallback, void* in_pCookie);
// End Prepare Event
// Prepare Game Syncs
typedef AKRESULT(__cdecl* tPrepareGameSyncs_UInt32_UInt32_UInt32_Callback_Void)(PreparationType in_PreparationType, AkGroupType in_eGameSyncType, AkUInt32 in_GroupID, AkUInt32* in_paGameSyncID, AkUInt32 in_uNumGameSyncs, AkBankCallbackFunc in_pfnBankCallback, void* in_pCookie);
typedef AKRESULT(__cdecl* tPrepareGameSyncs_UInt32_UInt32_UInt32)(PreparationType in_PreparationType, AkGroupType in_eGameSyncType, AkUInt32 in_GroupID, AkUInt32* in_paGameSyncID, AkUInt32 in_uNumGameSyncs);
typedef AKRESULT(__cdecl* tPrepareGameSyncs_Char_Char_UInt32_Callback_Void)(PreparationType in_PreparationType, AkGroupType in_eGameSyncType, const char* in_pszGroupName, const char** in_ppszGameSyncName, AkUInt32 in_uNumGameSyncs, AkBankCallbackFunc in_pfnBankCallback, void* in_pCookie);
typedef AKRESULT(__cdecl* tPrepareGameSyncs_Char_Char_UInt32)(PreparationType in_PreparationType, AkGroupType in_eGameSyncType, const char* in_pszGroupName, const char** in_ppszGameSyncName, AkUInt32 in_uNumGameSyncs);
// End Prepare Game Syncs
// Query
	// GetActiveGameObjects (Personal || Requires AkArray which has a lot of dependancies)
typedef AKRESULT(__cdecl* tQuery_GetActiveListeners)(AkGameObjectID in_GameObjectID, AkUInt32* out_ruListenerMask);
typedef AKRESULT(__cdecl* tQuery_GetCustomPropertyValue_Int32)(AkUniqueID in_ObjectID, AkUInt32 in_uPropID, AkInt32* out_iValue);
typedef AKRESULT(__cdecl* tQuery_GetCustomPropertyValue_Real32)(AkUniqueID in_ObjectID, AkUInt32 in_uPropID, AkReal32* out_fValue);
typedef AkUniqueID(__cdecl* tQuery_GetEventIDFromPlayingID)(AkPlayingID in_playingID);
typedef AKRESULT(__cdecl* tQuery_GetGameObjectAuxSendValues)(AkGameObjectID in_gameObjectID, AkAuxSendValue* out_paAuxSendValues, AkUInt32 &io_ruNumSendValues);
typedef AKRESULT(__cdecl* tQuery_GetGameObjectDryLevelValue)(AkGameObjectID in_gameObjectID, AkReal32* out_rfControlValue);
typedef AkGameObjectID(__cdecl* tQuery_GetGameObjectFromPlayingID)(AkPlayingID in_playingID);
typedef bool(__cdecl* tQuery_GetIsGameObjectActive)(AkGameObjectID in_GameObjId);
typedef AKRESULT(__cdecl* tQuery_GetListenerPosition)(AkUInt32 in_uIndex, AkListenerPosition* out_rPosition);
	// GetMaxRadius(AkArray) (Personal || Requires AkArray which has a lot of dependancies)
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
	// RegisterBusVolumeCallback (Custom)
typedef AKRESULT(__cdecl* tRegisterCodec)(AkUInt32 in_ulCompanyID, AkUInt32 in_ulCodecID, AkCreateFileSourceCallback in_pFileCreateFunc, AkCreateBankSourceCallback in_pBankCreateFunc);
	// RegisterGameObj (Custom)
typedef AKRESULT(__cdecl* tRegisterGlobalCallback)(AkGlobalCallbackFunc in_pCallback);
typedef AKRESULT(__cdecl* tRegisterPlugin)(AkPluginType in_eType, AkUInt32 in_ulCompanyID, AkUInt32 in_ulPluginID, AkCreatePluginCallback in_pCreateFunc, AkCreateParamCallback in_pCreateParamFunc);
	// RemoveBehavioralExtension (Custom)
typedef AKRESULT(__cdecl* tRenderAudio)(void);
	// ResetRTPCValue (Custom) (x2)
	// SeekOnEvent (Custom) (x4)
typedef AKRESULT(__cdecl* tSetActiveListeners)(AkGameObjectID in_gameObjectID, AkUInt32 in_uiListenerMask);
typedef AKRESULT(__cdecl* tSetActorMixerEffect)(AkUniqueID in_audioNodeID, AkUInt32 in_uFXIndex, AkUniqueID in_shareSetID);
typedef AKRESULT(__cdecl* tSetAttenuationScalingFactor)(AkGameObjectID in_GameObjectID, AkReal32 in_fAttenuationScalingFactor);
typedef AKRESULT(__cdecl* tSetBankLoadIOSettings)(AkReal32 in_fThroughput, AkPriority in_priority);
typedef AKRESULT(__cdecl* tSetBusEffect_UniqueID)(AkUniqueID in_audioNodeID, AkUInt32 in_uFXIndex, AkUniqueID in_shareSetID);
typedef AKRESULT(__cdecl* tSetBusEffect_Char)(const char* in_pszBusName, AkUInt32 in_uFXIndex, AkUniqueID in_shareSetID);
	// SetEffectParam (Custom)
typedef AKRESULT(__cdecl* tSetGameObjectAuxSendValues)(AkGameObjectID in_gameObjectID, AkAuxSendValue* in_aAuxSendValues, AkUInt32 in_uNumSendValues);
typedef AKRESULT(__cdecl* tSetGameObjectOutputBusVolume)(AkGameObjectID in_gameObjectID, AkReal32 in_fControlValue);
typedef AKRESULT(__cdecl* tSetListenerPipeline)(AkUInt32 in_uIndex, bool in_bAudio, bool in_bMotion);
typedef AKRESULT(__cdecl* tSetListenerPosition)(const AkListenerPosition* in_rPosition, AkUInt32 in_uiIndex);
typedef AKRESULT(__cdecl* tSetListenerScalingFactor)(AkUInt32 in_uiIndex, AkReal32 in_fAttenuationScalingFactor);
	// SetListenerSpatialization (Speaker Volumes has a lot of dependancies)
typedef AKRESULT(__cdecl* tSetMaxNumVoicesLimit)(AkUInt16 in_maxNumberVoices);
typedef AKRESULT(__cdecl* tSetMultiplePositions)(AkGameObjectID in_GameObjectID, const AkSoundPosition* in_pPositions, AkUInt16 in_NumPositions, MultiPositionType in_eMultiPositionType);
typedef AKRESULT(__cdecl* tSetObjectObstructionAndOcclusion)(AkGameObjectID in_ObjectID, AkUInt32 in_uListener, AkReal32 in_fObstructionLevel, AkReal32 in_fOcclusionLevel);
	// SetPanningRule (Personal || A lot of Dependancies)
typedef AKRESULT(__cdecl* tSetPosition)(AkGameObjectID in_GameObjectID, const AkSoundPosition* in_Position);
	// SetPositionInternal (Custom)
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
	// StopSourcePlugin (Custom)
typedef AKRESULT(__cdecl* tTerm)(void);
typedef AKRESULT(__cdecl* tUnloadBank_BankID_MemPoolID)(AkBankID in_bankID, const void* in_pInMemoryBankPtr, AkMemPoolId* out_pMemPoolId);
typedef AKRESULT(__cdecl* tUnloadBank_BankID_Callback)(AkBankID in_bankID, const void* in_pInMemoryBankPtr, AkBankCallbackFunc in_pfnBankCallback, void* in_pCookie);
typedef AKRESULT(__cdecl* tUnloadBank_Char_MemPoolID)(const char* in_pszString, const void* in_pInMemoryBankPtr, AkMemPoolId* out_pMemPoolId);
typedef AKRESULT(__cdecl* tUnloadBank_Char_Callback)(const char* in_pszString, const void* in_pInMemoryBankPtr, AkBankCallbackFunc in_pfnBankCallback, void* in_pCookie);
	// UnloadBankUnique (Custom)
typedef AKRESULT(__cdecl* tUnregisterAllGameObj)(void);
typedef AKRESULT(__cdecl* tUnregisterGameObj)(AkGameObjectID in_gameObjectID);
typedef AKRESULT(__cdecl* tUnregisterGlobalCallback)(AkGlobalCallbackFunc in_pCallback);
// End Sound Engine
// StreamMgr
// This Section has way too many dependancies for us to really use it.
// End StreamMgr

// TODO: Add Functions that had no definition in the Wwise Documentation (may be custom funcs or stuff removed between WWise 2013 <-> Wwise 2015).
// TODO: Fill Types to Variables List
// TODO: Add Mem Addresses to Offsets
// TODO: Merge Function with Type-Variable in DLLMain::InitEngineFunctions()
// Types to Variables
tSetRTPCValue_Char SetRTPCValue;
tQuery_GetRTPCValue_Char GetRTPCValue;
tClearBanks ClearBanks;
tUnloadBank_BankID_MemPoolID UnloadBank;
/*--------------------------- D3D ---------------------------------*/

extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
typedef LRESULT(CALLBACK* WNDPROC)(HWND, UINT, WPARAM, LPARAM);

typedef HRESULT(WINAPI* tDrawIndexedPrimitive)(IDirect3DDevice9*, D3DPRIMITIVETYPE, INT, UINT, UINT, UINT, UINT);
tDrawIndexedPrimitive oDrawIndexedPrimitive;

typedef HRESULT(WINAPI* tBeginScene)(IDirect3DDevice9*);
tBeginScene oBeginScene;

typedef HRESULT(WINAPI* tDrawPrimitive)(IDirect3DDevice9*, D3DPRIMITIVETYPE, UINT, UINT);
tDrawPrimitive oDrawPrimitive;

typedef HRESULT(__stdcall* tEndScene)(IDirect3DDevice9*);
tEndScene oEndScene;

typedef HRESULT(APIENTRY* tReset)(IDirect3DDevice9*, D3DPRESENT_PARAMETERS*);
tReset oReset;

typedef HRESULT(APIENTRY* tSetStreamSource)(IDirect3DDevice9*, UINT, IDirect3DVertexBuffer9*, UINT, UINT);
tSetStreamSource oSetStreamSource;

typedef HRESULT(APIENTRY* tSetVertexDeclaration)(IDirect3DDevice9*, IDirect3DVertexDeclaration9*);
tSetVertexDeclaration oSetVertexDeclaration;

typedef HRESULT(APIENTRY* tSetVertexShaderConstantF)(IDirect3DDevice9*, UINT, const float*, UINT);
tSetVertexShaderConstantF oSetVertexShaderConstantF;

typedef HRESULT(APIENTRY* tSetVertexShader)(IDirect3DDevice9*, IDirect3DVertexShader9*);
tSetVertexShader oSetVertexShader;

typedef HRESULT(APIENTRY* tSetPixelShader)(IDirect3DDevice9*, IDirect3DPixelShader9*);;
tSetPixelShader oSetPixelShader;

typedef HRESULT(APIENTRY* tSetTexture)(IDirect3DDevice9*, DWORD, IDirect3DBaseTexture9*);
tSetTexture oSetTexture;

typedef HRESULT(APIENTRY* tPresent) (IDirect3DDevice9*, const RECT*, const RECT*, HWND, const RGNDATA*);
tPresent oPresent;