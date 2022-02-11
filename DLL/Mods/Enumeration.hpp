#include "../Offsets.hpp"
#include "../MemUtil.hpp"
#include <vector>
#include <filesystem>

namespace Enumeration {
	void ForceEnumeration();
	int GetCurrentDLCCount();
	int GetFileCount(std::filesystem::path path);
};
