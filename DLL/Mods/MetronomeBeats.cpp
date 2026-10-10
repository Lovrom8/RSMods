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

	// SNG names are fixed-size and NUL-padded, but a full-length one has no terminator.
	template <size_t Size>
	std::string Name(const char (&name)[Size]) {
		return std::string(name, strnlen(name, Size));
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

	std::optional<TimeRange> GameCountInOf(uintptr_t chart) {
		std::vector<std::string> phraseNames;
		for (const PhraseRecord& phrase : ReadRecords<PhraseRecord>(chart, kPhrasesVector))
			phraseNames.push_back(Name(phrase.name));

		std::vector<Metronome::PhraseIteration> iterations;
		for (const PhraseIterationRecord& iteration : ReadRecords<PhraseIterationRecord>(chart, kPhraseIterationsVector))
			iterations.push_back({ iteration.phraseId, iteration.startTime, iteration.nextPhraseTime });

		std::vector<Metronome::ChartEvent> events;
		for (const EventRecord& event : ReadRecords<EventRecord>(chart, kEventsVector))
			events.push_back({ event.time, Name(event.name) });

		return Metronome::GameCountIn(phraseNames, iterations, events);
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

	ChartBeats chartBeats{ ToBeats(ReadRecords<BpmRecord>(chart, kBeatsVector)), GameCountInOf(chart) };
	if (chartBeats.beats.empty()) return std::nullopt;

	if (chartBeats.countIn) RemoveBeatsIn(chartBeats.beats, *chartBeats.countIn);
	lastBounds = *bounds;
	return chartBeats;
}

void BeatMapSource::Forget() {
	lastBounds = {};
}
