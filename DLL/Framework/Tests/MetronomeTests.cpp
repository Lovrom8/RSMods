#include "../../Mods/MetronomeClickMixer.hpp"
#include "../../Mods/MetronomeCountIn.hpp"
#include "../../Mods/MetronomeSongClock.hpp"

#include <cmath>
#include <cstdint>
#include <functional>
#include <iostream>
#include <optional>
#include <string>
#include <vector>

using Metronome::BeatMap;
using Metronome::ChartEvent;
using Metronome::ClickMixer;
using Metronome::ClockReading;
using Metronome::MonoSound;
using Metronome::OutputBuffer;
using Metronome::PhraseIteration;
using Metronome::SampleFormat;
using Metronome::SongClock;
using Metronome::SongPosition;

namespace {
	int g_failures = 0;

	void Check(const std::string& name, bool ok) {
		if (ok) {
			std::cout << "  PASS  " << name << "\n";
			return;
		}
		++g_failures;
		std::cout << "  FAIL  " << name << "\n";
	}

	constexpr uint32_t kRate = 48000;
	constexpr uint32_t kBufferFrames = 128;
	constexpr int64_t kClickFrames = 1440; // The built-in click lasts 30 ms.

	bool Near(double a, double b, double tolerance) { return std::abs(a - b) <= tolerance; }

	// The first beat starts a measure; times in seconds.
	BeatMap Beats(std::initializer_list<double> seconds) {
		BeatMap beats;
		for (double s : seconds) beats.push_back({ s, beats.empty() });
		return beats;
	}

	BeatMap BeatsEvery(double interval, double until) {
		BeatMap beats;
		for (double s = interval; s < until; s += interval) beats.push_back({ s, false });
		return beats;
	}

	using SongAt = std::function<std::optional<SongPosition>(int64_t outputFrame)>;

	SongAt SongInStep(double songFramesAhead = 0.0, double rate = 1.0) {
		return [=](int64_t outputFrame) { return SongPosition{ outputFrame * rate + songFramesAhead, rate, kRate }; };
	}

	// Mixes into silent stereo float buffers and returns the left channel.
	std::vector<float> Render(ClickMixer& mixer, int64_t frames, const SongAt& songAt, uint32_t outputRate = kRate) {
		std::vector<float> left;
		for (int64_t start = 0; start < frames; start += kBufferFrames) {
			std::vector<float> buffer(kBufferFrames * 2);
			mixer.MixInto(OutputBuffer{ buffer.data(), SampleFormat::Float32, 2, kBufferFrames, outputRate }, ClockReading{ start, songAt(start) });
			for (uint32_t i = 0; i < kBufferFrames; ++i) left.push_back(buffer[i * 2]);
		}
		return left;
	}

	// Output frames where a click starts. A click's first sample is 0 (it ramps in), so its start is one before the
	// first audible sample after a stretch of silence.
	std::vector<int64_t> ClickStarts(const std::vector<float>& samples) {
		std::vector<int64_t> starts;
		int64_t silentRun = kClickFrames;
		for (int64_t i = 0; i < static_cast<int64_t>(samples.size()); ++i) {
			const bool audible = std::abs(samples[i]) > 1e-7f;
			if (audible && silentRun >= 200) starts.push_back(i - 1);
			silentRun = audible ? 0 : silentRun + 1;
		}
		return starts;
	}

	float Peak(const std::vector<float>& samples) {
		float peak = 0.f;
		for (float s : samples) peak = std::max(peak, std::abs(s));
		return peak;
	}

	void TestClickLandsOnTheBeatsSample() {
		ClickMixer mixer;
		mixer.SetBeats(Beats({ 0.01, 0.5 }));
		const auto starts = ClickStarts(Render(mixer, kRate, SongInStep()));
		Check("click starts on the beat's exact output sample", starts == std::vector<int64_t>{ 480, 24000 });
	}

	void TestSongPositionMapsToOutput() {
		ClickMixer mixer;
		mixer.SetBeats(Beats({ 0.1 }));
		const auto starts = ClickStarts(Render(mixer, kRate / 2, SongInStep(1000.0)));
		Check("click follows the song position the clock reports", starts == std::vector<int64_t>{ 4800 - 1000 });
	}

	void TestEveryBeatClicksOnce() {
		ClickMixer mixer;
		mixer.SetBeats(BeatsEvery(0.25, 2.0));
		Check("every beat clicks exactly once", ClickStarts(Render(mixer, 2 * kRate, SongInStep())).size() == 7);
	}

	// The clock's estimate and its next tag disagree by a little from buffer to buffer under Riff Repeater.
	void TestJitteringClockStillClicksEachBeatOnce() {
		ClickMixer mixer;
		mixer.SetBeats(BeatsEvery(0.25, 2.0));
		auto jitter = [](int64_t outputFrame) {
			const double wobble = (outputFrame / kBufferFrames) % 2 ? 20.0 : -20.0;
			return std::optional<SongPosition>(SongPosition{ outputFrame + wobble, 1.0, kRate });
		};
		const auto starts = ClickStarts(Render(mixer, 2 * kRate, jitter));
		bool onTime = starts.size() == 7;
		for (size_t i = 0; onTime && i < starts.size(); ++i) onTime = Near(static_cast<double>(starts[i]), (i + 1) * 12000.0, 40.0);
		Check("a jittering song position neither doubles nor drops a beat", onTime);
	}

	void TestRiffRepeaterKeepsTheClickUnstretched() {
		ClickMixer mixer;
		mixer.SetBeats(Beats({ 0.1 }));
		const auto samples = Render(mixer, kRate / 2, SongInStep(0.0, 0.5));
		const auto starts = ClickStarts(samples);
		int64_t lastAudible = 0;
		for (int64_t i = 0; i < static_cast<int64_t>(samples.size()); ++i)
			if (std::abs(samples[i]) > 1e-7f) lastAudible = i;
		Check("at half speed a beat lands at twice its output time", starts == std::vector<int64_t>{ 9600 });
		Check("at half speed the click keeps its length", !starts.empty() && lastAudible - starts[0] < kClickFrames);
	}

	void TestSeekBackPlaysBeatsAgain() {
		ClickMixer mixer;
		mixer.SetBeats(Beats({ 0.1 }));
		auto rewind = [](int64_t outputFrame) {
			const int64_t played = outputFrame % kRate; // Back to the start every second.
			return std::optional<SongPosition>(SongPosition{ static_cast<double>(played), 1.0, kRate });
		};
		Check("rewinding plays the beat again", ClickStarts(Render(mixer, 2 * kRate, rewind)).size() == 2);
	}

	void TestMutedMixerIsSilent() {
		ClickMixer mixer;
		mixer.SetBeats(BeatsEvery(0.25, 1.0));
		mixer.Mute();
		Check("muted clicks are silent", Peak(Render(mixer, kRate, SongInStep())) == 0.f);
	}

	void TestPauseLetsTheSoundingClickFinish() {
		ClickMixer mixer;
		mixer.SetBeats(Beats({ 0.01, 0.5 }));
		auto pausedAfterFirstBeat = [](int64_t outputFrame) -> std::optional<SongPosition> {
			if (outputFrame >= 512) return std::nullopt;
			return SongPosition{ static_cast<double>(outputFrame), 1.0, kRate };
		};
		const auto samples = Render(mixer, kRate, pausedAfterFirstBeat);
		Check("no new click while paused", ClickStarts(samples) == std::vector<int64_t>{ 480 });
		Check("the click sounding at the pause finishes", Peak(std::vector<float>(samples.begin() + 900, samples.begin() + 1200)) > 0.f);
	}

	void TestOffsetMovesTheClicks() {
		ClickMixer mixer;
		mixer.SetBeats(Beats({ 0.1 }));
		mixer.SetOffset(std::chrono::milliseconds(10));
		Check("a positive offset clicks later", ClickStarts(Render(mixer, kRate / 2, SongInStep())) == std::vector<int64_t>{ 4800 + 480 });
	}

	void TestIntegerOutput() {
		ClickMixer mixer;
		mixer.SetBeats(Beats({ 0.01 }));
		std::vector<int16_t> buffer(kBufferFrames * 2);
		bool audible = false;
		for (int64_t start = 0; start < 2048; start += kBufferFrames) {
			std::fill(buffer.begin(), buffer.end(), int16_t{ 0 });
			mixer.MixInto(OutputBuffer{ buffer.data(), SampleFormat::Int16, 2, kBufferFrames, kRate },
				ClockReading{ start, SongPosition{ static_cast<double>(start), 1.0, kRate } });
			for (int16_t s : buffer) audible = audible || s != 0;
		}
		Check("16-bit output gets the click", audible);
	}

	void TestUnsupportedRateIsSilent() {
		ClickMixer mixer;
		mixer.SetBeats(Beats({ 0.01 }));
		Check("an unsupported output rate stays silent", Peak(Render(mixer, 4096, SongInStep(), 22050)) == 0.f);
	}

	void TestCustomSoundMatchesTheBuiltInLoudness() {
		ClickMixer builtIn;
		builtIn.SetBeats(Beats({ 0.01 }));
		const float builtInPeak = Peak(Render(builtIn, 4096, SongInStep()));

		MonoSound loud{ std::vector<float>(2400), 24000 };
		for (size_t i = 0; i < loud.samples.size(); ++i) loud.samples[i] = 0.9f * std::sin(i * 0.3f);
		ClickMixer custom;
		custom.SetBeats(Beats({ 0.01 }));
		custom.SetSounds(loud, std::nullopt);
		const float customPeak = Peak(Render(custom, 8192, SongInStep()));

		Check("a custom sound plays as loud as the built-in click", Near(customPeak, builtInPeak, 0.02));
	}

	void TestCustomSoundIsCutToOneSecond() {
		MonoSound long_{ std::vector<float>(3 * kRate, 0.5f), kRate };
		ClickMixer mixer;
		mixer.SetBeats(Beats({ 0.01 }));
		mixer.SetSounds(long_, std::nullopt);
		const auto samples = Render(mixer, 3 * kRate, SongInStep());
		Check("a long custom sound stops after a second", Peak(std::vector<float>(samples.begin() + kRate + 1000, samples.end())) == 0.f);
	}

	void TestIsSongStream() {
		ClickMixer mixer;
		mixer.SetBeats(Beats({ 10.0, 200.0 }));
		Check("a stream reaching the last beat is the song", mixer.IsSongStream(201 * kRate, kRate));
		Check("a short stream (preview, ambience) is not the song", !mixer.IsSongStream(30 * kRate, kRate));
	}

	// Song clock

	void TestClockReportsATagExactly() {
		SongClock clock;
		clock.TagSlot(3, 1000, kRate);
		const ClockReading reading = clock.Read(3, kBufferFrames, kRate);
		Check("a fresh tag is the buffer's song position", reading.song && reading.song->songFrame == 1000.0 && reading.outputFrame == 0);
	}

	void TestClockEstimatesBetweenTags() {
		SongClock clock;
		clock.TagSlot(3, 1000, kRate);
		clock.Read(3, kBufferFrames, kRate);
		const ClockReading next = clock.Read(4, kBufferFrames, kRate);
		Check("a buffer without a tag continues from the last one", next.song && Near(next.song->songFrame, 1000.0 + kBufferFrames, 0.01));
	}

	void TestClockMeasuresRiffRepeaterSpeed() {
		SongClock clock;
		std::optional<SongPosition> last;
		for (uint32_t buffer = 0; buffer < 64; ++buffer) {
			const uint32_t slot = buffer % SongClock::kRingSlots;
			if (buffer % 2 == 0) clock.TagSlot(slot, buffer / 2 * kBufferFrames, kRate); // One song block per two buffers.
			last = clock.Read(slot, kBufferFrames, kRate).song;
		}
		Check("half speed is measured as half a song frame per output frame", last && Near(last->songFramesPerOutputFrame, 0.5, 0.01));
	}

	void TestClockGoesStaleWhenPaused() {
		SongClock clock;
		clock.TagSlot(0, 0, kRate);
		clock.Read(0, kBufferFrames, kRate);
		std::optional<SongPosition> song;
		for (uint32_t buffer = 1; buffer < 32; ++buffer) song = clock.Read(buffer % SongClock::kRingSlots, kBufferFrames, kRate).song;
		Check("without tags the song counts as stopped", !song);
	}

	void TestClockResetsItsSpeedAfterASeek() {
		SongClock clock;
		for (uint32_t buffer = 0; buffer < 32; ++buffer) {
			const uint32_t slot = buffer % SongClock::kRingSlots;
			if (buffer % 2 == 0) clock.TagSlot(slot, buffer / 2 * kBufferFrames, kRate);
			clock.Read(slot, kBufferFrames, kRate);
		}
		clock.TagSlot(0, 10 * kRate, kRate);
		const auto song = clock.Read(0, kBufferFrames, kRate).song;
		Check("a seek drops the measured speed", song && song->songFrame == 10.0 * kRate && song->songFramesPerOutputFrame == 1.0);
	}

	void TestClockAccountsForResampling() {
		SongClock clock;
		clock.TagSlot(0, 0, 44100);
		const auto song = clock.Read(0, kBufferFrames, kRate).song;
		Check("a 44.1 kHz song on a 48 kHz device advances slower", song && Near(song->songFramesPerOutputFrame, 44100.0 / kRate, 1e-9));
	}

	// Count-in

	const std::vector<std::string> kPhrases{ "COUNT", "p0", "END" };
	const std::vector<PhraseIteration> kIterations{ { 0, 8.922, 10.351 }, { 1, 10.351, 21.779 }, { 2, 21.779, 30.0 } };

	void TestCountInWithClicksIsSilenced() {
		const auto range = Metronome::GameCountIn(kPhrases, kIterations, { { 8.922, "B0" }, { 9.398, "B0" }, { 9.874, "B0" } });
		Check("COUNT with B0 events is the game's count-in", range && range->start == 8.922 && range->end == 10.351);
		Check("B1 events count too", Metronome::GameCountIn(kPhrases, kIterations, { { 9.0, "B1" } }).has_value());
	}

	void TestCountInWithoutClicksPlays() {
		Check("COUNT without B0/B1 events leaves the metronome on", !Metronome::GameCountIn(kPhrases, kIterations, { { 8.922, "TS:6/8" } }));
	}

	void TestClicksWithoutCountInPlay() {
		const std::vector<std::string> noCount{ "intro", "p0" };
		Check("B0 events without a COUNT phrase leave the metronome on", !Metronome::GameCountIn(noCount, kIterations, { { 8.922, "B0" } }));
	}

	void TestClicksOutsideCountInDontCount() {
		Check("B0 events outside COUNT don't make it a count-in", !Metronome::GameCountIn(kPhrases, kIterations, { { 12.0, "B0" } }));
	}

	void TestCountInWithoutIterationPlays() {
		Check("a COUNT phrase never placed in the chart is ignored", !Metronome::GameCountIn(kPhrases, { { 1, 10.351, 21.779 } }, { { 9.0, "B0" } }));
	}
}

int main() {
	std::cout << "Click mixer\n";
	TestClickLandsOnTheBeatsSample();
	TestSongPositionMapsToOutput();
	TestEveryBeatClicksOnce();
	TestJitteringClockStillClicksEachBeatOnce();
	TestRiffRepeaterKeepsTheClickUnstretched();
	TestSeekBackPlaysBeatsAgain();
	TestMutedMixerIsSilent();
	TestPauseLetsTheSoundingClickFinish();
	TestOffsetMovesTheClicks();
	TestIntegerOutput();
	TestUnsupportedRateIsSilent();
	TestCustomSoundMatchesTheBuiltInLoudness();
	TestCustomSoundIsCutToOneSecond();
	TestIsSongStream();

	std::cout << "Song clock\n";
	TestClockReportsATagExactly();
	TestClockEstimatesBetweenTags();
	TestClockMeasuresRiffRepeaterSpeed();
	TestClockGoesStaleWhenPaused();
	TestClockResetsItsSpeedAfterASeek();
	TestClockAccountsForResampling();

	std::cout << "Count-in\n";
	TestCountInWithClicksIsSilenced();
	TestCountInWithoutClicksPlays();
	TestClicksWithoutCountInPlay();
	TestClicksOutsideCountInDontCount();
	TestCountInWithoutIterationPlays();

	if (g_failures > 0) {
		std::cout << g_failures << " metronome test(s) failed\n";
		return 1;
	}
	std::cout << "ALL METRONOME TESTS PASSED\n";
	return 0;
}
