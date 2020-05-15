#include "windows.h"
#include <vector>
#include <string>
#include "Patch.h"
#include "IniReader.h"
#include "Settings.h"

#pragma once
class RandomMemStuff
{
public:
	RandomMemStuff();
	~RandomMemStuff();

	void AddVolume(float add);
	void DecreaseVolume(float remove);
	void ToggleLoft();
	void ToggleLoftWhenSongStarts();
	void ShowSongTimer();
	void PatchSongListAppendages();
	void HookSongLists();
	void HookSongListsKoko();
	void SetFakeListNames();
	void LoadSettings();

	uintptr_t FindDMAAddy(uintptr_t ptr, std::vector<unsigned int> offsets);
};

