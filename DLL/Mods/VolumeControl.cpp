#include "VolumeControl.hpp"

/// <summary>
/// Increase Volume of Mixer's Backend
/// </summary>
/// <param name="amountToIncrease"> - How much should we increase by?</param>
/// <param name="mixerToIncrease"> - Name of Mixer Value</param>
void VolumeControl::IncreaseVolume(int amountToIncrease, std::string mixerToIncrease) {
	_LOG_INIT;
	_LOG_SETLEVEL(LogLevel::Error);

	float volume = 0;
	RTPCValue_type type = RTPCValue_GameObject;

	// Mixer sent is not a valid mixer.
	if (!MemHelpers::Contains(mixerToIncrease, mixerNames)) {
		_LOG("That mixer doesn't exist" << std::endl);
		return;
	}

	_LOG_SETLEVEL(LogLevel::Info);

	// Fill Volume Variable With Current Volume
	Wwise::SoundEngine::Query::GetRTPCValue(mixerToIncrease.c_str(), AK_INVALID_GAME_OBJECT, &volume, &type); 

	// Increase the volume by the amountToIncrease.
	if (volume <= (100.0f - amountToIncrease))
		volume += amountToIncrease;
	else
		volume = 100.0f; // Incase the volume is within the amountToIncrease we can't throw it over 100.
	
	// Set Volume
	Wwise::SoundEngine::SetRTPCValue(mixerToIncrease.c_str(), (float)volume, AK_INVALID_GAME_OBJECT, 0, AkCurveInterpolation_Linear);
	Wwise::SoundEngine::SetRTPCValue(mixerToIncrease.c_str(), (float)volume, 0x1234, 0, AkCurveInterpolation_Linear);

	_LOG("Increase volume of " << mixerToIncrease << " by " << amountToIncrease << " with a new volume of " << volume << std::endl);
}

/// <summary>
/// Decrease Volume of Mixer's Backend
/// </summary>
/// <param name="amountToDecrease"> - How much show we decrease by?</param>
/// <param name="mixerToDecrease"> - Name of Mixer Value</param>
void VolumeControl::DecreaseVolume(int amountToDecrease, std::string mixerToDecrease) {
	_LOG_INIT;
	_LOG_SETLEVEL(LogLevel::Error);

	float volume = 0;
	RTPCValue_type type = RTPCValue_GameObject;

	// Mixer sent is not a valid mixer.
	if (!MemHelpers::Contains(mixerToDecrease, mixerNames)) {
		_LOG("That mixer doesn't exist" << std::endl);
		return;
	}

	_LOG_SETLEVEL(LogLevel::Info);

	// Fill Volume Variable With Current Volume
	Wwise::SoundEngine::Query::GetRTPCValue(mixerToDecrease.c_str(), AK_INVALID_GAME_OBJECT, &volume, &type); 

	// Decrease the volume by the amountToDecrease.
	if (volume >= (0.0f + amountToDecrease))
		volume -= amountToDecrease;
	else
		volume = 0.0f; // Incase the volume is within the amountToDecrease we can't throw it below 0.

	// Set Volume
	Wwise::SoundEngine::SetRTPCValue(mixerToDecrease.c_str(), (float)volume, AK_INVALID_GAME_OBJECT, 0, AkCurveInterpolation_Linear);
	Wwise::SoundEngine::SetRTPCValue(mixerToDecrease.c_str(), (float)volume, 0x1234, 0, AkCurveInterpolation_Linear);
	
	_LOG("Decrease volume of " << mixerToDecrease << " by " << amountToDecrease << " with a new volume of " << volume << std::endl);
}

/// <summary>
/// Disables the song previews when hovering over a song.
/// </summary>
void VolumeControl::DisableSongPreviewAudio() {
	_LOG_INIT;
	_LOG_SETLEVEL(LogLevel::Warning);

	if (!disabledSongPreviewAudio) {
		// Changes the string "Play_%s_Preview" to "Play_%s_Invalid" so song previews never play.
		MemUtil::PatchAdr((void*)Offsets::patch_SongPreviewWwiseEvent, "Play_%s_Invalid", 16);
		disabledSongPreviewAudio = true;
	}
	else {
		_LOG("Tried to disable song previews when they are already disabled!" << std::endl);
	}
}

void VolumeControl::MutePlayer(bool player2)
{
	_LOG_INIT;

	RTPCValue_type type = RTPCValue_GameObject;

	const char* mixer = player2 ? "Mixer_Player2" : "Mixer_Player1";

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

	_LOG("Muted " << mixer << std::endl);
}

void VolumeControl::UnmutePlayer(bool player2)
{
	_LOG_INIT;

	RTPCValue_type type = RTPCValue_GameObject;

	const char* mixer = player2 ? "Mixer_Player2" : "Mixer_Player1";
	
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

	_LOG("Unmuted " << mixer << std::endl);
}

/// <summary>
/// Enables the song previews when hovering over a song.
/// </summary>
void VolumeControl::EnableSongPreviewAudio() {
	_LOG_INIT;
	_LOG_SETLEVEL(LogLevel::Warning);

	if (disabledSongPreviewAudio) {
		// Changes the string "Play_%s_Invalid" to "Play_%s_Preview" so song previews will play again.
		MemUtil::PatchAdr((void*)Offsets::patch_SongPreviewWwiseEvent, "Play_%s_Preview", 16);
		disabledSongPreviewAudio = false;
	}
	else {
		_LOG("Tried to enable song previews when they are already enabled!" << std::endl);
	}
}

/// <summary>
/// Allows the user to play music in game while Alt+Tabbed.
/// </summary>
void VolumeControl::AllowAltTabbingWithAudio() {
	_LOG_INIT;

	char patch[] = { 0x1 };
	MemUtil::PatchAdr((void*)Offsets::ptr_WindowNotInFocusValue, patch, 1); // Return with the value of 1, "window in focus", every time you alt+tab.
	*(char*)Offsets::ptr_IsWindowInFocus = (char)0x1; // Tell the game that you currently have the window in focus.
	_LOG("Allowed audio to be played in the background!" << std::endl);
	allowedAltTabbingWithAudio = true;
}


/// <summary>
/// Pauses the audio when the user Alt+Tabs. This is the standard behavior of Rocksmith 2014.
/// </summary>
void VolumeControl::DisableAltTabbingWithAudio() {
	_LOG_INIT;

	char patch[] = { 0x0 };
	MemUtil::PatchAdr((void*)Offsets::ptr_WindowNotInFocusValue, patch, 1);  // Return with the value of 0, "window out of focus", every time you alt+tab.
	*(char*)Offsets::ptr_IsWindowInFocus = (char)0x0; // Tell the game that you are currently alt+tabbed.
	_LOG("Stopped audio from being played in the background!" << std::endl);
	allowedAltTabbingWithAudio = false;
}