#include "../stdafx.h"
#include "MetronomeBeats.hpp"

// TODO: Load the beats extracted from the song's SNG. Plan: a helper command in RSMods.exe (like RSModsPlus'
// "--speaker-cache-extract") finds the song's PSARC by its key, reads the SNG beat section (time, measure,
// beat index; beat index 0 starts a measure) and writes them to a cache file. The extraction has to be
// requested earlier (song select / pre-song tuner) so this only reads the finished file.
Metronome::BeatMap Metronome::BeatMapLoader::Load(std::string_view) const {
	return {};
}
