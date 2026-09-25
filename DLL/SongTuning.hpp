#pragma once

#include <array>

#include "Tuning.h"

namespace SongTuning {
	std::array<byte, 6> GetCurrentTuning(bool verbose = false);
	bool IsExtendedRangeSong();
	std::array<int, 2> GetHighestLowestString(bool alwaysIgnoreBlankBassStrings = false);
	std::array<int, 2> GetHighestLowestString(Tuning tuningOverride, bool alwaysIgnoreBlankBassStrings = false);
	bool IsSongInDrop(Tuning tuning);
	bool IsSongInStandard(Tuning tuning);
	int GetTrueTuning();
	void InstallTunerHook();
	Tuning GetTuningAtTuner(bool logFailures = true);
	bool IsExtendedRangeTuner();
};
