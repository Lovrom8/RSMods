#include "windows.h"
#include <vector>
#include <string>
#include "Patch.h"

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
	void PatchSongLists();
	void HookSongLists();
	void HookSongLists2();

	uintptr_t FindDMAAddy(uintptr_t ptr, std::vector<unsigned int> offsets);
	
};

