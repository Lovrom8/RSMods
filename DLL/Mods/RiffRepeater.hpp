#pragma once

#include <iostream>
#include "Windows.h"
#include <string>

#include "../MemHelpers.hpp"
#include "../Offsets.hpp"
#include "../Wwise/SoundEngine.hpp"


namespace RiffRepeater {
	float GetSpeed(bool realSpeed = false);
	void SetSpeed(float newSpeed, bool isRealSpeed = false);
	float ConvertSpeed(float speed);
	void EnableTimeStretch();
	void DisableTimeStretch();
	void EnableLinearSpeeds();
	void DisableLinearSpeeds();
	bool LogSongID(std::string songKey);

	inline std::map<std::string, AkUInt32> SongObjectIDs;
	inline AkUInt32 currentSongID;
	inline bool readyToLogSongID;
	inline bool loggedCurrentSongID = false;

	inline bool currentlyEnabled_Above100 = false;
	inline bool currentlyEnabled_LinearRR = false;
}