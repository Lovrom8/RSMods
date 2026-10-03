#pragma once

#include <atomic>
#include <chrono>
#include <cstdint>
#include <memory>
#include <vector>

#include "MetronomeBeats.hpp"
#include "MetronomeSongClock.hpp"

namespace Metronome {
	// Linear gains: 0 is silent, 1 the click at its normal level, up to 3.
	struct ClickLevels {
		float accent = 1.f; // First beat of a measure.
		float beat = 1.f;   // Every other beat.
	};

	enum class SampleFormat { Float32, Int16, Int32 };

	// A buffer of the game's final mix, about to go to the audio device.
	struct OutputBuffer {
		void* interleavedSamples = nullptr;
		SampleFormat format = SampleFormat::Float32;
		uint32_t channels = 0;
		uint32_t frameCount = 0;
		uint32_t sampleRate = 0;
	};

	// Adds a click at each beat of the current song into the game's final mix, after its volume mixer, so the clicks
	// keep their own volume whatever the in-game Song volume, and stay crisp when Riff Repeater slows the song down.
	//
	// Configured from MainThread; the audio threads must never block, lock, allocate or log (a lock there has
	// deadlocked the game before, see RSModsPlus docs/wwise-plugin-internals.md).
	class ClickMixer {
	public:
		ClickMixer();
		~ClickMixer();

		void SetBeats(BeatMap beats);
		void ClearBeats();
		void SetLevels(ClickLevels levels);
		void SetOffset(std::chrono::milliseconds offset);

		void Mute();
		void Unmute();
		bool IsMuted() const;

		// Render thread: whether a decoded music stream is the song whose beats are loaded.
		bool IsSongStream(uint32_t totalFrames, uint32_t sampleRate) const;

		// Output thread, once per buffer.
		void MixInto(const OutputBuffer& buffer, const ClockReading& clock);

	private:
		struct ClickSounds {
			uint32_t sampleRate = 0;
			std::vector<float> accent;
			std::vector<float> beat;
		};

		struct ActiveClick {
			int64_t startFrame = 0; // Output frame.
			const std::vector<float>* sound = nullptr;
			float level = 0.f;
		};

		static constexpr int kMaxActiveClicks = 4;

		std::vector<ClickSounds> clickSounds; // One set per supported output rate, built up front.

		// The audio threads read `published` without a lock. A replaced map is kept as `retired` until the next
		// replacement, at least a MainThread tick later, by which time the audio threads are done with it.
		std::atomic<const BeatMap*> published = nullptr;
		std::unique_ptr<const BeatMap> current;
		std::unique_ptr<const BeatMap> retired;

		std::atomic<float> accentLevel = 1.f;
		std::atomic<float> beatLevel = 1.f;
		std::atomic<int> offsetMs = 0;
		std::atomic<bool> muted = false;

		// Output thread only.
		ActiveClick activeClicks[kMaxActiveClicks];
		int activeClickCount = 0;
		bool scheduling = false;
		double scheduledUpToSongFrame = 0.0;

		void Publish(std::unique_ptr<const BeatMap> beats);
		const ClickSounds* SoundsFor(uint32_t sampleRate) const;
		void ScheduleClicks(const BeatMap& beats, const ClickSounds& sounds, const SongPosition& song, int64_t bufferStart, uint32_t frameCount);
		double ScheduleFrom(const SongPosition& song, double bufferSongFrames);
		void Start(const ActiveClick& click);
		void RenderActiveClicks(const OutputBuffer& buffer, int64_t bufferStart);
	};
}
