#include "Enumeration.h"
#include "Offsets.h"
#include "MemUtil.h"
#include <vector>

cEnum Enumeration;

void cEnum::ForceEnumeration() {
		uintptr_t rsSteamServiceFlagsPtr = MemUtil.FindDMAAddy(Offsets.baseHandle + Offsets.ptr_enumerateService, Offsets.ptr_enumerateServiceOffsets);

		*(BYTE*)rsSteamServiceFlagsPtr = 1;
		*(BYTE*)(rsSteamServiceFlagsPtr + 1) = 1;
}

int cEnum::GetFileCount(std::filesystem::path path) {
	using std::filesystem::recursive_directory_iterator;
	return std::distance(recursive_directory_iterator(path), recursive_directory_iterator{});
}

int cEnum::GetCurrentDLCCount() {
	std::filesystem::path dlcFolderPath = std::filesystem::current_path(); //tread carefully, this stuff likes to crash if the path is wrong
	dlcFolderPath /= "dlc";

	return GetFileCount(dlcFolderPath);
}


