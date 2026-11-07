#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace Metronome {
	// Chart time range, start included, end excluded.
	struct TimeRange {
		double start = 0.0;
		double end = 0.0;
	};

	struct PhraseIteration {
		int32_t phraseId = 0; // Index into the chart's phrase names.
		double start = 0.0;
		double end = 0.0;
	};

	struct ChartEvent {
		double time = 0.0;
		std::string name;
	};

	// The game clicks its own count-in where a COUNT phrase holds B0/B1 events. That phrase's time range, so the
	// metronome stays silent there instead of clicking twice; empty in any other chart, where the metronome plays.
	std::optional<TimeRange> GameCountIn(const std::vector<std::string>& phraseNames,
		const std::vector<PhraseIteration>& iterations, const std::vector<ChartEvent>& events);
}
