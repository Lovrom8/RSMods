#include "../stdafx.h"
#include "VolumeControl.hpp"
namespace Setting = Settings::Setting;

/// <summary>
/// Increase Volume of Mixer's Backend
/// </summary>
/// <param name="amountToIncrease"> - How much should we increase by?</param>
/// <param name="mixerToIncrease"> - Name of Mixer Value</param>
void VolumeControl::IncreaseVolume(int amountToIncrease, std::string mixerToIncrease) {
	float volume = 0;
	RTPCValue_type type = RTPCValue_GameObject;

	// Fill Volume Variable With Current Volume
	Wwise::SoundEngine::Query::GetRTPCValue(mixerToIncrease.c_str(), AK_INVALID_GAME_OBJECT, &volume, &type); 

	// Increase the volume by the amountToIncrease.
	if (volume <= (100.0f - amountToIncrease))
		volume += amountToIncrease;
	else
		volume = 100.0f; // In case the volume is within the amountToIncrease we can't throw it over 100.
	
	// Set Volume
	Wwise::SoundEngine::SetRTPCValue(mixerToIncrease.c_str(), (float)volume, AK_INVALID_GAME_OBJECT, 0, AkCurveInterpolation_Linear);
	Wwise::SoundEngine::SetRTPCValue(mixerToIncrease.c_str(), (float)volume, 0x1234, 0, AkCurveInterpolation_Linear);

	LOG_INFO("Increase volume of " << mixerToIncrease << " by " << amountToIncrease << " with a new volume of " << volume << std::endl);
}

/// <summary>
/// Decrease Volume of Mixer's Backend
/// </summary>
/// <param name="amountToDecrease"> - How much show we decrease by?</param>
/// <param name="mixerToDecrease"> - Name of Mixer Value</param>
void VolumeControl::DecreaseVolume(int amountToDecrease, std::string mixerToDecrease) {
	float volume = 0;
	RTPCValue_type type = RTPCValue_GameObject;

	// Fill Volume Variable With Current Volume
	Wwise::SoundEngine::Query::GetRTPCValue(mixerToDecrease.c_str(), AK_INVALID_GAME_OBJECT, &volume, &type); 

	// Decrease the volume by the amountToDecrease.
	if (volume >= (0.0f + amountToDecrease))
		volume -= amountToDecrease;
	else
		volume = 0.0f; // In case the volume is within the amountToDecrease we can't throw it below 0.

	// Set Volume
	Wwise::SoundEngine::SetRTPCValue(mixerToDecrease.c_str(), (float)volume, AK_INVALID_GAME_OBJECT, 0, AkCurveInterpolation_Linear);
	Wwise::SoundEngine::SetRTPCValue(mixerToDecrease.c_str(), (float)volume, 0x1234, 0, AkCurveInterpolation_Linear);
	
	LOG_INFO("Decrease volume of " << mixerToDecrease << " by " << amountToDecrease << " with a new volume of " << volume << std::endl);
}

/// <summary>
/// Reads the live Wwise volume (0-100) for a mixer channel.
/// </summary>
/// <param name="mixer"> - Name of the Mixer Value (see Settings::Setting::Channel).</param>
float VolumeControl::CurrentVolume(const char* mixer) {
	float volume = 0;
	RTPCValue_type type = RTPCValue_GameObject;

	Wwise::SoundEngine::Query::GetRTPCValue(mixer, AK_INVALID_GAME_OBJECT, &volume, &type);
	return volume;
}

/// <summary>
/// Disables the song previews when hovering over a song.
/// </summary>
void VolumeControl::DisableSongPreviewAudio() {
	if (!disabledSongPreviewAudio) {
		// Changes the string "Play_%s_Preview" to "Play_%s_Invalid" so song previews never play.
		MemUtil::PatchAdr(Offsets::patch_SongPreviewWwiseEvent, "Play_%s_Invalid", 16);
		disabledSongPreviewAudio = true;
	}
	else {
		LOG_WARNING("Tried to disable song previews when they are already disabled!" << std::endl);
	}
}

void VolumeControl::MutePlayer(bool player2) {
	RTPCValue_type type = RTPCValue_GameObject;

	const char* mixer = player2 ? Setting::Channel::Player2 : Setting::Channel::Player1;

	// Save current volume.
	if (player2)
	{
		Wwise::SoundEngine::Query::GetRTPCValue(mixer, AK_INVALID_GAME_OBJECT, &player2VolumeBeforeMute, &type);
		player2Muted = true;
	}
	else {
		Wwise::SoundEngine::Query::GetRTPCValue(mixer, AK_INVALID_GAME_OBJECT, &player1VolumeBeforeMute, &type);
		player1Muted = true;
	}

	// Mute (set volume to 0).
	Wwise::SoundEngine::SetRTPCValue(mixer, 0.f, AK_INVALID_GAME_OBJECT, 0, AkCurveInterpolation_Linear);
	Wwise::SoundEngine::SetRTPCValue(mixer, 0.f, 0x1234, 0, AkCurveInterpolation_Linear);

	LOG_INFO("Muted " << mixer << std::endl);
}

void VolumeControl::UnmutePlayer(bool player2)
{
	RTPCValue_type type = RTPCValue_GameObject;

	const char* mixer = player2 ? Setting::Channel::Player2 : Setting::Channel::Player1;

	// Unmute
	if (player2)
	{
		Wwise::SoundEngine::SetRTPCValue(mixer, player2VolumeBeforeMute, AK_INVALID_GAME_OBJECT, 0, AkCurveInterpolation_Linear);
		Wwise::SoundEngine::SetRTPCValue(mixer, player2VolumeBeforeMute, 0x1234, 0, AkCurveInterpolation_Linear);
		player2Muted = false;
	}
	else {
		Wwise::SoundEngine::SetRTPCValue(mixer, player1VolumeBeforeMute, AK_INVALID_GAME_OBJECT, 0, AkCurveInterpolation_Linear);
		Wwise::SoundEngine::SetRTPCValue(mixer, player1VolumeBeforeMute, 0x1234, 0, AkCurveInterpolation_Linear);
		player1Muted = false;
	}

	LOG_INFO("Unmuted " << mixer << std::endl);
}

/// <summary>
/// Enables the song previews when hovering over a song.
/// </summary>
void VolumeControl::EnableSongPreviewAudio() {
	if (disabledSongPreviewAudio) {
		// Changes the string "Play_%s_Invalid" to "Play_%s_Preview" so song previews will play again.
		MemUtil::PatchAdr(Offsets::patch_SongPreviewWwiseEvent, "Play_%s_Preview", 16);
		disabledSongPreviewAudio = false;
	}
	else {
		LOG_WARNING("Tried to enable song previews when they are already enabled!" << std::endl);
	}
}

/// <summary>
/// Allows the user to play music in game while Alt+Tabbed.
/// </summary>
void VolumeControl::AllowAltTabbingWithAudio() {
	char patch[] = { 0x1 };

	MemUtil::PatchAdr(Offsets::ptr_WindowNotInFocusValue, patch, 1); // Return with the value of 1, "window in focus", every time you alt+tab.
	MemUtil::PatchAdr(Offsets::ptr_IsWindowInFocus, "\x01", 1);

	LOG_INFO("Allowed audio to be played in the background!" << std::endl);
	allowedAltTabbingWithAudio = true;
}


/// <summary>
/// Pauses the audio when the user Alt+Tabs. This is the standard behavior of Rocksmith 2014.
/// </summary>
void VolumeControl::DisableAltTabbingWithAudio() {
	char patch[] = { 0x0 };

	MemUtil::PatchAdr(Offsets::ptr_WindowNotInFocusValue, patch, 1);  // Return with the value of 0, "window out of focus", every time you alt+tab.
	MemUtil::PatchAdr(Offsets::ptr_IsWindowInFocus, "\x00", 1);

	LOG_INFO("Stopped audio from being played in the background!" << std::endl);
	allowedAltTabbingWithAudio = false;
}