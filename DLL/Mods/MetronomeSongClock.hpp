#pragma once

#include <atomic>
#include <cstdint>
#include <optional>

namespace Metronome {
	// Where the song is in a buffer leaving Wwise's output callback.
	struct SongPosition {
		double songFrame = 0.0;                // Song frame at the buffer's first output frame.
		double songFramesPerOutputFrame = 1.0; // Below 1 while Riff Repeater slows the song down.
		uint32_t songSampleRate = 0;
	};

	struct ClockReading {
		int64_t outputFrame = 0;           // Output frames played before this buffer.
		std::optional<SongPosition> song;  // Empty while the song isn't playing (paused, menus).
	};

	// Wwise renders its output into a ring of slots, one decoder block of the song per slot at normal speed, and its
	// output callback copies one slot per call. Tagging each slot with the song frame decoded into it gives the song
	// position of every output buffer, whatever the device's buffer size or latency. Measured exact to the sample.
	//
	// TagSlot runs on Wwise's render thread, Read on the output thread; neither locks.
	class SongClock {
	public:
		static constexpr uint32_t kRingSlots = 16;

		// Render thread: the song block starting at `songFrame` is being rendered into `slot`.
		void TagSlot(uint32_t slot, uint32_t songFrame, uint32_t songSampleRate);

		// Output thread, once per buffer: the buffer about to be played came from `slot`.
		ClockReading Read(uint32_t slot, uint32_t frameCount, uint32_t outputSampleRate);

	private:
		struct Tag {
			std::atomic<uint32_t> sequence = 0;
			std::atomic<uint32_t> songFrame = 0;
			std::atomic<uint32_t> songSampleRate = 0;
		};

		struct Anchor {
			double songFrame = 0.0;
			int64_t outputFrame = 0;
		};

		static constexpr int kAnchorHistory = 16;

		Tag tags[kRingSlots];
		std::atomic<uint32_t> lastSequence = 0;

		// Output thread only.
		uint32_t seenSequence[kRingSlots] = {};
		int64_t outputFrame = 0;
		Anchor anchors[kAnchorHistory];
		int anchorCount = 0;
		int newestAnchor = 0;
		double songFramesPerOutputFrame = 1.0;
		uint32_t songSampleRate = 0;

		std::optional<Anchor> FreshAnchor(uint32_t slot, int64_t bufferStart);
		void AddAnchor(const Anchor& anchor, uint32_t outputSampleRate);
		const Anchor& Newest() const;
		const Anchor& Oldest() const;
	};
}
