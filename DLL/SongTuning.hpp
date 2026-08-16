#pragma once

#include "stdafx.h"

namespace SongTuning {
	std::array<byte, 6> GetCurrentTuning(bool verbose = false);
	bool IsExtendedRangeSong();
	std::array<int, 2> GetHighestLowestString();
	std::array<int, 2> GetHighestLowestString(Tuning tuningOverride);
	bool IsSongInDrop(Tuning tuning);
	bool IsSongInStandard(Tuning tuning);
	bool TryGetTrueTuning(float& trueTuning, uintptr_t& address);
	int GetTrueTuning();
	Tuning GetTuningAtTuner();
	bool IsExtendedRangeTuner();
};
