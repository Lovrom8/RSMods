#pragma once

namespace DropPedalHooks
{
	void Install();
	void Poll();
	void LogPendingOverrides();
	void PushPitchToLiveShifters();
	void SetInputShifterActive(bool active);
	bool IsInputShifterActive();
	bool ConsumeInputShifterTransitionFailure();
	bool TryGetAuthoredTrueTuning(float& trueTuning);
	void ReportInputShifterUnavailable();
	unsigned long long GetEngineNoticeTick();
	void HandleArrangementTuning();
	void ResetSongState();
}
