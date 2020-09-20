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
typedef AKRESULT(__cdecl* tSetRTPCValue) (const char* in_pszRtpcName, AkRtpcValue in_value, AkGameObjectID in_gameObjectID, AkTimeMs in_uValueChangeDuration, AkCurveInterpolation in_eFadeCurve);

typedef AKRESULT(__cdecl* tUnloadBank)(AkBankID in_bankID, const void* in_pInMemoryBankPtr, AkMemPoolId* out_pMemPoolId);

typedef void(__cdecl* tCancelBankCallbackCookie)(void* in_pCookie);
typedef void(__cdecl* tCancelEventCallback)(AkPlayingID in_playingID);
typedef void(__cdecl* tCancelEventCallbackCookie)(void* in_pCookie);
typedef AKRESULT(__cdecl* tClearBanks)(void);
typedef AKRESULT(__cdecl* tClearPreparedEvents)(void);
// Dynamic Dialogue
typedef AkUniqueID(__cdecl* tDynamicDialogue_ResolveDialogueEvent_UniqueID)(AkUniqueID in_eventID, AkArgumentValueID* in_aArgumentValues, AkUInt32 in_uNumArguments, AkPlayingID in_idSequence);
typedef AkUniqueID(__cdecl* tDynamicDialogue_ResolveDialogueEvent_Char)(const char* in_pszEventName, const char** in_aArgumentValueNames, AkUInt32 in_uNumArguments, AkPlayingID in_idSequence);
//End Dynamic Dialogue
// Dynamic Sequence
typedef AKRESULT(__cdecl* tDynamicSequence_Break)(AkPlayingID in_playingID);
typedef AKRESULT(__cdecl* tDynamicSequence_Close)(AkPlayingID in_playingID);
typedef AkPlayingID(__cdecl* tDynamicSequence_Open)(AkGameObjectID in_gameObjectID, AkUInt32 in_uFlags, AkCallbackFunc in_pfnCallback, void* in_pCookie, DynamicSequenceType in_eDynamicSequenceType);
typedef AKRESULT(__cdecl* tDynamicSequence_Pause)(AkPlayingID in_playingID, AkTimeMs in_uTransitionDuration, AkCurveInterpolation in_eFadeCurve);
typedef AKRESULT(__cdecl* tDynamicSequence_Play)(AkPlayingID in_playingID, AkTimeMs in_uTransitionDuration, AkCurveInterpolation in_eFadeCurve);
typedef AKRESULT(__cdecl* tDynamicSequence_Resume)(AkPlayingID in_playingID, AkTimeMs in_uTransitionDuration, AkCurveInterpolation in_eFadeCurve);
typedef AKRESULT(__cdecl* tDynamicSequence_Stop)(AkPlayingID in_playingID, AkTimeMs in_uTransitionDuration, AkCurveInterpolation in_eFadeCurve);
// End Dynamic Sequence
typedef AKRESULT(__cdecl* tExecuteActionOnEvent_UniqueID)(AkUniqueID in_eventID, AkActionOnEventType in_ActionType, AkGameObjectID in_gameObjectID, AkTimeMs in_uTransitionDuration, AkCurveInterpolation in_eFadeCurve, AkPlayingID in_PlayingID);
typedef AKRESULT(__cdecl* tExecuteActionOnEvent_Char)(const char* in_pszEventName, AkActionOnEventType in_ActionType, AkGameObjectID in_gameObjectID, AkTimeMs in_uTransitionDuration, AkCurveInterpolation in_eFadeCurve, AkPlayingID in_PlayingID);
typedef void(__cdecl* tGetDefaultInitSettings)(AkCommSettings& out_settings);
typedef AkUInt32(__cdecl* tGetIDFromString)(const char* in_pszString);
typedef AKRESULT(__cdecl* tGetSourcePlayPosition)(AkPlayingID in_PlayingID, AkTimeMs* out_puPosition, bool in_bExtrapolate);
typedef AkUInt32(__cdecl* tGetSpeakerConfiguration)(void);
typedef bool(__cdecl* tIsInitialized)(void);
// Load Banks
typedef AKRESULT(__cdecl* tLoadBank_BankID_MemPoolID)(AkBankID in_bankID, AkMemPoolId in_memPoolId);
typedef AKRESULT(__cdecl* tLoadBank_Void_UInt32_BankID)(const void* in_plnMemoryBankPtr, AkUInt32 in_ulnMemoryBankSize, AkBankID* out_bankID);
typedef AKRESULT(__cdecl* tLoadBank_BankID_CallBack)(AkBankID in_BankID, AkBankCallbackFunc in_pfnBankCallback, void* in_pCookie, AkMemPoolId in_memPoolId);
typedef AKRESULT(__cdecl* tLoadBank_Void_UInt32_CallBack)(const void* in_plnMemoryBankPtr, AkUInt32 in_ulnMemoryBankSize, AkBankCallbackFunc in_pfnBankCallback, void* in_pCookie, AkBankID* out_bankID);
typedef AKRESULT(__cdecl* tLoadBank_Char_MemPoolID)(const char* in_pszString, AkMemPoolId in_memPoolId, AkBankID* out_bankID);
typedef AKRESULT(__cdecl* tLoadBank_Char_CallBack)(const char* in_pszString, AkBankCallbackFunc in_pfnBankCallback, void* in_pCookie, AkMemPoolId in_memPoolId, AkBankID* out_bankId);
// End Load Banks
// Post Event
typedef AkPlayingID(__cdecl* tPostEvent_Char)(const char* in_pszEventName, AkGameObjectID in_gameObjectID, AkUInt32 in_uFlags, AkCallbackFunc in_pfnCallback, void* in_pCookie, AkUInt32 in_cExternals, AkExternalSourceInfo* in_pExternalSources, AkPlayingID in_PlayingID);
typedef AkPlayingID(__cdecl* tPostEven_UniqueID)(AkUniqueID in_eventID, AkGameObjectID in_gameObjectID, AkUInt32 in_uFlags, AkCallbackFunc in_pfnCallback, void* in_pCookie, AkUInt32 in_cExternals, AkExternalSourceInfo* in_pExternalSources, AkPlayingID in_PlayingID);
// End Post Event
// Post Trigger
typedef AKRESULT(__cdecl* tPostTrigger_TriggerID)(AkTriggerID in_triggerID, AkGameObjectID in_gameObjectID);
typedef AKRESULT(__cdecl* tPostTrigger_Char)(char* in_szTriggerName, AkGameObjectID in_gameObjectID);
// End Post Trigger
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
typedef AKRESULT(__cdecl* tQuery_GetActiveListeners)(AkGameObjectID in_GameObjectID, AkUInt32* out_ruListenerMask);
typedef AKRESULT(__cdecl* tQuery_GetCustomPropertyValue_Int32)(AkUniqueID in_ObjectID, AkUInt32 in_uPropID, AkInt32* out_iValue);
typedef AKRESULT(__cdecl* tQuery_GetCustomPropertyValue_Real32)(AkUniqueID in_ObjectID, AkUInt32 in_uPropID, AkReal32* out_fValue);
typedef AkUniqueID(__cdecl* tQuery_GetEventIDFromPlayingID)(AkPlayingID in_playingID);
typedef AKRESULT(__cdecl* tQuery_GetGameObjectAuxSendValues)(AkGameObjectID in_gameObjectID, AkAuxSendValue* out_paAuxSendValues, AkUInt32 &io_ruNumSendValues);
typedef AKRESULT(__cdecl* tQuery_GetGameObjectDryLevelValue)(AkGameObjectID in_gameObjectID, AkReal32* out_rfControlValue);
typedef AkGameObjectID(__cdecl* tQuery_GetGameObjectFromPlayingID)(AkPlayingID in_playingID);
typedef bool(__cdecl* tQuery_GetIsGameObjectActive)(AkGameObjectID in_GameObjId);
typedef AKRESULT(__cdecl* tQuery_GetListenerPosition)(AkUInt32 in_uIndex, AkListenerPosition* out_rPosition);
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
// TODO: Finish RegisterBusVolumeCallback <-> UnregisterGlobalCallback

// Types to Variables
tSetRTPCValue SetRTPCValue;
tQuery_GetRTPCValue_Char GetRTPCValue;
tClearBanks ClearBanks;
tUnloadBank UnloadBank;
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