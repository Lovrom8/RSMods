#include "../stdafx.h"
#include "MetronomeClickMixer.hpp"

#include <algorithm>
#include <numbers>

using Metronome::AudioBlock;
using Metronome::Beat;
using Metronome::BeatMap;
using Metronome::ClickMixer;

namespace {
	constexpr uint32_t kSupportedSampleRates[] = { 44100, 48000 };

	// Built-in click: a short sine burst with a fast decay, higher pitched on the accent.
	constexpr double kClickSeconds = 0.03;
	constexpr double kAttackSeconds = 0.001; // Ramp in, so the burst doesn't start with a pop.
	constexpr double kDecaySeconds = 0.006;
	constexpr double kAccentHz = 1760.0;
	constexpr double kBeatHz = 1320.0;
	constexpr float kClickPeak = 0.9f; // Of full scale. Songs are mastered near full scale, so ducking makes the room.

	// A song stream reaches its last beat; shorter streams (ambience, crowd loops) are left alone.
	constexpr double kStreamLengthToleranceSeconds = 1.0;

	std::vector<float> SynthesizeClick(double frequencyHz, uint32_t sampleRate) {
		const size_t length = static_cast<size_t>(kClickSeconds * sampleRate);
		std::vector<float> samples(length);
		for (size_t i = 0; i < length; ++i) {
			const double t = static_cast<double>(i) / sampleRate;
			const double envelope = std::min(1.0, t / kAttackSeconds) * std::exp(-t / kDecaySeconds);
			samples[i] = static_cast<float>(kClickPeak * envelope * std::sin(2.0 * std::numbers::pi * frequencyHz * t));
		}
		return samples;
	}

	// How far the song dips under a click, 0 to 1: up quickly with the click, back down smoothly by its end.
	std::vector<float> DuckingEnvelope(uint32_t sampleRate) {
		const size_t length = static_cast<size_t>(kClickSeconds * sampleRate);
		std::vector<float> envelope(length);
		for (size_t i = 0; i < length; ++i) {
			const double t = static_cast<double>(i) / sampleRate;
			const double fadeOut = 0.5 * (1.0 + std::cos(std::numbers::pi * t / kClickSeconds));
			envelope[i] = static_cast<float>(std::min(1.0, t / kAttackSeconds) * fadeOut);
		}
		return envelope;
	}

	int16_t DuckAndAdd(int16_t songSample, float songGain, float click) {
		const float mixed = static_cast<float>(songSample) * songGain + click * INT16_MAX;
		return static_cast<int16_t>(std::clamp(mixed, static_cast<float>(INT16_MIN), static_cast<float>(INT16_MAX)));
	}

	struct ClickVoice {
		const std::vector<float>& click;
		const std::vector<float>& duckingEnvelope;
		float level;
		float ducking; // Deepest dip of the song, 0 (none) to 1 (silent).
	};

	// Mixes the part of a click that falls inside the block. A click near the block's end continues in the next.
	void MixClick(const AudioBlock& block, int64_t clickStart, const ClickVoice& voice) {
		const int64_t blockStart = block.firstFrame;
		const int64_t blockEnd = blockStart + block.frameCount;
		const int64_t from = std::max(clickStart, blockStart);
		const int64_t to = std::min(clickStart + static_cast<int64_t>(voice.click.size()), blockEnd);

		for (int64_t frame = from; frame < to; ++frame) {
			const size_t i = static_cast<size_t>(frame - clickStart);
			const float click = voice.click[i] * voice.level;
			const float songGain = 1.f - voice.ducking * voice.duckingEnvelope[i];
			int16_t* stereoFrame = block.interleavedStereo + static_cast<ptrdiff_t>(frame - blockStart) * 2;
			stereoFrame[0] = DuckAndAdd(stereoFrame[0], songGain, click);
			stereoFrame[1] = DuckAndAdd(stereoFrame[1], songGain, click);
		}
	}

	bool StreamCoversBeats(const AudioBlock& block, const BeatMap& beats) {
		const double streamSeconds = static_cast<double>(block.totalFrames) / block.sampleRate;
		return streamSeconds >= beats.back().seconds - kStreamLengthToleranceSeconds;
	}
}

ClickMixer::ClickMixer() {
	for (uint32_t sampleRate : kSupportedSampleRates)
		clickSounds.push_back({ sampleRate, SynthesizeClick(kAccentHz, sampleRate), SynthesizeClick(kBeatHz, sampleRate), DuckingEnvelope(sampleRate) });
}

ClickMixer::~ClickMixer() = default;

void ClickMixer::SetBeats(BeatMap newBeats) {
	Publish(std::make_unique<const BeatMap>(std::move(newBeats)));
}

void ClickMixer::ClearBeats() {
	Publish(nullptr);
}

void ClickMixer::Publish(std::unique_ptr<const BeatMap> beats) {
	published.store(beats.get());
	retired = std::move(current);
	current = std::move(beats);
}

void ClickMixer::SetLevels(ClickLevels levels) {
	accentLevel.store(levels.accent);
	beatLevel.store(levels.beat);
}

void ClickMixer::SetDucking(float depth) {
	ducking.store(depth);
}

void ClickMixer::SetOffset(std::chrono::milliseconds offset) {
	offsetMs.store(static_cast<int>(offset.count()));
}

void ClickMixer::Mute() {
	muted.store(true);
}

void ClickMixer::Unmute() {
	muted.store(false);
}

bool ClickMixer::IsMuted() const {
	return muted.load();
}

const ClickMixer::ClickSounds* ClickMixer::SoundsFor(uint32_t sampleRate) const {
	for (const ClickSounds& sounds : clickSounds)
		if (sounds.sampleRate == sampleRate) return &sounds;
	return nullptr;
}

void ClickMixer::MixInto(const AudioBlock& block) {
	const BeatMap* beats = published.load();
	if (beats == nullptr || beats->empty() || muted.load()) return;

	const ClickSounds* sounds = SoundsFor(block.sampleRate);
	if (sounds == nullptr || !StreamCoversBeats(block, *beats)) return;

	const double offsetSeconds = offsetMs.load() / 1000.0;
	const int64_t blockEnd = static_cast<int64_t>(block.firstFrame) + block.frameCount;

	// Start from the earliest beat whose click could still be sounding at the block's first frame.
	const double earliestSeconds = static_cast<double>(block.firstFrame) / block.sampleRate - kClickSeconds - offsetSeconds;
	auto beat = std::lower_bound(beats->begin(), beats->end(), earliestSeconds,
		[](const Beat& b, double seconds) { return b.seconds < seconds; });

	const float duckingDepth = ducking.load();
	for (; beat != beats->end(); ++beat) {
		const int64_t clickStart = std::llround((beat->seconds + offsetSeconds) * block.sampleRate);
		if (clickStart >= blockEnd) break;

		const bool accent = beat->startsMeasure;
		const ClickVoice voice{ accent ? sounds->accent : sounds->beat, sounds->duckingEnvelope,
			accent ? accentLevel.load() : beatLevel.load(), duckingDepth };
		MixClick(block, clickStart, voice);
	}
}
