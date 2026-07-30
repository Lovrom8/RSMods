#pragma once

namespace DropPedalHooks
{
	void Install();
	void Poll();
	void LogPendingOverrides();
	void PushPitchToLiveShifters();
	void SetInputShifterActive(bool active);
	bool IsInputShifterActive();
	unsigned long long GetEngineNoticeTick();
	void HandleTuningInSong();
	void ResetSongState();
}
