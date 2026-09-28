#pragma once

#include <string_view>
#include <vector>

namespace Metronome {
	// One beat of the chart's beat grid (the SNG "ebeats").
	struct Beat {
		double seconds = 0.0;       // Chart time of the beat.
		bool startsMeasure = false; // First beat of a measure; played with the accent sound.
	};

	// Beats in chart order, ascending by time.
	using BeatMap = std::vector<Beat>;

	// Reads the beat grid of a song.
	class BeatMapLoader {
	public:
		// Empty when the song's beats aren't available (yet). Runs on MainThread, so it must not block.
		BeatMap Load(std::string_view songKey) const;
	};
}
