#include "windows.h"
#include <vector>
#include <string>

#include "MemUtil.h"
#include "Settings.h"
#include "Structs.h"
#include "Offsets.h"

#pragma once
class RandomMemStuff
{
public:
	void AddVolume(float add);
	void DecreaseVolume(float remove);
	void ToggleLoft();
	void ToggleLoftWhenSongStarts();
	void ShowSongTimer();
	void PatchSongListAppendages();
	void HookSongListsKoko();
	void SetFakeListNames();
	void LoadSettings();
	void EnumerateBrah();
	void ShowCurrentTuning();
	void DoRainbow();
	void Toggle7StringMode();
	bool LoadModsWhenSongsLoad(std::string ModToRun);
};

