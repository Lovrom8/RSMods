#pragma once

#include <cstdint>
#include <optional>
#include <vector>

#include "MetronomeCountIn.hpp"

namespace Metronome {
	// One beat of the chart's beat grid (the SNG "ebeats").
	struct Beat {
		double seconds = 0.0;       // Chart time of the beat.
		bool startsMeasure = false; // First beat of a measure; played with the accent sound.
	};

	// Beats in chart order, ascending by time.
	using BeatMap = std::vector<Beat>;

	struct ChartBeats {
		BeatMap beats;                    // Without the beats of a silenced count-in.
		std::optional<TimeRange> countIn; // The chart's own count-in, whose beats were left out.
	};

	// Where the game keeps a chart's beat records; another chart's live somewhere else.
	struct BeatVectorBounds {
		uintptr_t begin = 0;
		uintptr_t end = 0;
		bool operator==(const BeatVectorBounds&) const = default;
	};

	// Reads the beat grid of the arrangement being played from the game's chart object, which holds the SNG's
	// records as loaded (Offsets::ptr_chart).
	//
	// Charts with a COUNT phrase holding B0/B1 events have the game play its own count-in clicks, so the metronome
	// stays silent for that phrase rather than click twice.
	class BeatMapSource {
	public:
		// The playing chart's beats when they differ from the last ones returned, so a chart that loads a tick late,
		// or replaces the previous one, is picked up. Nothing while no chart is loaded.
		std::optional<ChartBeats> PollChanges();

		// The next poll returns the playing chart's beats again, even if they haven't moved.
		void Forget();

	private:
		BeatVectorBounds lastBounds;
	};
}
