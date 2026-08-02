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

	// Session state is written by the hotkey thread and read by Wwise, rendering and
	// game-loop threads.
	std::atomic<int> targetSemitones{ 0 };

	// From [Drop Pedal] in RSMods.ini, read once at startup. Values follow the ini's
	// lowercase convention (on / off / automatic).
	bool isConfiguredEnabled = false;
	std::string engineSetting = "automatic";

	// Enabled state lives in the session rather than in the settings map, because
	// the settings reload during boot and would wipe it. Starts on, so a session
	// never silently begins with the pedal off; the toggle key still turns it off
	// for the session. Read by SetParam on other threads.
	std::atomic<bool> isEnabledSession{ true };

	// The tuning the player's guitar is physically in, as semitones from E standard.
	// Everything the mod shows is relative to this, so a player who lives in Eb sees
	// tunings named from Eb rather than being told to do the arithmetic themselves.
	// Session state like the shift itself: a player in Eb sets it once per launch.
	std::atomic<int> baseTuningSemitones{ 0 };

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
	return isEnabledSession.load(std::memory_order_relaxed);
}

bool DropPedalState::ToggleEnabled()
{
	// Only the hotkey thread writes this, so a plain load-flip-store is race-free.
	const bool next = !isEnabledSession.load(std::memory_order_relaxed);
	isEnabledSession.store(next, std::memory_order_relaxed);
	return next;
}

/// <summary>
/// Move the target immediately, so the on-screen tuning tracks the player's key
/// presses without lag. Does nothing while the mod is off, so the pitch keys are
/// inert until toggled on.
/// </summary>
bool DropPedalState::AdjustTarget(int semitoneDelta)
{
	if (!IsEnabled())
	{
		return false;
	}

	const int adjusted = GetTargetSemitones() + semitoneDelta;
	if (adjusted < MIN_TARGET_SEMITONES || adjusted > MAX_TARGET_SEMITONES)
	{
		return false;
	}

	targetSemitones.store(adjusted, std::memory_order_relaxed);
	return true;
}

bool DropPedalState::AdjustBaseTuning(int semitoneDelta)
{
	const int adjusted = baseTuningSemitones.load(std::memory_order_relaxed) + semitoneDelta;
	if (adjusted < MIN_BASE_TUNING_SEMITONES || adjusted > MAX_BASE_TUNING_SEMITONES)
	{
		return false;
	}

	baseTuningSemitones.store(adjusted, std::memory_order_relaxed);
	return true;
}

int DropPedalState::GetTargetSemitones()
{
	return targetSemitones.load(std::memory_order_relaxed);
}

float DropPedalState::GetTargetCents()
{
	return (float)GetTargetSemitones() * CENTS_PER_SEMITONE;
}

/// <summary>
/// Name the tuning the player's guitar is heard in: their physical tuning moved by
/// the current shift, in the form a tuner would show it.
/// </summary>
std::string DropPedalState::GetTuningName()
{
	const int semitones = GetTargetSemitones();

	const int baseSemitones = baseTuningSemitones.load(std::memory_order_relaxed);
	int stepsBelowE = (-(baseSemitones + semitones)) % SEMITONES_PER_OCTAVE;
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
	const int baseSemitones = baseTuningSemitones.load(std::memory_order_relaxed);
	int stepsBelowE = (-baseSemitones) % SEMITONES_PER_OCTAVE;
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
