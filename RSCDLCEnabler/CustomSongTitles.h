#include "MemUtil.h"
#include "Offsets.h"
#include "Settings.h"
#include <string>
#include <vector>

#pragma once
class cCustomST {
public:
	void PatchSongListAppendages();
	void SetFakeListNames();
	void HookSongListsKoko();
	void LoadSettings();

private:
	int len = 6;
};

extern cCustomST CustomSongTitles;