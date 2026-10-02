#pragma once

#include "MetronomeClickMixer.hpp"

// Hands every decoded block of streamed music to the metronome's mixer.
//
// The path through Wwise (Vorbis file-source factory, then the decoder's output function and its output
// layout) was found by Cheesewizard for RSModsPlus' Speaker Mode (DLL/Audio/SongShift/WwiseMusicHook.cpp,
// https://github.com/Cheesewizard/RSModsPlus). This is a separate implementation of the same idea.
//
// Menu sound effects come through Wwise's bank-source factory instead and are never touched.
namespace Metronome::AudioHook {
	// Must run before the game registers its codecs: early in startup, i.e. from OnInitialize.
	void Install(ClickMixer& mixer);

	// False until the first music decoder has been hooked.
	bool IsHooked();

	// MainThread. Logs what the hooks reported since the last call; the audio thread can't log.
	void LogStatus();
}
