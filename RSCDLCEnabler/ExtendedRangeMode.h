#pragma once
#include "windows.h"
#include <vector>
#include <string>
#include <iostream>
#include "Structs.h"
#include "Offsets.h"
#include "MemUtil.h"
#include "Settings.h"
#include "MemHelpers.h"

class cERMode {
public:
	void Toggle7StringMode();
	void DoRainbow();
	void ToggleRainbowMode();

	bool Is7StringSong = false;
	bool RainbowEnabled = false;
};

extern cERMode ERMode;



