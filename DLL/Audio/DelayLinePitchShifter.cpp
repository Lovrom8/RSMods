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
		// 1360 real samples: ~35Hz. Reaches below drop-tuned low strings and bass, and
		// covers the shared fundamental of close-interval double stops: a fourth (4:3)
		// repeats at ~49Hz, outside the old 50Hz floor, so no jump could align both
		// notes and every splice tore one of them. With the shared period in range the
		// detector locks it and splices both notes cleanly, like it always did fifths.
		constexpr int MAX_LAG = 340;

		// A shorter near-minimum lag wins over the global minimum to avoid locking onto
		// two periods, the classic octave-down detection error.
		constexpr float OCTAVE_BIAS = 1.15f;

		// Acquiring a new period needs the strict gate; tracking the one already locked
		// is allowed at looser confidence. A decaying string falls below the strict gate
		// long before the note ends, and a frozen period drifts out of phase with
		// vibrato and intonation, so splices late in held notes land misaligned and are
		// heard as crackle on clean tones.
		constexpr float ACQUIRE_CONFIDENCE = 0.4f;
		constexpr float TRACK_CONFIDENCE = 0.6f;

		// Splice quality gates, in AlignJump's normalized units (0 identical segments,
		// ~1 uncorrelated). Above DEFER_QUALITY no acceptable alignment exists right now
		// (pick attack in the window, or a chord with no common period in range), so the
		// splice waits for better content while ring headroom lasts. Above EXTEND_QUALITY
		// the splice commits with a longer crossfade so residual misalignment smears into
		// softness instead of a pop. Tuned against a synthetic-signal measurement harness.
		constexpr float DEFER_QUALITY = 0.5f;
		constexpr float EXTEND_QUALITY = 0.25f;
		constexpr int HOLDOFF_SAMPLES = 64;
		constexpr double DEFER_MARGIN = 512.0;

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
		spliceHoldoff = 0;

		decimated.assign(DECIMATED_RING, 0.0f);
		decimatedPosition = 0;
		decimationAccumulator = 0.0f;
		decimationPhase = 0;
		samplesSinceDetect = 0;
		periodSamples = 480.0;	// ~100Hz starting guess until the detector locks
		candidatePeriod = 0.0;
		candidateVotes = 0;
	}

	float DelayLinePitchShifter::ReadTap(double delay) const
	{
		// Clamped, not asserted: extreme pitch ratios can walk a fading tap past either
		// edge, and reading the nearest valid sample for a few fade-tail samples is
		// inaudible where an out-of-range index is a crash.
		if (delay < 2.0) delay = 2.0;
		if (delay > RING_SAMPLES - 3.0) delay = RING_SAMPLES - 3.0;

		double readPosition = (double)writePosition - delay;
		if (readPosition < 0.0) readPosition += RING_SAMPLES;

		constexpr uint32_t mask = RING_SAMPLES - 1;
		const uint32_t index0 = (uint32_t)readPosition;
		const float t = (float)(readPosition - index0);

		// 4-point Catmull-Rom instead of linear: a fractionally moving linear tap is a
		// crude time-varying low-pass whose error modulates sample by sample, audible
		// as constant fuzz on clean tones with nothing downstream to mask it.
		const float xm1 = ring[(index0 - 1) & mask];
		const float x0 = ring[index0];
		const float x1 = ring[(index0 + 1) & mask];
		const float x2 = ring[(index0 + 2) & mask];

		const float c1 = 0.5f * (x1 - xm1);
		const float c2 = xm1 - 2.5f * x0 + 2.0f * x1 - 0.5f * x2;
		const float c3 = 0.5f * (x2 - xm1) + 1.5f * (x0 - x1);

		return ((c3 * t + c2) * t + c1) * t + x0;
	}

	double DelayLinePitchShifter::RefineAtFullRate(int coarsePeriod) const
	{
		// The detector's lag is quantized to the decimation factor, so a period built
		// from it is up to that many samples off. A narrow full-rate search around the
		// coarse value recovers the integer, and a parabola fit through the winning
		// valley recovers the fraction; a string's true period is never a whole number
		// of samples, and a rounded period misplaces every jump built from it.
		constexpr int SPAN = 6;
		constexpr uint32_t WINDOW = 256;
		constexpr uint32_t mask = RING_SAMPLES - 1;

		const uint32_t base = writePosition;

		float sums[2 * SPAN + 1];

		float bestSum = 1e9f;
		int bestIndex = -1;

		for (int offset = -SPAN; offset <= SPAN; ++offset)
		{
			const int period = coarsePeriod + offset;

			if (period < 8)
			{
				sums[offset + SPAN] = 1e9f;
				continue;
			}

			float sum = 0.0f;

			for (uint32_t n = 0; n < WINDOW; ++n)
			{
				const uint32_t a = (base - n) & mask;
				const uint32_t b = (base - n - (uint32_t)period) & mask;
				sum += std::fabs(ring[a] - ring[b]);
			}

			sums[offset + SPAN] = sum;

			if (sum < bestSum)
			{
				bestSum = sum;
				bestIndex = offset + SPAN;
			}
		}

		if (bestIndex < 0) return (double)coarsePeriod;

		double refined = (double)(coarsePeriod + bestIndex - SPAN);

		if (bestIndex > 0 && bestIndex < 2 * SPAN)
		{
			const float previous = sums[bestIndex - 1];
			const float next = sums[bestIndex + 1];

			if (previous < 1e8f && next < 1e8f)
			{
				const float denominator = previous - 2.0f * bestSum + next;

				if (denominator > 1e-9f)
				{
					float fraction = 0.5f * (previous - next) / denominator;
					if (fraction < -0.5f) fraction = -0.5f;
					if (fraction > 0.5f) fraction = 0.5f;
					refined += fraction;
				}
			}
		}

		return refined;
	}

	AlignedJump DelayLinePitchShifter::AlignJump(double fromDelay, double nominalJump, double maxJump) const
	{
		// H949-style de-glitching: the detector only centers this search, it does not
		// decide the splice. The committed offset is the one where the incoming tap is
		// measurably in phase with the outgoing one on the actual ring content, so the
		// crossfaded segments are near-identical and the splice disappears.
		//
		// The reach scales with the jump. The same relative period error grows with the
		// jump size, and on lightly played or noisy input the detector's confidence gate
		// stops updating entirely, leaving a stale nominal from the previous note. The
		// search has to be wide enough to recover true alignment on its own, WSOLA-style,
		// or weak notes warble and their crossfade cancellations dip the level far enough
		// for noise gates in the tone chain to chop the note.
		constexpr int MIN_SEARCH = 12;
		constexpr int MAX_SEARCH = 64;
		constexpr uint32_t WINDOW = 128;
		constexpr uint32_t mask = RING_SAMPLES - 1;

		const int center = (int)(nominalJump + 0.5);

		int search = center / 16;
		if (search < MIN_SEARCH) search = MIN_SEARCH;
		if (search > MAX_SEARCH) search = MAX_SEARCH;

		const uint32_t base = (uint32_t)((double)writePosition - fromDelay + RING_SAMPLES);

		// Window energy, for normalizing the winning difference into a level-independent
		// quality. An uncorrelated pair sums to roughly twice the window energy; identical
		// segments sum to zero. Silence has nothing to tear, so it counts as perfect.
		float baseSum = 0.0f;
		for (uint32_t n = 0; n < WINDOW; ++n)
		{
			baseSum += std::fabs(ring[(base - n) & mask]);
		}

		float sums[2 * MAX_SEARCH + 1];

		float bestSum = 1e9f;
		int bestIndex = -1;

		for (int offset = -search; offset <= search; ++offset)
		{
			const int jump = center + offset;

			if (jump < 8 || (double)jump > maxJump)
			{
				sums[offset + search] = 1e9f;
				continue;
			}

			float sum = 0.0f;

			for (uint32_t n = 0; n < WINDOW; ++n)
			{
				const uint32_t a = (base - n) & mask;
				const uint32_t b = (base - n - (uint32_t)jump) & mask;
				sum += std::fabs(ring[a] - ring[b]);
			}

			sums[offset + search] = sum;

			if (sum < bestSum)
			{
				bestSum = sum;
				bestIndex = offset + search;
			}
		}

		if (bestIndex < 0) return { nominalJump > maxJump ? maxJump : nominalJump, 1.0f };

		const float quality = baseSum > 1e-4f ? bestSum / (2.0f * baseSum) : 0.0f;

		double aligned = (double)(center + bestIndex - search);

		if (bestIndex > 0 && bestIndex < 2 * search)
		{
			const float previous = sums[bestIndex - 1];
			const float next = sums[bestIndex + 1];

			if (previous < 1e8f && next < 1e8f)
			{
				const float denominator = previous - 2.0f * bestSum + next;

				if (denominator > 1e-9f)
				{
					float fraction = 0.5f * (previous - next) / denominator;
					if (fraction < -0.5f) fraction = -0.5f;
					if (fraction > 0.5f) fraction = 0.5f;
					aligned += fraction;
				}
			}
		}

		return { aligned, quality };
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

		if (average <= 0.0f) return;

		const float confidence = best[chosenLag] / average;

		if (confidence < ACQUIRE_CONFIDENCE)
		{
			const double refined = RefineAtFullRate(chosenLag * (int)DECIMATION);

			// Small movements track immediately so vibrato and intonation stay live, but
			// a different period must win three consecutive confident detections before
			// the splicer believes it. A decaying string hovers at the confidence
			// threshold and its harmonic balance drifts, so single stray detections and
			// octave flips would otherwise yank the splice size around at random, which
			// is heard as pops on top of an otherwise clean note.
			const double tolerance = periodSamples / 8.0 + 2.0;

			if (std::fabs(refined - periodSamples) <= tolerance)
			{
				periodSamples = periodSamples * 0.75 + refined * 0.25;
				candidateVotes = 0;
			}
			else
			{
				const double candidateTolerance = refined / 8.0 + 2.0;

				if (std::fabs(refined - candidatePeriod) <= candidateTolerance)
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
		else if (confidence < TRACK_CONFIDENCE)
		{
			const double refined = RefineAtFullRate(chosenLag * (int)DECIMATION);
			const double tolerance = periodSamples / 8.0 + 2.0;

			if (std::fabs(refined - periodSamples) <= tolerance)
			{
				periodSamples = periodSamples * 0.75 + refined * 0.25;
			}
			else if (candidateVotes > 0)
			{
				--candidateVotes;
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
				// Splice by roughly a whole number of periods, sized so every jump is
				// about the same length regardless of the note. High notes would
				// otherwise splice many times more often with fades a quarter of their
				// tiny period, which is exactly where the popping concentrated; jumping
				// several periods at once restores low-string splice rates and fade
				// lengths everywhere. The nominal jump only centers AlignJump's search;
				// the committed jump is whatever actually lands in phase.
				int wholePeriods = (int)(TARGET_JUMP_SAMPLES / periodSamples + 0.5);
				if (wholePeriods < 1) wholePeriods = 1;
				if (wholePeriods > 8) wholePeriods = 8;

				const double nominalJump = wholePeriods * periodSamples;

				int nextFade = (int)(nominalJump * 0.25);
				if (nextFade < MIN_FADE_SAMPLES) nextFade = MIN_FADE_SAMPLES;
				if (nextFade > MAX_FADE_SAMPLES) nextFade = MAX_FADE_SAMPLES;

				const double upperBound = nominalJump + nextFade + 4.0;

				bool spliced = false;

				// Down-shifts drift behind; up-shifts catch the write head. A down-shift
				// jump is capped so the widened search can never carry the tap past the
				// write head and pin it on the ReadTap clamp. Splices with no acceptable
				// alignment are deferred (holdoff throttles the re-check so the search
				// does not run every sample) until content improves or headroom runs out,
				// then commit with a longer fade so the tear smears instead of popping.
				if (spliceHoldoff > 0)
				{
					--spliceHoldoff;
				}
				else if (readDelay > upperBound)
				{
					const AlignedJump aligned = AlignJump(readDelay - nominalJump, nominalJump, readDelay - 8.0);
					const bool canDefer = readDelay < RING_SAMPLES - DEFER_MARGIN;

					if (canDefer && aligned.quality > DEFER_QUALITY)
					{
						spliceHoldoff = HOLDOFF_SAMPLES;
					}
					else
					{
						if (aligned.quality > EXTEND_QUALITY)
						{
							nextFade *= 3;
							if (nextFade > MAX_FADE_SAMPLES) nextFade = MAX_FADE_SAMPLES;
						}

						fadeFromDelay = readDelay;
						readDelay -= aligned.jump;
						fadeLength = nextFade;
						fadeRemaining = nextFade;
						spliced = true;
					}
				}
				else if (readDelay < nextFade + 2.0)
				{
					const AlignedJump aligned = AlignJump(readDelay, nominalJump, RING_SAMPLES / 2.0);
					const bool canDefer = readDelay > 12.0;

					if (canDefer && aligned.quality > DEFER_QUALITY)
					{
						spliceHoldoff = 24;
					}
					else
					{
						if (aligned.quality > EXTEND_QUALITY)
						{
							nextFade *= 3;
							if (nextFade > MAX_FADE_SAMPLES) nextFade = MAX_FADE_SAMPLES;
						}

						fadeFromDelay = readDelay;
						readDelay += aligned.jump;

						// Up-shifts drain the outgoing tap toward zero delay during the
						// fade, and at extreme ratios a full-length fade would walk it
						// negative. Shorten the fade to the headroom the tap actually has.
						if (drift < 0.0)
						{
							const int headroom = (int)(fadeFromDelay / -drift) - 1;
							if (headroom < nextFade) nextFade = headroom < 8 ? 8 : headroom;
						}

						fadeLength = nextFade;
						fadeRemaining = nextFade;
						spliced = true;
					}
				}

				// The commit sample renders as the fade's gain-zero step: the old tap.
				// Rendering the new tap here put one full-amplitude new-tap sample
				// between two old-tap samples - a one-sample glitch at every splice,
				// inaudible when aligned and the dominant pop source when not.
				if (spliced)
				{
					samples[i] = ReadTap(fadeFromDelay);
					--fadeRemaining;
				}
				else
				{
					samples[i] = ReadTap(readDelay);
				}
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
