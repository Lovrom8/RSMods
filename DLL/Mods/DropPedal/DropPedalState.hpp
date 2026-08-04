#pragma once

#include "DropPedalPlayer.hpp"

#include <string>

namespace DropPedalState
{
	void Configure(const std::string& enabledSetting, const std::string& engineSetting);
	bool IsConfiguredEnabled();
	bool IsAsioEngine();
	bool IsCableEngine();
	bool IsEnabled();
	bool ToggleEnabled();
	bool AdjustTarget(DropPedal::Player player, int semitoneDelta);
	bool AdjustBaseTuning(DropPedal::Player player, int semitoneDelta);
	int GetTargetSemitones(DropPedal::Player player);
	int GetBaseTuningSemitones(DropPedal::Player player);
	float GetTargetCents(DropPedal::Player player);
	std::string GetTuningName(DropPedal::Player player);
	std::string GetBaseTuningName(DropPedal::Player player);
	int GetShiftDirection(DropPedal::Player player);
}
