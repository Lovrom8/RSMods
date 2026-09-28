#pragma once

#include <atomic>
#include <chrono>
#include <cstdint>
#include <memory>

#include "MetronomeBeats.hpp"

namespace Metronome {
	// Linear gains, 0 (silent) to 1 (the click at its recorded level).
	struct ClickLevels {
		float accent = 1.f; // First beat of a measure.
		float beat = 1.f;   // Every other beat.
	};

	// Adds a click at each beat of the current song into the song's decoded audio.
	//
	// Configured from MainThread; MixInto runs on the audio thread, which must never block, lock or log
	// (a lock there has deadlocked the game before, see RSModsPlus docs/wwise-plugin-internals.md).
	// Hence every shared field is an atomic.
	class ClickMixer {
	public:
		void SetBeats(BeatMap beats);
		void ClearBeats();
		void SetLevels(ClickLevels levels);
		void SetOffset(std::chrono::milliseconds offset);

		void Mute();
		void Unmute();
		bool IsMuted() const;

		// Audio thread. `firstFrame` is the block's absolute position in the song, so seeks, loops and
		// rewinds need no special handling.
		void MixInto(int16_t* interleavedStereo, uint32_t frameCount, uint32_t firstFrame, uint32_t sampleRate) const;

	private:
		// TODO: The last reference to an old beat map must not be released on the audio thread (it frees memory).
		std::atomic<std::shared_ptr<const BeatMap>> beats;
		std::atomic<float> accentLevel = 1.f;
		std::atomic<float> beatLevel = 1.f;
		std::atomic<int> offsetMs = 0;
		std::atomic<bool> muted = false;
	};
}
