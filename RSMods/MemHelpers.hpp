#pragma once

#include "windows.h"
#include <vector>
#include <string>
#include <iostream>
#include <fstream>
#include <filesystem>
#include <codecvt>
#include "Lib/DirectX/d3dx9.h"
#include "Lib/DirectX/d3d9types.h"
#include "Lib/Json/json.hpp"
#include "D3D/D3DHooks.hpp"

#include "MemUtil.hpp"
#include "Offsets.hpp"
#include "Structs.hpp"
#include "Settings.hpp"

namespace MemHelpers {
	byte getLowestStringTuning();
	byte* GetCurrentTuning(bool verbose = false);
	void ToggleLoft();
	float SongTimer();
	bool IsExtendedRangeSong();
	int* GetHighestLowestString();
	int* GetHighestLowestString(Tuning tuningOverride);
	bool IsSongInDrop(Tuning tuning);
	bool IsSongInStandard(Tuning tuning);
	int GetTrueTuning();
	Resolution GetWindowSize();
	bool IsInStringArray(std::string stringToCheckIfInsideArray, std::vector<std::string> stringVector = std::vector<std::string>());
	void DX9DrawText(std::string textToDraw, int textColorHex, int topLeftX, int topLeftY, int bottomRightX, int bottomRightY, LPDIRECT3DDEVICE9 pDevice, Resolution setFontSize = { NULL, NULL });
	void ToggleDrunkMode(bool enable);
	bool IsInSong();
	Tuning GetTuningAtTuner();
	bool IsExtendedRangeTuner();
	bool IsMultiplayer();
	std::string CurrentSelectedUser();
	std::string GetSongKey();
	float GetGreyNoteTimer();
	void SetGreyNoteTimer(float timeInSeconds);

	inline std::string lastSongKey = "";

	std::string GetCurrentMenu(bool GameNotLoaded=false);
	void ToggleCB(bool enabled);

	inline static std::string lastMenu = "";
	inline static bool canGetRealMenu = false;

	inline HRESULT CustomDX9Font = NULL;
	inline ID3DXFont* DX9FontEncapsulation = NULL;
	inline int WindowSizeX = NULL, WindowSizeY = NULL;
};

namespace Util {
	inline void SendKey(unsigned int key) {
		PostMessage(FindWindow(NULL, L"Rocksmith 2014"), WM_KEYDOWN, key, 0);
		Sleep(30);
		PostMessage(FindWindow(NULL, L"Rocksmith 2014"), WM_KEYUP, key, 0);
	}
};
