#include "MetronomeSongClock.hpp"

#include <algorithm>
#include <cmath>

using Metronome::ClockReading;
using Metronome::SongClock;
using Metronome::SongPosition;

namespace {
	// With no fresh tag for this long the song has stopped (paused, or left), so no position is reported.
	constexpr int64_t kStaleOutputFrames = 2048;

	// A tag this far from where the song should be is a seek, rewind or restart: the rate history no longer applies.
	constexpr double kJumpSongFrames = 4096.0;

	// Riff Repeater goes down to 1% speed; above full speed only resampling (e.g. a 44.1 kHz song on a 48 kHz device) moves it.
	constexpr double kMinRate = 0.005;
	constexpr double kMaxRate = 2.5;
}

void SongClock::TagSlot(uint32_t slot, uint32_t songFrame, uint32_t sampleRate) {
	Tag& tag = tags[slot % kRingSlots];
	tag.songFrame.store(songFrame);
	tag.songSampleRate.store(sampleRate);
	tag.sequence.store(lastSequence.fetch_add(1) + 1);
}

ClockReading SongClock::Read(uint32_t slot, uint32_t frameCount, uint32_t outputSampleRate) {
	const int64_t bufferStart = outputFrame;
	outputFrame += frameCount;

	if (std::optional<Anchor> anchor = FreshAnchor(slot, bufferStart)) {
		AddAnchor(*anchor, outputSampleRate);
		return { bufferStart, SongPosition{ anchor->songFrame, songFramesPerOutputFrame, songSampleRate } };
	}

	// Riff Repeater feeds fewer song blocks than output buffers, so some buffers carry no tag of their own.
	if (anchorCount == 0 || bufferStart - Newest().outputFrame > kStaleOutputFrames)
		return { bufferStart, std::nullopt };

	const double songFrame = Newest().songFrame + (bufferStart - Newest().outputFrame) * songFramesPerOutputFrame;
	return { bufferStart, SongPosition{ songFrame, songFramesPerOutputFrame, songSampleRate } };
}

// A tag counts once, and only if it wasn't rewritten while being read.
std::optional<SongClock::Anchor> SongClock::FreshAnchor(uint32_t slot, int64_t bufferStart) {
	Tag& tag = tags[slot % kRingSlots];
	const uint32_t sequence = tag.sequence.load();
	if (sequence == 0 || sequence == seenSequence[slot % kRingSlots]) return std::nullopt;

	const uint32_t songFrame = tag.songFrame.load();
	const uint32_t sampleRate = tag.songSampleRate.load();
	if (tag.sequence.load() != sequence) return std::nullopt;

	seenSequence[slot % kRingSlots] = sequence;
	songSampleRate = sampleRate;
	return Anchor{ static_cast<double>(songFrame), bufferStart };
}

void SongClock::AddAnchor(const Anchor& anchor, uint32_t outputSampleRate) {
	const bool continuesSong = anchorCount > 0
		&& std::abs(anchor.songFrame - (Newest().songFrame + (anchor.outputFrame - Newest().outputFrame) * songFramesPerOutputFrame)) < kJumpSongFrames;
	if (!continuesSong) {
		anchorCount = 0;
		songFramesPerOutputFrame = outputSampleRate > 0 ? static_cast<double>(songSampleRate) / outputSampleRate : 1.0;
	}

	newestAnchor = (newestAnchor + 1) % kAnchorHistory;
	anchors[newestAnchor] = anchor;
	anchorCount = std::min(anchorCount + 1, kAnchorHistory);

	// Measured over several anchors: under Riff Repeater, single steps alternate between one and two buffers.
	const int64_t elapsed = Newest().outputFrame - Oldest().outputFrame;
	if (anchorCount >= 2 && elapsed > 0)
		songFramesPerOutputFrame = std::clamp((Newest().songFrame - Oldest().songFrame) / elapsed, kMinRate, kMaxRate);
}

const SongClock::Anchor& SongClock::Newest() const {
	return anchors[newestAnchor];
}

const SongClock::Anchor& SongClock::Oldest() const {
	return anchors[(newestAnchor - anchorCount + 1 + kAnchorHistory) % kAnchorHistory];
}
