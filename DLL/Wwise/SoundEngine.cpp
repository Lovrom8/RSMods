#include "SoundEngine.hpp"

// Most accurate documentation is available here: https://www.audiokinetic.com/library/2015.1.9_5624/?source=SDK&id=namespace_a_k_1_1_sound_engine.html

namespace Wwise::SoundEngine {

	/// <summary>
	/// Cancel all event callbacks associated with a specific callback cookie specified while loading Banks of preparing events.
	/// </summary>
	/// <param name="in_pCookie"> - Callback cookie to be cancelled</param>
	void CancelBankCallbackCookie(void* in_pCookie) {
		tCancelBankCallbackCookie func = (tCancelBankCallbackCookie)Wwise::Exports::func_Wwise_Sound_CancelBankCallbackCookie;
		return func(in_pCookie);
	}

	/// <summary>
	/// Cancel all event callbacks for a specific playing ID.
	/// </summary>
	/// <param name="in_playingID"> - Playing ID of the event that must not use callbacks</param>
	void CancelEventCallback(AkPlayingID in_playingID) {
		tCancelEventCallback func = (tCancelEventCallback)Wwise::Exports::func_Wwise_Sound_CancelEventCallback;
		return func(in_playingID);
	}

	/// <summary>
	/// Cancel all event callbacks associated with a specific callback cookie.
	/// </summary>
	/// <param name="in_pCookie"> - Callback cookie to be cancelled</param>
	void CancelEventCallbackCookie(void* in_pCookie) {
		tCancelEventCallbackCookie func = (tCancelEventCallbackCookie)Wwise::Exports::func_Wwise_Sound_CancelEventCallbackCookie;
		return func(in_pCookie);
	}

	/// <summary>
	/// Clones an effect from one bus to another
	/// </summary>
	/// <param name="toBus"> - Bus to send the effect to</param>
	/// <param name="indexFX"> - Effect Index for the bus to send to</param>
	/// <param name="fromBus"> - Bus to get the effect from</param>
	/// <returns></returns>
	AKRESULT CloneBusEffect(AkUniqueID toBus, AkUInt32 indexFX, AkUniqueID fromBus)
	{
		tCloneBusEffect func = (tCloneBusEffect)Wwise::Exports::func_Wwise_Sound_CloneBusEffect;
		return func(toBus, indexFX, fromBus);
	}

	/// <summary>
	/// Unload all currently loaded banks. It also internally calls ClearPreparedEvents() since at least one bank must have been loaded to allow preparing events.
	/// </summary>
	/// <returns>AK_Success if successful.AK_Fail if the sound engine was not correctly initialized or if there is not enough memory to handle the command.</returns>
	AKRESULT ClearBanks() {
		tClearBanks func = (tClearBanks)Wwise::Exports::func_Wwise_Sound_ClearBanks;
		return func();
	}

	/// <summary>
	/// Clear all previously prepared events.
	/// </summary>
	/// <returns>AK_Success if successful.AK_Fail if the sound engine was not correctly initialized or if there is not enough memory to handle the command.</returns>
	AKRESULT ClearPreparedEvents() {
		tClearPreparedEvents func = (tClearPreparedEvents)Wwise::Exports::func_Wwise_Sound_ClearPreparedEvents;
		return func();
	}

	// Most accurate documentation is available here: https://www.audiokinetic.com/library/2015.1.9_5624/?source=SDK&id=namespace_a_k_1_1_sound_engine_1_1_dynamic_dialogue.html
	namespace DynamicDialogue {

		/// <summary>
		/// Resolve a dialogue event into an audio node ID based on the specified argument path.
		/// </summary>
		/// <param name="in_eventID"> - Unique ID of dialogue event</param>
		/// <param name="in_aArgumentValues"> - Argument path, as array of argument value IDs. AK_FALLBACK_ARGUMENTVALUE_ID indicates a fallback argument value</param>
		/// <param name="in_uNumArguments"> - Number of argument value IDs in in_aArgumentValues</param>
		/// <param name="in_idSequence"> - Optional sequence ID in which the token will be inserted (for profiling purposes)</param>
		/// <returns>Unique ID of audio node, or AK_INVALID_UNIQUE_ID if no audio node is defined for specified argument path.</returns>
		AkUniqueID ResolveDialogueEvent(AkUniqueID in_eventID, AkArgumentValueID* in_aArgumentValues, AkUInt32 in_uNumArguments, AkPlayingID in_idSequence) {
			tDynamicDialogue_ResolveDialogueEvent_UniqueID func = (tDynamicDialogue_ResolveDialogueEvent_UniqueID)Wwise::Exports::func_Wwise_Sound_DynamicDialogue_ResolveDialogueEvent_UniqueID;
			return func(in_eventID, in_aArgumentValues, in_uNumArguments, in_idSequence);
		}

		/// <summary>
		/// Resolve a dialogue event into an audio node ID based on the specified argument path.
		/// </summary>
		/// <param name="in_pszEventName"> - Name of dialogue event</param>
		/// <param name="in_aArgumentValueNames"> - Argument path, as array of argument value names. "" indicates a fallback argument value</param>
		/// <param name="in_uNumArguments"> - Number of argument value names in in_aArgumentValueNames</param>
		/// <param name="in_idSequence"> - Optional sequence ID in which the token will be inserted (for profiling purposes)</param>
		/// <returns>Unique ID of audio node, or AK_INVALID_UNIQUE_ID if no audio node is defined for specified argument path</returns>
		AkUniqueID ResolveDialogueEvent(const char* in_pszEventName, const char** in_aArgumentValueNames, AkUInt32 in_uNumArguments, AkPlayingID in_idSequence) {
			tDynamicDialogue_ResolveDialogueEvent_Char func = (tDynamicDialogue_ResolveDialogueEvent_Char)Wwise::Exports::func_Wwise_Sound_DynamicDialogue_ResolveDialogueEvent_Char;
			return func(in_pszEventName, in_aArgumentValueNames, in_uNumArguments, in_idSequence);
		}
	}

	// Most accurate documentation is available here: https://www.audiokinetic.com/library/2015.1.9_5624/?source=SDK&id=namespace_a_k_1_1_sound_engine_1_1_dynamic_sequence.html
	namespace DynamicSequence {
		
		/// <summary>
		/// Break specified Dynamic Sequence. The sequence will stop after the current item.
		/// </summary>
		/// <param name="in_playingID"> - AkPlayingID returned by DynamicSequence::Open</param>
		/// <returns></returns>
		AKRESULT Break(AkPlayingID in_playingID) {
			tDynamicSequence_Break func = (tDynamicSequence_Break)Wwise::Exports::func_Wwise_Sound_DynamicSequence_Break;
			return func(in_playingID);
		}

		/// <summary>
		/// Close specified Dynamic Sequence. The Dynamic Sequence will play until finished and then deallocate itself.
		/// </summary>
		/// <param name="in_playingID"> - AkPlayingID returned by DynamicSequence::Open</param>
		/// <returns></returns>
		AKRESULT Close(AkPlayingID in_playingID) {
			tDynamicSequence_Close func = (tDynamicSequence_Close)Wwise::Exports::func_Wwise_Sound_DynamicSequence_Close;
			return func(in_playingID);
		}

		/// <summary>
		/// Lock the Playlist for editing. Needs a corresponding UnlockPlaylist call.
		/// </summary>
		/// <param name="in_playingID"> - AkPlayingID returned by DynamicSequence::Open</param>
		/// <returns>Pointer to locked Playlist if successful, NULL otherwise</returns>
		Playlist* LockPlaylist(AkPlayingID in_playingID) {
			tDynamicSequence_LockPlaylist func = (tDynamicSequence_LockPlaylist)Wwise::Exports::func_Wwise_Sound_DynamicSequence_LockPlaylist;
			return func(in_playingID);
		}

		/// <summary>
		/// Open a new Dynamic Sequence.
		/// </summary>
		/// <param name="in_gameObjectID"> - Associated game object ID</param>
		/// <param name="in_uFlags"> - Bitmask: see AkCallbackType</param>
		/// <param name="in_pfnCallback"> - Callback function</param>
		/// <param name="in_pCookie"> - Callback cookie that will be sent to the callback function along with additional information;</param>
		/// <param name="in_eDynamicSequenceType"> See: AK::SoundEngine::DynamicSequence::DynamicSequenceType</param>
		/// <returns>Playing ID of the dynamic sequence, or AK_INVALID_PLAYING_ID in failure case</returns>
		AkPlayingID Open(AkGameObjectID in_gameObjectID, AkUInt32 in_uFlags, AkCallbackFunc in_pfnCallback, void* in_pCookie, DynamicSequenceType in_eDynamicSequenceType) {
			tDynamicSequence_Open func = (tDynamicSequence_Open)Wwise::Exports::func_Wwise_Sound_DynamicSequence_Open;
			return func(in_gameObjectID, in_uFlags, in_pfnCallback, in_pCookie, in_eDynamicSequenceType);
		}

		/// <summary>
		/// Pause specified Dynamic Sequence. To restart the sequence, call Resume. The item paused will resume its playback, followed by the rest of the sequence.
		/// </summary>
		/// <param name="in_playingID"> - AkPlayingID returned by DynamicSequence::Open</param>
		/// <param name="in_uTransitionDuration"> - Fade duration</param>
		/// <param name="in_eFadeCurve"> - Curve type to be used for the transition</param>
		/// <returns></returns>
		AKRESULT Pause(AkPlayingID in_playingID, AkTimeMs in_uTransitionDuration, AkCurveInterpolation in_eFadeCurve) {
			tDynamicSequence_Pause func = (tDynamicSequence_Pause)Wwise::Exports::func_Wwise_Sound_DynamicSequence_Pause;
			return func(in_playingID, in_uTransitionDuration, in_eFadeCurve);
		}

		/// <summary>
		/// Play specified Dynamic Sequence.
		/// </summary>
		/// <param name="in_playingID"> - AkPlayingID returned by DynamicSequence::Open</param>
		/// <param name="in_uTransitionDuration"> - Fade duration</param>
		/// <param name="in_eFadeCurve"> - Curve type to be used for the transition</param>
		/// <returns></returns>
		AKRESULT Play(AkPlayingID in_playingID, AkTimeMs in_uTransitionDuration, AkCurveInterpolation in_eFadeCurve) {
			tDynamicSequence_Play func = (tDynamicSequence_Play)Wwise::Exports::func_Wwise_Sound_DynamicSequence_Play;
			return func(in_playingID, in_uTransitionDuration, in_eFadeCurve);
		}

		/// <summary>
		/// Resume specified Dynamic Sequence.
		/// </summary>
		/// <param name="in_playingID"> - AkPlayingID returned by DynamicSequence::Open</param>
		/// <param name="in_uTransitionDuration"> - Fade duration</param>
		/// <param name="in_eFadeCurve"> - Curve type to be used for the transition</param>
		/// <returns></returns>
		AKRESULT Resume(AkPlayingID in_playingID, AkTimeMs in_uTransitionDuration, AkCurveInterpolation in_eFadeCurve) {
			tDynamicSequence_Resume func = (tDynamicSequence_Resume)Wwise::Exports::func_Wwise_Sound_DynamicSequence_Resume;
			return func(in_playingID, in_uTransitionDuration, in_eFadeCurve);
		}

		/// <summary>
		/// Stop specified Dynamic Sequence immediately. To restart the sequence, call Play. The sequence will restart with the item that was in the playlist after the item that was stopped.
		/// </summary>
		/// <param name="in_playingID"> - AkPlayingID returned by DynamicSequence::Open</param>
		/// <param name="in_uTransitionDuration"> - Fade duration</param>
		/// <param name="in_eFadeCurve"> - Curve type to be used for the transition</param>
		/// <returns></returns>
		AKRESULT Stop(AkPlayingID in_playingID, AkTimeMs in_uTransitionDuration, AkCurveInterpolation in_eFadeCurve) {
			tDynamicSequence_Stop func = (tDynamicSequence_Stop)Wwise::Exports::func_Wwise_Sound_DynamicSequence_Stop;
			return func(in_playingID, in_uTransitionDuration, in_eFadeCurve);
		}

		/// <summary>
		/// Unlock the playlist.
		/// </summary>
		/// <param name="in_playingID"> - AkPlayingID returned by DynamicSequence::Open</param>
		/// <returns></returns>
		AKRESULT UnlockPlaylist(AkPlayingID in_playingID) {
			tDynamicSequence_UnlockPlaylist func = (tDynamicSequence_UnlockPlaylist)Wwise::Exports::func_Wwise_Sound_DynamicSequence_UnlockPlaylist;
			return func(in_playingID);
		}
	}

	/// <summary>
	/// Executes an action on all nodes that are referenced in the specified event in an action of type play.
	/// </summary>
	/// <param name="in_eventID"> - Unique ID of the event</param>
	/// <param name="in_ActionType"> - Action to execute on all the elements that were played using the specified event.</param>
	/// <param name="in_gameObjectID"> - Associated game object ID</param>
	/// <param name="in_uTransitionDuration"> - Fade duration</param>
	/// <param name="in_eFadeCurve"> - Curve type to be used for the transition</param>
	/// <param name="in_PlayingID"> - Associated PlayingID</param>
	/// <returns></returns>
	AKRESULT ExecuteActionOnEvent(AkUniqueID in_eventID, AkActionOnEventType in_ActionType, AkGameObjectID in_gameObjectID, AkTimeMs in_uTransitionDuration, AkCurveInterpolation in_eFadeCurve, AkPlayingID in_PlayingID) {
		tExecuteActionOnEvent_UniqueID func = (tExecuteActionOnEvent_UniqueID)Wwise::Exports::func_Wwise_Sound_ExecuteActionOnEvent_UniqueID;
		return func(in_eventID, in_ActionType, in_gameObjectID, in_uTransitionDuration, in_eFadeCurve, in_PlayingID);
	}

	/// <summary>
	/// Executes an action on all nodes that are referenced in the specified event in an action of type play.
	/// </summary>
	/// <param name="in_pszEventName"> - Name of the event</param>
	/// <param name="in_ActionType"> - Action to execute on all the elements that were played using the specified event.</param>
	/// <param name="in_gameObjectID"> - Associated game object ID</param>
	/// <param name="in_uTransitionDuration"> - Fade duration</param>
	/// <param name="in_eFadeCurve"> - Curve type to be used for the transition</param>
	/// <param name="in_PlayingID"> - Associated PlayingID</param>
	/// <returns></returns>
	AKRESULT ExecuteActionOnEvent(const char* in_pszEventName, AkActionOnEventType in_ActionType, AkGameObjectID in_gameObjectID, AkTimeMs in_uTransitionDuration, AkCurveInterpolation in_eFadeCurve, AkPlayingID in_PlayingID) {
		tExecuteActionOnEvent_Char func = (tExecuteActionOnEvent_Char)Wwise::Exports::func_Wwise_Sound_ExecuteActionOnEvent_Char;
		return func(in_pszEventName, in_ActionType, in_gameObjectID, in_uTransitionDuration, in_eFadeCurve, in_PlayingID);
	}

	/// <summary>
	/// Get the default values of the platform-independent initialization settings.
	/// </summary>
	/// <param name="out_settings"> - Returned default platform-independent sound engine settings</param>
	void GetDefaultInitSettings(AkInitSettings* out_settings) {
		tGetDefaultInitSettings func = (tGetDefaultInitSettings)Wwise::Exports::func_Wwise_Sound_GetDefaultInitSettings;
		return func(out_settings);
	}

	/// <summary>
	/// Get the default values of the platform-specific initialization settings.
	/// </summary>
	/// <param name="out_platformSettings"> - Returned default platform-specific sound engine settings</param>
	void GetDefaultPlatformInitSettings(AkPlatformInitSettings* out_platformSettings) {
		tGetDefaultPlatformInitSettings func = (tGetDefaultPlatformInitSettings)Wwise::Exports::func_Wwise_Sound_GetDefaultPlatformInitSettings;
		return func(out_platformSettings);
	}

	/// <summary>
	/// Universal converter from string to ID for the sound engine. 
	/// </summary>
	/// <param name="in_pszString"> - Name to convert to Id</param>
	/// <returns>This function will hash the name based on a algorithm, and return the ID.</returns>
	AkUInt32 GetIDFromString(const char* in_pszString) {
		tGetIDFromString func = (tGetIDFromString)Wwise::Exports::func_Wwise_Sound_GetIDFromString;
		return func(in_pszString);
	}

	/// <summary>
	/// Get the panning rule of the specified output.
	/// </summary>
	/// <param name="out_ePanningRule"> - Returned panning rule (AkPanningRule_Speakers or AkPanningRule_Headphone) for given output.</param>
	/// <param name="in_eSinkType"> - Output sink type. Pass AkOutput_Main for main output.</param>
	/// <param name="in_iOutputID"> - Player number or device-unique identifier. Pass 0 for main.</param>
	/// <returns></returns>
	AKRESULT GetPanningRule(AkPanningRule* out_ePanningRule, AkAudioOutputType in_eSinkType, AkUInt32 in_iOutputID) {
		tGetPanningRule func = (tGetPanningRule)Wwise::Exports::func_Wwise_Sound_GetPanningRule;
		return func(out_ePanningRule, in_eSinkType, in_iOutputID);
	}

	/// <summary>
	/// Get the current position of the source associated with this playing ID, obtained from PostEvent().
	/// </summary>
	/// <param name="in_PlayingID"> - Playing ID returned by AK::SoundEngine::PostEvent()</param>
	/// <param name="out_puPosition"> - Position of the source (in ms) associated with that playing ID</param>
	/// <param name="in_bExtrapolate"> - Position is extrapolated based on time elapsed since last sound engine update.</param>
	/// <returns></returns>
	AKRESULT GetSourcePlayPosition(AkPlayingID in_PlayingID, AkTimeMs* out_puPosition, bool in_bExtrapolate) {
		tGetSourcePlayPosition func = (tGetSourcePlayPosition)Wwise::Exports::func_Wwise_Sound_GetSourcePlayPosition;
		return func(in_PlayingID, out_puPosition, in_bExtrapolate);
	}

	/// <summary>
	/// Get the output speaker configuration of the specified output.
	/// </summary>
	/// <returns></returns>
	AkUInt32 GetSpeakerConfiguration() {
		tGetSpeakerConfiguration func = (tGetSpeakerConfiguration)Wwise::Exports::func_Wwise_Sound_GetSpeakerConfiguration;
		return func();
	}

	/// <summary>
	/// Initialize the sound engine.
	/// </summary>
	/// <param name="in_pSettings"> - Initialization settings (can be NULL, to use the default values)</param>
	/// <param name="in_pPlatformSettings"> - Platform-specific settings (can be NULL, to use the default values)</param>
	/// <returns></returns>
	AKRESULT Init(AkInitSettings* in_pSettings, AkPlatformInitSettings* in_pPlatformSettings) {
		tInit func = (tInit)Wwise::Exports::func_Wwise_Sound_Init;
		return func(in_pSettings, in_pPlatformSettings);
	}

	/// <summary>
	/// Query whether or not the sound engine has been successfully initialized.
	/// </summary>
	/// <returns>True if the sound engine has been initialized, False otherwise</returns>
	bool IsInitialized() {
		tIsInitialized func = (tIsInitialized)Wwise::Exports::func_Wwise_Sound_IsInitialized;
		return func();
	}

	/// <summary>
	/// Load a bank synchronously (by ID).
	/// </summary>
	/// <param name="in_bankID"> - Bank ID of the bank to load</param>
	/// <param name="in_memPoolId"> - Memory pool ID (the pool is created if AK_DEFAULT_POOL_ID is passed)</param>
	/// <returns></returns>
	AKRESULT LoadBank(AkBankID in_bankID, AkMemPoolId in_memPoolId) {
		tLoadBank_BankID_MemPoolID func = (tLoadBank_BankID_MemPoolID)Wwise::Exports::func_Wwise_Sound_LoadBank_BankID_MemPoolID;
		return func(in_bankID, in_memPoolId);
	}

	/// <summary>
	/// Load a bank synchronously (from in-memory data, in-place).
	/// </summary>
	/// <param name="in_plnMemoryBankPtr"> - Pointer to the in-memory bank to load (pointer is stored in sound engine, memory must remain valid)</param>
	/// <param name="in_ulnMemoryBankSize"> - Size of the in-memory bank to load</param>
	/// <param name="out_bankID"> - Returned bank ID</param>
	/// <returns></returns>
	AKRESULT LoadBank(const void* in_plnMemoryBankPtr, AkUInt32 in_ulnMemoryBankSize, AkBankID* out_bankID) {
		tLoadBank_Void_UInt32_BankID func = (tLoadBank_Void_UInt32_BankID)Wwise::Exports::func_Wwise_Sound_LoadBank_Void_UInt32_BankID;
		return func(in_plnMemoryBankPtr, in_ulnMemoryBankSize, out_bankID);
	}

	/// <summary>
	/// Load a bank asynchronously (by ID).
	/// </summary>
	/// <param name="in_BankID"> - Bank ID of the bank to load</param>
	/// <param name="in_pfnBankCallback"> - Callback function</param>
	/// <param name="in_pCookie"> - Callback cookie (reserved to user, passed to the callback function, and also to AK::StreamMgr::IAkFileLocationResolver::Open() as AkFileSystemFlags::pCustomParam)</param>
	/// <param name="in_memPoolId"> - Memory pool ID (the pool is created if AK_DEFAULT_POOL_ID is passed)</param>
	/// <returns></returns>
	AKRESULT LoadBank(AkBankID in_BankID, AkBankCallbackFunc in_pfnBankCallback, void* in_pCookie, AkMemPoolId in_memPoolId) {
		tLoadBank_BankID_Callback func = (tLoadBank_BankID_Callback)Wwise::Exports::func_Wwise_Sound_LoadBank_BankID_Callback;
		return func(in_BankID, in_pfnBankCallback, in_pCookie, in_memPoolId);
	}

	/// <summary>
	/// Load a bank asynchronously (from in-memory data, in-place).
	/// </summary>
	/// <param name="in_plnMemoryBankPtr"> - Pointer to the in-memory bank to load (pointer is stored in sound engine, memory must remain valid)</param>
	/// <param name="in_ulnMemoryBankSize"> - Size of the in-memory bank to load</param>
	/// <param name="in_pfnBankCallback"> - Callback function</param>
	/// <param name="in_pCookie"> - Callback cookie</param>
	/// <param name="out_bankID"> - Returned bank ID</param>
	/// <returns></returns>
	AKRESULT LoadBank(const void* in_plnMemoryBankPtr, AkUInt32 in_ulnMemoryBankSize, AkBankCallbackFunc in_pfnBankCallback, void* in_pCookie, AkBankID* out_bankID) {
		tLoadBank_Void_UInt32_Callback func = (tLoadBank_Void_UInt32_Callback)Wwise::Exports::func_Wwise_Sound_LoadBank_Void_UInt32_Callback;
		return func(in_plnMemoryBankPtr, in_ulnMemoryBankSize, in_pfnBankCallback, in_pCookie, out_bankID);
	}

	/// <summary>
	/// Load a bank synchronously.
	/// </summary>
	/// <param name="in_pszString"> - Name of the bank to load</param>
	/// <param name="in_memPoolId"> - Memory pool ID (the pool is created if AK_DEFAULT_POOL_ID is passed)</param>
	/// <param name="out_bankID"> - Returned bank ID</param>
	/// <returns></returns>
	AKRESULT LoadBank(const char* in_pszString, AkMemPoolId in_memPoolId, AkBankID* out_bankID) {
		tLoadBank_Char_MemPoolID func = (tLoadBank_Char_MemPoolID)Wwise::Exports::func_Wwise_Sound_LoadBank_Char_MemPoolID;
		return func(in_pszString, in_memPoolId, out_bankID);
	}

	/// <summary>
	/// Load a bank asynchronously.
	/// </summary>
	/// <param name="in_pszString"> - Name/path of the bank to load</param>
	/// <param name="in_pfnBankCallback"> - Callback function</param>
	/// <param name="in_pCookie"> - Callback cookie (reserved to user, passed to the callback function, and also to AK::StreamMgr::IAkFileLocationResolver::Open() as AkFileSystemFlags::pCustomParam)</param>
	/// <param name="in_memPoolId"> - Memory pool ID (the pool is created if AK_DEFAULT_POOL_ID is passed)</param>
	/// <param name="out_bankId"> - Returned bank ID</param>
	/// <returns></returns>
	AKRESULT LoadBank(const char* in_pszString, AkBankCallbackFunc in_pfnBankCallback, void* in_pCookie, AkMemPoolId in_memPoolId, AkBankID* out_bankId) {
		tLoadBank_Char_Callback func = (tLoadBank_Char_Callback)Wwise::Exports::func_Wwise_Sound_LoadBank_Char_Callback;
		return func(in_pszString, in_pfnBankCallback, in_pCookie, in_memPoolId, out_bankId);
	}

	/// <summary>
	/// (Undocumented) Load a bank asynchronously.
	/// </summary>
	/// <param name="in_pszString"> - Name/path of the bank to load</param>
	/// <param name="in_pfnBankCallback"> - Callback function</param>
	/// <param name="in_pCookie"> - Callback cookie (reserved to user, passed to the callback function, and also to AK::StreamMgr::IAkFileLocationResolver::Open() as AkFileSystemFlags::pCustomParam)</param>
	/// <param name="in_memPoolId"> - Memory pool ID (the pool is created if AK_DEFAULT_POOL_ID is passed)</param>
	/// <param name="out_bankId"> - Returned bank ID</param>
	/// <returns></returns>
	AKRESULT LoadBankUnique(const char* in_pszString, AkBankCallbackFunc in_pfnBankCallback, void* in_pCookie, AkMemPoolId in_memPoolId, AkBankID* out_bankId) {
		tLoadBankUnique func = (tLoadBankUnique)Wwise::Exports::func_Wwise_Sound_LoadBankUnique;
		return func(in_pszString, in_pfnBankCallback, in_pCookie, in_memPoolId, out_bankId);
	}

	/// <summary>
	/// Undocumented
	/// </summary>
	/// <param name="param_1"></param>
	/// <param name="param_2"></param>
	/// <param name="param_3"></param>
	/// <returns></returns>
	AkUInt32 PlaySourcePlugin(AkUInt32 param_1, AkUInt32 param_2, AkUInt32 param_3) {
		tPlaySourcePlugin func = (tPlaySourcePlugin)Wwise::Exports::func_Wwise_Sound_PlaySourcePlugin;
		return func(param_1, param_2, param_3);
	}

	/// <summary>
	/// Post an event to the sound engine (by event name), using callbacks.
	/// </summary>
	/// <param name="in_pszEventName"> - Name of the event</param>
	/// <param name="in_gameObjectID"> - Associated game object ID</param>
	/// <param name="in_uFlags"> - Bitmask: see AkCallbackType</param>
	/// <param name="in_pfnCallback"> - Callback function</param>
	/// <param name="in_pCookie"> - Callback cookie that will be sent to the callback function along with additional information.</param>
	/// <param name="in_cExternals"> - Optional count of external source structures</param>
	/// <param name="in_pExternalSources"> - Optional array of external source resolution information</param>
	/// <param name="in_PlayingID"> - Optional (advanced users only) Specify the playing ID to target with the event. Will Cause active actions in this event to target an existing Playing ID. Let it be AK_INVALID_PLAYING_ID or do not specify any for normal playback.</param>
	/// <returns>The playing ID of the event launched, or AK_INVALID_PLAYING_ID if posting the event failed</returns>
	AkPlayingID PostEvent(const char* in_pszEventName, AkGameObjectID in_gameObjectID, AkUInt32 in_uFlags, AkCallbackFunc in_pfnCallback, void* in_pCookie, AkUInt32 in_cExternals, AkExternalSourceInfo* in_pExternalSources, AkPlayingID in_PlayingID) {
		tPostEvent_Char func = (tPostEvent_Char)Wwise::Exports::func_Wwise_Sound_PostEvent_Char;
		return func(in_pszEventName, in_gameObjectID, in_uFlags, in_pfnCallback, in_pCookie, in_cExternals, in_pExternalSources, in_PlayingID);
	}

	/// <summary>
	/// Asynchronously post an event to the sound engine (by event ID).
	/// </summary>
	/// <param name="in_eventID"> - Unique ID of the event</param>
	/// <param name="in_gameObjectID"> - Associated game object ID</param>
	/// <param name="in_uFlags"> - Bitmask: see AkCallbackType</param>
	/// <param name="in_pfnCallback"> - Callback function</param>
	/// <param name="in_pCookie"> - Callback cookie that will be sent to the callback function along with additional information</param>
	/// <param name="in_cExternals"> - Optional count of external source structures</param>
	/// <param name="in_pExternalSources"> - Optional array of external source resolution information</param>
	/// <param name="in_PlayingID"> - Optional (advanced users only) Specify the playing ID to target with the event. Will Cause active actions in this event to target an existing Playing ID. Let it be AK_INVALID_PLAYING_ID or do not specify any for normal playback.</param>
	/// <returns>The playing ID of the event launched, or AK_INVALID_PLAYING_ID if posting the event failed</returns>
	AkPlayingID PostEvent(AkUniqueID in_eventID, AkGameObjectID in_gameObjectID, AkUInt32 in_uFlags, AkCallbackFunc in_pfnCallback, void* in_pCookie, AkUInt32 in_cExternals, AkExternalSourceInfo* in_pExternalSources, AkPlayingID in_PlayingID) {
		tPostEvent_UniqueID func = (tPostEvent_UniqueID)Wwise::Exports::func_Wwise_Sound_PostEvent_UniqueID;
		return func(in_eventID, in_gameObjectID, in_uFlags, in_pfnCallback, in_pCookie, in_cExternals, in_pExternalSources, in_PlayingID);
	}

	/// <summary>
	/// Post the specified trigger (by IDs).
	/// </summary>
	/// <param name="in_triggerID"> - ID of the trigger</param>
	/// <param name="in_gameObjectID"> - Associated game object ID</param>
	/// <returns>Always returns AK_Success</returns>
	AKRESULT PostTrigger(AkTriggerID in_triggerID, AkGameObjectID in_gameObjectID) {
		tPostTrigger_TriggerID func = (tPostTrigger_TriggerID)Wwise::Exports::func_Wwise_Sound_PostTrigger_TriggerID;
		return func(in_triggerID, in_gameObjectID);
	}

	/// <summary>
	/// Post the specified trigger.
	/// </summary>
	/// <param name="in_szTriggerName"> - Name of the trigger</param>
	/// <param name="in_gameObjectID"> - Associated game object ID</param>
	/// <returns>AK_Success if successful. AK_IDNotFound if the trigger name was not resolved to an existing ID.</returns>
	AKRESULT PostTrigger(char* in_szTriggerName, AkGameObjectID in_gameObjectID) {
		tPostTrigger_Char func = (tPostTrigger_Char)Wwise::Exports::func_Wwise_Sound_PostTrigger_Char;
		return func(in_szTriggerName, in_gameObjectID);
	}

	/// <summary>
	/// This function will load the events, structural content, and optionally, the media content from the SoundBank.
	/// </summary>
	/// <param name="in_PreparationType"> - Preparation type ( Preparation_Load or Preparation_Unload )</param>
	/// <param name="in_bankID"> - ID of the bank to Prepare/Unprepare.</param>
	/// <param name="in_pfnBankCallback"> - Callback function</param>
	/// <param name="in_pCookie"> - Callback cookie (reserved to user, passed to the callback function)</param>
	/// <param name="in_uFlags"> - Structures only (including events) or all content.</param>
	/// <returns></returns>
	AKRESULT PrepareBank(PreparationType in_PreparationType, AkBankID in_bankID, AkBankCallbackFunc in_pfnBankCallback, void* in_pCookie, AkBankContent in_uFlags) {
		tPrepareBank_BankID_Callback func = (tPrepareBank_BankID_Callback)Wwise::Exports::func_Wwise_Sound_PrepareBank_BankID_Callback;
		return func(in_PreparationType, in_bankID, in_pfnBankCallback, in_pCookie, in_uFlags);
	}

	/// <summary>
	/// This function will load the events, structural content, and optionally, the media content from the SoundBank.
	/// </summary>
	/// <param name="in_PreparationType"> - Preparation type ( Preparation_Load or Preparation_Unload )</param>
	/// <param name="in_bankID"> - ID of the bank to Prepare/Unprepare.</param>
	/// <param name="in_uFlags"> - Structures only (including events) or all content.</param>
	/// <returns></returns>
	AKRESULT PrepareBank(PreparationType in_PreparationType, AkBankID in_bankID, AkBankContent in_uFlags) {
		tPrepareBank_BankID_BankContent func = (tPrepareBank_BankID_BankContent)Wwise::Exports::func_Wwise_Sound_PrepareBank_BankID_BankContent;
		return func(in_PreparationType, in_bankID, in_uFlags);
	}

	/// <summary>
	/// This function will load the events, structural content, and optionally, the media content from the SoundBank.
	/// </summary>
	/// <param name="in_PreparationType"> - Preparation type ( Preparation_Load or Preparation_Unload )</param>
	/// <param name="in_pszString"> - Name of the bank to Prepare/Unprepare.</param>
	/// <param name="in_pfnBankCallback"> - Callback function</param>
	/// <param name="in_pCookie"> - Callback cookie (reserved to user, passed to the callback function)</param>
	/// <param name="in_uFlags"> - Structures only (including events) or all content.</param>
	/// <returns></returns>
	AKRESULT PrepareBank(PreparationType in_PreparationType, const char* in_pszString, AkBankCallbackFunc in_pfnBankCallback, void* in_pCookie, AkBankContent in_uFlags) {
		tPrepareBank_Char_Callback func = (tPrepareBank_Char_Callback)Wwise::Exports::func_Wwise_Sound_PrepareBank_Char_Callback;
		return func(in_PreparationType, in_pszString, in_pfnBankCallback, in_pCookie, in_uFlags);
	}

	/// <summary>
	/// This function will load the events, structural content, and optionally, the media content from the SoundBank.
	/// </summary>
	/// <param name="in_PreparationType"> - Preparation type ( Preparation_Load or Preparation_Unload )</param>
	/// <param name="in_pszString"> - Name of the bank to Prepare/Unprepare.</param>
	/// <param name="in_uFlags"> - Structures only (including events) or all content.</param>
	/// <returns></returns>
	AKRESULT PrepareBank(PreparationType in_PreparationType, const char* in_pszString, AkBankContent in_uFlags) {
		tPrepareBank_Char_BankContent func = (tPrepareBank_Char_BankContent)Wwise::Exports::func_Wwise_Sound_PrepareBank_Char_BankContent;
		return func(in_PreparationType, in_pszString, in_uFlags);
	}

	/// <summary>
	/// Prepare or un-prepare events synchronously (by ID).
	/// </summary>
	/// <param name="in_PreparationType"> - Preparation type ( Preparation_Load or Preparation_Unload )</param>
	/// <param name="in_pEventID"> - Array of event IDs</param>
	/// <param name="in_uNumEvent"> - Number of event IDs in the array</param>
	/// <returns>AK_Success if scheduling is was successful, AK_Fail otherwise.</returns>
	AKRESULT PrepareEvent(PreparationType in_PreparationType, AkUniqueID* in_pEventID, AkUInt32 in_uNumEvent) {
		tPrepareEvent_EventID_UInt32 func = (tPrepareEvent_EventID_UInt32)Wwise::Exports::func_Wwise_Sound_PrepareEvent_EventID_UInt32;
		return func(in_PreparationType, in_pEventID, in_uNumEvent);
	}

	/// <summary>
	/// Prepare or un-prepare events asynchronously (by ID).
	/// </summary>
	/// <param name="in_PreparationType"> - Preparation type ( Preparation_Load or Preparation_Unload )</param>
	/// <param name="in_pEventID"> - Array of event IDs</param>
	/// <param name="in_uNumEvent"> - Number of event IDs in the array</param>
	/// <param name="in_pfnBankCallback"> - Callback function</param>
	/// <param name="in_pCookie"> - Callback cookie (reserved to user, passed to the callback function)</param>
	/// <returns>AK_Success if scheduling is was successful, AK_Fail otherwise.</returns>
	AKRESULT PrepareEvent(PreparationType in_PreparationType, AkUniqueID* in_pEventID, AkUInt32 in_uNumEvent, AkBankCallbackFunc in_pfnBankCallback, void* in_pCookie) {
		tPrepareEvent_EventID_UInt32_Callback_Void func = (tPrepareEvent_EventID_UInt32_Callback_Void)Wwise::Exports::func_Wwise_Sound_PrepareEvent_EventID_UInt32_Callback_Void;
		return func(in_PreparationType, in_pEventID, in_uNumEvent, in_pfnBankCallback, in_pCookie);
	}

	/// <summary>
	/// Prepare or un-prepare events synchronously.
	/// </summary>
	/// <param name="in_PreparationType"> - Preparation type ( Preparation_Load or Preparation_Unload )</param>
	/// <param name="in_ppszString"> - Array of event names</param>
	/// <param name="in_uNumEvent"> - Number of events in the array</param>
	/// <returns>AK_Success if scheduling is was successful, AK_Fail otherwise.</returns>
	AKRESULT PrepareEvent(PreparationType in_PreparationType, const char** in_ppszString, AkUInt32 in_uNumEvent) {
		tPrepareEvent_Char_UInt32 func = (tPrepareEvent_Char_UInt32)Wwise::Exports::func_Wwise_Sound_PrepareEvent_Char_UInt32;
		return func(in_PreparationType, in_ppszString, in_uNumEvent);
	}

	/// <summary>
	/// Prepare or un-prepare an event asynchronously.
	/// </summary>
	/// <param name="in_PreparationType"> - Preparation type ( Preparation_Load or Preparation_Unload )</param>
	/// <param name="in_ppszString"> - Array of event names</param>
	/// <param name="in_uNumEvent"> - Number of events in the array</param>
	/// <param name="in_pfnBankCallback"> - Callback function</param>
	/// <param name="in_pCookie"> - Callback cookie (reserved to user, passed to the callback function)</param>
	/// <returns>AK_Success if scheduling is was successful, AK_Fail otherwise.</returns>
	AKRESULT PrepareEvent(PreparationType in_PreparationType, const char** in_ppszString, AkUInt32 in_uNumEvent, AkBankCallbackFunc in_pfnBankCallback, void* in_pCookie) {
		tPrepareEvent_Char_UInt32_Callback_Void func = (tPrepareEvent_Char_UInt32_Callback_Void)Wwise::Exports::func_Wwise_Sound_PrepareEvent_Char_UInt32_Callback_Void;
		return func(in_PreparationType, in_ppszString, in_uNumEvent, in_pfnBankCallback, in_pCookie);
	}

	/// <summary>
	/// Prepare or un-prepare game syncs asynchronously (by ID).
	/// </summary>
	/// <param name="in_PreparationType"> - Preparation type ( Preparation_Load or Preparation_Unload )</param>
	/// <param name="in_eGameSyncType"> - The type of game sync.</param>
	/// <param name="in_GroupID"> - The state group ID or the Switch Group ID.</param>
	/// <param name="in_paGameSyncID"> - Array of ID of the gamesyncs to either support or not support.</param>
	/// <param name="in_uNumGameSyncs"> - The number of game sync ID in the array.</param>
	/// <param name="in_pfnBankCallback"> - Callback function</param>
	/// <param name="in_pCookie"> - Callback cookie (reserved to user, passed to the callback function)</param>
	/// <returns>AK_Success if scheduling is was successful, AK_Fail otherwise.</returns>
	AKRESULT PrepareGameSyncs(PreparationType in_PreparationType, AkGroupType in_eGameSyncType, AkUInt32 in_GroupID, AkUInt32* in_paGameSyncID, AkUInt32 in_uNumGameSyncs, AkBankCallbackFunc in_pfnBankCallback, void* in_pCookie) {
		tPrepareGameSyncs_UInt32_UInt32_UInt32_Callback_Void func = (tPrepareGameSyncs_UInt32_UInt32_UInt32_Callback_Void)Wwise::Exports::func_Wwise_Sound_PrepareGameSyncs_UInt32_UInt32_UInt32_Callback_Void;
		return func(in_PreparationType, in_eGameSyncType, in_GroupID, in_paGameSyncID, in_uNumGameSyncs, in_pfnBankCallback, in_pCookie);
	}

	/// <summary>
	/// Prepare or un-prepare game syncs synchronously (by ID).
	/// </summary>
	/// <param name="in_PreparationType"> - Preparation type ( Preparation_Load or Preparation_Unload )</param>
	/// <param name="in_eGameSyncType"> - The type of game sync.</param>
	/// <param name="in_GroupID"> - The state group ID or the Switch Group ID.</param>
	/// <param name="in_paGameSyncID"> - Array of ID of the gamesyncs to either support or not support.</param>
	/// <param name="in_uNumGameSyncs"> - The number of game sync ID in the array.</param>
	/// <returns>AK_Success if scheduling is was successful, AK_Fail otherwise.</returns>
	AKRESULT PrepareGameSyncs(PreparationType in_PreparationType, AkGroupType in_eGameSyncType, AkUInt32 in_GroupID, AkUInt32* in_paGameSyncID, AkUInt32 in_uNumGameSyncs) {
		tPrepareGameSyncs_UInt32_UInt32_UInt32 func = (tPrepareGameSyncs_UInt32_UInt32_UInt32)Wwise::Exports::func_Wwise_Sound_PrepareGameSyncs_UInt32_UInt32_UInt32;
		return func(in_PreparationType, in_eGameSyncType, in_GroupID, in_paGameSyncID, in_uNumGameSyncs);
	}

	/// <summary>
	/// Prepare or un-prepare game syncs asynchronously.
	/// </summary>
	/// <param name="in_PreparationType"> - Preparation type ( Preparation_Load or Preparation_Unload )</param>
	/// <param name="in_eGameSyncType"> - The type of game sync.</param>
	/// <param name="in_pszGroupName"> - The state group Name or the Switch Group Name.</param>
	/// <param name="in_ppszGameSyncName"> - The specific ID of the state to either support or not support.</param>
	/// <param name="in_uNumGameSyncs"> - The number of game sync in the string array.</param>
	/// <param name="in_pfnBankCallback"> - Callback function</param>
	/// <param name="in_pCookie"> - Callback cookie (reserved to user, passed to the callback function)</param>
	/// <returns>AK_Success if scheduling is was successful, AK_Fail otherwise.</returns>
	AKRESULT PrepareGameSyncs(PreparationType in_PreparationType, AkGroupType in_eGameSyncType, const char* in_pszGroupName, const char** in_ppszGameSyncName, AkUInt32 in_uNumGameSyncs, AkBankCallbackFunc in_pfnBankCallback, void* in_pCookie) {
		tPrepareGameSyncs_Char_Char_UInt32_Callback_Void func = (tPrepareGameSyncs_Char_Char_UInt32_Callback_Void)Wwise::Exports::func_Wwise_Sound_PrepareGameSyncs_Char_Char_UInt32_Callback_Void;
		return func(in_PreparationType, in_eGameSyncType, in_pszGroupName, in_ppszGameSyncName, in_uNumGameSyncs, in_pfnBankCallback, in_pCookie);
	}

	/// <summary>
	/// Prepare or un-prepare game syncs synchronously.
	/// </summary>
	/// <param name="in_PreparationType"> - Preparation type ( Preparation_Load or Preparation_Unload )</param>
	/// <param name="in_eGameSyncType"> - The type of game sync.</param>
	/// <param name="in_pszGroupName"> - The state group Name or the Switch Group Name.</param>
	/// <param name="in_ppszGameSyncName"> - The specific ID of the state to either support or not support.</param>
	/// <param name="in_uNumGameSyncs"> - The number of game sync in the string array.</param>
	/// <returns>AK_Success if scheduling is was successful, AK_Fail otherwise.</returns>
	AKRESULT PrepareGameSyncs(PreparationType in_PreparationType, AkGroupType in_eGameSyncType, const char* in_pszGroupName, const char** in_ppszGameSyncName, AkUInt32 in_uNumGameSyncs) {
		tPrepareGameSyncs_Char_Char_UInt32 func = (tPrepareGameSyncs_Char_Char_UInt32)Wwise::Exports::func_Wwise_Sound_PrepareGameSyncs_Char_Char_UInt32;
		return func(in_PreparationType, in_eGameSyncType, in_pszGroupName, in_ppszGameSyncName, in_uNumGameSyncs);
	}

	// Most accurate documentation is available here: https://www.audiokinetic.com/library/2015.1.9_5624/?source=SDK&id=namespace_a_k_1_1_sound_engine_1_1_query.html
	namespace Query {

		/// <summary>
		/// Fill the provided list with all the game object IDs that are currently active in the sound engine.
		/// </summary>
		/// <param name="io_GameObjectList"> - Returned list of active game objects.</param>
		/// <returns></returns>
		AKRESULT GetActiveGameObjects(AkGameObjectsList* io_GameObjectList) {
			tQuery_GetActiveGameObjects func = (tQuery_GetActiveGameObjects)Wwise::Exports::func_Wwise_Sound_Query_GetActiveGameObjects;
			return func(io_GameObjectList);
		}

		/// <summary>
		/// Get a game object's active listeners.
		/// </summary>
		/// <param name="in_GameObjectID"> - Game object identifier</param>
		/// <param name="out_ruListenerMask"> - Bitmask representing the active listeners (LSB = Listener 0, set to 1 means active)</param>
		/// <returns>AK_Success if succeeded, or AK_IDNotFound if the game object was not registered</returns>
		AKRESULT GetActiveListeners(AkGameObjectID in_GameObjectID, AkUInt32* out_ruListenerMask) {
			tQuery_GetActiveListeners func = (tQuery_GetActiveListeners)Wwise::Exports::func_Wwise_Sound_Query_GetActiveListeners;
			return func(in_GameObjectID, out_ruListenerMask);
		}

		/// <summary>
		/// Get the value of a custom property of integer or boolean type.
		/// </summary>
		/// <param name="in_ObjectID"> - Object ID</param>
		/// <param name="in_uPropID"> - Property ID</param>
		/// <param name="out_iValue"> - Property Value</param>
		/// <returns></returns>
		AKRESULT GetCustomPropertyValue(AkUniqueID in_ObjectID, AkUInt32 in_uPropID, AkInt32* out_iValue) {
			tQuery_GetCustomPropertyValue_Int32 func = (tQuery_GetCustomPropertyValue_Int32)Wwise::Exports::func_Wwise_Sound_Query_GetCustomPropertyValue_Int32;
			return func(in_ObjectID, in_uPropID, out_iValue);
		}

		/// <summary>
		/// Get the value of a custom property of real (float) type.
		/// </summary>
		/// <param name="in_ObjectID"> - Object ID</param>
		/// <param name="in_uPropID"> - Property ID</param>
		/// <param name="out_fValue"> - Property Value</param>
		/// <returns></returns>
		AKRESULT GetCustomPropertyValue(AkUniqueID in_ObjectID, AkUInt32 in_uPropID, AkReal32* out_fValue) {
			tQuery_GetCustomPropertyValue_Real32 func = (tQuery_GetCustomPropertyValue_Real32)Wwise::Exports::func_Wwise_Sound_Query_GetCustomPropertyValue_Real32;
			return func(in_ObjectID, in_uPropID, out_fValue);
		}

		/// <summary>
		/// Get the Event ID associated to the specified PlayingID.
		/// </summary>
		/// <param name="in_playingID"> - Associated PlayingID</param>
		/// <returns>Event ID if successful. AK_INVALID_UNIQUE_ID on failure.</returns>
		AkUniqueID GetEventIDFromPlayingID(AkPlayingID in_playingID) {
			tQuery_GetEventIDFromPlayingID func = (tQuery_GetEventIDFromPlayingID)Wwise::Exports::func_Wwise_Sound_Query_GetEventIDFromPlayingID;
			return func(in_playingID);
		}

		/// <summary>
		/// Get the environmental ratios used by the specified game object.
		/// </summary>
		/// <param name="in_gameObjectID"> - Associated game object ID</param>
		/// <param name="out_paAuxSendValues"> - Variable-size array of AkAuxSendValue structures (it may be NULL if no aux send must be set, and its size cannot exceed AK_MAX_AUX_PER_OBJ)</param>
		/// <param name="io_ruNumSendValues"> - The number of Auxilliary busses at the pointer's address (it must be 0 if no aux bus is set, and can not exceed AK_MAX_AUX_PER_OBJ)</param>
		/// <returns>AK_Success if succeeded, or AK_InvalidParameter if io_ruNumEnvValues is 0 or out_paEnvironmentValues is NULL, or AK_PartialSuccess if more environments exist than io_ruNumEnvValues AK_InvalidParameter</returns>
		AKRESULT GetGameObjectAuxSendValues(AkGameObjectID in_gameObjectID, AkAuxSendValue* out_paAuxSendValues, AkUInt32* io_ruNumSendValues) {
			tQuery_GetGameObjectAuxSendValues func = (tQuery_GetGameObjectAuxSendValues)Wwise::Exports::func_Wwise_Sound_Query_GetGameObjectAuxSendValues;
			return func(in_gameObjectID, out_paAuxSendValues, io_ruNumSendValues);
		}

		/// <summary>
		/// Get the environmental dry level to be used for the specified game object. The control value is a number ranging from 0.0f to 1.0f. 0.0f stands for 0% dry, while 1.0f stands for 100% dry.
		/// </summary>
		/// <param name="in_gameObjectID"> - Associated game object ID</param>
		/// <param name="out_rfControlValue"> - Dry level control value, ranging from 0.0f to 1.0f (0.0f stands for 0% dry, while 1.0f stands for 100% dry)</param>
		/// <returns>AK_Success if succeeded, or AK_IDNotFound if the game object was not registered</returns>
		AKRESULT GetGameObjectDryLevelValue(AkGameObjectID in_gameObjectID, AkReal32* out_rfControlValue) {
			tQuery_GetGameObjectDryLevelValue func = (tQuery_GetGameObjectDryLevelValue)Wwise::Exports::func_Wwise_Sound_Query_GetGameObjectDryLevelValue;
			return func(in_gameObjectID, out_rfControlValue);
		}

		/// <summary>
		/// Get the ObjectID associated to the specified PlayingID. This function does not acquire the main audio lock.
		/// </summary>
		/// <param name="in_playingID"> - Associated PlayingID</param>
		/// <returns>GameObjectId if successful. AK_INVALID_GAME_OBJECT on failure.</returns>
		AkGameObjectID GetGameObjectFromPlayingID(AkPlayingID in_playingID) {
			tQuery_GetGameObjectFromPlayingID func = (tQuery_GetGameObjectFromPlayingID)Wwise::Exports::func_Wwise_Sound_Query_GetGameObjectFromPlayingID;
			return func(in_playingID);
		}

		/// <summary>
		/// Query if the specified game object is currently active. Being active means that either a sound is playing or pending to be played using this game object.
		/// </summary>
		/// <param name="in_GameObjId"> - Game object ID</param>
		/// <returns>Is game object active?</returns>
		bool GetIsGameObjectActive(AkGameObjectID in_GameObjId) {
			tQuery_GetIsGameObjectActive func = (tQuery_GetIsGameObjectActive)Wwise::Exports::func_Wwise_Sound_Query_GetIsGameObjectActive;
			return func(in_GameObjId);
		}

		/// <summary>
		/// Get a listener's position.
		/// </summary>
		/// <param name="in_uIndex"> - Listener index (0: first listener, 7: 8th listener)</param>
		/// <param name="out_rPosition"> - Position set</param>
		/// <returns>AK_Success if succeeded, or AK_InvalidParameter if the index is out of range</returns>
		AKRESULT GetListenerPosition(AkUInt32 in_uIndex, AkListenerPosition* out_rPosition) {
			tQuery_GetListenerPosition func = (tQuery_GetListenerPosition)Wwise::Exports::func_Wwise_Sound_Query_GetListenerPosition;
			return func(in_uIndex, out_rPosition);
		}

		/// <summary>
		/// Returns the maximum distance used in attenuations associated to all sounds currently playing.
		/// </summary>
		/// <param name="io_RadiusList"> - List that will be filled with AK::SoundEngine::Query::GameObjDst objects.</param>
		/// <returns>AK_Success if succeeded. AK_InsuficientMemory if there was not enough memory.</returns>
		AKRESULT GetMaxRadius(AkRadiusList* io_RadiusList) {
			tQuery_GetMaxRadius_RadiusList func = (tQuery_GetMaxRadius_RadiusList)Wwise::Exports::func_Wwise_Sound_Query_GetMaxRadius_RadiusList;
			return func(io_RadiusList);
		}

		/// <summary>
		/// Returns the maximum distance used in attenuations associated to sounds playing using the specified game object.
		/// </summary>
		/// <param name="in_GameObjId"> - Game object ID</param>
		/// <returns>A negative number if the game object specified is not playing. 0, if the game object was only associated to sounds playing using no distance attenuation ( like 2D sounds ). A positive number represents the maximum of all the distance attenuations playing on this game object.</returns>
		AkReal32 GetMaxRadius(AkGameObjectID in_GameObjId) {
			tQuery_GetMaxRadius_GameObject func = (tQuery_GetMaxRadius_GameObject)Wwise::Exports::func_Wwise_Sound_Query_GetMaxRadius_GameObject;
			return func(in_GameObjId);
		}

		/// <summary>
		/// Get a game object's obstruction and occlusion levels.
		/// </summary>
		/// <param name="in_ObjectID"> - Associated game object ID</param>
		/// <param name="in_uListener"> - Listener index (0: first listener, 7: 8th listener)</param>
		/// <param name="out_rfObstructionLevel"> - ObstructionLevel: [0.0f..1.0f]</param>
		/// <param name="out_rfOcclusionLevel"> - OcclusionLevel: [0.0f..1.0f]</param>
		/// <returns>AK_Success if succeeded, AK_IDNotFound if the game object was not registered</returns>
		AKRESULT GetObjectObstructionAndOcclusion(AkGameObjectID in_ObjectID, AkUInt32 in_uListener, AkReal32* out_rfObstructionLevel, AkReal32* out_rfOcclusionLevel) {
			tQuery_GetObjectObstructionAndOcclusion func = (tQuery_GetObjectObstructionAndOcclusion)Wwise::Exports::func_Wwise_Sound_Query_GetObjectObstructionAndOcclusion;
			return func(in_ObjectID, in_uListener, out_rfObstructionLevel, out_rfOcclusionLevel);
		}

		/// <summary>
		/// Get the list PlayingIDs associated with the given game object.
		/// </summary>
		/// <param name="in_GameObjId"> - Game object ID</param>
		/// <param name="io_ruNumIds"> - Number of items in array provided / Number of items filled in array</param>
		/// <param name="out_aPlayingIDs"> - Array of AkPlayingID items to fill</param>
		/// <returns>AK_Success if succeeded, AK_InvalidParameter if out_aPlayingIDs is NULL while io_ruNumItems > 0</returns>
		AKRESULT GetPlayingIDsFromGameObject(AkGameObjectID in_GameObjId, AkUInt32* io_ruNumIds, AkPlayingID* out_aPlayingIDs) {
			tQuery_GetPlayingIDsFromGameObject func = (tQuery_GetPlayingIDsFromGameObject)Wwise::Exports::func_Wwise_Sound_Query_GetPlayingIDsFromGameObject;
			return func(in_GameObjId, io_ruNumIds, out_aPlayingIDs);
		}

		/// <summary>
		/// Get the position of a game object.
		/// </summary>
		/// <param name="in_GameObjectID"> - Game object identifier</param>
		/// <param name="out_rPosition"> - Position to get</param>
		/// <returns>AK_Success if succeeded, or AK_IDNotFound if the game object was not registered</returns>
		AKRESULT GetPosition(AkGameObjectID in_GameObjectID, AkSoundPosition* out_rPosition) {
			tQuery_GetPosition func = (tQuery_GetPosition)Wwise::Exports::func_Wwise_Sound_Query_GetPosition;
			return func(in_GameObjectID, out_rPosition);
		}

		/// <summary>
		/// Get positioning information associated to an audio object.
		/// </summary>
		/// <param name="in_ObjectID"> - Audio object ID</param>
		/// <param name="out_rPositioningInfo"> - Positioning information structure to be filled</param>
		/// <returns>AK_Success if succeeded, AK_IDNotFound if the object ID cannot be found, AK_NotCompatible if the audio object cannot expose positioning</returns>
		AKRESULT GetPositioningInfo(AkUniqueID in_ObjectID, AkPositioningInfo* out_rPositioningInfo) {
			tQuery_GetPositioningInfo func = (tQuery_GetPositioningInfo)Wwise::Exports::func_Wwise_Sound_Query_GetPositioningInfo;
			return func(in_ObjectID, out_rPositioningInfo);
		}

		/// <summary>
		/// Get the value of a real-time parameter control.
		/// </summary>
		/// <param name="in_pszRtpcName"> - String name of the RTPC</param>
		/// <param name="in_gameObjectID"> - Associated game object ID</param>
		/// <param name="out_rValue"> - Value returned</param>
		/// <param name="io_rValueType"> - In/Out value, the user must specify the requested type. The function will return in this variable the type of the returned value.</param>
		/// <returns>AK_Success if succeeded, AK_IDNotFound if the game object was not registered or the rtpc name could not be found, or AK_Fail if the RTPC value could not be obtained</returns>
		AKRESULT GetRTPCValue(const char* in_pszRtpcName, AkGameObjectID in_gameObjectID, AkRtpcValue* out_rValue, RTPCValue_type* io_rValueType) {
			tQuery_GetRTPCValue_Char func = (tQuery_GetRTPCValue_Char)Wwise::Exports::func_Wwise_Sound_Query_GetRTPCValue_Char;
			return func(in_pszRtpcName, in_gameObjectID, out_rValue, io_rValueType);
		}

		/// <summary>
		/// Get the value of a real-time parameter control (by ID).
		/// </summary>
		/// <param name="in_rtpcID"> - ID of the RTPC</param>
		/// <param name="in_gameObjectID"> - Associated game object ID</param>
		/// <param name="out_rValue"> - Value returned</param>
		/// <param name="io_rValueType"> - In/Out value, the user must specify the requested type. The function will return in this variable the type of the returned value.</param>
		/// <returns>AK_Success if succeeded, AK_IDNotFound if the game object was not registered, or AK_Fail if the RTPC value could not be obtained.</returns>
		AKRESULT GetRTPCValue(AkRtpcID in_rtpcID, AkGameObjectID in_gameObjectID, AkRtpcValue* out_rValue, RTPCValue_type* io_rValueType) {
			tQuery_GetRTPCValue_RTPCID func = (tQuery_GetRTPCValue_RTPCID)Wwise::Exports::func_Wwise_Sound_Query_GetRTPCValue_RTPCID;
			return func(in_rtpcID, in_gameObjectID, out_rValue, io_rValueType);
		}

		/// <summary>
		/// Get the state of a state group (by IDs).
		/// </summary>
		/// <param name="in_stateGroup"> - ID of the state group</param>
		/// <param name="out_rState"> - ID of the state</param>
		/// <returns>AK_Success if succeeded</returns>
		AKRESULT GetState(AkStateGroupID in_stateGroup, AkStateID* out_rState) {
			tQuery_GetState_StateGroupID func = (tQuery_GetState_StateGroupID)Wwise::Exports::func_Wwise_Sound_Query_GetState_StateGroupID;
			return func(in_stateGroup, out_rState);
		}

		/// <summary>
		/// Get the state of a state group.
		/// </summary>
		/// <param name="in_pstrStateGroupName"> - String name of the state group</param>
		/// <param name="out_rState"> - ID of the state</param>
		/// <returns>AK_Success if succeeded, or AK_IDNotFound if the state group name can not be found.</returns>
		AKRESULT GetState(const char* in_pstrStateGroupName, AkStateID* out_rState) {
			tQuery_GetState_Char func = (tQuery_GetState_Char)Wwise::Exports::func_Wwise_Sound_Query_GetState_Char;
			return func(in_pstrStateGroupName, out_rState);
		}

		/// <summary>
		/// Get the state of a switch group (by IDs).
		/// </summary>
		/// <param name="in_switchGroup"> - ID of the switch group</param>
		/// <param name="in_gameObjectID"> - Associated game object ID</param>
		/// <param name="out_rSwitchState"> - ID of the switch</param>
		/// <returns>AK_Success if succeeded, or AK_IDNotFound if the game object was not registered.</returns>
		AKRESULT GetSwitch(AkSwitchGroupID in_switchGroup, AkGameObjectID in_gameObjectID, AkSwitchStateID* out_rSwitchState) {
			tQuery_GetSwitch_SwitchGroupID func = (tQuery_GetSwitch_SwitchGroupID)Wwise::Exports::func_Wwise_Sound_Query_GetSwitch_SwitchGroupID;
			return func(in_switchGroup, in_gameObjectID, out_rSwitchState);
		}

		/// <summary>
		/// Get the state of a switch group.
		/// </summary>
		/// <param name="in_pstrSwitchGroupName"> - String name of the switch group</param>
		/// <param name="in_GameObj"> - Associated game object ID</param>
		/// <param name="out_rSwitchState"> - ID of the switch</param>
		/// <returns>AK_Success if succeeded, or AK_IDNotFound if the game object was not registered or the switch group name can not be found</returns>
		AKRESULT GetSwitch(const char* in_pstrSwitchGroupName, AkGameObjectID in_GameObj, AkSwitchStateID* out_rSwitchState) {
			tQuery_GetSwitch_Char func = (tQuery_GetSwitch_Char)Wwise::Exports::func_Wwise_Sound_Query_GetSwitch_Char;
			return func(in_pstrSwitchGroupName, in_GameObj, out_rSwitchState);
		}

		/// <summary>
		/// Get the list of audio object IDs associated to an event.
		/// </summary>
		/// <param name="in_eventID"> - Event ID</param>
		/// <param name="io_ruNumItems"> - Number of items in array provided / Number of items filled in array</param>
		/// <param name="out_aObjectInfos"> - Array of AkObjectInfo items to fill</param>
		/// <returns>AK_Success if succeeded, AK_IDNotFound if the eventID cannot be found, AK_InvalidParameter if out_aObjectInfos is NULL while io_ruNumItems > 0 or AK_UnknownObject if the event contains an unknown audio object, or AK_PartialSuccess if io_ruNumItems was set to 0 to query the number of available items.</returns>
		AKRESULT QueryAudioObjectIDs(AkUniqueID in_eventID, AkUInt32* io_ruNumItems, AkObjectInfo* out_aObjectInfos) {
			tQuery_QueryAudioObjectIDs_UniqueID func = (tQuery_QueryAudioObjectIDs_UniqueID)Wwise::Exports::func_Wwise_Sound_Query_QueryAudioObjectIDs_UniqueID;
			return func(in_eventID, io_ruNumItems, out_aObjectInfos);
		}

		/// <summary>
		/// Get the list of audio object IDs associated to an event name.
		/// </summary>
		/// <param name="in_pszEventName"> - Event name</param>
		/// <param name="io_ruNumItems"> - Number of items in array provided / Number of items filled in array</param>
		/// <param name="out_aObjectInfos"> - Array of AkObjectInfo items to fill</param>
		/// <returns>AK_Success if succeeded, AK_IDNotFound if the event name cannot be found, AK_InvalidParameter if out_aObjectInfos is NULL while io_ruNumItems > 0 or AK_UnknownObject if the event contains an unknown audio object, or AK_PartialSuccess if io_ruNumItems was set to 0 to query the number of available items.</returns>
		AKRESULT QueryAudioObjectIDs(const char* in_pszEventName, AkUInt32* io_ruNumItems, AkObjectInfo* out_aObjectInfos) {
			tQuery_QueryAudioObjectIDs_Char func = (tQuery_QueryAudioObjectIDs_Char)Wwise::Exports::func_Wwise_Sound_Query_QueryAudioObjectIDs_Char;
			return func(in_pszEventName, io_ruNumItems, out_aObjectInfos);
		}
	}

	/// <summary>
	/// Register a codec type with the sound engine and set the callback functions to create the codec's file source and bank source nodes.
	/// </summary>
	/// <param name="in_ulCompanyID"> - Company identifier (as declared in the plug-in description XML file)</param>
	/// <param name="in_ulCodecID"> - Codec identifier (as declared in the plug-in description XML file)</param>
	/// <param name="in_pFileCreateFunc"> - Pointer to the codec's file source node creation function</param>
	/// <param name="in_pBankCreateFunc"> - Pointer to the codec's bank source node creation function</param>
	/// <returns>AK_Success if successful, AK_InvalidParameter if invalid parameters were provided, or Ak_Fail otherwise.</returns>
	AKRESULT RegisterCodec(AkUInt32 in_ulCompanyID, AkUInt32 in_ulCodecID, AkCreateFileSourceCallback in_pFileCreateFunc, AkCreateBankSourceCallback in_pBankCreateFunc) {
		tRegisterCodec func = (tRegisterCodec)Wwise::Exports::func_Wwise_Sound_RegisterCodec;
		return func(in_ulCompanyID, in_ulCodecID, in_pFileCreateFunc, in_pBankCreateFunc);
	}

	/// <summary>
	/// Register a game object.
	/// </summary>
	/// <param name="in_gameObjectID"> - ID of the game object to be registered</param>
	/// <returns>AK_Success if successful. AK_Fail if the specified AkGameObjectID is invalid(0 and -1 are invalid) </returns>
	AKRESULT RegisterGameObj(AkGameObjectID in_gameObjectID) {
		tRegisterGameObj func = (tRegisterGameObj)Wwise::Exports::func_Wwise_Sound_RegisterGameObj;
		return func(in_gameObjectID);
	}

	/// <summary>
	/// Register a global callback function.
	/// </summary>
	/// <param name="in_pCallback"> - Function to register as a global callback.</param>
	/// <returns></returns>
	AKRESULT RegisterGlobalCallback(AkGlobalCallbackFunc in_pCallback) {
		tRegisterGlobalCallback func = (tRegisterGlobalCallback)Wwise::Exports::func_Wwise_Sound_RegisterGlobalCallback;
		return func(in_pCallback);
	}

	/// <summary>
	/// Register a plug-in with the sound engine and set the callback functions to create the plug-in and its parameter node.
	/// </summary>
	/// <param name="in_eType"> - Plug-in type (for example, source or effect)</param>
	/// <param name="in_ulCompanyID"> - Company identifier (as declared in the plug-in description XML file)</param>
	/// <param name="in_ulPluginID"> - Plug-in identifier (as declared in the plug-in description XML file)</param>
	/// <param name="in_pCreateFunc"> - Pointer to the plug-in's creation function</param>
	/// <param name="in_pCreateParamFunc"> - Pointer to the plug-in's parameter node creation function</param>
	/// <returns>AK_Success if successful, AK_InvalidParameter if invalid parameters were provided or Ak_Fail otherwise.</returns>
	AKRESULT RegisterPlugin(AkPluginType in_eType, AkUInt32 in_ulCompanyID, AkUInt32 in_ulPluginID, AkCreatePluginCallback in_pCreateFunc, AkCreateParamCallback in_pCreateParamFunc) {
		tRegisterPlugin func = (tRegisterPlugin)Wwise::Exports::func_Wwise_Sound_RegisterPlugin;
		return func(in_eType, in_ulCompanyID, in_ulPluginID, in_pCreateFunc, in_pCreateParamFunc);
	}

	/// <summary>
	/// Process all events in the sound engine's queue. This method has to be called periodically (usually once per game frame).
	/// </summary>
	/// <returns></returns>
	AKRESULT RenderAudio() {
		tRenderAudio func = (tRenderAudio)Wwise::Exports::func_Wwise_Sound_RenderAudio;
		return func();
	}

	/// <summary>
	/// Reset the value of the game parameter to its default value, as specified in the Wwise project.
	/// </summary>
	/// <param name="in_rtpcID"> - ID of the game parameter</param>
	/// <param name="in_gameObjectID"> - Associated game object ID</param>
	/// <param name="in_uValueChangeDuration"> - Duration during which the game parameter is interpolated towards its default value</param>
	/// <param name="in_curveInterpolation"> - Curve type to be used for the game parameter interpolation</param>
	/// <returns>Always AK_Success</returns>
	AKRESULT ResetRTPCValue(AkRtpcID in_rtpcID, AkGameObjectID in_gameObjectID, AkTimeMs in_uValueChangeDuration, AkCurveInterpolation in_curveInterpolation) {
		tResetRTPCValue_RTPCID func = (tResetRTPCValue_RTPCID)Wwise::Exports::func_Wwise_Sound_ResetRTPCValue_RTPCID;
		return func(in_rtpcID, in_gameObjectID, in_uValueChangeDuration, in_curveInterpolation);
	}

	/// <summary>
	/// Reset the value of the game parameter to its default value, as specified in the Wwise project.
	/// </summary>
	/// <param name="in_pszRtpcName"> - Name of the game parameter</param>
	/// <param name="in_gameObjectID"> - Associated game object ID</param>
	/// <param name="in_uValueChangeDuration"> - Duration during which the game parameter is interpolated towards its default value</param>
	/// <param name="in_curveInterpolation"> - Curve type to be used for the game parameter interpolation</param>
	/// <returns>AK_Success if successful. AK_IDNotFound if in_pszParamName is NULL.</returns>
	AKRESULT ResetRTPCValue(const char* in_pszRtpcName, AkGameObjectID in_gameObjectID, AkTimeMs in_uValueChangeDuration, AkCurveInterpolation in_curveInterpolation) {
		tResetRTPCValue_Char func = (tResetRTPCValue_Char)Wwise::Exports::func_Wwise_Sound_ResetRTPCValue_Char;
		return func(in_pszRtpcName, in_gameObjectID, in_uValueChangeDuration, in_curveInterpolation);
	}

	/// <summary>
	/// Seek inside all playing objects that are referenced in play actions of the specified event.
	/// </summary>
	/// <param name="in_eventID"> - Unique ID of the event</param>
	/// <param name="in_gameObjectID"> - Associated game object ID; use AK_INVALID_GAME_OBJECT to affect all game objects</param>
	/// <param name="in_iPosition"> - Desired position where playback should restart, in milliseconds</param>
	/// <param name="in_bSeekToNearestMarker"> - If true, the final seeking position will be made equal to the nearest marker (see note above)</param>
	/// <returns></returns>
	AKRESULT SeekOnEvent(AkUniqueID in_eventID, AkGameObjectID in_gameObjectID, AkTimeMs in_iPosition, bool in_bSeekToNearestMarker) {
		tSeekOnEvent_UniqueID_Int32 func = (tSeekOnEvent_UniqueID_Int32)Wwise::Exports::func_Wwise_Sound_SeekOnEvent_UniqueID_Int32;
		return func(in_eventID, in_gameObjectID, in_iPosition, in_bSeekToNearestMarker);
	}

	/// <summary>
	/// Seek inside all playing objects that are referenced in play actions of the specified event.
	/// </summary>
	/// <param name="in_pszEventName"> - Name of the event</param>
	/// <param name="in_gameObjectID"> - Associated game object ID; use AK_INVALID_GAME_OBJECT to affect all game objects</param>
	/// <param name="in_iPosition"> - Desired position where playback should restart, in milliseconds</param>
	/// <param name="in_bSeekToNearestMarker"> - If true, the final seeking position will be made equal to the nearest marker (see note above)</param>
	/// <returns></returns>
	AKRESULT SeekOnEvent(const char* in_pszEventName, AkGameObjectID in_gameObjectID, AkTimeMs in_iPosition, bool in_bSeekToNearestMarker) {
		tSeekOnEvent_Char_Int32 func = (tSeekOnEvent_Char_Int32)Wwise::Exports::func_Wwise_Sound_SeekOnEvent_Char_Int32;
		return func(in_pszEventName, in_gameObjectID, in_iPosition, in_bSeekToNearestMarker);
	}

	/// <summary>
	/// Seek inside all playing objects that are referenced in play actions of the specified event.
	/// </summary>
	/// <param name="in_eventID"> - Unique ID of the event</param>
	/// <param name="in_gameObjectID"> - Associated game object ID; use AK_INVALID_GAME_OBJECT to affect all game objects</param>
	/// <param name="in_iPosition"> - Desired position where playback should restart, expressed in a percentage of the file's total duration, between 0 and 1.f (see note above about infinite looping sounds)</param>
	/// <param name="in_bSeekToNearestMarker"> - If true, the final seeking position will be made equal to the nearest marker (see note above)</param>
	/// <returns></returns>
	AKRESULT SeekOnEvent(AkUniqueID in_eventID, AkGameObjectID in_gameObjectID, AkReal32 in_iPosition, bool in_bSeekToNearestMarker) {
		tSeekOnEvent_UniqueID_Float func = (tSeekOnEvent_UniqueID_Float)Wwise::Exports::func_Wwise_Sound_SeekOnEvent_UniqueID_Float;
		return func(in_eventID, in_gameObjectID, in_iPosition, in_bSeekToNearestMarker);
	}

	/// <summary>
	/// Seek inside all playing objects that are referenced in play actions of the specified event.
	/// </summary>
	/// <param name="in_pszEventName"> - Name of the event</param>
	/// <param name="in_gameObjectID"> - Associated game object ID; use AK_INVALID_GAME_OBJECT to affect all game objects</param>
	/// <param name="in_iPosition"> - Desired position where playback should restart, expressed in a percentage of the file's total duration, between 0 and 1.f (see note above about infinite looping sounds)</param>
	/// <param name="in_bSeekToNearestMarker"> - If true, the final seeking position will be made equal to the nearest marker (see notes above).</param>
	/// <returns></returns>
	AKRESULT SeekOnEvent(const char* in_pszEventName, AkGameObjectID in_gameObjectID, AkReal32 in_iPosition, bool in_bSeekToNearestMarker) {
		tSeekOnEvent_Char_Float func = (tSeekOnEvent_Char_Float)Wwise::Exports::func_Wwise_Sound_SeekOnEvent_Char_Float;
		return func(in_pszEventName, in_gameObjectID, in_iPosition, in_bSeekToNearestMarker);
	}

	/// <summary>
	/// Set a game object's active listeners. By default, all new game objects only have the listener 0 active. Inactive listeners are not computed.
	/// </summary>
	/// <param name="in_gameObjectID"> - Game object identifier</param>
	/// <param name="in_uiListenerMask"> - Bitmask representing the active listeners (LSB = Listener 0, set to 1 means active)</param>
	/// <returns>Always returns AK_Success</returns>
	AKRESULT SetActiveListeners(AkGameObjectID in_gameObjectID, AkUInt32 in_uiListenerMask) {
		tSetActiveListeners func = (tSetActiveListeners)Wwise::Exports::func_Wwise_Sound_SetActiveListeners;
		return func(in_gameObjectID, in_uiListenerMask);
	}

	/// <summary>
	/// Set an effect shareset at the specified audio node and effect slot index.
	/// </summary>
	/// <param name="in_audioNodeID"> - Can be a member of the actor-mixer or interactive music hierarchy (not a bus)</param>
	/// <param name="in_uFXIndex"> - Effect slot index (0-3)</param>
	/// <param name="in_shareSetID"> - ShareSet ID; pass AK_INVALID_UNIQUE_ID to clear the effect slot</param>
	/// <returns>Always returns AK_Success</returns>
	AKRESULT SetActorMixerEffect(AkUniqueID in_audioNodeID, AkUInt32 in_uFXIndex, AkUniqueID in_shareSetID) {
		tSetActorMixerEffect func = (tSetActorMixerEffect)Wwise::Exports::func_Wwise_Sound_SetActorMixerEffect;
		return func(in_audioNodeID, in_uFXIndex, in_shareSetID);
	}

	/// <summary>
	/// Set the scaling factor of a game object.
	/// </summary>
	/// <param name="in_GameObjectID"> - Game object identifier</param>
	/// <param name="in_fAttenuationScalingFactor"> - Scaling Factor, 1 means 100%, 0.5 means 50%, 2 means 200%, and so on.</param>
	/// <returns>AK_Success when successful. AK_InvalidParameter if the scaling factor specified was 0 or negative.</returns>
	AKRESULT SetAttenuationScalingFactor(AkGameObjectID in_GameObjectID, AkReal32 in_fAttenuationScalingFactor) {
		tSetAttenuationScalingFactor func = (tSetAttenuationScalingFactor)Wwise::Exports::func_Wwise_Sound_SetAttenuationScalingFactor;
		return func(in_GameObjectID, in_fAttenuationScalingFactor);
	}

	/// <summary>
	/// Set the I/O settings of the bank load and prepare event processes.
	/// </summary>
	/// <param name="in_fThroughput"> - Average throughput of bank data streaming (bytes/ms) (the default value is AK_DEFAULT_BANK_THROUGHPUT)</param>
	/// <param name="in_priority"> - Priority of bank streaming (the default value is AK_DEFAULT_PRIORITY)</param>
	/// <returns>AK_Success if successful. AK_Fail if the sound engine was not correctly initialized. AK_InvalidParameter if some parameters are invalid.</returns>
	AKRESULT SetBankLoadIOSettings(AkReal32 in_fThroughput, AkPriority in_priority) {
		tSetBankLoadIOSettings func = (tSetBankLoadIOSettings)Wwise::Exports::func_Wwise_Sound_SetBankLoadIOSettings;
		return func(in_fThroughput, in_priority);
	}

	/// <summary>
	/// Set an effect shareset at the specified bus and effect slot index.
	/// </summary>
	/// <param name="in_audioNodeID"> - Bus Short ID.</param>
	/// <param name="in_uFXIndex"> - Effect slot index (0-3)</param>
	/// <param name="in_shareSetID"> - ShareSet ID; pass AK_INVALID_UNIQUE_ID to clear the effect slot</param>
	/// <returns>Always returns AK_Success</returns>
	AKRESULT SetBusEffect(AkUniqueID in_audioNodeID, AkUInt32 in_uFXIndex, AkUniqueID in_shareSetID) {
		tSetBusEffect_UniqueID func = (tSetBusEffect_UniqueID)Wwise::Exports::func_Wwise_Sound_SetBusEffect_UniqueID;
		return func(in_audioNodeID, in_uFXIndex, in_shareSetID);
	}

	/// <summary>
	/// Set an effect shareset at the specified bus and effect slot index.
	/// </summary>
	/// <param name="in_pszBusName"> - Bus name</param>
	/// <param name="in_uFXIndex"> - Effect slot index (0-3)</param>
	/// <param name="in_shareSetID"> - ShareSet ID; pass AK_INVALID_UNIQUE_ID to clear the effect slot.</param>
	/// <returns>AK_IDNotFound is name not resolved, returns AK_Success otherwise.</returns>
	AKRESULT SetBusEffect(const char* in_pszBusName, AkUInt32 in_uFXIndex, AkUniqueID in_shareSetID) {
		tSetBusEffect_Char func = (tSetBusEffect_Char)Wwise::Exports::func_Wwise_Sound_SetBusEffect_Char;
		return func(in_pszBusName, in_uFXIndex, in_shareSetID);
	}

	/// <summary>
	/// Undocumented.
	/// </summary>
	/// <param name="param_1"></param>
	/// <param name="param_2"></param>
	/// <param name="in_pCookie"></param>
	/// <returns></returns>
	AKRESULT SetEffectParam(AkUInt32 param_1, AkUInt16 param_2, void* in_pCookie) {
		tSetEffectParam func = (tSetEffectParam)Wwise::Exports::func_Wwise_Sound_SetEffectParam;
		return func(param_1, param_2, in_pCookie);
	}

	/// <summary>
	/// Set the auxiliary busses to route the specified game object.
	/// </summary>
	/// <param name="in_gameObjectID"> - Associated game object ID</param>
	/// <param name="in_aAuxSendValues"> - Variable-size array of AkAuxSendValue structures (it may be NULL if no environment must be set, and its size cannot exceed AK_MAX_AUX_PER_OBJ)</param>
	/// <param name="in_uNumSendValues"> - The number of auxiliary busses at the pointer's address (it must be 0 if no environment is set, and can not exceed AK_MAX_AUX_PER_OBJ)</param>
	/// <returns>AK_Success if successful. AK_InvalidParameter if the array size exceeds AK_MAX_AUX_PER_OBJ, or if a duplicated environment is found in the array.</returns>
	AKRESULT SetGameObjectAuxSendValues(AkGameObjectID in_gameObjectID, AkAuxSendValue* in_aAuxSendValues, AkUInt32 in_uNumSendValues) {
		tSetGameObjectAuxSendValues func = (tSetGameObjectAuxSendValues)Wwise::Exports::func_Wwise_Sound_SetGameObjectAuxSendValues;
		return func(in_gameObjectID, in_aAuxSendValues, in_uNumSendValues);
	}

	/// <summary>
	/// Set the output bus volume (direct) to be used for the specified game object. The control value is a number ranging from 0.0f to 1.0f.
	/// </summary>
	/// <param name="in_gameObjectID"> - Associated game object ID</param>
	/// <param name="in_fControlValue"> - Dry level control value, ranging from 0.0f to 1.0f (0.0f stands for 0% dry, while 1.0f stands for 100% dry)</param>
	/// <returns></returns>
	AKRESULT SetGameObjectOutputBusVolume(AkGameObjectID in_gameObjectID, AkReal32 in_fControlValue) {
		tSetGameObjectOutputBusVolume func = (tSetGameObjectOutputBusVolume)Wwise::Exports::func_Wwise_Sound_SetGameObjectOutputBusVolume;
		return func(in_gameObjectID, in_fControlValue);
	}

	/// <summary>
	/// Set a listener's ability to listen to audio and motion events. By default, all listeners are enabled for audio and disabled for motion.
	/// </summary>
	/// <param name="in_uIndex"> - Listener index (0: first listener, 7: 8th listener)</param>
	/// <param name="in_bAudio"> - True=Listens to audio events (by default it is true)</param>
	/// <param name="in_bMotion"> - True=Listens to motion events (by default it is false)</param>
	/// <returns>Always returns AK_Success</returns>
	AKRESULT SetListenerPipeline(AkUInt32 in_uIndex, bool in_bAudio, bool in_bMotion) {
		tSetListenerPipeline func = (tSetListenerPipeline)Wwise::Exports::func_Wwise_Sound_SetListenerPipeline;
		return func(in_uIndex, in_bAudio, in_bMotion);
	}

	/// <summary>
	/// Set a listener's position.
	/// </summary>
	/// <param name="in_rPosition"> - Position to set</param>
	/// <param name="in_uiIndex"> - Listener index (0: first listener, 7: 8th listener)</param>
	/// <returns>Always returns AK_Success</returns>
	AKRESULT SetListenerPosition(const AkListenerPosition* in_rPosition, AkUInt32 in_uiIndex) {
		tSetListenerPosition func = (tSetListenerPosition)Wwise::Exports::func_Wwise_Sound_SetListenerPosition;
		return func(in_rPosition, in_uiIndex);
	}

	/// <summary>
	/// Set the scaling factor for a listener. A larger factor means that the listener will hear sounds with less attenuation based on distance.
	/// </summary>
	/// <param name="in_uiIndex"></param>
	/// <param name="in_fAttenuationScalingFactor"></param>
	/// <returns>AK_Success when successful. AK_InvalidParameter if the scaling factor specified was 0 or negative.</returns>
	AKRESULT SetListenerScalingFactor(AkUInt32 in_uiIndex, AkReal32 in_fAttenuationScalingFactor) {
		tSetListenerScalingFactor func = (tSetListenerScalingFactor)Wwise::Exports::func_Wwise_Sound_SetListenerScalingFactor;
		return func(in_uiIndex, in_fAttenuationScalingFactor);
	}

	/// <summary>
	/// Set a listener's spatialization parameters.
	/// </summary>
	/// <param name="in_uIndex"> - Listener index (0: first listener, 7: 8th listener)</param>
	/// <param name="in_bSpatialized"> - Spatialization toggle (True : enable spatialization, False : disable spatialization)</param>
	/// <param name="in_channelConfig"> - Channel configuration associated with volumes in_pVolumeOffsets. Ignored if in_pVolumeOffsets is NULL.</param>
	/// <param name="in_pVolumeOffsets"> - Per-speaker volume offset, in dB. See AkSpeakerVolumes.h for how to manipulate this vector.</param>
	/// <returns>AK_Success if message was successfully posted to sound engine queue, AK_Fail otherwise.</returns>
	AKRESULT SetListenerSpatialization(AkUInt32 in_uIndex, bool in_bSpatialized, AkChannelConfig in_channelConfig, VectorPtr in_pVolumeOffsets) {
		tSetListenerSpatialization func = (tSetListenerSpatialization)Wwise::Exports::func_Wwise_Sound_SetListenerSpatialization;
		return func(in_uIndex, in_bSpatialized, in_channelConfig, in_pVolumeOffsets);
	}

	/// <summary>
	/// Allows the game to set the maximum number of non virtual voices to be played simultaneously.
	/// </summary>
	/// <param name="in_maxNumberVoices"> - Maximun number of non-virtual voices.</param>
	/// <returns>AK_Success if successful. AK_InvalidParameter if the threshold was not between 1 and MaxUInt16.</returns>
	AKRESULT SetMaxNumVoicesLimit(AkUInt16 in_maxNumberVoices) {
		tSetMaxNumVoicesLimit func = (tSetMaxNumVoicesLimit)Wwise::Exports::func_Wwise_Sound_SetMaxNumVoicesLimit;
		return func(in_maxNumberVoices);
	}

	/// <summary>
	/// Set multiple positions to a single game object.
	/// </summary>
	/// <param name="in_GameObjectID"> - Game object identifier.</param>
	/// <param name="in_pPositions"> - Array of positions to apply.</param>
	/// <param name="in_NumPositions"> - Number of positions specified in the provided array.</param>
	/// <param name="in_eMultiPositionType"> - AK::SoundEngine::MultiPositionType</param>
	/// <returns>AK_Success when successful. AK_InvalidParameter if parameters are not valid.</returns>
	AKRESULT SetMultiplePositions(AkGameObjectID in_GameObjectID, const AkSoundPosition* in_pPositions, AkUInt16 in_NumPositions, MultiPositionType in_eMultiPositionType) {
		tSetMultiplePositions func = (tSetMultiplePositions)Wwise::Exports::func_Wwise_Sound_SetMultiplePositions;
		return func(in_GameObjectID, in_pPositions, in_NumPositions, in_eMultiPositionType);
	}

	/// <summary>
	/// Set a game object's obstruction and occlusion levels. This method is used to affect how an object should be heard by a specific listener.
	/// </summary>
	/// <param name="in_ObjectID"> - Associated game object ID</param>
	/// <param name="in_uListener"> - Listener index (0: first listener, 7: 8th listener)</param>
	/// <param name="in_fObstructionLevel"> - ObstructionLevel: [0.0f..1.0f]</param>
	/// <param name="in_fOcclusionLevel"> - OcclusionLevel: [0.0f..1.0f]</param>
	/// <returns>Always returns AK_Success</returns>
	AKRESULT SetObjectObstructionAndOcclusion(AkGameObjectID in_ObjectID, AkUInt32 in_uListener, AkReal32 in_fObstructionLevel, AkReal32 in_fOcclusionLevel) {
		tSetObjectObstructionAndOcclusion func = (tSetObjectObstructionAndOcclusion)Wwise::Exports::func_Wwise_Sound_SetObjectObstructionAndOcclusion;
		return func(in_ObjectID, in_uListener, in_fObstructionLevel, in_fOcclusionLevel);
	}

	/// <summary>
	/// Set the panning rule of the specified output.
	/// </summary>
	/// <param name="in_ePanningRule"> - Panning rule.</param>
	/// <param name="in_eSinkType"> - Output sink type. Pass AkOutput_Main for main output.</param>
	/// <param name="in_iOutputID"> - Player number or device-unique identifier. Pass 0 for main.</param>
	/// <returns></returns>
	AKRESULT SetPanningRule(AkPanningRule in_ePanningRule, AkAudioOutputType in_eSinkType, AkUInt32 in_iOutputID) {
		tSetPanningRule func = (tSetPanningRule)Wwise::Exports::func_Wwise_Sound_SetPanningRule;
		return func(in_ePanningRule, in_eSinkType, in_iOutputID);
	}

	/// <summary>
	/// Set the position of a game object.
	/// </summary>
	/// <param name="in_GameObjectID"> - Game object identifier</param>
	/// <param name="in_Position"> - Position to set; in_Position.Orientation must be normalized.</param>
	/// <returns>AK_Success when successful. AK_InvalidParameter if parameters are not valid.</returns>
	AKRESULT SetPosition(AkGameObjectID in_GameObjectID, const AkSoundPosition* in_Position) {
		tSetPosition func = (tSetPosition)Wwise::Exports::func_Wwise_Sound_SetPosition;
		return func(in_GameObjectID, in_Position);
	}

	/// <summary>
	/// Undocumented. Set the position of a game object.
	/// </summary>
	/// <param name="in_GameObjectID"> - Game object identifier</param>
	/// <param name="in_soundPosition"> - Position to set; in_Position.Orientation must be normalized.</param>
	/// <returns>AK_Success when successful. AK_InvalidParameter if parameters are not valid.</returns>
	AKRESULT SetPositionInternal(AkGameObjectID in_GameObjectID, AkSoundPosition* in_soundPosition) {
		tSetPositionInternal func = (tSetPositionInternal)Wwise::Exports::func_Wwise_Sound_SetPositionInternal;
		return func(in_GameObjectID, in_soundPosition);
	}

	/// <summary>
	/// Set the value of a real-time parameter control (by ID).
	/// </summary>
	/// <param name="in_rtpcID"> - ID of the game parameter</param>
	/// <param name="in_value"> - Value to set</param>
	/// <param name="in_gameObjectID"> - Associated game object ID</param>
	/// <param name="in_uValueChangeDuration"> - Duration during which the game parameter is interpolated towards in_value</param>
	/// <param name="in_eFadeCruve"> - Curve type to be used for the game parameter interpolation</param>
	/// <param name="in_bBypassInternalValueInterpolation"> - True if you want to bypass the internal "slew rate" or "over time filtering" specified by the sound designer. This is meant to be used when for example loading a level and you dont want the values to interpolate.</param>
	/// <returns>Always AK_Success</returns>
	AKRESULT SetRTPCValue(AkRtpcID in_rtpcID, AkRtpcValue in_value, AkGameObjectID in_gameObjectID, AkTimeMs in_uValueChangeDuration, AkCurveInterpolation in_eFadeCruve) {
		tSetRTPCValue_RTPCID func = (tSetRTPCValue_RTPCID)Wwise::Exports::func_Wwise_Sound_SetRTPCValue_RTPCID;
		return func(in_rtpcID, in_value, in_gameObjectID, in_uValueChangeDuration, in_eFadeCruve);
	}

	/// <summary>
	/// Set the value of a real-time parameter control.
	/// </summary>
	/// <param name="in_pszRtpcName"> - Name of the game parameter</param>
	/// <param name="in_value"> - Value to set</param>
	/// <param name="in_gameObjectID"> - Associated game object ID</param>
	/// <param name="in_uValueChangeDuration"> - Duration during which the game parameter is interpolated towards in_value</param>
	/// <param name="in_eFadeCurve"> - Curve type to be used for the game parameter interpolation</param>
	/// <returns>AK_Success if successful. AK_IDNotFound if in_pszRtpcName is NULL.</returns>
	AKRESULT SetRTPCValue(const char* in_pszRtpcName, AkRtpcValue in_value, AkGameObjectID in_gameObjectID, AkTimeMs in_uValueChangeDuration, AkCurveInterpolation in_eFadeCurve) {
		tSetRTPCValue_Char func = (tSetRTPCValue_Char)Wwise::Exports::func_Wwise_Sound_SetRTPCValue_Char;
		return func(in_pszRtpcName, in_value, in_gameObjectID, in_uValueChangeDuration, in_eFadeCurve);
	}

	/// <summary>
	/// Set the state of a state group (by IDs).
	/// </summary>
	/// <param name="in_stateGroup"> - ID of the state group</param>
	/// <param name="in_state"> - ID of the state</param>
	/// <returns>Always returns AK_Success</returns>
	AKRESULT SetState(AkStateGroupID in_stateGroup, AkStateID in_state) {
		tSetState_StateGroupID func = (tSetState_StateGroupID)Wwise::Exports::func_Wwise_Sound_SetState_StateGroupID;
		return func(in_stateGroup, in_state);
	}

	/// <summary>
	/// Set the state of a state group.
	/// </summary>
	/// <param name="in_pszStateGroup"> - Name of the state group</param>
	/// <param name="in_pszState"> - Name of the state</param>
	/// <returns>AK_Success if successful. AK_IDNotFound if the state or state group name was not resolved to an existing ID.</returns>
	AKRESULT SetState(const char* in_pszStateGroup, const char* in_pszState) {
		tSetState_Char func = (tSetState_Char)Wwise::Exports::func_Wwise_Sound_SetState_Char;
		return func(in_pszStateGroup, in_pszState);
	}

	/// <summary>
	/// Set the state of a switch group (by IDs).
	/// </summary>
	/// <param name="in_switchGroup"> - ID of the switch group</param>
	/// <param name="in_switchState"> - ID of the switch</param>
	/// <param name="in_gameObjectID"> - Associated game object ID</param>
	/// <returns>Always returns AK_Success</returns>
	AKRESULT SetSwitch(AkSwitchGroupID in_switchGroup, AkSwitchStateID in_switchState, AkGameObjectID in_gameObjectID) {
		tSetSwitch_SwitchGroupID func = (tSetSwitch_SwitchGroupID)Wwise::Exports::func_Wwise_Sound_SetSwitch_SwitchGroupID;
		return func(in_switchGroup, in_switchState, in_gameObjectID);
	}

	/// <summary>
	/// Set the state of a switch group.
	/// </summary>
	/// <param name="in_pszSwitchGroup"> - Name of the switch group</param>
	/// <param name="in_pszSwitchState"> - Name of the switch</param>
	/// <param name="in_gameObjectID"> - Associated game object ID</param>
	/// <returns>AK_Success if successful. AK_IDNotFound if the switch or switch group name was not resolved to an existing ID.</returns>
	AKRESULT SetSwitch(const char* in_pszSwitchGroup, const char* in_pszSwitchState, AkGameObjectID in_gameObjectID) {
		tSetSwitch_Char func = (tSetSwitch_Char)Wwise::Exports::func_Wwise_Sound_SetSwitch_Char;
		return func(in_pszSwitchGroup, in_pszSwitchState, in_gameObjectID);
	}

	/// <summary>
	/// Allows the game to set the volume threshold to be used by the sound engine to determine if a voice must go virtual.
	/// </summary>
	/// <param name="in_fVolumeThresholdDB"> - Volume Threshold, must be a value between 0 and -96.3 dB.</param>
	/// <returns>AK_Success if successful. AK_InvalidParameter if the threshold was not between 0 and -96.3 dB.</returns>
	AKRESULT SetVolumeThreshold(AkReal32 in_fVolumeThresholdDB) {
		tSetVolumeThreshold func = (tSetVolumeThreshold)Wwise::Exports::func_Wwise_Sound_SetVolumeThreshold;
		return func(in_fVolumeThresholdDB);
	}

	/// <summary>
	/// Start recording the sound engine audio output.
	/// </summary>
	/// <param name="in_CaptureFileName"> - Name of the output capture file</param>
	/// <returns>AK_Success if successful, AK_Fail if there was a problem starting the output capture.</returns>
	AKRESULT StartOutputCapture(const AkOSChar* in_CaptureFileName) {
		tStartOutputCapture func = (tStartOutputCapture)Wwise::Exports::func_Wwise_Sound_StartOutputCapture;
		return func(in_CaptureFileName);
	}

	/// <summary>
	/// Stop the current content playing associated to the specified game object ID. If no game object is specified, all sounds will be stopped.
	/// </summary>
	/// <param name="in_gameObjectID"> - (Optional)Specify a game object to stop only playback associated to the provided game object ID.</param>
	void StopAll(AkGameObjectID in_gameObjectID) {
		tStopAll func = (tStopAll)Wwise::Exports::func_Wwise_Sound_StopAll;
		return func(in_gameObjectID);
	}

	/// <summary>
	/// Stop recording the sound engine audio output.
	/// </summary>
	/// <returns>AK_Success if successful, AK_Fail if there was a problem stopping the output capture.</returns>
	AKRESULT StopOutputCapture() {
		tStopOutputCapture func = (tStopOutputCapture)Wwise::Exports::func_Wwise_Sound_StopOutputCapture;
		return func();
	}

	/// <summary>
	/// Stop the current content playing associated to the specified playing ID.
	/// </summary>
	/// <param name="in_playingID"> - Playing ID to be stopped.</param>
	/// <param name="in_uTransitionDuration"> - Fade duration</param>
	/// <param name="in_eFadeCurve"> - Curve type to be used for the transition</param>
	/// <returns></returns>
	AKRESULT StopPlayingID(AkPlayingID in_playingID, AkTimeMs in_uTransitionDuration, AkCurveInterpolation in_eFadeCurve) {
		tStopPlayingID func = (tStopPlayingID)Wwise::Exports::func_Wwise_Sound_StopPlayingID;
		return func(in_playingID, in_uTransitionDuration, in_eFadeCurve);
	}

	/// <summary>
	/// Undocumented.
	/// </summary>
	/// <param name="param_1"></param>
	/// <param name="param_2"></param>
	/// <param name="param_3"></param>
	/// <returns></returns>
	AKRESULT StopSourcePlugin(AkUInt32 param_1, AkUInt32 param_2, AkUInt32 param_3) {
		tStopSourcePlugin func = (tStopSourcePlugin)Wwise::Exports::func_Wwise_Sound_StopSourcePlugin;
		return func(param_1, param_2, param_3);
	}

	/// <summary>
	/// Terminate the sound engine. If some sounds are still playing or events are still being processed when this function is called, they will be stopped.
	/// </summary>
	void Term() {
		tTerm func = (tTerm)Wwise::Exports::func_Wwise_Sound_Term;
		return func();
	}

	/// <summary>
	/// Unload a bank synchronously (by ID and memory pointer).
	/// </summary>
	/// <param name="in_bankID"> - ID of the bank to unload</param>
	/// <param name="in_pInMemoryBankPtr"> - Memory pointer from where the bank was initially loaded from. (REQUIRED to determine which bank associated to a memory pointer must be unloaded). Pass NULL only if NULL was passed when loading the bank.</param>
	/// <param name="out_pMemPoolId"> - Returned memory pool ID used with LoadBank() (can pass NULL)</param>
	/// <returns>AK_Success if successful, AK_Fail otherwise. AK_Success is returned when the bank was not loaded.</returns>
	AKRESULT UnloadBank(AkBankID in_bankID, const void* in_pInMemoryBankPtr, AkMemPoolId* out_pMemPoolId) {
		tUnloadBank_BankID_MemPoolID func = (tUnloadBank_BankID_MemPoolID)Wwise::Exports::func_Wwise_Sound_UnloadBank_BankID_MemPoolID;
		return func(in_bankID, in_pInMemoryBankPtr, out_pMemPoolId);
	}

	/// <summary>
	/// Unload a bank asynchronously (by ID and memory pointer).
	/// </summary>
	/// <param name="in_bankID"> - ID of the bank to unload</param>
	/// <param name="in_pInMemoryBankPtr"> - Memory pointer from where the bank was initially loaded from. (REQUIRED to determine which bank associated toa memory pointer must be unloaded). Pass NULL only if NULL was passed when loading the bank.</param>
	/// <param name="in_pfnBankCallback"> - Callback function</param>
	/// <param name="in_pCookie"> - Callback cookie (reserved to user, passed to the callback function)</param>
	/// <returns>AK_Success if scheduling successful (use a callback to be notified when completed)</returns>
	AKRESULT UnloadBank(AkBankID in_bankID, const void* in_pInMemoryBankPtr, AkBankCallbackFunc in_pfnBankCallback, void* in_pCookie) {
		tUnloadBank_BankID_Callback func = (tUnloadBank_BankID_Callback)Wwise::Exports::func_Wwise_Sound_UnloadBank_BankID_Callback;
		return func(in_bankID, in_pInMemoryBankPtr, in_pfnBankCallback, in_pCookie);
	}

	/// <summary>
	/// Unload a bank synchronously.
	/// </summary>
	/// <param name="in_pszString"> - Name of the bank to unload</param>
	/// <param name="in_pInMemoryBankPtr"> - Memory pointer from where the bank was initially loaded from. (REQUIRED to determine which bank associated toa memory pointer must be unloaded). Pass NULL only if NULL was passed when loading the bank.</param>
	/// <param name="out_pMemPoolId"> - Returned memory pool ID used with LoadBank() (can pass NULL)</param>
	/// <returns>AK_Success if successful, AK_Fail otherwise. AK_Success is returned when the bank was not loaded.</returns>
	AKRESULT UnloadBank(const char* in_pszString, const void* in_pInMemoryBankPtr, AkMemPoolId* out_pMemPoolId) {
		tUnloadBank_Char_MemPoolID func = (tUnloadBank_Char_MemPoolID)Wwise::Exports::func_Wwise_Sound_UnloadBank_Char_MemPoolID;
		return func(in_pszString, in_pInMemoryBankPtr, out_pMemPoolId);
	}

	/// <summary>
	/// Unload a bank asynchronously.
	/// </summary>
	/// <param name="in_pszString"> - Name of the bank to unload</param>
	/// <param name="in_pInMemoryBankPtr"> - Memory pointer from where the bank was initially loaded from. (REQUIRED to determine which bank associated toa memory pointer must be unloaded). Pass NULL only if NULL was passed when loading the bank.</param>
	/// <param name="in_pfnBankCallback"> - Callback function</param>
	/// <param name="in_pCookie"> - Callback cookie (reserved to user, passed to the callback function)</param>
	/// <returns>AK_Success if scheduling successful (use a callback to be notified when completed)</returns>
	AKRESULT UnloadBank(const char* in_pszString, const void* in_pInMemoryBankPtr, AkBankCallbackFunc in_pfnBankCallback, void* in_pCookie) {
		tUnloadBank_Char_Callback func = (tUnloadBank_Char_Callback)Wwise::Exports::func_Wwise_Sound_UnloadBank_Char_Callback;
		return func(in_pszString, in_pInMemoryBankPtr, in_pfnBankCallback, in_pCookie);
	}

	/// <summary>
	/// Undocumented.
	/// </summary>
	/// <param name="in_pszString"></param>
	/// <param name="in_pfnBankCallback"></param>
	/// <param name="in_pCookie"></param>
	/// <returns></returns>
	AKRESULT UnloadBankUnique(const char* in_pszString, AkBankCallbackFunc in_pfnBankCallback, void* in_pCookie) {
		tUnloadBankUnique func = (tUnloadBankUnique)Wwise::Exports::func_Wwise_Sound_UnloadBankUnique;
		return func(in_pszString, in_pfnBankCallback, in_pCookie);
	}

	/// <summary>
	/// Unregister all game objects.
	/// </summary>
	/// <returns>Always returns AK_Success</returns>
	AKRESULT UnregisterAllGameObj() {
		tUnregisterAllGameObj func = (tUnregisterAllGameObj)Wwise::Exports::func_Wwise_Sound_UnregisterAllGameObj;
		return func();
	}

	/// <summary>
	/// Unregister a game object.
	/// </summary>
	/// <param name="in_gameObjectID"> - ID of the game object to be unregistered. Use AK_INVALID_GAME_OBJECT to unregister all game objects.</param>
	/// <returns>AK_Success if successful. AK_Fail if the specified AkGameObjectID is invalid (0 is an invalid ID).</returns>
	AKRESULT UnregisterGameObj(AkGameObjectID in_gameObjectID) {
		tUnregisterGameObj func = (tUnregisterGameObj)Wwise::Exports::func_Wwise_Sound_UnregisterGameObj;
		return func(in_gameObjectID);
	}

	/// <summary>
	/// Unregister a global callback function, previously registered using RegisterGlobalCallback.
	/// </summary>
	/// <param name="in_pCallback"> - Function to unregister as a global callback.</param>
	/// <returns></returns>
	AKRESULT UnregisterGlobalCallback(AkGlobalCallbackFunc in_pCallback) {
		tUnregisterGlobalCallback func = (tUnregisterGlobalCallback)Wwise::Exports::func_Wwise_Sound_UnregisterGlobalCallback;
		return func(in_pCallback);
	}
}

