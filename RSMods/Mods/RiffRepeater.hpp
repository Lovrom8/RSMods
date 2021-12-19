#pragma once

#include <iostream>
#include "Windows.h"
#include <string>
#include "../MemHelpers.hpp"
#include "../WwiseHijack.hpp"


namespace RiffRepeater {
	float GetSpeed(bool realSpeed = false);
	void SetSpeed(float newSpeed, bool isRealSpeed = false);
	float ConvertSpeed(float speed);
	void EnableTimeStretch();
	void DisableTimeStretch();
	bool LogSongID(std::string songKey);

	inline std::map<std::string, AkUInt32> SongObjectIDs;
	inline AkUInt32 currentSongID;
	inline bool readyToLogSongID;
	inline bool loggedCurrentSongID = false;

	inline bool currentlyEnabled = false;
}