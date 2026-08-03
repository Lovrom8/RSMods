#include "../../stdafx.h"
#include "DropPedalHooks.hpp"
#include "DropPedalInput.hpp"
#include "DropPedalState.hpp"

namespace
{
	constexpr ULONGLONG PITCH_PUSH_DELAY_MILLISECONDS = 150;

	// Defaults, replaced from [Keybinds] in RSMods.ini by LoadKeybinds.
	int pitchDownKey = VK_OEM_COMMA;
	int pitchUpKey = VK_OEM_PERIOD;
	int toggleKey = VK_F7;
	int baseTuningDownKey = VK_F9;
	int baseTuningUpKey = VK_F10;

	bool wasLowerKeyDown = false;
	bool wasRaiseKeyDown = false;
	bool wasToggleKeyDown = false;
	bool wasBaseDownKeyDown = false;
	bool wasBaseUpKeyDown = false;
	std::atomic<ULONGLONG> pushDeadlineTick{ 0 };

	void AdjustTarget(int semitoneDelta)
	{
		if (!DropPedalState::AdjustTarget(semitoneDelta))
		{
			return;
		}

		pushDeadlineTick.store(
			GetTickCount64() + PITCH_PUSH_DELAY_MILLISECONDS,
			std::memory_order_release);
	}

	void ToggleEnabled()
	{
		const bool isEnabled = DropPedalState::ToggleEnabled();

		// Both directions push immediately: enabling applies the shift, disabling
		// restores each shifter's authored pitch, keeping audio and note detection
		// in agreement without waiting for a tone load.
		if (!DropPedalHooks::IsInputShifterActive())
		{
			DropPedalHooks::PushPitchToLiveShifters();
		}

		if (isEnabled)
		{
			LOG_INFO("Drop pedal enabled, target " << DropPedalState::GetTuningName() << std::endl);
		}
		else
		{
			LOG_INFO("Drop pedal disabled, tones restored to their authored pitch" << std::endl);
		}
	}

	void AdjustBaseTuning(int semitoneDelta)
	{
		if (!DropPedalState::AdjustBaseTuning(semitoneDelta))
		{
			return;
		}

		LOG_INFO("Drop pedal base tuning now " << DropPedalState::GetBaseTuningName() << std::endl);
	}
}

void DropPedalInput::LoadKeybinds()
{
	pitchDownKey = Settings::GetKeyBind("DropPedalPitchDownKey");
	pitchUpKey = Settings::GetKeyBind("DropPedalPitchUpKey");
	toggleKey = Settings::GetKeyBind("DropPedalToggleKey");
	baseTuningDownKey = Settings::GetKeyBind("DropPedalBaseTuningDownKey");
	baseTuningUpKey = Settings::GetKeyBind("DropPedalBaseTuningUpKey");
}

void DropPedalInput::PollHotkeys()
{
	// GetAsyncKeyState reads global keyboard state, so without this guard the pedal
	// retunes while the player is typing in another window.
	DWORD foregroundProcessId = 0;
	GetWindowThreadProcessId(GetForegroundWindow(), &foregroundProcessId);
	if (foregroundProcessId != GetCurrentProcessId()) return;

	const bool isToggleKeyDown = (GetAsyncKeyState(toggleKey) & 0x8000) != 0;

	if (isToggleKeyDown && !wasToggleKeyDown)
	{
		ToggleEnabled();
	}

	wasToggleKeyDown = isToggleKeyDown;

	// Disabled means disabled: the game ignores the pedal's output, so no key besides
	// the toggle may change its state either. The latches still update below so a key
	// held across re-enabling does not fire on the first enabled poll.
	const bool acceptAdjustments = DropPedalState::IsEnabled();

	const bool isBaseDownKeyDown = (GetAsyncKeyState(baseTuningDownKey) & 0x8000) != 0;
	const bool isBaseUpKeyDown = (GetAsyncKeyState(baseTuningUpKey) & 0x8000) != 0;

	if (acceptAdjustments && isBaseDownKeyDown && !wasBaseDownKeyDown)
	{
		AdjustBaseTuning(-1);
	}

	if (acceptAdjustments && isBaseUpKeyDown && !wasBaseUpKeyDown)
	{
		AdjustBaseTuning(1);
	}

	wasBaseDownKeyDown = isBaseDownKeyDown;
	wasBaseUpKeyDown = isBaseUpKeyDown;

	const bool isLowerKeyDown = (GetAsyncKeyState(pitchDownKey) & 0x8000) != 0;
	const bool isRaiseKeyDown = (GetAsyncKeyState(pitchUpKey) & 0x8000) != 0;

	if (acceptAdjustments && isLowerKeyDown && !wasLowerKeyDown)
	{
		AdjustTarget(-1);
	}

	if (acceptAdjustments && isRaiseKeyDown && !wasRaiseKeyDown)
	{
		AdjustTarget(1);
	}

	wasLowerKeyDown = isLowerKeyDown;
	wasRaiseKeyDown = isRaiseKeyDown;
}

void DropPedalInput::PollPendingPitchPush()
{
	ULONGLONG deadline = pushDeadlineTick.load(std::memory_order_acquire);
	if (deadline == 0 || GetTickCount64() < deadline)
	{
		return;
	}

	if (!pushDeadlineTick.compare_exchange_strong(
		deadline,
		0,
		std::memory_order_acq_rel,
		std::memory_order_acquire)) return;

	DropPedalHooks::PushPitchToLiveShifters();

	LOG_INFO("Drop pedal target now " << DropPedalState::GetTuningName() << std::endl);
}
