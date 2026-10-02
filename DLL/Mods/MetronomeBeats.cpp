#include "../stdafx.h"
#include "MetronomeBeats.hpp"

#include "../MemUtil.hpp"
#include "../Offsets.hpp"

using Metronome::Beat;
using Metronome::BeatMap;
using Metronome::BeatMapSource;
using Metronome::BeatVectorBounds;

namespace {
	// The SNG's BPM record, which the game keeps unchanged in memory.
	struct BpmRecord {
		float time;
		int16_t measure;
		int16_t beat; // 0 on the first beat of a measure.
		int32_t phraseIteration;
		int32_t mask;
	};
	static_assert(sizeof(BpmRecord) == 16);

	// Far above any real song (a 10-minute song at 300 BPM has 3000 beats); more means the pointers are garbage.
	constexpr size_t kMaxBeats = 20000;

	std::optional<BeatVectorBounds> ReadBeatVectorBounds() {
		const uintptr_t vector = MemUtil::FindDMAAddy(Offsets::baseHandle + Offsets::ptr_chartBeats, Offsets::ptr_chartBeatsOffsets, true);

		BeatVectorBounds bounds;
		if (!MemUtil::TryRead(vector, bounds.begin) || !MemUtil::TryRead(vector + sizeof(uintptr_t), bounds.end))
			return std::nullopt;
		return bounds;
	}

	bool LooksLikeBeatVector(const BeatVectorBounds& bounds) {
		if (bounds.begin == 0 || bounds.end <= bounds.begin) return false;

		const uintptr_t byteCount = bounds.end - bounds.begin;
		return byteCount % sizeof(BpmRecord) == 0 && byteCount / sizeof(BpmRecord) <= kMaxBeats;
	}

	// Empty if any record can't be read or the times don't ascend, which means the chart is being torn down.
	BeatMap CopyBeats(const BeatVectorBounds& bounds) {
		BeatMap beats;
		beats.reserve((bounds.end - bounds.begin) / sizeof(BpmRecord));

		for (uintptr_t address = bounds.begin; address < bounds.end; address += sizeof(BpmRecord)) {
			BpmRecord record;
			if (!MemUtil::TryRead(address, record)) return {};
			if (!beats.empty() && record.time < beats.back().seconds) return {};

			beats.push_back({ record.time, record.beat == 0 });
		}
		return beats;
	}
}

std::optional<BeatMap> BeatMapSource::PollChanges() {
	const std::optional<BeatVectorBounds> bounds = ReadBeatVectorBounds();
	if (!bounds || !LooksLikeBeatVector(*bounds)) return std::nullopt;

	if (*bounds == lastBounds) return std::nullopt;

	BeatMap beats = CopyBeats(*bounds);
	if (beats.empty()) return std::nullopt;

	lastBounds = *bounds;
	return beats;
}

void BeatMapSource::Forget() {
	lastBounds = {};
}
