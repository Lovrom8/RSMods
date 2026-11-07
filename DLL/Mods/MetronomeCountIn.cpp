#include "MetronomeCountIn.hpp"

#include <algorithm>

using Metronome::ChartEvent;
using Metronome::PhraseIteration;
using Metronome::TimeRange;

namespace {
	constexpr char kCountInPhrase[] = "COUNT";
	constexpr const char* kCountInEvents[] = { "B0", "B1" }; // The game's own count-in click, plain and accented.

	std::optional<TimeRange> CountInPhrase(const std::vector<std::string>& phraseNames, const std::vector<PhraseIteration>& iterations) {
		const auto countIn = std::ranges::find(phraseNames, kCountInPhrase);
		if (countIn == phraseNames.end()) return std::nullopt;

		const auto countInId = static_cast<int32_t>(countIn - phraseNames.begin());
		const auto iteration = std::ranges::find(iterations, countInId, &PhraseIteration::phraseId);
		if (iteration == iterations.end()) return std::nullopt;
		return TimeRange{ iteration->start, iteration->end };
	}

	bool HasCountInClick(const std::vector<ChartEvent>& events, const TimeRange& range) {
		return std::ranges::any_of(events, [&](const ChartEvent& event) {
			const bool countInClick = std::ranges::find(kCountInEvents, event.name) != std::end(kCountInEvents);
			return countInClick && event.time >= range.start && event.time < range.end;
		});
	}
}

std::optional<TimeRange> Metronome::GameCountIn(const std::vector<std::string>& phraseNames,
	const std::vector<PhraseIteration>& iterations, const std::vector<ChartEvent>& events) {
	const std::optional<TimeRange> countIn = CountInPhrase(phraseNames, iterations);
	return countIn && HasCountInClick(events, *countIn) ? countIn : std::nullopt;
}
