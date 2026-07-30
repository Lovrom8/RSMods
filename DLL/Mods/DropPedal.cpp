#include "../stdafx.h"
#include "DropPedal.hpp"
#include "DropPedalHooks.hpp"
#include "DropPedalInput.hpp"
#include "DropPedalState.hpp"

bool DropPedal::IsEnabled()
{
	return DropPedalState::IsEnabled();
}

void DropPedal::HandleTuningInSong()
{
	DropPedalHooks::HandleTuningInSong();
}

void DropPedal::ResetSongState()
{
	DropPedalHooks::ResetSongState();
}

int DropPedal::GetTargetSemitones()
{
	return DropPedalState::GetTargetSemitones();
}

std::string DropPedal::GetTuningName()
{
	return DropPedalState::GetTuningName();
}

std::string DropPedal::GetBaseTuningName()
{
	return DropPedalState::GetBaseTuningName();
}

int DropPedal::GetShiftDirection()
{
	return DropPedalState::GetShiftDirection();
}

void DropPedal::InstallHooks()
{
	DropPedalHooks::Install();
}

void DropPedal::SetInputShifterActive(bool active)
{
	DropPedalHooks::SetInputShifterActive(active);
}

bool DropPedal::IsInputShifterActive()
{
	return DropPedalHooks::IsInputShifterActive();
}

unsigned long long DropPedal::GetEngineNoticeTick()
{
	return DropPedalHooks::GetEngineNoticeTick();
}

void DropPedal::PollHotkeys()
{
	DropPedalInput::PollHotkeys();
}

void DropPedal::Poll()
{
	DropPedalHooks::Poll();

	if (!DropPedalHooks::IsInputShifterActive())
	{
		DropPedalInput::PollPendingPitchPush();
		DropPedalHooks::LogPendingOverrides();
	}
}