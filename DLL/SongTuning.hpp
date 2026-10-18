#pragma once

#include <array>

#include "Tuning.h"

namespace SongTuning {
	std::array<byte, 6> GetCurrentTuning(bool verbose = false);
	bool IsExtendedRangeSong();
	std::array<int, 2> GetHighestLowestString(bool bass = false);
	std::array<int, 2> GetHighestLowestString(Tuning tuningOverride, bool bass = false);
	bool IsSongInDrop(Tuning tuning);
	bool IsSongInStandard(Tuning tuning);
	int GetTrueTuning();
	void InstallTunerHook();
	Tuning GetTuningAtTuner(bool logFailures = true);
	bool IsPlayerOnBass();

	enum class Path { Unknown = -1, Lead = 0, Rhythm = 1, Bass = 2 };
	Path GetPath(bool* resolved, unsigned playerIndex = 0); // The arrangement route; works in songs too, unlike the tuner.
	bool IsOnBassPath(); // GetPath when the game can tell, else what the last tuner saw.
	bool IsExtendedRangeTuner();
};
