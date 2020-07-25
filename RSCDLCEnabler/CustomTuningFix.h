#pragma once

#include "windows.h"
#include "MemUtil.h"
#include "Offsets.h"
#include "Settings.h"
#include <string>
#include <vector>

class cCustomTuningFix {
public:
	char __stdcall customTuningFix(int numberInMainGameCSV, char* newValue);
private:

};

extern cCustomTuningFix CustomTuningFix;