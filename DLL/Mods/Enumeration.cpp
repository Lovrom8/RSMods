#include "Enumeration.hpp"

/// <summary>
/// Trigger Enumeration
/// </summary>
void Enumeration::ForceEnumeration() {
		// Get memory address for Enumeration flag
		uintptr_t rsSteamServiceFlagsPtr = MemUtil::FindDMAAddy(Offsets::baseHandle + Offsets::ptr_enumerateService, Offsets::ptr_enumerateServiceOffsets);

		// Set Enumeration flags to 1.
		*(BYTE*)rsSteamServiceFlagsPtr = 1;
		*(BYTE*)(rsSteamServiceFlagsPtr + 1) = 1;
}

/// <summary>
/// Get amount of files in a path. Recursive.
/// </summary>
/// <param name="path"> - Path</param>
/// <returns>Amount of files</returns>
int Enumeration::GetFileCount(std::filesystem::path path) {
	using std::filesystem::recursive_directory_iterator;
	return std::distance(recursive_directory_iterator(path, std::filesystem::directory_options::follow_directory_symlink), recursive_directory_iterator{});
}

/// <summary>
/// Get amount of files in DLC folder.
/// </summary>
/// <returns>Amount of files in the Rocksmith2014/dlc folder. Recursive.</returns>
int Enumeration::GetCurrentDLCCount() {
	std::filesystem::path dlcFolderPath = std::filesystem::current_path(); //tread carefully, this stuff likes to crash if the path is wrong
	dlcFolderPath /= "dlc";

	return GetFileCount(dlcFolderPath);
}


