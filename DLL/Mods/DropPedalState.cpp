#include "../stdafx.h"
#include "DropPedalState.hpp"

namespace
{
	constexpr float CENTS_PER_SEMITONE = 100.0f;
	constexpr int MIN_TARGET_SEMITONES = -24;
	constexpr int MAX_TARGET_SEMITONES = 24;
	constexpr int MIN_BASE_TUNING_SEMITONES = -11;
	constexpr int MAX_BASE_TUNING_SEMITONES = 11;
	constexpr int SEMITONES_PER_OCTAVE = 12;

	volatile float targetCents = 0.0f;
	bool isConfiguredEnabled = true;
	std::string engineSetting = "Automatic";
	volatile bool isEnabledSession = true;
	int baseTuningSemitones = 0;

	const char* GetTuningNameAtIndex(int index)
	{
		static const char* tuningNames[SEMITONES_PER_OCTAVE] = {
			"E", "Eb", "D", "C#", "C", "B", "Bb", "A", "Ab", "G", "F#", "F"
		};

		return tuningNames[index];
	}
}

void DropPedalState::Configure(const std::string& enabledSetting, const std::string& selectedEngine)
{
	isConfiguredEnabled = enabledSetting == "on";

	if (selectedEngine == "Automatic" || selectedEngine == "Asio" || selectedEngine == "Cable")
	{
		engineSetting = selectedEngine;
		return;
	}

	engineSetting.clear();
	isConfiguredEnabled = false;
	LOG_ERROR("Drop pedal disabled because [Drop Pedal] Engine is invalid: "
		<< selectedEngine << ". Expected Automatic, Asio or Cable." << std::endl);
}

bool DropPedalState::IsConfiguredEnabled()
{
	return isConfiguredEnabled;
}

bool DropPedalState::IsAutomaticEngine()
{
	return engineSetting == "Automatic";
}

bool DropPedalState::IsAsioEngine()
{
	return engineSetting == "Asio";
}

bool DropPedalState::IsCableEngine()
{
	return engineSetting == "Cable";
}

bool DropPedalState::IsEnabled()
{
	return isEnabledSession;
}

bool DropPedalState::ToggleEnabled()
{
	isEnabledSession = !isEnabledSession;
	return isEnabledSession;
}

bool DropPedalState::AdjustTarget(int semitoneDelta)
{
	if (!isEnabledSession)
	{
		return false;
	}

	const int adjusted = GetTargetSemitones() + semitoneDelta;
	if (adjusted < MIN_TARGET_SEMITONES || adjusted > MAX_TARGET_SEMITONES)
	{
		return false;
	}

	targetCents = adjusted * CENTS_PER_SEMITONE;
	return true;
}

bool DropPedalState::AdjustBaseTuning(int semitoneDelta)
{
	const int adjusted = baseTuningSemitones + semitoneDelta;
	if (adjusted < MIN_BASE_TUNING_SEMITONES || adjusted > MAX_BASE_TUNING_SEMITONES)
	{
		return false;
	}

	baseTuningSemitones = adjusted;
	return true;
}

int DropPedalState::GetTargetSemitones()
{
	return (int)(targetCents / CENTS_PER_SEMITONE);
}

float DropPedalState::GetTargetCents()
{
	return targetCents;
}

std::string DropPedalState::GetTuningName()
{
	const int semitones = GetTargetSemitones();

	int stepsBelowE = (-(baseTuningSemitones + semitones)) % SEMITONES_PER_OCTAVE;
	if (stepsBelowE < 0)
	{
		stepsBelowE += SEMITONES_PER_OCTAVE;
	}

	std::ostringstream name;
	name << GetTuningNameAtIndex(stepsBelowE) << " standard";

	if (semitones != 0)
	{
		name << " (" << (semitones > 0 ? "+" : "") << semitones << ")";
	}

	return name.str();
}

std::string DropPedalState::GetBaseTuningName()
{
	int stepsBelowE = (-baseTuningSemitones) % SEMITONES_PER_OCTAVE;
	if (stepsBelowE < 0)
	{
		stepsBelowE += SEMITONES_PER_OCTAVE;
	}

	return std::string(GetTuningNameAtIndex(stepsBelowE)) + " standard";
}

int DropPedalState::GetShiftDirection()
{
	const int semitones = GetTargetSemitones();
	if (semitones < 0)
	{
		return -1;
	}

	return semitones > 0 ? 1 : 0;
}
