#pragma once

#include "IInputProcessor.hpp"

namespace Audio
{
	// Leaves the signal untouched, so the capture hook can be verified end to end before
	// any pitch shifting exists to confuse the result. If the guitar still sounds and
	// detects normally with this installed, the interception point is correct.
	class PassthroughProcessor final : public IInputProcessor
	{
	public:
		void Prepare(const CaptureFormat&) override {}
		void Process(float*, uint32_t) override {}
		uint32_t GetLatencyFrames() const override { return 0; }
	};
}
