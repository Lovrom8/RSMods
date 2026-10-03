#pragma once

#include "MetronomeClickMixer.hpp"

// Mixes the metronome into the game's final output.
//
// Two hooks: Wwise's Vorbis decoder tells which song frame goes into each slot of Wwise's output ring, and Wwise's
// output callback, which copies one slot per buffer to the audio device, is where the clicks are added.
//
// The decoder path (Vorbis file-source factory, the decoder's output function and its output layout) was found by
// Cheesewizard for RSModsPlus' Speaker Mode (DLL/Audio/SongShift/WwiseMusicHook.cpp,
// https://github.com/Cheesewizard/RSModsPlus); this is a separate implementation. Menu sound effects come through
// Wwise's bank-source factory instead and are never touched.
namespace Metronome::AudioHook {
	// Must run before the game registers its codecs: early in startup, i.e. from OnInitialize.
	void Install(ClickMixer& mixer);

	// False until both the music decoder and the output callback are hooked.
	bool IsHooked();

	// MainThread. Logs what the hooks reported since the last call; the audio thread can't log.
	void LogStatus();
}
