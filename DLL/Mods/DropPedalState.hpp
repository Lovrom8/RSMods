#pragma once

#include <string>

namespace DropPedalState
{
	bool IsEnabled();
	bool ToggleEnabled();
	bool AdjustTarget(int semitoneDelta);
	bool AdjustBaseTuning(int semitoneDelta);
	int GetTargetSemitones();
	float GetTargetCents();
	std::string GetTuningName();
	std::string GetBaseTuningName();
	int GetShiftDirection();
}
