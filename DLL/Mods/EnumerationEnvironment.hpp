#pragma once

#include <string>
#include <string_view>
#include <vector>

namespace EnumerationEnvironment {
	enum class SeekPenalty { None, Yes, Unknown };

	struct Volume {
		std::wstring mountPoint;
		std::wstring fileSystem;
		std::string bus;
		std::string device;
		SeekPenalty seekPenalty = SeekPenalty::Unknown;
		bool removable = false;
		unsigned long long freeBytes = 0;
		unsigned long long totalBytes = 0;
	};

	// Resolve links before selecting the file volume.
	bool ResolveMountPoint(std::string_view path, std::wstring& finalPath, std::wstring& mountPoint);

	// Unknown device fields remain empty; this query does not read file data.
	Volume QueryVolume(const std::wstring& mountPoint);

	const char* SeekPenaltyName(SeekPenalty penalty);

	// Probe sampled package volumes on a background thread because queries may block.
	void LogAsync(std::vector<std::string> samplePaths);

	// Memory snapshot only; performs no disk I/O.
	std::string MemoryLine();

	std::string Narrow(const std::wstring& text);
}
