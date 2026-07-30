#pragma once

#include <string>

namespace DropPedalState
{
	void Configure(const std::string& enabledSetting, const std::string& engineSetting);
	bool IsConfiguredEnabled();
	bool IsAutomaticEngine();
	bool IsAsioEngine();
	bool IsCableEngine();
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
