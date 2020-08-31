#include "MemHelpers.hpp"
#pragma warning(disable: 4805) // Mix of byte & bool

byte MemHelpers::getLowestStringTuning() {
	uintptr_t addrTuning = MemUtil::FindDMAAddy(Offsets::baseHandle + Offsets::ptr_tuning, Offsets::ptr_tuningOffsets);

	if (!addrTuning)
		return NULL;

	return *(BYTE*)addrTuning;
}

float MemHelpers::GetCurrentMusicVolume() { // Alternative to GetRTPCValue
	uintptr_t volumeAddr = MemUtil::FindDMAAddy(Offsets::baseHandle + Offsets::ptr_volume, Offsets::ptr_volumeOffsets);

	if (!volumeAddr)
		return false;

	return *(float*)volumeAddr;
}

bool MemHelpers::IsExtendedRangeSong() {
	uintptr_t addrTimer = MemUtil::FindDMAAddy(Offsets::baseHandle + Offsets::ptr_timer, Offsets::ptr_timerOffsets);

	if (!addrTimer)
		return false;

	byte currentTuning = MemHelpers::getLowestStringTuning();

	if (currentTuning != 0 && currentTuning <= (256 + Settings::GetModSetting("ExtendedRangeMode")))
		return true;
	return false;
}

std::string MemHelpers::GetCurrentMenu(bool GameNotLoaded) {
	if (GameNotLoaded) { // It seems like the third level of the pointer isn't initialized until you reach the UPLAY login screen, but the second level actually is, and in there it keeps either an empty string, "TitleMenu", "MainOverlay" (before you reach the login) or some gibberish that's always the same (after that) 
		uintptr_t preMainMenuAdr = MemUtil::FindDMAAddy(Offsets::ptr_currentMenu, Offsets::ptr_preMainMenuOffsets, GameNotLoaded);

		std::string currentMenu((char*)preMainMenuAdr);

		if (lastMenu == "TitleScreen" && lastMenu != currentMenu)  // I.e. check if its neither one of the possible states
			canGetRealMenu = true;
		else {
			lastMenu = currentMenu;
			return "pre_enter_prompt";
		}
	}

	if (!canGetRealMenu)
		return "pre_enter_prompt";

	uintptr_t currentMenuAdr = MemUtil::FindDMAAddy(Offsets::ptr_currentMenu, Offsets::ptr_currentMenuOffsets, GameNotLoaded); // If game hasn't loaded, take the safer, but possibly slower route

	// Thank you for serving the cause comrade, but you aren't needed any more
	//if (currentMenuAdr > 0x30000000) // Ghetto checks for life, if you haven't eneterd the game it usually points to 0x6XXXXXXX and if you try to dereference that, you get yourself a nice crash
	//	return "pre_enter_prompt";

	if (!currentMenuAdr)
		return "where are we actually";

	std::string currentMenu((char*)currentMenuAdr);
	//std::cout << currentMenu << std::endl;
	return currentMenu;
}

void MemHelpers::ToggleLoft() {

	// Old Method (Works for most builds but bugged out for some)
	//uintptr_t nearAddr = MemUtil::FindDMAAddy(Offsets::baseHandle + Offsets::ptr_loft, Offsets::ptr_loft_nearOffsets);
	//if (*(float*)nearAddr == 10)
	//	*(float*)nearAddr = 10000;
	//else
	//	*(float*)nearAddr = 10;

	uintptr_t farAddr = MemUtil::FindDMAAddy(Offsets::baseHandle + Offsets::ptr_loft, Offsets::ptr_loft_farOffsets);

	if (*(float*)farAddr == 10000)
		*(float*)farAddr = 1;
	else
		*(float*)farAddr = 10000;
}

void MemHelpers::ShowSongTimer() {
	uintptr_t addrTimer = MemUtil::FindDMAAddy(Offsets::baseHandle + Offsets::ptr_timer, Offsets::ptr_timerOffsets);

	if (!addrTimer)
		return;

	std::string valStr = std::to_string(*(float*)addrTimer);
	std::cout << "Current song time: " << valStr << std::endl;
}


void MemHelpers::ShowCurrentTuning() {
	byte lowestStringTuning = MemHelpers::getLowestStringTuning();
	if (lowestStringTuning == NULL)
		return;

	std::string valStr = std::to_string(lowestStringTuning);
	std::cout << valStr << std::endl;
}

void MemHelpers::ToggleCB(bool enabled) {
	uintptr_t addrTimer = MemUtil::FindDMAAddy(Offsets::baseHandle + Offsets::ptr_timer, Offsets::ptr_timerOffsets);
	uintptr_t cbEnabled = MemUtil::FindDMAAddy(Offsets::baseHandle + Offsets::ptr_colorBlindMode, Offsets::ptr_colorBlindModeOffsets);

	if (!addrTimer)
		return;

	if (*(byte*)cbEnabled != enabled) //JIC, no need to write the same value constantly
		*(byte*)cbEnabled = enabled;
}

void MemHelpers::PatchCDLCCheck() {
	uint8_t* VerifySignatureOffset = Offsets::cdlcCheckAdr;

	if (VerifySignatureOffset) {
		if (!MemUtil::PatchAdr(VerifySignatureOffset + 8, (UINT*)Offsets::patch_CDLCCheck, 2))
			printf("Failed to patch verify_signature!\n");
		else
			printf("Patch verify_signature success!\n");
	}
}

int* MemHelpers::GetWindowSize() {
	RECT WindowSize;
	if (GetWindowRect(FindWindow(NULL, L"Rocksmith 2014"), &WindowSize))
	{
		int width = WindowSize.right - WindowSize.left;
		int height = WindowSize.bottom - WindowSize.top;

		int* dimensions = new int[2]{ width, height };

		return dimensions;
	}
}

bool MemHelpers::IsInStringArray(std::string stringToCheckIfInsideArray, std::string* stringArray, std::vector<std::string> stringVector) {
	if (stringArray != NULL) {
		for (int i = 0; i < stringArray->length(); i++) {
			if (stringToCheckIfInsideArray == stringArray[i])
				return true;
		}
	}
	else if (stringVector != std::vector<std::string>()) {
		for (int i = 0; i < stringVector.size(); i++) {
			if (stringToCheckIfInsideArray == stringVector[i])
				return true;
		}
	}

	return false;
}

void MemHelpers::DrawTextOnScreen(std::string textToDraw, COLORREF textColor, int topLeftX, int topLeftY, int bottomRightX, int bottomRightY, bool transparentBackground) {
	HDC RocksmithHDC = GetDC(FindWindow(NULL, L"Rocksmith 2014"));

	RECT* TextRectangle = new RECT();

	TextRectangle->left = topLeftX;
	TextRectangle->top = topLeftY;
	TextRectangle->right = bottomRightX;
	TextRectangle->bottom = bottomRightY;

	SetTextColor(RocksmithHDC, textColor);
	if (transparentBackground)
		SetBkMode(RocksmithHDC, TRANSPARENT);

	DrawTextA(RocksmithHDC, textToDraw.c_str(), textToDraw.size(), TextRectangle, DT_NOCLIP);
}