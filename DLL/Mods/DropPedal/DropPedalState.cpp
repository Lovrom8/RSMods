#include "../../stdafx.h"
#include "DropPedalState.hpp"

namespace
{
	constexpr float CENTS_PER_SEMITONE = 100.0f;
	constexpr int MIN_TARGET_SEMITONES = -24;
	constexpr int MAX_TARGET_SEMITONES = 24;
	constexpr int MIN_BASE_TUNING_SEMITONES = -11;
	constexpr int MAX_BASE_TUNING_SEMITONES = 11;
	constexpr int SEMITONES_PER_OCTAVE = 12;

	// Semitones the guitar has to move, in cents. Written by the game loop and read
	// by SetParam on other threads: a torn read of a float is impossible on x86, and
	// a briefly stale value is harmless.
	volatile float targetCents = 0.0f;

	// From [Drop Pedal] in RSMods.ini, read once at startup. Values follow the ini's
	// lowercase convention (on / off / automatic).
	bool isConfiguredEnabled = false;
	std::string engineSetting = "automatic";

	// Enabled state lives in the session rather than in the settings map, because
	// the settings reload during boot and would wipe it. Starts on, so a session
	// never silently begins with the pedal off; the toggle key still turns it off
	// for the session. Read by SetParam on other threads.
	volatile bool isEnabledSession = true;

	// The tuning the player's guitar is physically in, as semitones from E standard.
	// Everything the mod shows is relative to this, so a player who lives in Eb sees
	// tunings named from Eb rather than being told to do the arithmetic themselves.
	// Session state like the shift itself: a player in Eb sets it once per launch.
	int baseTuningSemitones = 0;

	const char* GetTuningNameAtIndex(int index)
	{
		// The names read downwards from E, so a downward step indexes straight into them.
		static const char* tuningNames[SEMITONES_PER_OCTAVE] = {
			"E", "Eb", "D", "C#", "C", "B", "Bb", "A", "Ab", "G", "F#", "F"
		};

		return tuningNames[index];
	}
}

void DropPedalState::Configure(const std::string& enabledSetting, const std::string& selectedEngine)
{
	isConfiguredEnabled = enabledSetting == "on";

	if (selectedEngine == "automatic" || selectedEngine == "asio" || selectedEngine == "cable")
	{
		engineSetting = selectedEngine;
		return;
	}

	engineSetting.clear();
	isConfiguredEnabled = false;
	LOG_ERROR("Drop pedal disabled because [Drop Pedal] Engine is invalid: "
		<< selectedEngine << ". Expected automatic, asio or cable." << std::endl);
}

bool DropPedalState::IsConfiguredEnabled()
{
	return isConfiguredEnabled;
}

bool DropPedalState::IsAsioEngine()
{
	return engineSetting == "asio";
}

bool DropPedalState::IsCableEngine()
{
	return engineSetting == "cable";
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

/// <summary>
/// Move the target immediately, so the on-screen tuning tracks the player's key
/// presses without lag. Does nothing while the mod is off, so the pitch keys are
/// inert until toggled on.
/// </summary>
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

/// <summary>
/// Name the tuning the player's guitar is heard in: their physical tuning moved by
/// the current shift, in the form a tuner would show it.
/// </summary>
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

/// <summary>
/// Name the tuning the player's guitar is physically in, with no shift applied.
/// </summary>
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
