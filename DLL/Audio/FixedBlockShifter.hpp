#pragma once

#include "IInputProcessor.hpp"

#include <cstdint>
#include <vector>

namespace Audio
{
	// Delays the signal by a fixed number of frames without altering it, standing in for a
	// pitch shifter whose algorithmic latency we want to judge in game before the DSP
	// exists. If playing through this delay still feels right and detects cleanly, a
	// shifter with the same latency is viable; if not, the shifter must be configured
	// smaller than this before it is worth integrating.
	class DelayProbeProcessor final : public IInputProcessor
	{
	public:
		explicit DelayProbeProcessor(uint32_t delayFrames);

		void Prepare(const CaptureFormat& format) override;
		void Process(float* samples, uint32_t frameCount) override;
		uint32_t GetLatencyFrames() const override;

	private:
		const uint32_t delayFrames;
		std::vector<float> ring;
		uint32_t ringPosition = 0;
	};
}
