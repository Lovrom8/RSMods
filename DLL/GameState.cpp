#include "stdafx.h"
#include "GameState.hpp"

#include <cctype>

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

	constexpr std::string_view prefix = "Play_";
	return sv.size() >= prefix.size() && sv.substr(0, prefix.size()) == prefix;
}

bool IsArrangementIdStringValid(const char* str, size_t maxLen)
{
	if (!str)
		return false;

	if (MemUtil::IsBadReadPtr((void*)str))
		return false;

	const size_t strLen = strlen(str);
	if (strLen < 8 || strLen > maxLen)
		return false;

	for (size_t i = 0; i < strLen; ++i)
	{
		const unsigned char c = static_cast<unsigned char>(str[i]);
		if (!(std::isalnum(c) || c == '-' || c == '_'))
		{
			return false;
		}
	}

	return true;
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

std::string GameState::GetArrangementID() {
	uintptr_t arrangementHashPtr = MemUtil::FindDMAAddy(Offsets::ptr_currentMenu, Offsets::ptr_arrangementHashOffsets, true);

	if (arrangementHashPtr) {
		const char* arrangementHash = reinterpret_cast<const char*>(arrangementHashPtr);
		if (IsArrangementIdStringValid(arrangementHash, 64)) {
			lastArrangementID = arrangementHash;
		}
	}

	return lastArrangementID;
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
			// I.e. check if its neither one of the possible states
			std::string currentMenu((char*)preMainMenuAdr);

			if (lastMenu == "TitleScreen" && lastMenu != currentMenu)
				canGetRealMenu = true;
			else {
				lastMenu = currentMenu;
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

	uintptr_t currentMenuAddr = MemUtil::FindDMAAddy(Offsets::ptr_currentMenu, Offsets::ptr_currentMenuOffsets, GameNotLoaded);
	if (!currentMenuAddr) {
		//LOG_ERROR("Invalid Pointer: GetCurrentMenu(" << std::boolalpha << GameNotLoaded << ") @ LVL 3" << std::endl);
		return "where are we actually";
	}

	std::string currentMenu((char*)currentMenuAddr);
	return currentMenu;
}

/// <summary>
/// Should we turn on / off ColorBlind colors
/// </summary>
/// <param name="enabled"> - Should we turn on colors or turn off?</param>
void GameState::ToggleCB(bool enabled) {
	uintptr_t addrTimer = MemUtil::FindDMAAddy(Offsets::baseHandle + Offsets::ptr_timer, Offsets::ptr_timerBaseOffsets);
	uintptr_t cbEnabled = MemUtil::FindDMAAddy(Offsets::baseHandle + Offsets::ptr_colorBlindMode, Offsets::ptr_colorBlindModeOffsets);

	if (!addrTimer || !cbEnabled) {
		// LOG_ERROR("Invalid Pointers: ToggleCB(" << std::boolalpha << enabled << ")" << std::endl); // Disabled because it causes log to get huge real quick
		return;
	}

	// JIC, no need to write the same value constantly
	if (*(byte*)cbEnabled != (byte)enabled)
		*(byte*)cbEnabled = enabled;
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
