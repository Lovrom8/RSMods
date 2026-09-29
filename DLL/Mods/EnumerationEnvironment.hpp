#pragma once

#include <string>
#include <string_view>
#include <vector>

// Storage and system facts for the DLC scan's log: which volume(s) the library is on and what kind of drive (NVMe / SATA / USB,
// SSD or spinning), the file system, whether the dlc folder is a link, which antivirus products are active and whether Defender
// excludes the library, and memory. Enough to tell a slow disk from a real-time scanner from a small file cache, from the log alone.
namespace EnumerationEnvironment {
	enum class SeekPenalty { None, Yes, Unknown };	// None = SSD / NVMe, Yes = spinning disk.

	struct Volume {
		std::wstring mountPoint;
		std::wstring fileSystem;
		std::string bus;			// "NVMe", "SATA", "USB", ... or "unknown".
		std::string device;			// Vendor and product id, as the drive reports them.
		SeekPenalty seekPenalty = SeekPenalty::Unknown;
		bool removable = false;
		unsigned long long freeBytes = 0;
		unsigned long long totalBytes = 0;
	};

	// Follows links (a symlinked dlc folder, or a folder in it) to the file's final path and the mount point of its volume.
	bool ResolveMountPoint(std::string_view path, std::wstring& finalPath, std::wstring& mountPoint);

	// Touches the volume device, not the disk's data. Fields it can't find stay empty / Unknown.
	Volume QueryVolume(const std::wstring& mountPoint);

	const char* SeekPenaltyName(SeekPenalty penalty);

	// Logs the environment from a background thread, since resolving paths and asking the drives can touch a cold disk.
	// samplePaths: packages spread over the scan queue, to find every volume the library is on.
	void LogAsync(std::vector<std::string> samplePaths);

	// Available RAM and the size of the system file cache, for the progress lines. Cheap, no disk access.
	std::string MemoryLine();

	std::string Narrow(const std::wstring& text);
}
