#pragma once

#include "windows.h"
#include <vector>
#include <string>
#include <iostream>
#include <fstream>
#include "Lib/DirectX/d3dx9.h"
#include "Lib/DirectX/d3d9types.h"
#include "D3D/D3DHooks.hpp"

#include "MemUtil.hpp"
#include "Offsets.hpp"
#include "Structs.hpp"
#include "Settings.hpp"

namespace MemHelpers {
	byte getLowestStringTuning();
	byte* GetCurrentTuning(bool verbose = false);
	void ToggleLoft();
	std::string ShowSongTimer();
	bool IsExtendedRangeSong();
	bool NewIsExtendedRangeSong();
	bool IsSongInDrop();
	bool IsSongInStandard();
	int GetTrueTuning();
	int* GetWindowSize();
	bool IsInStringArray(std::string stringToCheckIfInsideArray, std::string* stringArray = NULL, std::vector<std::string> stringVector = std::vector<std::string>());
	void DX9DrawText(std::string textToDraw, int textColorHex, int topLeftX, int topLeftY, int bottomRightX, int bottomRightY, LPDIRECT3DDEVICE9 pDevice);
	void ToggleDrunkMode(bool enable);
	bool IsInSong();
	float RiffRepeaterSpeed(float newSpeed = NULL);
	void AutomatedOpenRRSpeedAbuse();

	std::string GetCurrentMenu(bool GameNotLoaded=false);
	void ToggleCB(bool enabled);

	void PatchCDLCCheck();

	inline static std::string lastMenu = "";
	inline static bool canGetRealMenu = false;

	inline HRESULT CustomDX9Font = NULL;
	inline ID3DXFont* DX9FontEncapsulation = NULL;
	inline int WindowSizeX = NULL, WindowSizeY = NULL;
};

namespace Util { // TODO: put in a more appropriate place
	inline void SendKey(unsigned int key) {
		PostMessage(FindWindow(NULL, L"Rocksmith 2014"), WM_KEYDOWN, key, 0);
		Sleep(30);
		PostMessage(FindWindow(NULL, L"Rocksmith 2014"), WM_KEYUP, key, 0);
	}
};
