#pragma once

#include <string>

namespace SongTimer {
	float SongTimer();
	std::string FormatTime(float timeInSeconds);
	float GetGreyNoteTimer();
	void SetGreyNoteTimer(float timeInSeconds);
	double GetNonStopPlayTimer();
	void SetNonStopPlayTimer(double NewTimer);
}
