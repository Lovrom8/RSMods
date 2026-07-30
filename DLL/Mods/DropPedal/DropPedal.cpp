#include "../../stdafx.h"
#include "DropPedal.hpp"
#include "DropPedalHooks.hpp"
#include "DropPedalInput.hpp"
#include "DropPedalState.hpp"

void DropPedal::LoadSettings()
{
	DropPedalState::Configure(
		Settings::ReturnSettingValue("EnableDropPedal"),
		Settings::ReturnSettingValue("DropPedalEngine"));

	DropPedalInput::LoadKeybinds();
}

bool DropPedal::IsConfiguredEnabled()
{
	return DropPedalState::IsConfiguredEnabled();
}

bool DropPedal::ShouldInstallInputHooks()
{
	return DropPedalState::IsConfiguredEnabled() && !DropPedalState::IsCableEngine();
}

bool DropPedal::RequiresInputShifter()
{
	return DropPedalState::IsConfiguredEnabled() && DropPedalState::IsAsioEngine();
}

void DropPedal::ReportInputShifterUnavailable()
{
	DropPedalHooks::ReportInputShifterUnavailable();
}

bool DropPedal::IsEnabled()
{
	return DropPedalState::IsConfiguredEnabled() && DropPedalState::IsEnabled();
}

void DropPedal::HandleTuningInSong()
{
	if (!DropPedalState::IsConfiguredEnabled()) return;

	DropPedalHooks::HandleTuningInSong();
}

void DropPedal::ResetSongState()
{
	if (!DropPedalState::IsConfiguredEnabled()) return;

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
	if (!DropPedalState::IsConfiguredEnabled()) return;

	DropPedalHooks::Install();
}

void DropPedal::SetInputShifterActive(bool active)
{
	if (!DropPedalState::IsConfiguredEnabled()) return;

	DropPedalHooks::SetInputShifterActive(active);
}

bool DropPedal::IsInputShifterActive()
{
	return DropPedalState::IsConfiguredEnabled() && DropPedalHooks::IsInputShifterActive();
}

unsigned long long DropPedal::GetEngineNoticeTick()
{
	return DropPedalState::IsConfiguredEnabled() ? DropPedalHooks::GetEngineNoticeTick() : 0;
}

void DropPedal::PollHotkeys()
{
	if (!DropPedalState::IsConfiguredEnabled()) return;

	DropPedalInput::PollHotkeys();
}

void DropPedal::Poll()
{
	if (!DropPedalState::IsConfiguredEnabled()) return;

	DropPedalHooks::Poll();

	if (!DropPedalHooks::IsInputShifterActive())
	{
		DropPedalInput::PollPendingPitchPush();
		DropPedalHooks::LogPendingOverrides();
	}
}
