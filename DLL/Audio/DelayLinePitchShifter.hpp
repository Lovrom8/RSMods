#pragma once

#include "IInputProcessor.hpp"

#include <atomic>
#include <cstdint>
#include <vector>

namespace Audio
{
	// Pitch shifter using period-synchronous splicing, the technique real drop pedals use.
	// A single read tap trails the write head at the pitch ratio; whenever the tap drifts
	// one detected pitch period behind (or ahead), it jumps by exactly that period under a
	// short crossfade. Because the jump equals the waveform's own period, the output stays
	// periodic at the shifted pitch, unlike free-running dual-tap designs whose crossfade
	// cycle amplitude-modulates the signal at a rate guitar pitch trackers cannot ignore.
	//
	// The period comes from an AMDF detector running on a 4x-decimated copy of the input.
	// Chords have no single period, so they splice against the strongest one and come out
	// rougher than single notes, which is the accepted drop pedal trade.
	class DelayLinePitchShifter final : public IInputProcessor
	{
	public:
		explicit DelayLinePitchShifter(int semitones);

		void SetSemitones(int semitones);

		void Prepare(const CaptureFormat& format) override;
		void Process(float* samples, uint32_t frameCount) override;
		uint32_t GetLatencyFrames() const override;

	private:
		std::atomic<float> ratio;

		// Splicer state
		std::vector<float> ring;
		uint32_t writePosition = 0;
		double readDelay = 0.0;
		double fadeFromDelay = 0.0;
		int fadeLength = 64;
		int fadeRemaining = 0;

		// Period detector state
		std::vector<float> decimated;
		uint32_t decimatedPosition = 0;
		float decimationAccumulator = 0.0f;
		uint32_t decimationPhase = 0;
		uint32_t samplesSinceDetect = 0;
		int periodSamples = 0;
		int candidatePeriod = 0;
		int candidateVotes = 0;

		float ReadTap(double delay) const;
		void DetectPeriod();
		int RefineAtFullRate(int coarsePeriod) const;
	};
}
