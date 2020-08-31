#pragma once

#include "windows.h"
#include <vector>
#include <string>
#include <iostream>
#include <fstream>

#include "MemUtil.hpp"
#include "Offsets.hpp"
#include "Structs.hpp"
#include "Settings.hpp"

namespace MemHelpers {
	byte getLowestStringTuning();
	void ToggleLoft();
	void ShowSongTimer();
	void ShowCurrentTuning();
	bool IsExtendedRangeSong();
	float GetCurrentMusicVolume();
	int* GetWindowSize();
	bool IsInStringArray(std::string stringToCheckIfInsideArray, std::string* stringArray = NULL, std::vector<std::string> stringVector = std::vector<std::string>());

	std::string GetCurrentMenu(bool GameNotLoaded=false);
	void ToggleCB(bool enabled);

	void PatchCDLCCheck();

	inline static std::string lastMenu = "";
	inline static bool canGetRealMenu = false;
};
