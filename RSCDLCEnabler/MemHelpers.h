#pragma once

#include "windows.h"
#include <vector>
#include <string>
#include <iostream>
#include <fstream>

#include "MemUtil.h"
#include "Offsets.h"
#include "Structs.h"
#include "Settings.h"


class cMemHelpers
{
public:
	byte getLowestStringTuning();
	void ToggleLoft();
	void ShowSongTimer();
	void ShowCurrentTuning();
	bool IsExtendedRangeSong();

	std::string GetCurrentMenu();
	void ToggleCB(bool enabled);

	void PatchCDLCCheck();
};

extern cMemHelpers MemHelpers;