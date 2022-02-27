#pragma once

#include "Windows.h"

#include "../Log.hpp"
#include "../Lib/Detours/detours.h"

#include "Types.hpp"
#include "Exports.hpp"
#include "SoundEngine.hpp"

#include "../MemHelpers.hpp"

#ifdef _WWISE_LOGS
namespace Wwise::Logging {
	void Init();

	inline tSetRTPCValue_Char oSetRTPCValue_Char;
	inline tCloneBusEffect oCloneBusEffect;
	inline tSeekOnEvent_Char_Int32 oSeekOnEvent;
	inline tPostEvent_Char oPostEvent;


	/// <summary>
	/// SetRTPCValue gets ran a lot with the following objects. Don't log these.
	/// </summary>
	inline std::vector<std::string> SetRTPCBlacklist{
		"Player_Success",
		"Portal_Size",
		"LoftAmb_CamPosition",
		"AmbRadio_OnOff_Return",
		"Average_Song_Difficulty",
		"Lowest_Phrase_Difficulty_Level",
		"PortalClose_TailLP_Return",
		"PortalClose_TailRamping_Return",
		"P1_Instrument_Volume",
		"GateTime",
		"P1_Streak_Hit_Count",
		"P1_Streak_Miss",
		"P1_Streak_Note_Count",
		"P1_Streak_Chord_Count",
		"P1_Streak_Phrase_Count",
		"P2_Instrument_Volume",
		"MusicRamping"
	};
}
#endif