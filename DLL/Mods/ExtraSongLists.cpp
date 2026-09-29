#include "../stdafx.h"
#include "ExtraSongLists.hpp"

#include <algorithm>

namespace {
	// Decoded from the September 2022 remaster, identical in the December 2024 build.
	constexpr unsigned int off_service_root = 0x10;    // Song list service +0x10: the song lists object
	constexpr unsigned int off_root_songLists = 0x0C;  // Song lists object +0x0C: the array of lists
	constexpr unsigned int off_vtable_count = 0x78;    // The array's vtable +0x78: element count (thiscall, no arguments)

	constexpr unsigned int firstSongListId = 2;        // 0 = All Songs, 1 = Favorites, 2 = Song List 1
	constexpr unsigned int lastStockId = 7;            // The pool builder's switch covers ids 0-7
	constexpr unsigned int stockOptionCount = 8;
	constexpr unsigned int maxOptionCount = 22;        // All Songs, Favorites and the GUI's 20 song lists

	using ArrayCountFn = unsigned int(__thiscall*)(uintptr_t array);

	bool installed = false;

	// Number of song lists in the loaded profile, or -1 when there is none to read.
	int GetSongListCount() {
		uintptr_t service = 0, root = 0, songLists = 0, vtable = 0, countFn = 0;
		if (!MemUtil::TryRead(Offsets::ptr_songListService.GetValue(), service) || !service)
			return -1;
		if (!MemUtil::TryRead(service + off_service_root, root) || !root)
			return -1;
		if (!MemUtil::TryRead(root + off_root_songLists, songLists) || !songLists)
			return -1;
		if (!MemUtil::TryRead(songLists, vtable) || !vtable)
			return -1;
		if (!MemUtil::TryRead(vtable + off_vtable_count, countFn) || !countFn)
			return -1;

		return static_cast<int>(reinterpret_cast<ArrayCountFn>(countFn)(songLists));
	}

	// How many options Non-Stop Play's song list choice should have.
	unsigned int __cdecl GetOptionCount() {
		const int songLists = GetSongListCount();
		if (songLists < 0)
			return stockOptionCount;

		const unsigned int options = firstSongListId + static_cast<unsigned int>(songLists);
		return std::clamp(options, stockOptionCount, maxOptionCount);
	}

	// Whether a Non-Stop Play id above the stock range names a song list the profile has.
	int __stdcall IsUsableExtraSongList(unsigned int id) {
		const int songLists = GetSongListCount();
		return songLists >= 0 && id - firstSongListId < static_cast<unsigned int>(songLists);
	}

	// Replaces "inc ebx / cmp ebx, 8 / jc top" in the loop that pushes the option ids 0..n-1.
	void __declspec(naked) OptionIdsLoopHook() {
		__asm {
			inc ebx
			push eax
			push ecx
			push edx
			call GetOptionCount
			cmp ebx, eax
			pop edx										// pop leaves the flags alone
			pop ecx
			pop eax
			jb NextId

			push offset Offsets::ptr_nspSongListIdsLoopExit
			jmp MemUtil::JumpToVersioned

		NextId:
			push offset Offsets::ptr_nspSongListIdsLoopTop
			jmp MemUtil::JumpToVersioned
		}
	}

	// Replaces "mov ecx, [ebp-0x60] / inc ecx / mov [ebp-0x60], ecx / cmp ecx, 8 / jnz top" in the loop that
	// builds one label per option (the game's labeling call already names any list).
	void __declspec(naked) OptionLabelsLoopHook() {
		__asm {
			mov ecx, [ebp - 0x60]
			inc ecx
			mov [ebp - 0x60], ecx
			push eax
			push edx
			push ecx
			call GetOptionCount
			pop ecx
			cmp ecx, eax
			pop edx
			pop eax
			jb NextLabel

			push offset Offsets::ptr_nspSongListLabelsLoopExit
			jmp MemUtil::JumpToVersioned

		NextLabel:
			push offset Offsets::ptr_nspSongListLabelsLoopTop
			jmp MemUtil::JumpToVersioned
		}
	}

	// Replaces "cmp eax, 7 / ja empty" in the pool builder. EAX = the chosen id.
	// 0 and 1 keep the jump table, 2-7 go where the table sends them, higher ids go there too when the list
	// exists, and anything else still gets the empty pool.
	void __declspec(naked) PoolSongListHook() {
		__asm {
			cmp eax, 1
			jbe UseTable
			cmp eax, 7									// lastStockId (inline asm would read a named constant as memory)
			jbe UseSongList

			push eax
			push ecx
			push edx
			push eax
			call IsUsableExtraSongList
			test eax, eax
			pop edx
			pop ecx
			pop eax
			jz UseEmptyPool

		UseSongList:
			push offset Offsets::ptr_nspPoolSongList
			jmp MemUtil::JumpToVersioned

		UseTable:
			push offset Offsets::ptr_nspPoolDispatch
			jmp MemUtil::JumpToVersioned

		UseEmptyPool:
			push offset Offsets::ptr_nspPoolEmpty
			jmp MemUtil::JumpToVersioned
		}
	}

	struct Site {
		const char* name;
		VersioningStruct<uintptr_t>& address;
		void* hook;
		int length;									// Whole instructions replaced (see the byte patterns in Offsets.cpp)
	};
}

void ExtraSongLists::Install() {
	if (installed)
		return;

	if (!Offsets::ptr_songListService.GetValue() || !Offsets::ptr_nspPoolSongListSwitch.GetValue()) {
		LOG_WARNING("(SONG LISTS) Non-Stop Play extra song lists not installed: offsets not ported for this version" << std::endl);
		return;
	}

	const Site sites[] = {
		{ "option ids", Offsets::ptr_nspSongListIdsLoop, OptionIdsLoopHook, 6 },			// inc ebx / cmp ebx, 8 / jc top
		{ "option labels", Offsets::ptr_nspSongListLabelsLoop, OptionLabelsLoopHook, 16 },	// mov ecx, [ebp-0x60] / inc ecx / mov [ebp-0x60], ecx / cmp ecx, 8 / jnz top
		{ "pool switch", Offsets::ptr_nspPoolSongListSwitch, PoolSongListHook, 9 },			// cmp eax, 7 / ja empty
	};

	for (const Site& site : sites) {
		if (!MemUtil::PlaceHook(site.address, site.hook, site.length)) {
			LOG_ERROR("(SONG LISTS) Non-Stop Play extra song lists: failed to hook the " << site.name << " site" << std::endl);
			return;
		}
		FlushInstructionCache(GetCurrentProcess(), reinterpret_cast<void*>(site.address.GetValue()), site.length);
	}

	installed = true;
	LOG_INFO("(SONG LISTS) Non-Stop Play can use every song list in the profile" << std::endl);
}
