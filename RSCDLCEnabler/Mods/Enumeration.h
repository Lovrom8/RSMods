#include "../Offsets.h"
#include "../MemUtil.h"
#include <vector>
#include <filesystem>

class cEnum {
public:
	void ForceEnumeration();
	int GetCurrentDLCCount();

private:
	int GetFileCount(std::filesystem::path path);
};

extern cEnum Enumeration;