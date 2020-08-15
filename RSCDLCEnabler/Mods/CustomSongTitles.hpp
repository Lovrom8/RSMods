#include "../MemUtil.hpp"
#include "../Offsets.hpp"
#include "../Settings.hpp"
#include <string>
#include <vector>

#pragma once
namespace CustomSongTitles {
	void PatchSongListAppendages();
	void SetFakeListNames();
	void HookSongListsKoko();
	void LoadSettings();

	inline int len = 6;
};