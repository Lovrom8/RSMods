#pragma once

namespace VolumeControl {
	void IncreaseVolume(int amountToIncrease, std::string mixerToIncrease);
	void DecreaseVolume(int amountToDecrease, std::string mixerToDecrease);
	float CurrentVolume(const char* mixer);
	void MutePlayer(bool player2 = false);
	void UnmutePlayer(bool player2 = false);
	void DisableSongPreviewAudio();
	void EnableSongPreviewAudio();
	void AllowAltTabbingWithAudio();
	void DisableAltTabbingWithAudio();
	inline bool disabledSongPreviewAudio = false;
	inline bool allowedAltTabbingWithAudio = false;

	inline bool player1Muted = false;
	inline bool player2Muted = false;
}

inline float player1VolumeBeforeMute = 100.f;
inline float player2VolumeBeforeMute = 100.f;


