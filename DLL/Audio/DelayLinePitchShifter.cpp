#include "stdafx.h"
#include "DelayLinePitchShifter.hpp"

#include <cmath>

namespace Audio
{
	namespace
	{
		constexpr uint32_t RING_SAMPLES = 4096;

		// Crossfade bounds, and the preferred splice size in samples. Jumps are a whole
		// number of periods rounded to this target, so splice rate and fade length stay
		// in the same comfortable range across the fretboard.
		constexpr int TARGET_JUMP_SAMPLES = 256;
		constexpr int MIN_FADE_SAMPLES = 16;
		constexpr int MAX_FADE_SAMPLES = 256;

		// Detector geometry, all in 4x-decimated samples (12kHz at a 48kHz stream).
		constexpr uint32_t DECIMATION = 4;
		constexpr uint32_t DECIMATED_RING = 1024;
		constexpr uint32_t DETECT_INTERVAL_SAMPLES = 256;

		// 256 decimated samples is ~21ms, about 1.5 periods of a drop-shifted low E.
		// Anything shorter tracks the low strings badly, which shows up directly as
		// worse detection on exactly those strings.
		constexpr uint32_t DETECT_WINDOW = 256;
		constexpr int MIN_LAG = 12;		// 48 real samples: 1kHz, above any guitar fundamental
		constexpr int MAX_LAG = 240;	// 960 real samples: 50Hz, below drop-tuned low strings

		// A shorter near-minimum lag wins over the global minimum to avoid locking onto
		// two periods, the classic octave-down detection error.
		constexpr float OCTAVE_BIAS = 1.15f;

		float SemitonesToRatio(int semitones)
		{
			return std::pow(2.0f, (float)semitones / 12.0f);
		}
	}

	DelayLinePitchShifter::DelayLinePitchShifter(int semitones)
		: ratio(SemitonesToRatio(semitones))
	{
	}

	void DelayLinePitchShifter::SetSemitones(int semitones)
	{
		ratio.store(SemitonesToRatio(semitones), std::memory_order_relaxed);
	}

	void DelayLinePitchShifter::Prepare(const CaptureFormat&)
	{
		ring.assign(RING_SAMPLES, 0.0f);
		writePosition = 0;
		readDelay = 130.0;	// Above the splice floor for the starting period guess
		fadeFromDelay = 0.0;
		fadeLength = 64;
		fadeRemaining = 0;

		decimated.assign(DECIMATED_RING, 0.0f);
		decimatedPosition = 0;
		decimationAccumulator = 0.0f;
		decimationPhase = 0;
		samplesSinceDetect = 0;
		periodSamples = 480;	// ~100Hz starting guess until the detector locks
		candidatePeriod = 0;
		candidateVotes = 0;
	}

	float DelayLinePitchShifter::ReadTap(double delay) const
	{
		double readPosition = (double)writePosition - delay;
		if (readPosition < 0.0) readPosition += RING_SAMPLES;

		const uint32_t index0 = (uint32_t)readPosition;
		const uint32_t index1 = index0 + 1 == RING_SAMPLES ? 0 : index0 + 1;
		const float fraction = (float)(readPosition - index0);

		return ring[index0] + (ring[index1] - ring[index0]) * fraction;
	}

	int DelayLinePitchShifter::RefineAtFullRate(int coarsePeriod) const
	{
		// The detector's lag is quantized to the decimation factor, so a splice using it
		// lands up to that many samples out of phase, and every misaligned splice is an
		// audible pop. A narrow full-rate search around the coarse value recovers the
		// exact alignment.
		constexpr int SPAN = 6;
		constexpr uint32_t WINDOW = 128;
		constexpr uint32_t mask = RING_SAMPLES - 1;

		const uint32_t base = writePosition;

		float bestSum = 1e9f;
		int bestPeriod = coarsePeriod;

		for (int period = coarsePeriod - SPAN; period <= coarsePeriod + SPAN; ++period)
		{
			if (period < 8) continue;

			float sum = 0.0f;

			for (uint32_t n = 0; n < WINDOW; ++n)
			{
				const uint32_t a = (base - n) & mask;
				const uint32_t b = (base - n - (uint32_t)period) & mask;
				sum += std::fabs(ring[a] - ring[b]);
			}

			if (sum < bestSum)
			{
				bestSum = sum;
				bestPeriod = period;
			}
		}

		return bestPeriod;
	}

	void DelayLinePitchShifter::DetectPeriod()
	{
		// AMDF: for each candidate lag, average |x[n] - x[n-lag]| over the window. The
		// true period gives the deepest valley. Runs on the audio thread, so it is fixed
		// work over preallocated buffers, roughly 29k adds per call.
		float best[MAX_LAG + 1];

		const uint32_t newest = decimatedPosition;	// one past the last written sample

		float globalMin = 1e9f;
		int globalLag = 0;

		for (int lag = MIN_LAG; lag <= MAX_LAG; ++lag)
		{
			float sum = 0.0f;

			for (uint32_t n = 0; n < DETECT_WINDOW; ++n)
			{
				const uint32_t a = (newest - 1 - n) & (DECIMATED_RING - 1);
				const uint32_t b = (newest - 1 - n - lag) & (DECIMATED_RING - 1);
				sum += std::fabs(decimated[a] - decimated[b]);
			}

			best[lag] = sum;

			if (sum < globalMin)
			{
				globalMin = sum;
				globalLag = lag;
			}
		}

		if (globalLag == 0) return;

		// Prefer the shortest lag whose valley is nearly as deep as the global best.
		int chosenLag = globalLag;

		for (int lag = MIN_LAG; lag < globalLag; ++lag)
		{
			if (best[lag] <= globalMin * OCTAVE_BIAS)
			{
				chosenLag = lag;
				break;
			}
		}

		// Confidence: a tonal signal has a valley well below its average difference.
		float average = 0.0f;
		for (int lag = MIN_LAG; lag <= MAX_LAG; ++lag) average += best[lag];
		average /= (float)(MAX_LAG - MIN_LAG + 1);

		if (average > 0.0f && best[chosenLag] < average * 0.4f)
		{
			const int refined = RefineAtFullRate(chosenLag * (int)DECIMATION);

			// Small movements track immediately so vibrato and intonation stay live, but
			// a different period must win three consecutive confident detections before
			// the splicer believes it. A decaying string hovers at the confidence
			// threshold and its harmonic balance drifts, so single stray detections and
			// octave flips would otherwise yank the splice size around at random, which
			// is heard as pops on top of an otherwise clean note.
			const int tolerance = periodSamples / 8 + 2;

			if (refined - periodSamples <= tolerance && periodSamples - refined <= tolerance)
			{
				periodSamples = (periodSamples * 3 + refined) / 4;
				candidateVotes = 0;
			}
			else
			{
				const int candidateTolerance = refined / 8 + 2;
				const int distance = refined > candidatePeriod ? refined - candidatePeriod : candidatePeriod - refined;

				if (distance <= candidateTolerance)
				{
					if (++candidateVotes >= 3)
					{
						periodSamples = refined;
						candidateVotes = 0;
					}
				}
				else
				{
					candidatePeriod = refined;
					candidateVotes = 1;
				}
			}
		}
		else if (candidateVotes > 0)
		{
			--candidateVotes;
		}
	}

	void DelayLinePitchShifter::Process(float* samples, uint32_t frameCount)
	{
		if (ring.empty()) return;

		const float pitchRatio = ratio.load(std::memory_order_relaxed);
		const double drift = 1.0 - (double)pitchRatio;

		for (uint32_t i = 0; i < frameCount; ++i)
		{
			const float incoming = samples[i];

			ring[writePosition] = incoming;

			// Feed the detector a 4x-decimated copy.
			decimationAccumulator += incoming;
			if (++decimationPhase == DECIMATION)
			{
				decimated[decimatedPosition] = decimationAccumulator * (1.0f / DECIMATION);
				decimatedPosition = (decimatedPosition + 1) & (DECIMATED_RING - 1);
				decimationAccumulator = 0.0f;
				decimationPhase = 0;
			}

			if (++samplesSinceDetect >= DETECT_INTERVAL_SAMPLES)
			{
				samplesSinceDetect = 0;
				DetectPeriod();
			}

			readDelay += drift;

			if (fadeRemaining > 0)
			{
				fadeFromDelay += drift;

				const float progress = 1.0f - (float)fadeRemaining / (float)fadeLength;
				const float gain = 0.5f - 0.5f * std::cos(3.14159265f * progress);

				samples[i] = ReadTap(fadeFromDelay) * (1.0f - gain) + ReadTap(readDelay) * gain;

				--fadeRemaining;
			}
			else
			{
				// Splice by a whole number of periods, sized so every jump is roughly the
				// same length regardless of the note. High notes would otherwise splice
				// many times more often with fades a quarter of their tiny period, which
				// is exactly where the popping concentrated; jumping several periods at
				// once restores low-string splice rates and fade lengths everywhere.
				int wholePeriods = (TARGET_JUMP_SAMPLES + periodSamples / 2) / periodSamples;
				if (wholePeriods < 1) wholePeriods = 1;
				if (wholePeriods > 8) wholePeriods = 8;

				const double jump = (double)(wholePeriods * periodSamples);

				int nextFade = (int)jump / 4;
				if (nextFade < MIN_FADE_SAMPLES) nextFade = MIN_FADE_SAMPLES;
				if (nextFade > MAX_FADE_SAMPLES) nextFade = MAX_FADE_SAMPLES;

				const double upperBound = jump + nextFade + 4.0;

				// Down-shifts drift behind; up-shifts catch the write head.
				if (readDelay > upperBound)
				{
					fadeFromDelay = readDelay;
					readDelay -= jump;
					fadeLength = nextFade;
					fadeRemaining = nextFade;
				}
				else if (readDelay < nextFade + 2.0)
				{
					fadeFromDelay = readDelay;
					readDelay += jump;
					fadeLength = nextFade;
					fadeRemaining = nextFade;
				}

				samples[i] = ReadTap(readDelay);
			}

			if (++writePosition == RING_SAMPLES) writePosition = 0;
		}
	}

	uint32_t DelayLinePitchShifter::GetLatencyFrames() const
	{
		// The tap trails by up to one detected period, so real latency varies with the
		// note, like a hardware drop pedal. Report a mid-range fixed figure.
		return 480;
	}
}
