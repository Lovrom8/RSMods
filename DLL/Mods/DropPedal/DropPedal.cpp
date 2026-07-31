#include "../../stdafx.h"
#include "DropPedal.hpp"
#include "DropPedalHooks.hpp"
#include "DropPedalInput.hpp"
#include "DropPedalState.hpp"

namespace
{
	// RS_ASIO maps the game's second multiplayer input through [Asio.Input.1].
	bool HasSecondInputConfigured()
	{
		CSimpleIniA reader;
		if (reader.LoadFile("RS_ASIO.ini") < 0) return false;

		const char* secondInputDriver = reader.GetValue("Asio.Input.1", "Driver", "");
		return secondInputDriver != nullptr && *secondInputDriver != '\0';
	}
}

void DropPedal::LoadSettings()
{
	DropPedalState::Configure(
		Settings::ReturnSettingValue("EnableDropPedal"),
		Settings::ReturnSettingValue("DropPedalEngine"));

	// RS_ASIO's standard template fills in [Asio.Input.1] even for single-player
	// setups, so its presence is not evidence of multiplayer and must not disable
	// anything. The warning states the actual limitation: the pedal shifts only the
	// [Asio.Input.0] channel, so a second player plays unshifted. The cable engine's
	// tuning-reference redirect is additionally global; that hazard is documented in
	// the setup guide rather than guessed at here, since which mode the player will
	// enter cannot be known at settings time.
	if (DropPedalState::IsConfiguredEnabled() && HasSecondInputConfigured())
	{
		LOG_WARNING("RS_ASIO.ini configures a second input under [Asio.Input.1]. The drop "
			"pedal shifts only the [Asio.Input.0] channel; in multiplayer the second "
			"player is not shifted." << std::endl);
	}

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
