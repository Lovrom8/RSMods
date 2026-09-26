#pragma once

namespace Enumeration {
	void ForceEnumeration();
	int GetCurrentDLCCount();
	int GetFileCount(std::filesystem::path path);
	void HookEnumerationService();
	inline std::uint8_t* rsSteamServiceFlagsPtr = nullptr; // DLC service + 0x04: [0] = scan request, [1] = enabled.
};
