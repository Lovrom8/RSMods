#pragma once

namespace DropPedalHooks
{
	void Install();
	void Poll();
	void LogPendingOverrides();
	void PushPitchToLiveShifters();
	void SetInputShifterActive(bool active);
	bool IsInputShifterActive();
	void ReportInputShifterUnavailable();
	unsigned long long GetEngineNoticeTick();
	void HandleArrangementTuning();
	void ResetSongState();
}
