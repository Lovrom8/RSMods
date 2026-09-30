#pragma once

#include <filesystem>
#include <string>
#include <unordered_set>
#include <vector>

namespace Metronome {
	// One beat of the chart's beat grid (the SNG "ebeats").
	struct Beat {
		double seconds = 0.0;       // Chart time of the beat.
		bool startsMeasure = false; // First beat of a measure; played with the accent sound.
	};

	// Beats in chart order, ascending by time.
	using BeatMap = std::vector<Beat>;

	// Song beats, extracted from the song's archive by RSMods.exe ("--extract-beats", see the GUI's
	// ExtractBeatsCommand) into RSMods\MetronomeBeats\<songKey>.beats, where Load reads them.
	class BeatMapSource {
	public:
		BeatMapSource();

		// Starts the extraction in the background, once per song per session. Doesn't wait for it.
		void Request(const std::string& songKey);

		// Empty while the song's beats haven't been extracted (yet).
		BeatMap Load(const std::string& songKey) const;

	private:
		std::filesystem::path gameFolder;
		std::filesystem::path rsModsFolder;
		std::unordered_set<std::string> requestedSongKeys;

		std::filesystem::path BeatsFile(const std::string& songKey) const;
		void StartExtraction(const std::string& songKey) const;
	};
}
