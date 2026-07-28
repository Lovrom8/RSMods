#include "stdafx.h"
#include "FixedBlockShifter.hpp"

namespace Audio
{
	DelayProbeProcessor::DelayProbeProcessor(uint32_t delayFrames)
		: delayFrames(delayFrames)
	{
	}

	void DelayProbeProcessor::Prepare(const CaptureFormat&)
	{
		ring.assign(delayFrames, 0.0f);
		ringPosition = 0;
	}

	void DelayProbeProcessor::Process(float* samples, uint32_t frameCount)
	{
		if (delayFrames == 0 || ring.empty()) return;

		for (uint32_t i = 0; i < frameCount; ++i)
		{
			const float incoming = samples[i];
			samples[i] = ring[ringPosition];
			ring[ringPosition] = incoming;

			if (++ringPosition == delayFrames) ringPosition = 0;
		}
	}

	uint32_t DelayProbeProcessor::GetLatencyFrames() const
	{
		return delayFrames;
	}
}
