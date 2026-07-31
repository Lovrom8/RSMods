#pragma once

#include "CaptureFormat.h"

namespace Audio
{
	// Implementations run on the game's audio thread, inside IAudioCaptureClient::GetBuffer.
	// Process must not allocate, lock, log, or block for any reason.
	class IInputProcessor
	{
	public:
		virtual ~IInputProcessor() = default;

		// Called off the audio thread before any Process call, and again whenever the
		// capture format changes. Implementations do their allocating here.
		virtual void Prepare(const CaptureFormat& format) = 0;

		// frameCount frames of interleaved samples, channelCount per frame, edited in place.
		virtual void Process(float* samples, uint32_t frameCount) = 0;

		// Constant delay this implementation adds, in frames.
		//
		// Rocksmith calibrates input latency once. An implementation whose delay changed
		// when it engaged would shift every note's timing mid-session, so implementations
		// hold a fixed delay even while passing audio through untouched.
		virtual uint32_t GetLatencyFrames() const = 0;
	};
}
