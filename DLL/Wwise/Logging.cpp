#include "Logging.hpp"

#ifdef _WWISE_LOGS
namespace Wwise::Logging {

	std::string CurveInterpolation(AkCurveInterpolation curve)
	{
		switch (curve)
		{
			case AkCurveInterpolation_Log3:
				return "Log3";
			case AkCurveInterpolation_Sine:
				return "Sine";
			case AkCurveInterpolation_Log1:
				return "Log1";
			case AkCurveInterpolation_InvSCurve:
				return "Inverse S Curve";
			case AkCurveInterpolation_Linear:
				return "Linear";
			case AkCurveInterpolation_SCurve:
				return "S Curve";
			case AkCurveInterpolation_Exp1:
				return "Exp 1";
			case AkCurveInterpolation_SineRecip:
				return "Sine Reciprical";
			case AkCurveInterpolation_Exp3:
				return "Exp 3";
			case AkCurveInterpolation_Constant:
				return "Constant";
			default:
				return "Invalid curve";
		}
	}

	/// <summary>
	/// Console print out for Wwise::SoundEngine::PostEvent()
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
	AkPlayingID log_PostEvent_Name(const char* in_pszEventName, AkGameObjectID in_gameObjectID, AkUInt32 in_uFlags, AkCallbackFunc in_pfnCallback, void* in_pCookie, AkUInt32 in_cExternals, AkExternalSourceInfo* in_pExternalSources, AkPlayingID in_PlayingID)
	{
		_LOG_INIT;

		_LOG("(Wwise) PostEvent: "
				  << in_pszEventName
				  << " on game object 0x"
				  << std::hex << in_gameObjectID
				  << std::dec << std::endl);

		return oPostEvent(in_pszEventName, in_gameObjectID, in_uFlags, in_pfnCallback, in_pCookie, in_cExternals, in_pExternalSources, in_PlayingID);
	}

	/// <summary>
	/// Console print out for Wwise::SoundEngine::SeekOnEvent()
	/// </summary>
	/// <param name="in_pszEventName"> - Name of the event to seek on</param>
	/// <param name="in_gameObjectID"> - Object which we want to seek on</param>
	/// <param name="in_iPosition"> - Position we want to seek to (in milliseconds)</param>
	/// <param name="in_bSeekToNearestMarker"> - Should we skip to the nearest marker</param>
	AKRESULT log_SeekOnEvent(char* in_pszEventName, AkGameObjectID in_gameObjectID, AkTimeMs in_iPosition, bool in_bSeekToNearestMarker)
	{
		_LOG_INIT;

		_LOG("(Wwise) SeekOnEvent: "
				  << in_pszEventName
				  << " on object 0x"
				  << std::hex << in_gameObjectID
				  << " at time "
				  << (float)(in_iPosition / 1000)
				  << " seconds. Seek to nearest marker: "
				  << std::boolalpha << in_bSeekToNearestMarker
				  << std::endl);

		return oSeekOnEvent(in_pszEventName, in_gameObjectID, in_iPosition, in_bSeekToNearestMarker);
	}
	
	/// <summary>
	/// Clone an effect from one bus, to another
	/// </summary>
	/// <param name="toBus"> - Bus to send the effect to</param>
	/// <param name="indexFX"> - Effect slot to put the effect in</param>
	/// <param name="fromBus"> - Bus to read the effect from</param>
	AKRESULT log_CloneBusEffect(AkUniqueID toBus, AkUInt32 indexFX, AkUniqueID fromBus)
	{
		_LOG_INIT;

		_LOG("(Wwise) CloneBusEffect: From bus 0x"
				  << std::hex << fromBus
				  << " in slot "
				  << std::dec << indexFX
				  << " to bus 0x"
				  << std::hex << toBus
				  << std::dec << std::endl);

		return oCloneBusEffect(toBus, indexFX, fromBus);
	}

	/// <summary>
	/// Change the value of an RTPC
	/// </summary>
	/// <param name="in_pszRtpcName"> - Name of the RTPC</param>
	/// <param name="in_value"> - New value of the RTPC</param>
	/// <param name="in_gameObjectID"> - Parent game object</param>
	/// <param name="in_uValueChangeDuration"> - Amount of fade time between the previous value, and the new value.</param>
	/// <param name="in_eFadeCurve"> - Curve to fade into the new value.</param>
	AKRESULT log_SetRTPCValue(const char* in_pszRtpcName, AkRtpcValue in_value, AkGameObjectID in_gameObjectID, AkTimeMs in_uValueChangeDuration, AkCurveInterpolation in_eFadeCurve)
	{
		_LOG_INIT;

		if (!MemHelpers::Contains(std::string(in_pszRtpcName), SetRTPCBlacklist))
		{
			_LOG("(Wwise) SetRTPCValue: "
					  << in_pszRtpcName
					  << " to "
					  << in_value
					  << " on game object 0x"
					  << std::hex << in_gameObjectID
					  << " with duration of "
					  << std::dec << in_uValueChangeDuration
					  << "ms, with "
					  << CurveInterpolation(in_eFadeCurve)
					  << " curve."
					  << std::endl);
		}
		
		return oSetRTPCValue_Char(in_pszRtpcName, in_value, in_gameObjectID, in_uValueChangeDuration, in_eFadeCurve);
	}

	/// <summary>
	/// Enable Wwise Logging
	/// </summary>
	void Init()
	{
		oSetRTPCValue_Char = (tSetRTPCValue_Char)DetourFunction((PBYTE)Exports::func_Wwise_Sound_SetRTPCValue_Char, (PBYTE)log_SetRTPCValue);
		oCloneBusEffect = (tCloneBusEffect)DetourFunction((PBYTE)Exports::func_Wwise_Sound_CloneBusEffect, (PBYTE)log_CloneBusEffect);
		oSeekOnEvent = (tSeekOnEvent_Char_Int32)DetourFunction((PBYTE)Exports::func_Wwise_Sound_SeekOnEvent_Char_Int32, (PBYTE)log_SeekOnEvent);
		oPostEvent = (tPostEvent_Char)DetourFunction((PBYTE)Exports::func_Wwise_Sound_PostEvent_Char, (PBYTE)log_PostEvent_Name);
	}
}
#endif