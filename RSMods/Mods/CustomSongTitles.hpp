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

inline const int buffer_size = 10;
inline char string_buffer[buffer_size];