#include "../../stdafx.h"
#include "DropPedalState.hpp"

namespace
{
	constexpr float CENTS_PER_SEMITONE = 100.0f;
	constexpr int MIN_TARGET_SEMITONES = -24;
	constexpr int MAX_TARGET_SEMITONES = 24;
	constexpr int SEMITONES_PER_OCTAVE = 12;

	// Session state is written by WndProc key commands and read by Wwise, rendering and
	// game-loop threads.
	std::atomic<int> targetSemitones[DropPedal::PLAYER_COUNT] = {};

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
	std::atomic<int> baseTuningSemitones[DropPedal::PLAYER_COUNT] = {};

	const char* GetTuningNameAtIndex(int index)
	{
		// The names read downwards from E, so a downward step indexes straight into them.
		static const char* tuningNames[SEMITONES_PER_OCTAVE] = {
			"E", "Eb", "D", "C#", "C", "B", "Bb", "A", "Ab", "G", "F#", "F"
		};

		return tuningNames[index];
	}

	const char* GetTuningNameForSemitones(int semitonesFromE)
	{
		int stepsBelowE = (-semitonesFromE) % SEMITONES_PER_OCTAVE;
		if (stepsBelowE < 0)
		{
			stepsBelowE += SEMITONES_PER_OCTAVE;
		}

		return GetTuningNameAtIndex(stepsBelowE);
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
	// Only the WndProc thread writes this, so a plain load-flip-store is race-free.
	const bool next = !isEnabledSession.load(std::memory_order_relaxed);
	isEnabledSession.store(next, std::memory_order_relaxed);
	return next;
}

/// <summary>
/// Move the target immediately, so the on-screen tuning tracks the player's key
/// presses without lag. Does nothing while the mod is off, so the pitch keys are
/// inert until toggled on.
/// </summary>
bool DropPedalState::AdjustTarget(DropPedal::Player player, int semitoneDelta)
{
	if (!IsEnabled())
	{
		return false;
	}

	const int adjusted = GetTargetSemitones(player) + semitoneDelta;
	if (adjusted < MIN_TARGET_SEMITONES || adjusted > MAX_TARGET_SEMITONES)
	{
		return false;
	}

	targetSemitones[DropPedal::GetPlayerIndex(player)].store(adjusted, std::memory_order_relaxed);
	return true;
}

bool DropPedalState::CycleBaseTuning(DropPedal::Player player)
{
	const size_t playerIndex = DropPedal::GetPlayerIndex(player);
	int next = baseTuningSemitones[playerIndex].load(std::memory_order_relaxed) - 1;
	if (next <= -SEMITONES_PER_OCTAVE)
	{
		next = 0;
	}

	baseTuningSemitones[playerIndex].store(next, std::memory_order_relaxed);
	return true;
}

int DropPedalState::GetTargetSemitones(DropPedal::Player player)
{
	return targetSemitones[DropPedal::GetPlayerIndex(player)].load(std::memory_order_relaxed);
}

int DropPedalState::GetBaseTuningSemitones(DropPedal::Player player)
{
	return baseTuningSemitones[DropPedal::GetPlayerIndex(player)].load(std::memory_order_relaxed);
}

float DropPedalState::GetTargetCents(DropPedal::Player player)
{
	return static_cast<float>(GetTargetSemitones(player)) * CENTS_PER_SEMITONE;
}

/// <summary>
/// Name the tuning the player's guitar is heard in: their physical tuning moved by
/// the current shift, in the form a tuner would show it.
/// </summary>
std::string DropPedalState::GetTuningName(DropPedal::Player player)
{
	const int semitones = GetTargetSemitones(player);
	const int baseSemitones = GetBaseTuningSemitones(player);
	const char* baseName = GetTuningNameForSemitones(baseSemitones);

	std::ostringstream name;
	if (semitones == 0)
	{
		name << baseName;
	}
	else
	{
		name << baseName << " -> " << GetTuningNameForSemitones(baseSemitones + semitones)
			<< " (" << (semitones > 0 ? "+" : "") << semitones << ")";
	}

	return name.str();
}

/// <summary>
/// Name the tuning the player's guitar is physically in, with no shift applied.
/// </summary>
std::string DropPedalState::GetBaseTuningName(DropPedal::Player player)
{
	return std::string(GetTuningNameForSemitones(GetBaseTuningSemitones(player))) + " standard";
}

int DropPedalState::GetShiftDirection(DropPedal::Player player)
{
	const int semitones = GetTargetSemitones(player);
	if (semitones < 0)
	{
		return -1;
	}

	return semitones > 0 ? 1 : 0;
}
