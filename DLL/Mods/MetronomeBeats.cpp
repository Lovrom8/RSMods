#include "../stdafx.h"
#include "MetronomeBeats.hpp"

#include <cstring>

#include "../MemUtil.hpp"
#include "../Offsets.hpp"

using Metronome::Beat;
using Metronome::BeatMap;
using Metronome::BeatMapSource;
using Metronome::BeatVectorBounds;
using Metronome::ChartBeats;
using Metronome::TimeRange;

namespace {
	// The chart object keeps each SNG section as a std::vector of its records, unchanged.
	constexpr uintptr_t kBeatsVector = 0x34;
	constexpr uintptr_t kPhrasesVector = 0x58;
	constexpr uintptr_t kPhraseIterationsVector = 0x64;
	constexpr uintptr_t kEventsVector = 0xD0;

	struct BpmRecord {
		float time;
		int16_t measure;
		int16_t beat; // 0 on the first beat of a measure.
		int32_t phraseIteration;
		int32_t mask;
	};
	static_assert(sizeof(BpmRecord) == 16);

	struct PhraseRecord {
		uint8_t solo, disparity, ignore, padding;
		int32_t maxDifficulty;
		int32_t iterationLinks;
		char name[32];
	};
	static_assert(sizeof(PhraseRecord) == 44);

	struct PhraseIterationRecord {
		int32_t phraseId;
		float startTime;
		float nextPhraseTime;
		int32_t difficulty[3];
	};
	static_assert(sizeof(PhraseIterationRecord) == 24);

	struct EventRecord {
		float time;
		char name[256];
	};
	static_assert(sizeof(EventRecord) == 260);

	// Far above any real chart (a 10-minute song at 300 BPM has 3000 beats); more means the pointers are garbage.
	constexpr size_t kMaxRecords = 20000;

	constexpr char kCountInPhrase[] = "COUNT";
	constexpr const char* kCountInEvents[] = { "B0", "B1" }; // The game's own count-in click, plain and accented.

	uintptr_t ChartObject() {
		return MemUtil::FindDMAAddy(Offsets::baseHandle + Offsets::ptr_chart, Offsets::ptr_chartOffsets, true);
	}

	std::optional<BeatVectorBounds> ReadVectorBounds(uintptr_t chart, uintptr_t vectorOffset) {
		BeatVectorBounds bounds;
		if (!MemUtil::TryRead(chart + vectorOffset, bounds.begin) || !MemUtil::TryRead(chart + vectorOffset + sizeof(uintptr_t), bounds.end))
			return std::nullopt;
		return bounds;
	}

	template <typename Record>
	bool HoldsRecords(const BeatVectorBounds& bounds) {
		if (bounds.begin == 0 || bounds.end < bounds.begin) return false;

		const uintptr_t byteCount = bounds.end - bounds.begin;
		return byteCount % sizeof(Record) == 0 && byteCount / sizeof(Record) <= kMaxRecords;
	}

	// Empty if the vector doesn't look like one of these records or any record can't be read.
	template <typename Record>
	std::vector<Record> ReadRecords(uintptr_t chart, uintptr_t vectorOffset) {
		const std::optional<BeatVectorBounds> bounds = ReadVectorBounds(chart, vectorOffset);
		if (!bounds || !HoldsRecords<Record>(*bounds)) return {};

		std::vector<Record> records((bounds->end - bounds->begin) / sizeof(Record));
		for (size_t i = 0; i < records.size(); ++i)
			if (!MemUtil::TryRead(bounds->begin + i * sizeof(Record), records[i])) return {};
		return records;
	}

	template <size_t Size>
	bool NameIs(const char (&name)[Size], const char* expected) {
		return strncmp(name, expected, Size) == 0;
	}

	// Empty if the times don't ascend, which means the chart is being torn down.
	BeatMap ToBeats(const std::vector<BpmRecord>& records) {
		BeatMap beats;
		beats.reserve(records.size());
		for (const BpmRecord& record : records) {
			if (!beats.empty() && record.time < beats.back().seconds) return {};
			beats.push_back({ record.time, record.beat == 0 });
		}
		return beats;
	}

	std::optional<TimeRange> CountInPhrase(uintptr_t chart) {
		const std::vector<PhraseRecord> phrases = ReadRecords<PhraseRecord>(chart, kPhrasesVector);
		const auto countIn = std::ranges::find_if(phrases, [](const PhraseRecord& p) { return NameIs(p.name, kCountInPhrase); });
		if (countIn == phrases.end()) return std::nullopt;

		const int32_t countInId = static_cast<int32_t>(countIn - phrases.begin());
		for (const PhraseIterationRecord& iteration : ReadRecords<PhraseIterationRecord>(chart, kPhraseIterationsVector))
			if (iteration.phraseId == countInId) return TimeRange{ iteration.startTime, iteration.nextPhraseTime };
		return std::nullopt;
	}

	bool HasCountInClicks(uintptr_t chart, const TimeRange& range) {
		auto isCountInClick = [&](const EventRecord& event) {
			const bool countInName = std::ranges::any_of(kCountInEvents, [&](const char* name) { return NameIs(event.name, name); });
			return countInName && event.time >= range.start && event.time < range.end;
		};
		return std::ranges::any_of(ReadRecords<EventRecord>(chart, kEventsVector), isCountInClick);
	}

	// The game clicks its own count-in only where a COUNT phrase holds B0/B1 events; anywhere else the metronome plays.
	std::optional<TimeRange> SilencedCountIn(uintptr_t chart) {
		const std::optional<TimeRange> countIn = CountInPhrase(chart);
		return countIn && HasCountInClicks(chart, *countIn) ? countIn : std::nullopt;
	}

	void RemoveBeatsIn(BeatMap& beats, const TimeRange& range) {
		std::erase_if(beats, [&](const Beat& beat) { return beat.seconds >= range.start && beat.seconds < range.end; });
	}
}

std::optional<ChartBeats> BeatMapSource::PollChanges() {
	const uintptr_t chart = ChartObject();
	const std::optional<BeatVectorBounds> bounds = chart ? ReadVectorBounds(chart, kBeatsVector) : std::nullopt;
	if (!bounds || !HoldsRecords<BpmRecord>(*bounds) || bounds->begin == bounds->end) return std::nullopt;
	if (*bounds == lastBounds) return std::nullopt;

	ChartBeats chartBeats{ ToBeats(ReadRecords<BpmRecord>(chart, kBeatsVector)), SilencedCountIn(chart) };
	if (chartBeats.beats.empty()) return std::nullopt;

	if (chartBeats.countIn) RemoveBeatsIn(chartBeats.beats, *chartBeats.countIn);
	lastBounds = *bounds;
	return chartBeats;
}

void BeatMapSource::Forget() {
	lastBounds = {};
}
