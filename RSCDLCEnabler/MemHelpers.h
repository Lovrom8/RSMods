#pragma once
#include "windows.h"
#include <vector>
#include <string>
#include "MemUtil.h"
#include "Offsets.h"
#include "Structs.h"
#include "Settings.h"

class cMemHelpers
{
public:
	byte getLowestStringTuning();
	void ToggleLoft();
	void ToggleLoftWhenSongStarts();
	void ToggleSkyline();
	void ShowSongTimer();
	void ShowCurrentTuning();
	void PatchCDLCCheck();
	bool LoadModsWhenSongsLoad(std::string ModToRun);
	bool IsExtendedRangeSong();

	std::string GetCurrentMenu();
	void ToggleCB(bool enabled);
};

extern cMemHelpers MemHelpers;