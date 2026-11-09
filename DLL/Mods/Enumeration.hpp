#pragma once

namespace Enumeration {
	void ForceEnumeration();
	int GetCurrentDLCCount();
	int GetFileCount(std::filesystem::path path);
	void HookEnumerationService();
	// DLC service +0x04 request byte, +0x05 enabled byte.
	inline std::uint8_t* rsSteamServiceFlagsPtr = nullptr;
};
