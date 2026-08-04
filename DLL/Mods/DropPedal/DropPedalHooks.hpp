#pragma once

#include "DropPedalPlayer.hpp"

namespace DropPedalHooks
{
	void Install();
	void Poll();
	void LogPendingOverrides();
	void PushPitchToLiveShifters();
	void SetInputShifterActive(bool active);
	bool IsInputShifterActive();
	bool IsCableAttributionActive();
	bool HasLivePlayerPedalTone(DropPedal::Player player);
	bool ConsumeInputShifterTransitionFailure();
	bool TryGetAuthoredTrueTuning(float& trueTuning);
	void ReportInputShifterUnavailable();
	unsigned long long GetEngineNoticeTick();
	void HandleArrangementTuning();
	void ResetSongState();
}
