#include "SoundEngine.hpp"

namespace Wwise::SoundEngine {

	void CancelBankCallbackCookie(void* in_pCookie) {
		tCancelBankCallbackCookie func = (tCancelBankCallbackCookie)Wwise::Exports::func_Wwise_Sound_CancelBankCallbackCookie;
		return func(in_pCookie);
	}

	void CancelEventCallback(AkPlayingID in_playingID) {
		tCancelEventCallback func = (tCancelEventCallback)Wwise::Exports::func_Wwise_Sound_CancelEventCallback;
		return func(in_playingID);
	}

	void CancelEventCallbackCookie(void* in_pCookie) {
		tCancelEventCallbackCookie func = (tCancelEventCallbackCookie)Wwise::Exports::func_Wwise_Sound_CancelEventCallbackCookie;
		return func(in_pCookie);
	}

	AKRESULT ClearBanks() {
		tClearBanks func = (tClearBanks)Wwise::Exports::func_Wwise_Sound_ClearBanks;
		return func();
	}

	AKRESULT ClearPreparedEvents() {
		tClearPreparedEvents func = (tClearPreparedEvents)Wwise::Exports::func_Wwise_Sound_ClearPreparedEvents;
		return func();
	}

	namespace DynamicDialogue {
		AkUniqueID ResolveDialogueEvent(AkUniqueID in_eventID, AkArgumentValueID* in_aArgumentValues, AkUInt32 in_uNumArguments, AkPlayingID in_idSequence) {
			tDynamicDialogue_ResolveDialogueEvent_UniqueID func = (tDynamicDialogue_ResolveDialogueEvent_UniqueID)Wwise::Exports::func_Wwise_Sound_DynamicDialogue_ResolveDialogueEvent_UniqueID;
			return func(in_eventID, in_aArgumentValues, in_uNumArguments, in_idSequence);
		}

		AkUniqueID ResolveDialogueEvent(const char* in_pszEventName, const char** in_aArgumentValueNames, AkUInt32 in_uNumArguments, AkPlayingID in_idSequence) {
			tDynamicDialogue_ResolveDialogueEvent_Char func = (tDynamicDialogue_ResolveDialogueEvent_Char)Wwise::Exports::func_Wwise_Sound_DynamicDialogue_ResolveDialogueEvent_Char;
			return func(in_pszEventName, in_aArgumentValueNames, in_uNumArguments, in_idSequence);
		}
	}

	namespace DynamicSequence {
		AKRESULT Break(AkPlayingID in_playingID) {
			tDynamicSequence_Break func = (tDynamicSequence_Break)Wwise::Exports::func_Wwise_Sound_DynamicSequence_Break;
			return func(in_playingID);
		}

		AKRESULT Close(AkPlayingID in_playingID) {
			tDynamicSequence_Close func = (tDynamicSequence_Close)Wwise::Exports::func_Wwise_Sound_DynamicSequence_Close;
			return func(in_playingID);
		}

		Playlist* LockPlaylist(AkPlayingID in_playingID) {
			tDynamicSequence_LockPlaylist func = (tDynamicSequence_LockPlaylist)Wwise::Exports::func_Wwise_Sound_DynamicSequence_LockPlaylist;
			return func(in_playingID);
		}

		AkPlayingID Open(AkGameObjectID in_gameObjectID, AkUInt32 in_uFlags, AkCallbackFunc in_pfnCallback, void* in_pCookie, DynamicSequenceType in_eDynamicSequenceType) {
			tDynamicSequence_Open func = (tDynamicSequence_Open)Wwise::Exports::func_Wwise_Sound_DynamicSequence_Open;
			return func(in_gameObjectID, in_uFlags, in_pfnCallback, in_pCookie, in_eDynamicSequenceType);
		}

		AKRESULT Pause(AkPlayingID in_playingID, AkTimeMs in_uTransitionDuration, AkCurveInterpolation in_eFadeCurve) {
			tDynamicSequence_Pause func = (tDynamicSequence_Pause)Wwise::Exports::func_Wwise_Sound_DynamicSequence_Pause;
			return func(in_playingID, in_uTransitionDuration, in_eFadeCurve);
		}

		AKRESULT Play(AkPlayingID in_playingID, AkTimeMs in_uTransitionDuration, AkCurveInterpolation in_eFadeCurve) {
			tDynamicSequence_Play func = (tDynamicSequence_Play)Wwise::Exports::func_Wwise_Sound_DynamicSequence_Play;
			return func(in_playingID, in_uTransitionDuration, in_eFadeCurve);
		}

		AKRESULT Resume(AkPlayingID in_playingID, AkTimeMs in_uTransitionDuration, AkCurveInterpolation in_eFadeCurve) {
			tDynamicSequence_Resume func = (tDynamicSequence_Resume)Wwise::Exports::func_Wwise_Sound_DynamicSequence_Resume;
			return func(in_playingID, in_uTransitionDuration, in_eFadeCurve);
		}

		AKRESULT Stop(AkPlayingID in_playingID, AkTimeMs in_uTransitionDuration, AkCurveInterpolation in_eFadeCurve) {
			tDynamicSequence_Stop func = (tDynamicSequence_Stop)Wwise::Exports::func_Wwise_Sound_DynamicSequence_Stop;
			return func(in_playingID, in_uTransitionDuration, in_eFadeCurve);
		}

		AKRESULT UnlockPlaylist(AkPlayingID in_playingID) {
			tDynamicSequence_UnlockPlaylist func = (tDynamicSequence_UnlockPlaylist)Wwise::Exports::func_Wwise_Sound_DynamicSequence_UnlockPlaylist;
			return func(in_playingID);
		}
	}

	AKRESULT ExecuteActionOnEvent(AkUniqueID in_eventID, AkActionOnEventType in_ActionType, AkGameObjectID in_gameObjectID, AkTimeMs in_uTransitionDuration, AkCurveInterpolation in_eFadeCurve, AkPlayingID in_PlayingID) {
		tExecuteActionOnEvent_UniqueID func = (tExecuteActionOnEvent_UniqueID)Wwise::Exports::func_Wwise_Sound_ExecuteActionOnEvent_UniqueID;
		return func(in_eventID, in_ActionType, in_gameObjectID, in_uTransitionDuration, in_eFadeCurve, in_PlayingID);
	}

	AKRESULT ExecuteActionOnEvent(const char* in_pszEventName, AkActionOnEventType in_ActionType, AkGameObjectID in_gameObjectID, AkTimeMs in_uTransitionDuration, AkCurveInterpolation in_eFadeCurve, AkPlayingID in_PlayingID) {
		tExecuteActionOnEvent_Char func = (tExecuteActionOnEvent_Char)Wwise::Exports::func_Wwise_Sound_ExecuteActionOnEvent_Char;
		return func(in_pszEventName, in_ActionType, in_gameObjectID, in_uTransitionDuration, in_eFadeCurve, in_PlayingID);
	}

	void GetDefaultInitSettings(AkInitSettings* out_settings) {
		tGetDefaultInitSettings func = (tGetDefaultInitSettings)Wwise::Exports::func_Wwise_Sound_GetDefaultInitSettings;
		return func(out_settings);
	}

	void GetDefaultPlatformInitSettings(AkPlatformInitSettings* out_platformSettings) {
		tGetDefaultPlatformInitSettings func = (tGetDefaultPlatformInitSettings)Wwise::Exports::func_Wwise_Sound_GetDefaultPlatformInitSettings;
		return func(out_platformSettings);
	}

	AkUInt32 GetIDFromString(const char* in_pszString) {
		tGetIDFromString func = (tGetIDFromString)Wwise::Exports::func_Wwise_Sound_GetIDFromString;
		return func(in_pszString);
	}

	AKRESULT GetPanningRule(AkPanningRule* out_ePanningRule, AkAudioOutputType in_eSinkType, AkUInt32 in_iOutputID) {
		tGetPanningRule func = (tGetPanningRule)Wwise::Exports::func_Wwise_Sound_GetPanningRule;
		return func(out_ePanningRule, in_eSinkType, in_iOutputID);
	}

	AKRESULT GetSourcePlayPosition(AkPlayingID in_PlayingID, AkTimeMs* out_puPosition, bool in_bExtrapolate) {
		tGetSourcePlayPosition func = (tGetSourcePlayPosition)Wwise::Exports::func_Wwise_Sound_GetSourcePlayPosition;
		return func(in_PlayingID, out_puPosition, in_bExtrapolate);
	}

	AkUInt32 GetSpeakerConfiguration() {
		tGetSpeakerConfiguration func = (tGetSpeakerConfiguration)Wwise::Exports::func_Wwise_Sound_GetSpeakerConfiguration;
		return func();
	}

	AKRESULT Init(AkInitSettings* in_pSettings, AkPlatformInitSettings* in_pPlatformSettings) {
		tInit func = (tInit)Wwise::Exports::func_Wwise_Sound_Init;
		return func(in_pSettings, in_pPlatformSettings);
	}

	bool IsInitialized() {
		tIsInitialized func = (tIsInitialized)Wwise::Exports::func_Wwise_Sound_IsInitialized;
		return func();
	}

	AKRESULT LoadBank(AkBankID in_bankID, AkMemPoolId in_memPoolId) {
		tLoadBank_BankID_MemPoolID func = (tLoadBank_BankID_MemPoolID)Wwise::Exports::func_Wwise_Sound_LoadBank_BankID_MemPoolID;
		return func(in_bankID, in_memPoolId);
	}

	AKRESULT LoadBank(const void* in_plnMemoryBankPtr, AkUInt32 in_ulnMemoryBankSize, AkBankID* out_bankID) {
		tLoadBank_Void_UInt32_BankID func = (tLoadBank_Void_UInt32_BankID)Wwise::Exports::func_Wwise_Sound_LoadBank_Void_UInt32_BankID;
		return func(in_plnMemoryBankPtr, in_ulnMemoryBankSize, out_bankID);
	}

	AKRESULT LoadBank(AkBankID in_BankID, AkBankCallbackFunc in_pfnBankCallback, void* in_pCookie, AkMemPoolId in_memPoolId) {
		tLoadBank_BankID_Callback func = (tLoadBank_BankID_Callback)Wwise::Exports::func_Wwise_Sound_LoadBank_BankID_Callback;
		return func(in_BankID, in_pfnBankCallback, in_pCookie, in_memPoolId);
	}

	AKRESULT LoadBank(const void* in_plnMemoryBankPtr, AkUInt32 in_ulnMemoryBankSize, AkBankCallbackFunc in_pfnBankCallback, void* in_pCookie, AkBankID* out_bankID) {
		tLoadBank_Void_UInt32_Callback func = (tLoadBank_Void_UInt32_Callback)Wwise::Exports::func_Wwise_Sound_LoadBank_Void_UInt32_Callback;
		return func(in_plnMemoryBankPtr, in_ulnMemoryBankSize, in_pfnBankCallback, in_pCookie, out_bankID);
	}

	AKRESULT LoadBank(const char* in_pszString, AkMemPoolId in_memPoolId, AkBankID* out_bankID) {
		tLoadBank_Char_MemPoolID func = (tLoadBank_Char_MemPoolID)Wwise::Exports::func_Wwise_Sound_LoadBank_Char_MemPoolID;
		return func(in_pszString, in_memPoolId, out_bankID);
	}

	AKRESULT LoadBank(const char* in_pszString, AkBankCallbackFunc in_pfnBankCallback, void* in_pCookie, AkMemPoolId in_memPoolId, AkBankID* out_bankId) {
		tLoadBank_Char_Callback func = (tLoadBank_Char_Callback)Wwise::Exports::func_Wwise_Sound_LoadBank_Char_Callback;
		return func(in_pszString, in_pfnBankCallback, in_pCookie, in_memPoolId, out_bankId);
	}

	AKRESULT LoadBankUnique(const char* in_pszString, AkBankCallbackFunc in_pfnBankCallback, void* in_pCookie, AkMemPoolId in_memPoolId, AkBankID* out_bankId) {
		tLoadBankUnique func = (tLoadBankUnique)Wwise::Exports::func_Wwise_Sound_LoadBankUnique;
		return func(in_pszString, in_pfnBankCallback, in_pCookie, in_memPoolId, out_bankId);
	}

	AkUInt32 PlaySourcePlugin(AkUInt32 param_1, AkUInt32 param_2, AkUInt32 param_3) {
		tPlaySourcePlugin func = (tPlaySourcePlugin)Wwise::Exports::func_Wwise_Sound_PlaySourcePlugin;
		return func(param_1, param_2, param_3);
	}

	AkPlayingID PostEvent(const char* in_pszEventName, AkGameObjectID in_gameObjectID, AkUInt32 in_uFlags, AkCallbackFunc in_pfnCallback, void* in_pCookie, AkUInt32 in_cExternals, AkExternalSourceInfo* in_pExternalSources, AkPlayingID in_PlayingID) {
		tPostEvent_Char func = (tPostEvent_Char)Wwise::Exports::func_Wwise_Sound_PostEvent_Char;
		return func(in_pszEventName, in_gameObjectID, in_uFlags, in_pfnCallback, in_pCookie, in_cExternals, in_pExternalSources, in_PlayingID);
	}

	AkPlayingID PostEvent(AkUniqueID in_eventID, AkGameObjectID in_gameObjectID, AkUInt32 in_uFlags, AkCallbackFunc in_pfnCallback, void* in_pCookie, AkUInt32 in_cExternals, AkExternalSourceInfo* in_pExternalSources, AkPlayingID in_PlayingID) {
		tPostEvent_UniqueID func = (tPostEvent_UniqueID)Wwise::Exports::func_Wwise_Sound_PostEvent_UniqueID;
		return func(in_eventID, in_gameObjectID, in_uFlags, in_pfnCallback, in_pCookie, in_cExternals, in_pExternalSources, in_PlayingID);
	}

	AKRESULT PostTrigger(AkTriggerID in_triggerID, AkGameObjectID in_gameObjectID) {
		tPostTrigger_TriggerID func = (tPostTrigger_TriggerID)Wwise::Exports::func_Wwise_Sound_PostTrigger_TriggerID;
		return func(in_triggerID, in_gameObjectID);
	}

	AKRESULT PostTrigger(char* in_szTriggerName, AkGameObjectID in_gameObjectID) {
		tPostTrigger_Char func = (tPostTrigger_Char)Wwise::Exports::func_Wwise_Sound_PostTrigger_Char;
		return func(in_szTriggerName, in_gameObjectID);
	}

	AKRESULT PrepareBank(PreparationType in_PreparationType, AkBankID in_bankID, AkBankCallbackFunc in_pfnBankCallback, void* in_pCookie, AkBankContent in_uFlags) {
		tPrepareBank_BankID_Callback func = (tPrepareBank_BankID_Callback)Wwise::Exports::func_Wwise_Sound_PrepareBank_BankID_Callback;
		return func(in_PreparationType, in_bankID, in_pfnBankCallback, in_pCookie, in_uFlags);
	}

	AKRESULT PrepareBank(PreparationType in_PreparationType, AkBankID in_bankID, AkBankContent in_uFlags) {
		tPrepareBank_BankID_BankContent func = (tPrepareBank_BankID_BankContent)Wwise::Exports::func_Wwise_Sound_PrepareBank_BankID_BankContent;
		return func(in_PreparationType, in_bankID, in_uFlags);
	}

	AKRESULT PrepareBank(PreparationType in_PreparationType, const char* in_pszString, AkBankCallbackFunc in_pfnBankCallback, void* in_pCookie, AkBankContent in_uFlags) {
		tPrepareBank_Char_Callback func = (tPrepareBank_Char_Callback)Wwise::Exports::func_Wwise_Sound_PrepareBank_Char_Callback;
		return func(in_PreparationType, in_pszString, in_pfnBankCallback, in_pCookie, in_uFlags);
	}

	AKRESULT PrepareBank(PreparationType in_PreparationType, const char* in_pszString, AkBankContent in_uFlags) {
		tPrepareBank_Char_BankContent func = (tPrepareBank_Char_BankContent)Wwise::Exports::func_Wwise_Sound_PrepareBank_Char_BankContent;
		return func(in_PreparationType, in_pszString, in_uFlags);
	}

	AKRESULT PrepareEvent(PreparationType in_PreparationType, AkUniqueID* in_pEventID, AkUInt32 in_uNumEvent) {
		tPrepareEvent_EventID_UInt32 func = (tPrepareEvent_EventID_UInt32)Wwise::Exports::func_Wwise_Sound_PrepareEvent_EventID_UInt32;
		return func(in_PreparationType, in_pEventID, in_uNumEvent);
	}

	AKRESULT PrepareEvent(PreparationType in_PreparationType, AkUniqueID* in_pEventID, AkUInt32 in_uNumEvent, AkBankCallbackFunc in_pfnBankCallback, void* in_pCookie) {
		tPrepareEvent_EventID_UInt32_Callback_Void func = (tPrepareEvent_EventID_UInt32_Callback_Void)Wwise::Exports::func_Wwise_Sound_PrepareEvent_EventID_UInt32_Callback_Void;
		return func(in_PreparationType, in_pEventID, in_uNumEvent, in_pfnBankCallback, in_pCookie);
	}

	AKRESULT PrepareEvent(PreparationType in_PreparationType, const char** in_ppszString, AkUInt32 in_uNumEvent) {
		tPrepareEvent_Char_UInt32 func = (tPrepareEvent_Char_UInt32)Wwise::Exports::func_Wwise_Sound_PrepareEvent_Char_UInt32;
		return func(in_PreparationType, in_ppszString, in_uNumEvent);
	}

	AKRESULT PrepareEvent(PreparationType in_PreparationType, const char** in_ppszString, AkUInt32 in_uNumEvent, AkBankCallbackFunc in_pfnBankCallback, void* in_pCookie) {
		tPrepareEvent_Char_UInt32_Callback_Void func = (tPrepareEvent_Char_UInt32_Callback_Void)Wwise::Exports::func_Wwise_Sound_PrepareEvent_Char_UInt32_Callback_Void;
		return func(in_PreparationType, in_ppszString, in_uNumEvent, in_pfnBankCallback, in_pCookie);
	}

	AKRESULT PrepareGameSyncs(PreparationType in_PreparationType, AkGroupType in_eGameSyncType, AkUInt32 in_GroupID, AkUInt32* in_paGameSyncID, AkUInt32 in_uNumGameSyncs, AkBankCallbackFunc in_pfnBankCallback, void* in_pCookie) {
		tPrepareGameSyncs_UInt32_UInt32_UInt32_Callback_Void func = (tPrepareGameSyncs_UInt32_UInt32_UInt32_Callback_Void)Wwise::Exports::func_Wwise_Sound_PrepareGameSyncs_UInt32_UInt32_UInt32_Callback_Void;
		return func(in_PreparationType, in_eGameSyncType, in_GroupID, in_paGameSyncID, in_uNumGameSyncs, in_pfnBankCallback, in_pCookie);
	}

	AKRESULT PrepareGameSyncs(PreparationType in_PreparationType, AkGroupType in_eGameSyncType, AkUInt32 in_GroupID, AkUInt32* in_paGameSyncID, AkUInt32 in_uNumGameSyncs) {
		tPrepareGameSyncs_UInt32_UInt32_UInt32 func = (tPrepareGameSyncs_UInt32_UInt32_UInt32)Wwise::Exports::func_Wwise_Sound_PrepareGameSyncs_UInt32_UInt32_UInt32;
		return func(in_PreparationType, in_eGameSyncType, in_GroupID, in_paGameSyncID, in_uNumGameSyncs);
	}

	AKRESULT PrepareGameSyncs(PreparationType in_PreparationType, AkGroupType in_eGameSyncType, const char* in_pszGroupName, const char** in_ppszGameSyncName, AkUInt32 in_uNumGameSyncs, AkBankCallbackFunc in_pfnBankCallback, void* in_pCookie) {
		tPrepareGameSyncs_Char_Char_UInt32_Callback_Void func = (tPrepareGameSyncs_Char_Char_UInt32_Callback_Void)Wwise::Exports::func_Wwise_Sound_PrepareGameSyncs_Char_Char_UInt32_Callback_Void;
		return func(in_PreparationType, in_eGameSyncType, in_pszGroupName, in_ppszGameSyncName, in_uNumGameSyncs, in_pfnBankCallback, in_pCookie);
	}

	AKRESULT PrepareGameSyncs(PreparationType in_PreparationType, AkGroupType in_eGameSyncType, const char* in_pszGroupName, const char** in_ppszGameSyncName, AkUInt32 in_uNumGameSyncs) {
		tPrepareGameSyncs_Char_Char_UInt32 func = (tPrepareGameSyncs_Char_Char_UInt32)Wwise::Exports::func_Wwise_Sound_PrepareGameSyncs_Char_Char_UInt32;
		return func(in_PreparationType, in_eGameSyncType, in_pszGroupName, in_ppszGameSyncName, in_uNumGameSyncs);
	}


	namespace Query {
		AKRESULT GetActiveGameObjects(AkGameObjectsList* io_GameObjectList) {
			tQuery_GetActiveGameObjects func = (tQuery_GetActiveGameObjects)Wwise::Exports::func_Wwise_Sound_Query_GetActiveGameObjects;
			return func(io_GameObjectList);
		}

		AKRESULT GetActiveListeners(AkGameObjectID in_GameObjectID, AkUInt32* out_ruListenerMask) {
			tQuery_GetActiveListeners func = (tQuery_GetActiveListeners)Wwise::Exports::func_Wwise_Sound_Query_GetActiveListeners;
			return func(in_GameObjectID, out_ruListenerMask);
		}

		AKRESULT GetCustomPropertyValue(AkUniqueID in_ObjectID, AkUInt32 in_uPropID, AkInt32* out_iValue) {
			tQuery_GetCustomPropertyValue_Int32 func = (tQuery_GetCustomPropertyValue_Int32)Wwise::Exports::func_Wwise_Sound_Query_GetCustomPropertyValue_Int32;
			return func(in_ObjectID, in_uPropID, out_iValue);
		}

		AKRESULT GetCustomPropertyValue(AkUniqueID in_ObjectID, AkUInt32 in_uPropID, AkReal32* out_fValue) {
			tQuery_GetCustomPropertyValue_Real32 func = (tQuery_GetCustomPropertyValue_Real32)Wwise::Exports::func_Wwise_Sound_Query_GetCustomPropertyValue_Real32;
			return func(in_ObjectID, in_uPropID, out_fValue);
		}

		AkUniqueID GetEventIDFromPlayingID(AkPlayingID in_playingID) {
			tQuery_GetEventIDFromPlayingID func = (tQuery_GetEventIDFromPlayingID)Wwise::Exports::func_Wwise_Sound_Query_GetEventIDFromPlayingID;
			return func(in_playingID);
		}

		AKRESULT GetGameObjectAuxSendValues(AkGameObjectID in_gameObjectID, AkAuxSendValue* out_paAuxSendValues, AkUInt32* io_ruNumSendValues) {
			tQuery_GetGameObjectAuxSendValues func = (tQuery_GetGameObjectAuxSendValues)Wwise::Exports::func_Wwise_Sound_Query_GetGameObjectAuxSendValues;
			return func(in_gameObjectID, out_paAuxSendValues, io_ruNumSendValues);
		}

		AKRESULT GetGameObjectDryLevelValue(AkGameObjectID in_gameObjectID, AkReal32* out_rfControlValue) {
			tQuery_GetGameObjectDryLevelValue func = (tQuery_GetGameObjectDryLevelValue)Wwise::Exports::func_Wwise_Sound_Query_GetGameObjectDryLevelValue;
			return func(in_gameObjectID, out_rfControlValue);
		}

		AkGameObjectID GetGameObjectFromPlayingID(AkPlayingID in_playingID) {
			tQuery_GetGameObjectFromPlayingID func = (tQuery_GetGameObjectFromPlayingID)Wwise::Exports::func_Wwise_Sound_Query_GetGameObjectFromPlayingID;
			return func(in_playingID);
		}

		bool GetIsGameObjectActive(AkGameObjectID in_GameObjId) {
			tQuery_GetIsGameObjectActive func = (tQuery_GetIsGameObjectActive)Wwise::Exports::func_Wwise_Sound_Query_GetIsGameObjectActive;
			return func(in_GameObjId);
		}

		AKRESULT GetListenerPosition(AkUInt32 in_uIndex, AkListenerPosition* out_rPosition) {
			tQuery_GetListenerPosition func = (tQuery_GetListenerPosition)Wwise::Exports::func_Wwise_Sound_Query_GetListenerPosition;
			return func(in_uIndex, out_rPosition);
		}

		AKRESULT GetMaxRadius(AkRadiusList* io_RadiusList) {
			tQuery_GetMaxRadius_RadiusList func = (tQuery_GetMaxRadius_RadiusList)Wwise::Exports::func_Wwise_Sound_Query_GetMaxRadius_RadiusList;
			return func(io_RadiusList);
		}

		AKRESULT GetMaxRadius(AkGameObjectID in_GameObjId) {
			tQuery_GetMaxRadius_GameObject func = (tQuery_GetMaxRadius_GameObject)Wwise::Exports::func_Wwise_Sound_Query_GetMaxRadius_GameObject;
			return func(in_GameObjId);
		}

		AKRESULT GetObjectObstructionAndOcclusion(AkGameObjectID in_ObjectID, AkUInt32 in_uListener, AkReal32* out_rfObstructionLevel, AkReal32* out_rfOcclusionLevel) {
			tQuery_GetObjectObstructionAndOcclusion func = (tQuery_GetObjectObstructionAndOcclusion)Wwise::Exports::func_Wwise_Sound_Query_GetObjectObstructionAndOcclusion;
			return func(in_ObjectID, in_uListener, out_rfObstructionLevel, out_rfOcclusionLevel);
		}

		AKRESULT GetPlayingIDsFromGameObject(AkGameObjectID in_GameObjId, AkUInt32* io_ruNumIds, AkPlayingID* out_aPlayingIDs) {
			tQuery_GetPlayingIDsFromGameObject func = (tQuery_GetPlayingIDsFromGameObject)Wwise::Exports::func_Wwise_Sound_Query_GetPlayingIDsFromGameObject;
			return func(in_GameObjId, io_ruNumIds, out_aPlayingIDs);
		}

		AKRESULT GetPosition(AkGameObjectID in_GameObjectID, AkSoundPosition* out_rPosition) {
			tQuery_GetPosition func = (tQuery_GetPosition)Wwise::Exports::func_Wwise_Sound_Query_GetPosition;
			return func(in_GameObjectID, out_rPosition);
		}

		AKRESULT GetPositioningInfo(AkUniqueID in_ObjectID, AkPositioningInfo* out_rPositioningInfo) {
			tQuery_GetPositioningInfo func = (tQuery_GetPositioningInfo)Wwise::Exports::func_Wwise_Sound_Query_GetPositioningInfo;
			return func(in_ObjectID, out_rPositioningInfo);
		}

		AKRESULT GetRTPCValue(const char* in_pszRtpcName, AkGameObjectID in_gameObjectID, AkRtpcValue* out_rValue, RTPCValue_type* io_rValueType) {
			tQuery_GetRTPCValue_Char func = (tQuery_GetRTPCValue_Char)Wwise::Exports::func_Wwise_Sound_Query_GetRTPCValue_Char;
			return func(in_pszRtpcName, in_gameObjectID, out_rValue, io_rValueType);
		}

		AKRESULT GetRTPCValue(AkRtpcID in_rtpcID, AkGameObjectID in_gameObjectID, AkRtpcValue* out_rValue, RTPCValue_type* io_rValueType) {
			tQuery_GetRTPCValue_RTPCID func = (tQuery_GetRTPCValue_RTPCID)Wwise::Exports::func_Wwise_Sound_Query_GetRTPCValue_RTPCID;
			return func(in_rtpcID, in_gameObjectID, out_rValue, io_rValueType);
		}

		AKRESULT GetState(AkStateGroupID in_stateGroup, AkStateID* out_rState) {
			tQuery_GetState_StateGroupID func = (tQuery_GetState_StateGroupID)Wwise::Exports::func_Wwise_Sound_Query_GetState_StateGroupID;
			return func(in_stateGroup, out_rState);
		}

		AKRESULT GetState(const char* in_pstrStateGroupName, AkStateID* out_rState) {
			tQuery_GetState_Char func = (tQuery_GetState_Char)Wwise::Exports::func_Wwise_Sound_Query_GetState_Char;
			return func(in_pstrStateGroupName, out_rState);
		}

		AKRESULT GetSwitch(AkSwitchGroupID in_switchGroup, AkGameObjectID in_gameObjectID, AkSwitchStateID* out_rSwitchState) {
			tQuery_GetSwitch_SwitchGroupID func = (tQuery_GetSwitch_SwitchGroupID)Wwise::Exports::func_Wwise_Sound_Query_GetSwitch_SwitchGroupID;
			return func(in_switchGroup, in_gameObjectID, out_rSwitchState);
		}

		AKRESULT GetSwitch(const char* in_pstrSwitchGroupName, AkGameObjectID in_GameObj, AkSwitchStateID* out_rSwitchState) {
			tQuery_GetSwitch_Char func = (tQuery_GetSwitch_Char)Wwise::Exports::func_Wwise_Sound_Query_GetSwitch_Char;
			return func(in_pstrSwitchGroupName, in_GameObj, out_rSwitchState);
		}

		AKRESULT QueryAudioObjectIDs(AkUniqueID in_eventID, AkUInt32* io_ruNumItems, AkObjectInfo* out_aObjectInfos) {
			tQuery_QueryAudioObjectIDs_UniqueID func = (tQuery_QueryAudioObjectIDs_UniqueID)Wwise::Exports::func_Wwise_Sound_Query_QueryAudioObjectIDs_UniqueID;
			return func(in_eventID, io_ruNumItems, out_aObjectInfos);
		}

		AKRESULT QueryAudioObjectIDs(const char* in_pszEventName, AkUInt32* io_ruNumItems, AkObjectInfo* out_aObjectInfos) {
			tQuery_QueryAudioObjectIDs_Char func = (tQuery_QueryAudioObjectIDs_Char)Wwise::Exports::func_Wwise_Sound_Query_QueryAudioObjectIDs_Char;
			return func(in_pszEventName, io_ruNumItems, out_aObjectInfos);
		}
	}

	AKRESULT RegisterCodec(AkUInt32 in_ulCompanyID, AkUInt32 in_ulCodecID, AkCreateFileSourceCallback in_pFileCreateFunc, AkCreateBankSourceCallback in_pBankCreateFunc) {
		tRegisterCodec func = (tRegisterCodec)Wwise::Exports::func_Wwise_Sound_RegisterCodec;
		return func(in_ulCompanyID, in_ulCodecID, in_pFileCreateFunc, in_pBankCreateFunc);
	}

	AKRESULT RegisterGameObj(AkGameObjectID in_gameObjectID) {
		tRegisterGameObj func = (tRegisterGameObj)Wwise::Exports::func_Wwise_Sound_RegisterGameObj;
		return func(in_gameObjectID);
	}

	AKRESULT RegisterGlobalCallback(AkGlobalCallbackFunc in_pCallback) {
		tRegisterGlobalCallback func = (tRegisterGlobalCallback)Wwise::Exports::func_Wwise_Sound_RegisterGlobalCallback;
		return func(in_pCallback);
	}

	AKRESULT RegisterPlugin(AkPluginType in_eType, AkUInt32 in_ulCompanyID, AkUInt32 in_ulPluginID, AkCreatePluginCallback in_pCreateFunc, AkCreateParamCallback in_pCreateParamFunc) {
		tRegisterPlugin func = (tRegisterPlugin)Wwise::Exports::func_Wwise_Sound_RegisterPlugin;
		return func(in_eType, in_ulCompanyID, in_ulPluginID, in_pCreateFunc, in_pCreateParamFunc);
	}

	AKRESULT RenderAudio() {
		tRenderAudio func = (tRenderAudio)Wwise::Exports::func_Wwise_Sound_RenderAudio;
		return func();
	}

	AKRESULT ResetRTPCValue(AkRtpcID in_rtpcID, AkGameObjectID in_gameObjectID, AkTimeMs in_uValueChangeDuration, AkCurveInterpolation in_curveInterpolation) {
		tResetRTPCValue_RTPCID func = (tResetRTPCValue_RTPCID)Wwise::Exports::func_Wwise_Sound_ResetRTPCValue_RTPCID;
		return func(in_rtpcID, in_gameObjectID, in_uValueChangeDuration, in_curveInterpolation);
	}

	AKRESULT ResetRTPCValue(const char* in_pszRtpcName, AkGameObjectID in_gameObjectID, AkTimeMs in_uValueChangeDuration, AkCurveInterpolation in_curveInterpolation) {
		tResetRTPCValue_Char func = (tResetRTPCValue_Char)Wwise::Exports::func_Wwise_Sound_ResetRTPCValue_Char;
		return func(in_pszRtpcName, in_gameObjectID, in_uValueChangeDuration, in_curveInterpolation);
	}

	AKRESULT SeekOnEvent(AkUniqueID in_eventID, AkGameObjectID in_gameObjectID, AkTimeMs in_iPosition, bool in_bSeekToNearestMarker) {
		tSeekOnEvent_UniqueID_Int32 func = (tSeekOnEvent_UniqueID_Int32)Wwise::Exports::func_Wwise_Sound_SeekOnEvent_UniqueID_Int32;
		return func(in_eventID, in_gameObjectID, in_iPosition, in_bSeekToNearestMarker);
	}

	AKRESULT SeekOnEvent(const char* in_pszEventName, AkGameObjectID in_gameObjectID, AkTimeMs in_iPosition, bool in_bSeekToNearestMarker) {
		tSeekOnEvent_Char_Int32 func = (tSeekOnEvent_Char_Int32)Wwise::Exports::func_Wwise_Sound_SeekOnEvent_Char_Int32;
		return func(in_pszEventName, in_gameObjectID, in_iPosition, in_bSeekToNearestMarker);
	}

	AKRESULT SeekOnEvent(AkUniqueID in_eventID, AkGameObjectID in_gameObjectID, AkReal32 in_iPosition, bool in_bSeekToNearestMarker) {
		tSeekOnEvent_UniqueID_Float func = (tSeekOnEvent_UniqueID_Float)Wwise::Exports::func_Wwise_Sound_SeekOnEvent_UniqueID_Float;
		return func(in_eventID, in_gameObjectID, in_iPosition, in_bSeekToNearestMarker);
	}

	AKRESULT SeekOnEvent(const char* in_pszEventName, AkGameObjectID in_gameObjectID, AkReal32 in_iPosition, bool in_bSeekToNearestMarker) {
		tSeekOnEvent_Char_Float func = (tSeekOnEvent_Char_Float)Wwise::Exports::func_Wwise_Sound_SeekOnEvent_Char_Float;
		return func(in_pszEventName, in_gameObjectID, in_iPosition, in_bSeekToNearestMarker);
	}

	AKRESULT SetActiveListeners(AkGameObjectID in_gameObjectID, AkUInt32 in_uiListenerMask) {
		tSetActiveListeners func = (tSetActiveListeners)Wwise::Exports::func_Wwise_Sound_SetActiveListeners;
		return func(in_gameObjectID, in_uiListenerMask);
	}

	AKRESULT SetActorMixerEffect(AkUniqueID in_audioNodeID, AkUInt32 in_uFXIndex, AkUniqueID in_shareSetID) {
		tSetActorMixerEffect func = (tSetActorMixerEffect)Wwise::Exports::func_Wwise_Sound_SetActorMixerEffect;
		return func(in_audioNodeID, in_uFXIndex, in_shareSetID);
	}

	AKRESULT SetAttenuationScalingFactor(AkGameObjectID in_GameObjectID, AkReal32 in_fAttenuationScalingFactor) {
		tSetAttenuationScalingFactor func = (tSetAttenuationScalingFactor)Wwise::Exports::func_Wwise_Sound_SetAttenuationScalingFactor;
		return func(in_GameObjectID, in_fAttenuationScalingFactor);
	}

	AKRESULT SetBankLoadIOSettings(AkReal32 in_fThroughput, AkPriority in_priority) {
		tSetBankLoadIOSettings func = (tSetBankLoadIOSettings)Wwise::Exports::func_Wwise_Sound_SetBankLoadIOSettings;
		return func(in_fThroughput, in_priority);
	}

	AKRESULT SetBusEffect(AkUniqueID in_audioNodeID, AkUInt32 in_uFXIndex, AkUniqueID in_shareSetID) {
		tSetBusEffect_UniqueID func = (tSetBusEffect_UniqueID)Wwise::Exports::func_Wwise_Sound_SetBusEffect_UniqueID;
		return func(in_audioNodeID, in_uFXIndex, in_shareSetID);
	}

	AKRESULT SetBusEffect(const char* in_pszBusName, AkUInt32 in_uFXIndex, AkUniqueID in_shareSetID) {
		tSetBusEffect_Char func = (tSetBusEffect_Char)Wwise::Exports::func_Wwise_Sound_SetBusEffect_Char;
		return func(in_pszBusName, in_uFXIndex, in_shareSetID);
	}

	AKRESULT SetEffectParam(AkUInt32 param_1, AkUInt16 param_2, void* in_pCookie) {
		tSetEffectParam func = (tSetEffectParam)Wwise::Exports::func_Wwise_Sound_SetEffectParam;
		return func(param_1, param_2, in_pCookie);
	}

	AKRESULT SetGameObjectAuxSendValues(AkGameObjectID in_gameObjectID, AkAuxSendValue* in_aAuxSendValues, AkUInt32 in_uNumSendValues) {
		tSetGameObjectAuxSendValues func = (tSetGameObjectAuxSendValues)Wwise::Exports::func_Wwise_Sound_SetGameObjectAuxSendValues;
		return func(in_gameObjectID, in_aAuxSendValues, in_uNumSendValues);
	}

	AKRESULT SetGameObjectOutputBusVolume(AkGameObjectID in_gameObjectID, AkReal32 in_fControlValue) {
		tSetGameObjectOutputBusVolume func = (tSetGameObjectOutputBusVolume)Wwise::Exports::func_Wwise_Sound_SetGameObjectOutputBusVolume;
		return func(in_gameObjectID, in_fControlValue);
	}

	AKRESULT SetListenerPipeline(AkUInt32 in_uIndex, bool in_bAudio, bool in_bMotion) {
		tSetListenerPipeline func = (tSetListenerPipeline)Wwise::Exports::func_Wwise_Sound_SetListenerPipeline;
		return func(in_uIndex, in_bAudio, in_bMotion);
	}

	AKRESULT SetListenerPosition(const AkListenerPosition* in_rPosition, AkUInt32 in_uiIndex) {
		tSetListenerPosition func = (tSetListenerPosition)Wwise::Exports::func_Wwise_Sound_SetListenerPosition;
		return func(in_rPosition, in_uiIndex);
	}

	AKRESULT SetListenerScalingFactor(AkUInt32 in_uiIndex, AkReal32 in_fAttenuationScalingFactor) {
		tSetListenerScalingFactor func = (tSetListenerScalingFactor)Wwise::Exports::func_Wwise_Sound_SetListenerScalingFactor;
		return func(in_uiIndex, in_fAttenuationScalingFactor);
	}

	AKRESULT SetListenerSpatialization(AkUInt32 in_uIndex, bool in_bSpatialized, AkChannelConfig in_channelConfig, VectorPtr in_pVolumeOffsets) {
		tSetListenerSpatialization func = (tSetListenerSpatialization)Wwise::Exports::func_Wwise_Sound_SetListenerSpatialization;
		return func(in_uIndex, in_bSpatialized, in_channelConfig, in_pVolumeOffsets);
	}

	AKRESULT SetMaxNumVoicesLimit(AkUInt16 in_maxNumberVoices) {
		tSetMaxNumVoicesLimit func = (tSetMaxNumVoicesLimit)Wwise::Exports::func_Wwise_Sound_SetMaxNumVoicesLimit;
		return func(in_maxNumberVoices);
	}

	AKRESULT SetMultiplePositions(AkGameObjectID in_GameObjectID, const AkSoundPosition* in_pPositions, AkUInt16 in_NumPositions, MultiPositionType in_eMultiPositionType) {
		tSetMultiplePositions func = (tSetMultiplePositions)Wwise::Exports::func_Wwise_Sound_SetMultiplePositions;
		return func(in_GameObjectID, in_pPositions, in_NumPositions, in_eMultiPositionType);
	}

	AKRESULT SetObjectObstructionAndOcclusion(AkGameObjectID in_ObjectID, AkUInt32 in_uListener, AkReal32 in_fObstructionLevel, AkReal32 in_fOcclusionLevel) {
		tSetObjectObstructionAndOcclusion func = (tSetObjectObstructionAndOcclusion)Wwise::Exports::func_Wwise_Sound_SetObjectObstructionAndOcclusion;
		return func(in_ObjectID, in_uListener, in_fObstructionLevel, in_fOcclusionLevel);
	}

	AKRESULT SetPanningRule(AkPanningRule in_ePanningRule, AkAudioOutputType in_eSinkType, AkUInt32 in_iOutputID) {
		tSetPanningRule func = (tSetPanningRule)Wwise::Exports::func_Wwise_Sound_SetPanningRule;
		return func(in_ePanningRule, in_eSinkType, in_iOutputID);
	}

	AKRESULT SetPosition(AkGameObjectID in_GameObjectID, const AkSoundPosition* in_Position) {
		tSetPosition func = (tSetPosition)Wwise::Exports::func_Wwise_Sound_SetPosition;
		return func(in_GameObjectID, in_Position);
	}

	AKRESULT SetPositionInternal(AkGameObjectID in_GameObjectID, AkSoundPosition* in_soundPosition) {
		tSetPositionInternal func = (tSetPositionInternal)Wwise::Exports::func_Wwise_Sound_SetPositionInternal;
		return func(in_GameObjectID, in_soundPosition);
	}

	AKRESULT SetRTPCValue(AkRtpcID in_rtpcID, AkRtpcValue in_value, AkGameObjectID in_gameObjectID, AkTimeMs in_uValueChangeDuration, AkCurveInterpolation in_eFadeCruve, bool in_bBypassInternalValueInterpolation) {
		tSetRTPCValue_RTPCID func = (tSetRTPCValue_RTPCID)Wwise::Exports::func_Wwise_Sound_SetRTPCValue_RTPCID;
		return func(in_rtpcID, in_value, in_gameObjectID, in_uValueChangeDuration, in_eFadeCruve, in_bBypassInternalValueInterpolation);
	}

	AKRESULT SetRTPCValue(const char* in_pszRtpcName, AkRtpcValue in_value, AkGameObjectID in_gameObjectID, AkTimeMs in_uValueChangeDuration, AkCurveInterpolation in_eFadeCurve) {
		tSetRTPCValue_Char func = (tSetRTPCValue_Char)Wwise::Exports::func_Wwise_Sound_SetRTPCValue_Char;
		return func(in_pszRtpcName, in_value, in_gameObjectID, in_uValueChangeDuration, in_eFadeCurve);
	}

	AKRESULT SetState(AkStateGroupID in_stateGroup, AkStateID in_state) {
		tSetState_StateGroupID func = (tSetState_StateGroupID)Wwise::Exports::func_Wwise_Sound_SetState_StateGroupID;
		return func(in_stateGroup, in_state);
	}

	AKRESULT SetState(const char* in_pszStateGroup, const char* in_pszState) {
		tSetState_Char func = (tSetState_Char)Wwise::Exports::func_Wwise_Sound_SetState_Char;
		return func(in_pszStateGroup, in_pszState);
	}

	AKRESULT SetSwitch(AkSwitchGroupID in_switchGroup, AkSwitchStateID in_switchState, AkGameObjectID in_gameObjectID) {
		tSetSwitch_SwitchGroupID func = (tSetSwitch_SwitchGroupID)Wwise::Exports::func_Wwise_Sound_SetSwitch_SwitchGroupID;
		return func(in_switchGroup, in_switchState, in_gameObjectID);
	}

	AKRESULT SetSwitch(const char* in_pszSwitchGroup, const char* in_pszSwitchState, AkGameObjectID in_gameObjectID) {
		tSetSwitch_Char func = (tSetSwitch_Char)Wwise::Exports::func_Wwise_Sound_SetSwitch_Char;
		return func(in_pszSwitchGroup, in_pszSwitchState, in_gameObjectID);
	}

	AKRESULT SetVolumeThreshold(AkReal32 in_fVolumeThresholdDB) {
		tSetVolumeThreshold func = (tSetVolumeThreshold)Wwise::Exports::func_Wwise_Sound_SetVolumeThreshold;
		return func(in_fVolumeThresholdDB);
	}

	AKRESULT StartOutputCapture(const AkOSChar* in_CaptureFileName) {
		tStartOutputCapture func = (tStartOutputCapture)Wwise::Exports::func_Wwise_Sound_StartOutputCapture;
		return func(in_CaptureFileName);
	}

	void StopAll(AkGameObjectID in_gameObjectID) {
		tStopAll func = (tStopAll)Wwise::Exports::func_Wwise_Sound_StopAll;
		return func(in_gameObjectID);
	}

	AKRESULT StopOutputCapture() {
		tStopOutputCapture func = (tStopOutputCapture)Wwise::Exports::func_Wwise_Sound_StopOutputCapture;
		return func();
	}

	AKRESULT StopPlayingID(AkPlayingID in_playingID, AkTimeMs in_uTransitionDuration, AkCurveInterpolation in_eFadeCurve) {
		tStopPlayingID func = (tStopPlayingID)Wwise::Exports::func_Wwise_Sound_StopPlayingID;
		return func(in_playingID, in_uTransitionDuration, in_eFadeCurve);
	}

	AKRESULT StopSourcePlugin(AkUInt32 param_1, AkUInt32 param_2, AkUInt32 param_3) {
		tStopSourcePlugin func = (tStopSourcePlugin)Wwise::Exports::func_Wwise_Sound_StopSourcePlugin;
		return func(param_1, param_2, param_3);
	}

	AKRESULT Term() {
		tTerm func = (tTerm)Wwise::Exports::func_Wwise_Sound_Term;
		return func();
	}

	AKRESULT UnloadBank(AkBankID in_bankID, const void* in_pInMemoryBankPtr, AkMemPoolId* out_pMemPoolId) {
		tUnloadBank_BankID_MemPoolID func = (tUnloadBank_BankID_MemPoolID)Wwise::Exports::func_Wwise_Sound_UnloadBank_BankID_MemPoolID;
		return func(in_bankID, in_pInMemoryBankPtr, out_pMemPoolId);
	}

	AKRESULT UnloadBank(AkBankID in_bankID, const void* in_pInMemoryBankPtr, AkBankCallbackFunc in_pfnBankCallback, void* in_pCookie) {
		tUnloadBank_BankID_Callback func = (tUnloadBank_BankID_Callback)Wwise::Exports::func_Wwise_Sound_UnloadBank_BankID_Callback;
		return func(in_bankID, in_pInMemoryBankPtr, in_pfnBankCallback, in_pCookie);
	}

	AKRESULT UnloadBank(const char* in_pszString, const void* in_pInMemoryBankPtr, AkMemPoolId* out_pMemPoolId) {
		tUnloadBank_Char_MemPoolID func = (tUnloadBank_Char_MemPoolID)Wwise::Exports::func_Wwise_Sound_UnloadBank_Char_MemPoolID;
		return func(in_pszString, in_pInMemoryBankPtr, out_pMemPoolId);
	}

	AKRESULT UnloadBank(const char* in_pszString, const void* in_pInMemoryBankPtr, AkBankCallbackFunc in_pfnBankCallback, void* in_pCookie) {
		tUnloadBank_Char_Callback func = (tUnloadBank_Char_Callback)Wwise::Exports::func_Wwise_Sound_UnloadBank_Char_Callback;
		return func(in_pszString, in_pInMemoryBankPtr, in_pfnBankCallback, in_pCookie);
	}

	AKRESULT UnloadBankUnique(const char* in_pszString, AkBankCallbackFunc in_pfnBankCallback, void* in_pCookie) {
		tUnloadBankUnique func = (tUnloadBankUnique)Wwise::Exports::func_Wwise_Sound_UnloadBankUnique;
		return func(in_pszString, in_pfnBankCallback, in_pCookie);
	}

	AKRESULT UnregisterAllGameObj() {
		tUnregisterAllGameObj func = (tUnregisterAllGameObj)Wwise::Exports::func_Wwise_Sound_UnregisterAllGameObj;
		return func();
	}

	AKRESULT UnregisterGameObj(AkGameObjectID in_gameObjectID) {
		tUnregisterGameObj func = (tUnregisterGameObj)Wwise::Exports::func_Wwise_Sound_UnregisterGameObj;
		return func(in_gameObjectID);
	}

	AKRESULT UnregisterGlobalCallback(AkGlobalCallbackFunc in_pCallback) {
		tUnregisterGlobalCallback func = (tUnregisterGlobalCallback)Wwise::Exports::func_Wwise_Sound_UnregisterGlobalCallback;
		return func(in_pCallback);
	}
}

