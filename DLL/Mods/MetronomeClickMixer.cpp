#include "MetronomeClickMixer.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <numbers>

using Metronome::Beat;
using Metronome::BeatMap;
using Metronome::ClickMixer;
using Metronome::ClockReading;
using Metronome::OutputBuffer;
using Metronome::SampleFormat;
using Metronome::SongPosition;

namespace {
	constexpr uint32_t kSupportedSampleRates[] = { 44100, 48000, 88200, 96000 };

	// Built-in click: a short sine burst with a fast decay, higher pitched on the accent.
	constexpr double kClickSeconds = 0.03;
	constexpr double kAttackSeconds = 0.001; // Ramp in, so the burst doesn't start with a pop.
	constexpr double kDecaySeconds = 0.006;
	constexpr double kAccentHz = 1760.0;
	constexpr double kBeatHz = 1320.0;
	constexpr float kClickPeak = 0.4f; // Envelope scale at 100% volume; loud enough to cut through distorted guitars.

	// Custom sounds are cut to this length, so a long file picked by mistake can't flood the mix.
	constexpr double kMaxCustomSoundSeconds = 1.0;

	// Charts often put beats past the end of their audio, some by minutes, so the song stream only has to cover part of
	// the beat map. Much shorter streams (ambience loops) are left alone.
	constexpr double kMinSongStreamCoverage = 0.5;

	// Song position gaps up to this many buffers are the clock's estimate catching up, not a seek: beats inside are still played.
	constexpr double kContinuousGapBuffers = 4.0;

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

	float LinearSample(const std::vector<float>& samples, double position) {
		const size_t index = static_cast<size_t>(position);
		if (index + 1 >= samples.size()) return index < samples.size() ? samples[index] : 0.f;
		const float fraction = static_cast<float>(position - index);
		return samples[index] + (samples[index + 1] - samples[index]) * fraction;
	}

	float Peak(const std::vector<float>& samples) {
		float peak = 0.f;
		for (float sample : samples) peak = std::max(peak, std::fabs(sample));
		return peak;
	}

	// Resampled to the output rate and scaled to the built-in click's peak, so 100% sounds alike whatever the file.
	std::vector<float> PrepareCustomSound(const Metronome::MonoSound& sound, uint32_t sampleRate, float targetPeak) {
		const double step = static_cast<double>(sound.sampleRate) / sampleRate;
		const size_t length = std::min(static_cast<size_t>(sound.samples.size() / step), static_cast<size_t>(kMaxCustomSoundSeconds * sampleRate));

		std::vector<float> samples(length);
		for (size_t i = 0; i < length; ++i) samples[i] = LinearSample(sound.samples, i * step);

		const float peak = Peak(samples);
		if (peak > 0.f)
			for (float& sample : samples) sample *= targetPeak / peak;
		return samples;
	}

	std::vector<float> SoundFor(const std::optional<Metronome::MonoSound>& custom, double builtInHz, uint32_t sampleRate) {
		std::vector<float> builtIn = SynthesizeClick(builtInHz, sampleRate);
		return custom ? PrepareCustomSound(*custom, sampleRate, Peak(builtIn)) : builtIn;
	}

	template <typename Integer>
	void AddToInteger(Integer& sample, float addition) {
		constexpr double kFullScale = static_cast<double>(std::numeric_limits<Integer>::max());
		const double mixed = static_cast<double>(sample) + addition * kFullScale;
		sample = static_cast<Integer>(std::clamp(mixed, -kFullScale - 1.0, kFullScale));
	}

	// The click goes into every channel. Past full scale it clips, which only makes it louder.
	void AddToFrame(const OutputBuffer& buffer, uint32_t frame, float addition) {
		for (uint32_t channel = 0; channel < buffer.channels; ++channel) {
			const size_t index = static_cast<size_t>(frame) * buffer.channels + channel;
			switch (buffer.format) {
			case SampleFormat::Float32: {
				float& sample = static_cast<float*>(buffer.interleavedSamples)[index];
				sample = std::clamp(sample + addition, -1.f, 1.f);
				break;
			}
			case SampleFormat::Int16:
				AddToInteger(static_cast<int16_t*>(buffer.interleavedSamples)[index], addition);
				break;
			case SampleFormat::Int32:
				AddToInteger(static_cast<int32_t*>(buffer.interleavedSamples)[index], addition);
				break;
			}
		}
	}
}

ClickMixer::ClickMixer() {
	SetSounds(std::nullopt, std::nullopt);
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

void ClickMixer::SetOffset(std::chrono::milliseconds offset) {
	offsetMs.store(static_cast<int>(offset.count()));
}

void ClickMixer::SetSounds(const std::optional<MonoSound>& accent, const std::optional<MonoSound>& beat) {
	auto set = std::make_unique<ClickSoundSet>();
	for (uint32_t sampleRate : kSupportedSampleRates)
		set->push_back({ sampleRate, SoundFor(accent, kAccentHz, sampleRate), SoundFor(beat, kBeatHz, sampleRate) });

	publishedSounds.store(set.get());
	retiredSounds = std::move(currentSounds);
	currentSounds = std::move(set);
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

bool ClickMixer::IsSongStream(uint32_t totalFrames, uint32_t sampleRate) const {
	const BeatMap* beats = published.load();
	if (beats == nullptr || beats->empty() || sampleRate == 0) return false;

	const double streamSeconds = static_cast<double>(totalFrames) / sampleRate;
	return streamSeconds >= beats->back().seconds * kMinSongStreamCoverage;
}

const ClickMixer::ClickSounds* ClickMixer::SoundsFor(const ClickSoundSet& set, uint32_t sampleRate) {
	for (const ClickSounds& sounds : set)
		if (sounds.sampleRate == sampleRate) return &sounds;
	return nullptr;
}

void ClickMixer::MixInto(const OutputBuffer& buffer, const ClockReading& clock) {
	const ClickSoundSet* set = publishedSounds.load();
	const ClickSounds* sounds = set ? SoundsFor(*set, buffer.sampleRate) : nullptr;
	if (sounds == nullptr) return;

	const BeatMap* beats = published.load();
	const bool playing = clock.song && clock.song->songSampleRate > 0 && beats != nullptr && !beats->empty() && !muted.load();
	if (playing)
		ScheduleClicks(*beats, *set, *sounds, *clock.song, clock.outputFrame, buffer.frameCount);
	else
		scheduling = false;

	// A click already sounding finishes even if the song pauses or the clicks are switched off.
	RenderActiveClicks(buffer, clock.outputFrame, set);
}

void ClickMixer::ScheduleClicks(const BeatMap& beats, const ClickSoundSet& set, const ClickSounds& sounds, const SongPosition& song, int64_t bufferStart, uint32_t frameCount) {
	const double bufferSongFrames = frameCount * song.songFramesPerOutputFrame;
	const double fromSongFrame = ScheduleFrom(song, bufferSongFrames);
	const double toSongFrame = song.songFrame + bufferSongFrames;
	scheduledUpToSongFrame = toSongFrame;

	const double offsetSeconds = offsetMs.load() / 1000.0;
	const double fromSeconds = fromSongFrame / song.songSampleRate - offsetSeconds;
	auto beat = std::lower_bound(beats.begin(), beats.end(), fromSeconds,
		[](const Beat& b, double seconds) { return b.seconds < seconds; });

	for (; beat != beats.end(); ++beat) {
		const double beatSongFrame = (beat->seconds + offsetSeconds) * song.songSampleRate;
		if (beatSongFrame >= toSongFrame) break;

		// Converted at the output rate, so the click itself is never stretched; a late beat from a catch-up gap starts now.
		const double outputFramesIn = std::max(0.0, (beatSongFrame - song.songFrame) / song.songFramesPerOutputFrame);
		const bool accent = beat->startsMeasure;
		Start({ bufferStart + std::llround(outputFramesIn), &set, accent ? &sounds.accent : &sounds.beat,
			accent ? accentLevel.load() : beatLevel.load() });
	}
}

// Picks up where the last buffer's song range ended, so no beat is played twice or skipped when the clock's estimate
// and its next tag disagree slightly. A seek, rewind or restart starts afresh at the buffer.
double ClickMixer::ScheduleFrom(const SongPosition& song, double bufferSongFrames) {
	const double gap = song.songFrame - scheduledUpToSongFrame;
	const bool continuous = scheduling && std::abs(gap) <= kContinuousGapBuffers * bufferSongFrames;
	scheduling = true;
	return continuous ? scheduledUpToSongFrame : song.songFrame;
}

void ClickMixer::Start(const ActiveClick& click) {
	if (activeClickCount == kMaxActiveClicks) {
		std::move(activeClicks + 1, activeClicks + kMaxActiveClicks, activeClicks);
		--activeClickCount;
	}
	activeClicks[activeClickCount++] = click;
}

void ClickMixer::RenderActiveClicks(const OutputBuffer& buffer, int64_t bufferStart, const ClickSoundSet* soundSet) {
	const int64_t bufferEnd = bufferStart + buffer.frameCount;
	int stillActive = 0;

	for (int i = 0; i < activeClickCount; ++i) {
		const ActiveClick& click = activeClicks[i];
		if (click.soundSet != soundSet) continue;
		const int64_t clickEnd = click.startFrame + static_cast<int64_t>(click.sound->size());
		const int64_t from = std::max(click.startFrame, bufferStart);
		const int64_t to = std::min(clickEnd, bufferEnd);

		for (int64_t frame = from; frame < to; ++frame)
			AddToFrame(buffer, static_cast<uint32_t>(frame - bufferStart), (*click.sound)[static_cast<size_t>(frame - click.startFrame)] * click.level);

		if (clickEnd > bufferEnd) activeClicks[stillActive++] = click;
	}
	activeClickCount = stillActive;
}
