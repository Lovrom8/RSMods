#pragma once

#include <atomic>
#include <chrono>
#include <cstdint>
#include <memory>
#include <vector>

#include "MetronomeBeats.hpp"

namespace Metronome {
	// Linear gains: 0 is silent, 1 the click at full scale. Above 1 the click clips, making it louder still.
	struct ClickLevels {
		float accent = 1.f; // First beat of a measure.
		float beat = 1.f;   // Every other beat.
	};

	// One decoded block of a music stream, as the decoder hands it over.
	struct AudioBlock {
		int16_t* interleavedStereo = nullptr;
		uint32_t frameCount = 0;
		uint32_t firstFrame = 0;  // Absolute position in the stream, so seeks, loops and rewinds need no special handling.
		uint32_t totalFrames = 0; // Length of the whole stream.
		uint32_t sampleRate = 0;
	};

	// Adds a click at each beat of the current song into the song's decoded audio.
	//
	// Configured from MainThread; MixInto runs on the audio thread, which must never block, lock, allocate
	// or log (a lock there has deadlocked the game before, see RSModsPlus docs/wwise-plugin-internals.md).
	class ClickMixer {
	public:
		ClickMixer();
		~ClickMixer();

		void SetBeats(BeatMap beats);
		void ClearBeats();
		void SetLevels(ClickLevels levels);
		void SetOffset(std::chrono::milliseconds offset);
		// How far the song dips under each click so it cuts through: 0 (not at all) to 1 (silent).
		void SetDucking(float depth);

		void Mute();
		void Unmute();
		bool IsMuted() const;

		// Audio thread.
		void MixInto(const AudioBlock& block);

	private:
		struct ClickSounds {
			uint32_t sampleRate = 0;
			std::vector<float> accent;
			std::vector<float> beat;
			std::vector<float> duckingEnvelope;
		};

		std::vector<ClickSounds> clickSounds; // One set per supported sample rate, built up front.

		// The audio thread reads `published` without a lock. A replaced map is kept as `retired` until the next
		// replacement, at least a MainThread tick later, by which time the audio thread is done with it.
		std::atomic<const BeatMap*> published = nullptr;
		std::unique_ptr<const BeatMap> current;
		std::unique_ptr<const BeatMap> retired;

		std::atomic<float> accentLevel = 1.f;
		std::atomic<float> beatLevel = 1.f;
		std::atomic<float> ducking = 0.f;
		std::atomic<int> offsetMs = 0;
		std::atomic<bool> muted = false;

		void Publish(std::unique_ptr<const BeatMap> beats);
		const ClickSounds* SoundsFor(uint32_t sampleRate) const;
	};
}
