#include "stdafx.h"
#include "GameState.hpp"

namespace {
	// The menu name is an STLport std::string object: a short name is stored inline in the object, a longer one on the
	// heap through the pointer at +0. The end pointer at +0x10 lies inside the object exactly when the name is inline.
	constexpr size_t stringEndPointerOffset = 0x10;
	constexpr size_t stringHeaderSize = stringEndPointerOffset + sizeof(uintptr_t);

	bool ReadRaw(uintptr_t source, unsigned char* destination, size_t size) {
		__try {
			memcpy(destination, reinterpret_cast<const void*>(source), size);
			return true;
		}
		__except (EXCEPTION_EXECUTE_HANDLER) { return false; }
	}

	// Copies a printable, NUL-terminated name under SEH before any C++ string is built. Anything else (a stale
	// address, bytes that are not text) is an unknown state.
	bool CopyMenuName(uintptr_t source, char* destination, size_t capacity, size_t& length) {
		length = 0;
		__try {
			for (; length + 1 < capacity; ++length) {
				const unsigned char c = *reinterpret_cast<const unsigned char*>(source + length);
				if (c == 0) { destination[length] = 0; return true; }
				if (c < 0x20 || c > 0x7E) return false;
				destination[length] = static_cast<char>(c);
			}
		}
		__except (EXCEPTION_EXECUTE_HANDLER) { return false; }
		return false;
	}

	// Name held by the string object at |object|; false when it cannot be read as text.
	bool ReadMenuName(uintptr_t object, std::string& name) {
		unsigned char header[stringHeaderSize]{};
		if (!object || !ReadRaw(object, header, sizeof(header)))
			return false;

		uintptr_t end = 0, heap = 0;
		memcpy(&end, header + stringEndPointerOffset, sizeof(end));
		memcpy(&heap, header, sizeof(heap));
		const bool inlineName = end >= object && end <= object + stringEndPointerOffset;

		char bytes[256]{};
		size_t length = 0;
		if (!CopyMenuName(inlineName ? object : heap, bytes, std::size(bytes), length))
			return false;

		name.assign(bytes, length);
		return true;
	}
}

/// <summary>
/// Are we in a song?
/// </summary>
bool GameState::IsInSong() {
	if (!GameLoaded)
	{
		return false;
	}

	return Contains(GetCurrentMenu(), songModes);
}

/// <summary>
/// Get the status of if the user is in multiplayer
/// </summary>
/// <returns>Is the user in multiplayer</returns>
bool GameState::IsMultiplayer() {
	return *(int*)MemUtil::FindDMAAddy(Offsets::baseHandle + Offsets::ptr_multiplayer, Offsets::ptr_multiplayerOffsets); // No need to null check because if it's null, then we assume it's singleplayer (which is zero).
}

/// <summary>
/// Get the current selected profile name. **Only works on Profile Selection screen**
/// </summary>
/// <returns>Profile Name</returns>
std::string GameState::CurrentSelectedUser() {
	uintptr_t badValue = MemUtil::FindDMAAddy(Offsets::baseHandle + Offsets::ptr_selectedProfileName, Offsets::ptr_selectedProfileNameOffsets);

	// If the pointer is invalid just return nothing
	if (!badValue) {
		LOG_ERROR("Invalid Pointer: CurrentSelectedUser" << std::endl);
		return (std::string)"";
	}

	// This value 90% of the time starts with an invalid pointer. We must wait ~2.5 seconds to guarantee that it is correct, or until the pointer changes (whichever comes first).
	for (int i = 0; i < 25; i++)
	{
		if (badValue >= 0x10000000)
			break;

		badValue = MemUtil::FindDMAAddy(Offsets::baseHandle + Offsets::ptr_selectedProfileName, Offsets::ptr_selectedProfileNameOffsets);
	}

	return std::string((const char*)badValue);
}

bool IsSongKeyStringValid(const char* str, size_t max_len)
{
	if (!str)
		return false;

	if (MemUtil::IsBadReadPtr((void*)str))
		return false;

	size_t strLen = strlen(str);
	if (strLen <= 13 || strLen > max_len)
		return false;

	std::string_view sv(str, strLen);
	return sv.starts_with("Play_");
}

/// <summary>
/// Gets the SongKey of the current playing song, based on the initial preview.
/// </summary>
/// <returns>Last played Song Key</returns>
std::string GameState::GetSongKey() {
	uintptr_t previewEventPtr = MemUtil::FindDMAAddy(Offsets::baseHandle + Offsets::ptr_previewName, Offsets::ptr_previewNameOffsets);

	if (previewEventPtr) {
		const char* previewEvent = (char*)previewEventPtr;

		// Check if it's null terminated within a reasonable number of bytes
		if (IsSongKeyStringValid(previewEvent, 50))
		{
			auto previewName = std::string(previewEvent);

			// If the preview name contains "Play_", which is required by Wwise.
			if (previewName.length() > 13 && previewName._Starts_with("Play_")) {

				// Verify that we are working with a "_Preview" audio.
				// "_Invalid" is used when the user turns off their song previews.
				if (previewName.compare(previewName.length() - 9, 9, "_Preview") || previewName.compare(previewName.length() - 9, 9, "_Invalid")) {
					lastSongKey = previewName.substr(5, previewName.length() - 13);
				}
			}
		}
	}
	return lastSongKey;
}

/// <param name="GameNotLoaded"> - Should we trust the pointer?</param>
/// <returns>Internal Menu Name</returns>
std::string GameState::GetCurrentMenu(bool GameNotLoaded) {
	bool failedToReadPreMainMenuAddr = false;

	// It seems like the third level of the pointer isn't initialized until you reach the UPLAY login screen,
	// but the second level actually is, and in there it keeps either an empty string, "TitleMenu", "MainOverlay"
	// (before you reach the login) or some gibberish that's always the same (after that) 
	if (GameNotLoaded) {
		uintptr_t preMainMenuAdr = MemUtil::FindDMAAddy(Offsets::ptr_currentMenu, Offsets::ptr_preMainMenuOffsets, GameNotLoaded);

		if (preMainMenuAdr)
		{
			// I.e. check if its neither one of the possible states. After the login this level holds bytes that
			// are not text; that change away from "TitleScreen" is the signal that the real menu pointer is valid,
			// so unreadable bytes count as a different name, not as no state.
			std::string menuName;
			if (!ReadMenuName(preMainMenuAdr, menuName))
				menuName = "<not text>";

			if (lastMenu == "TitleScreen" && lastMenu != menuName)
				canGetRealMenu = true;
			else {
				lastMenu = menuName;
				return "pre_enter_prompt";
			}
		}
		else
		{
			//_LOG_INFO("Invalid Pointer: GetCurrentMenu(" << std::boolalpha << GameNotLoaded << ") @ LVL 2" << std::endl);
			failedToReadPreMainMenuAddr = true;
		}
	}

	if (!canGetRealMenu)
		return "pre_enter_prompt";


	// If game hasn't loaded, take the safer, but possibly slower route

	// The chain ends at the string object itself (the pre-main-menu offsets), not at its first pointer: a short name
	// such as "MainMenu" lives inside the object, and following +0 would read its first letters as an address.
	const uintptr_t object = MemUtil::FindDMAAddy(Offsets::ptr_currentMenu, Offsets::ptr_preMainMenuOffsets, GameNotLoaded);
	std::string menuName;
	if (!ReadMenuName(object, menuName)) {
		//LOG_ERROR("Invalid Pointer: GetCurrentMenu(" << std::boolalpha << GameNotLoaded << ") @ LVL 3" << std::endl);
		return "where are we actually";
	}

	if (menuName == "MainMenu")
		BootComplete = true;

	return menuName;
}

/// <summary>
/// Should we turn on / off ColorBlind colors
/// </summary>
/// <param name="enabled"> - Should we turn on colors or turn off?</param>
void GameState::ToggleCB(bool enabled) {
	uintptr_t addrTimer = MemUtil::FindDMAAddy(Offsets::baseHandle + Offsets::ptr_timer, Offsets::ptr_timerBaseOffsets);
	uintptr_t cbEnabledAddr = MemUtil::FindDMAAddy(Offsets::baseHandle + Offsets::ptr_colorBlindMode, Offsets::ptr_colorBlindModeOffsets);

	if (!addrTimer || !cbEnabledAddr) {
		// LOG_ERROR("Invalid Pointers: ToggleCB(" << std::boolalpha << enabled << ")" << std::endl); // Disabled because it causes log to get huge real quick
		return;
	}

	// JIC, no need to write the same value constantly
	if (*(byte*)cbEnabledAddr != (byte)enabled)
		*(byte*)cbEnabledAddr = enabled;
}

namespace GameState {
	namespace Menus {
		bool IsInMultiplayerTunerMenus() {
			return Contains(currentMenu, learnASongModes);
		}

		bool IsInScoreMenus() {
			return Contains(currentMenu, scoreScreens);
		}

		bool IsInTuningMenus() {
			return Contains(currentMenu, tuningMenus);
		}

		bool IsInPreSongTuner() {
			return Contains(currentMenu, preSongTuners);
		}

		bool IsInSongModes() {
			return Contains(currentMenu, songModes);
		}

		bool IsInScoreAttackModes() {
			return Contains(currentMenu, scoreAttackModes);
		}

		bool IsInLearnASongModes() {
			return Contains(currentMenu, learnASongModes);
		}

		bool IsInLearnASongPauseModes() {
			return Contains(currentMenu, lasPauseMenus);
		}

		bool IsInModesWithAllowedFastRiffRepeater() {
			return Contains(currentMenu, fastRRModes);
		}

		bool IsInOnlineModes() {
			return Contains(currentMenu, onlineModes);
		}

		bool IsInGuitarcadeGame() {
			return Contains(currentMenu, guitarcadeGameModes);
		}

		bool IsInLASPlayingModes() {
			return Contains(currentMenu, learnASongPlaying);
		}

		bool IsOnScoreScreens() {
			return Contains(currentMenu, scoreScreens);
		}

		bool IsInLessonModes() {
			return Contains(currentMenu, lessonModes);
		}

		bool IsInMenusWithDisallowedAutoEnter() {
			return Contains(currentMenu, dontAutoEnter);
		}

		bool IsInCalibrationMenus() {
			return Contains(currentMenu, calibrationMenus);
		}
	}
}

bool Contains(std::string_view text, std::string_view key) {
	return text.find(key) != std::string_view::npos;
}