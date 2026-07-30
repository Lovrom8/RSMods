#include "../stdafx.h"
#include "DropPedalHooks.hpp"
#include "DropPedalInput.hpp"
#include "DropPedalState.hpp"

namespace
{
	constexpr ULONGLONG PITCH_PUSH_DELAY_MILLISECONDS = 150;

	int pitchDownKey = VK_OEM_COMMA;
	int pitchUpKey = VK_OEM_PERIOD;
	int toggleKey = VK_F8;
	int baseTuningDownKey = VK_F9;
	int baseTuningUpKey = VK_F10;

	bool wasLowerKeyDown = false;
	bool wasRaiseKeyDown = false;
	bool wasToggleKeyDown = false;
	bool wasBaseDownKeyDown = false;
	bool wasBaseUpKeyDown = false;
	bool isPitchPushPending = false;
	ULONGLONG lastTargetChangeTime = 0;

	void AdjustTarget(int semitoneDelta)
	{
		if (!DropPedalState::AdjustTarget(semitoneDelta))
		{
			return;
		}

		isPitchPushPending = true;
		lastTargetChangeTime = GetTickCount64();
	}

	void ToggleEnabled()
	{
		const bool isEnabled = DropPedalState::ToggleEnabled();

		if (isEnabled)
		{
			LOG_INFO("Drop pedal enabled, target " << DropPedalState::GetTuningName() << std::endl);

			if (!DropPedalHooks::IsInputShifterActive())
			{
				DropPedalHooks::PushPitchToLiveShifters();
			}
		}
		else
		{
			LOG_INFO("Drop pedal disabled, tones return to how the player configured them"
				<< " on the next tone load or tone switch" << std::endl);
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
	DWORD foregroundProcessId = 0;
	GetWindowThreadProcessId(GetForegroundWindow(), &foregroundProcessId);
	if (foregroundProcessId != GetCurrentProcessId()) return;

	const bool isToggleKeyDown = (GetAsyncKeyState(toggleKey) & 0x8000) != 0;

	if (isToggleKeyDown && !wasToggleKeyDown)
	{
		ToggleEnabled();
	}

	wasToggleKeyDown = isToggleKeyDown;

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
	if (!isPitchPushPending || GetTickCount64() - lastTargetChangeTime < PITCH_PUSH_DELAY_MILLISECONDS)
	{
		return;
	}

	isPitchPushPending = false;
	DropPedalHooks::PushPitchToLiveShifters();

	LOG_INFO("Drop pedal target now " << DropPedalState::GetTuningName() << std::endl);
}
