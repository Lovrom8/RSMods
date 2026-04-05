#pragma once

#include <optional>
#include <string>
#include <Windows.h>

namespace QCAutomation::ToneSource {
	void Initialize();
	void HandleInSongState();
	void HandleOutOfSongState();
	void NotifyManualToneSlotHotkey(WPARAM keyPressed);
	bool TryGetCurrentToneSlot(int& outToneSlot);
	std::optional<std::string> GetCurrentAuthoredToneName(float playbackTimeSec);
	bool TryGetCurrentAuthoredToneName(std::string& outToneName);
}
