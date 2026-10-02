#pragma once

#include <cstdint>
#include <optional>
#include <vector>

namespace Metronome {
	// One beat of the chart's beat grid (the SNG "ebeats").
	struct Beat {
		double seconds = 0.0;       // Chart time of the beat.
		bool startsMeasure = false; // First beat of a measure; played with the accent sound.
	};

	// Beats in chart order, ascending by time.
	using BeatMap = std::vector<Beat>;

	// Where the game keeps a chart's beat records; another chart's live somewhere else.
	struct BeatVectorBounds {
		uintptr_t begin = 0;
		uintptr_t end = 0;
		bool operator==(const BeatVectorBounds&) const = default;
	};

	// Reads the beat grid of the arrangement being played from the game's chart object, which holds the SNG's
	// BPM records as loaded (Offsets::ptr_chartBeats).
	class BeatMapSource {
	public:
		// A new beat map when the playing chart's beats differ from the last ones returned, so a chart that
		// loads a tick late, or replaces the previous one, is picked up. Nothing while no chart is loaded.
		std::optional<BeatMap> PollChanges();

		// The next poll returns the playing chart's beats again, even if they haven't moved.
		void Forget();

	private:
		BeatVectorBounds lastBounds;
	};
}
