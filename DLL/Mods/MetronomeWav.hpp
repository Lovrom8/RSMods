#pragma once

#include <cstdint>
#include <filesystem>
#include <string>
#include <variant>
#include <vector>

namespace Metronome {
	// A sound mixed down to one channel, samples in -1..1.
	struct MonoSound {
		std::vector<float> samples;
		uint32_t sampleRate = 0;
	};

	// The sound, or why the file couldn't be used. Reads PCM (8, 16, 24 or 32 bits) and 32-bit float WAV files.
	std::variant<MonoSound, std::string> LoadWav(const std::filesystem::path& path);
}
