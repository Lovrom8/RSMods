#include "Enumeration.h"

cEnum Enumeration;

void cEnum::ForceEnumeration() {
		uintptr_t rsSteamServiceFlagsPtr = MemUtil.FindDMAAddy(Offsets.baseHandle + Offsets.ptr_enumerateService, Offsets.ptr_enumerateServiceOffsets);

		*(BYTE*)rsSteamServiceFlagsPtr = 1;
		*(BYTE*)(rsSteamServiceFlagsPtr + 1) = 1;
}

int cEnum::GetFileCount(std::filesystem::path path) {
	using std::filesystem::recursive_directory_iterator;
	return std::distance(recursive_directory_iterator(path, std::filesystem::directory_options::follow_directory_symlink), recursive_directory_iterator{});
}

int cEnum::GetCurrentDLCCount() {
	std::filesystem::path dlcFolderPath = std::filesystem::current_path(); //tread carefully, this stuff likes to crash if the path is wrong
	dlcFolderPath /= "dlc";

	return GetFileCount(dlcFolderPath);
}


