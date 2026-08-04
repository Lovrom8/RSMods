#include "../../stdafx.h"
#include "../../Audio/AsioHook.hpp"
#include "../../Audio/DelayLinePitchShifter.hpp"
#include "DropPedal.hpp"
#include "DropPedalHooks.hpp"
#include "DropPedalInput.hpp"
#include "DropPedalState.hpp"

namespace
{
	Audio::DelayLinePitchShifter playerOneInputPitchShifter{ 0 };
	Audio::DelayLinePitchShifter playerTwoInputPitchShifter{ 0 };

	Audio::DelayLinePitchShifter& GetInputPitchShifter(DropPedal::Player player)
	{
		return player == DropPedal::Player::One
			? playerOneInputPitchShifter
			: playerTwoInputPitchShifter;
	}
}

void DropPedal::LoadSettings()
{
	DropPedalState::Configure(
		Settings::ReturnSettingValue("EnableDropPedal"),
		Settings::ReturnSettingValue("DropPedalEngine"));
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

void DropPedal::InstallInputHooks()
{
	if (!ShouldInstallInputHooks()) return;

	Audio::AsioHook::Install();
	Audio::AsioHook::SetProcessor(GetPlayerIndex(Player::One), &playerOneInputPitchShifter);
	Audio::AsioHook::SetProcessor(GetPlayerIndex(Player::Two), &playerTwoInputPitchShifter);
	UpdateInputShifterPitch(Player::One);
	UpdateInputShifterPitch(Player::Two);
}

void DropPedal::UpdateInputShifterPitch(Player player)
{
	if (!ShouldInstallInputHooks()) return;

	const int targetSemitones = IsEnabled() ? GetTargetSemitones(player) : 0;
	GetInputPitchShifter(player).SetSemitones(targetSemitones);
}

bool DropPedal::IsEnabled()
{
	return DropPedalState::IsConfiguredEnabled() && DropPedalState::IsEnabled();
}

void DropPedal::HandleArrangementTuning()
{
	if (!DropPedalState::IsConfiguredEnabled()) return;

	DropPedalHooks::HandleArrangementTuning();
}

void DropPedal::ResetSongState()
{
	if (!DropPedalState::IsConfiguredEnabled()) return;

	DropPedalHooks::ResetSongState();
}

int DropPedal::GetTargetSemitones(Player player)
{
	return DropPedalState::GetTargetSemitones(player);
}

std::string DropPedal::GetTuningName(Player player)
{
	return DropPedalState::GetTuningName(player);
}

bool DropPedal::TryGetAuthoredTrueTuning(float& trueTuning)
{
	if (!DropPedalState::IsConfiguredEnabled()) return false;

	return DropPedalHooks::TryGetAuthoredTrueTuning(trueTuning);
}

std::string DropPedal::GetBaseTuningName(Player player)
{
	return DropPedalState::GetBaseTuningName(player);
}

int DropPedal::GetBaseTuningSemitones(Player player)
{
	return DropPedalState::GetBaseTuningSemitones(player);
}

int DropPedal::GetShiftDirection(Player player)
{
	return DropPedalState::GetShiftDirection(player);
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

bool DropPedal::IsPlayerShiftAvailable(Player player)
{
	if (player == Player::One) return true;

	if (!IsInputShifterActive())
	{
		return DropPedalState::IsConfiguredEnabled()
			&& DropPedalHooks::IsCableAttributionActive();
	}

	const size_t routeIndex = GetPlayerIndex(player);
	return Audio::AsioHook::IsInputConfigured(routeIndex)
		&& Audio::AsioHook::IsInputReady(routeIndex);
}

bool DropPedal::HasLivePedalTone(Player player)
{
	return DropPedalState::IsConfiguredEnabled()
		&& DropPedalHooks::HasLivePlayerPedalTone(player);
}

bool DropPedal::ConsumeInputShifterTransitionFailure()
{
	return DropPedalState::IsConfiguredEnabled()
		&& DropPedalHooks::ConsumeInputShifterTransitionFailure();
}

unsigned long long DropPedal::GetEngineNoticeTick()
{
	return DropPedalState::IsConfiguredEnabled() ? DropPedalHooks::GetEngineNoticeTick() : 0;
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
