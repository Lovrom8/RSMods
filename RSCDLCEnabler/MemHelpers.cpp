#include "MemHelpers.h"
#include <fstream>
#include <iostream>

int len = 0;
bool rainbowEnabled = false;

cMemHelpers MemHelpers;

byte getLowestStringTuning() {
	uintptr_t addrTuning = MemUtil.FindDMAAddy(Offsets.baseHandle + Offsets.ptr_tuning, Offsets.ptr_tuningOffsets);

	if (!addrTuning)
		return NULL;

	return *(BYTE*)addrTuning;
}

bool cMemHelpers::IsExtendedRangeSong() {
	uintptr_t addrTimer = MemUtil.FindDMAAddy(Offsets.baseHandle + Offsets.ptr_timer, Offsets.ptr_timerOffsets);

	if (!addrTimer) 
		return false;

	byte currTuning = getLowestStringTuning();
	if (currTuning == 0 || currTuning > (255 + Settings.GetModSetting("ExtendedRangeMode"))) //tunings are in negative values*, so things go backwards ;) 
		return false;
	else
		return true;
}

std::string cMemHelpers::GetCurrentMenu() {
	uintptr_t currentMenuAdr = MemUtil.FindDMAAddy(Offsets.baseHandle + Offsets.ptr_currentMenu, Offsets.ptr_currentMenuOffsets);

	if (currentMenuAdr > 0x30000000) //pls don't kill me for doing a check like this, but I really don't know a more reliable way to check if it points to an actual value (it gets initialized when you click on first Enter prompt)... instead it usually points to 0x6XXXXXXX
		return "pre_enter_prompt";

	if (!currentMenuAdr)
		return "";

	std::string currentMenu((char*)currentMenuAdr);

	return currentMenu;
}


bool cMemHelpers::LoadModsWhenSongsLoad(std::string ModToRun) {
		MessageBoxA(NULL, GetCurrentMenu().c_str(), NULL, NULL);
		std::string currentMenu = GetCurrentMenu();

		uintptr_t addrTimer = MemUtil.FindDMAAddy(Offsets.baseHandle + Offsets.ptr_timer, Offsets.ptr_timerOffsets);
		if (!addrTimer) { //don't try to read before a song started, otherwise crashes are inbound 
			return false;
		}


		if (currentMenu == "GuidedExperience_Game" || currentMenu == "GuidedExperience_Pause") {
			// Toggle Loft Will Break Lesson Mode
			if (ModToRun == "RainbowStrings") {
				return true;
			}
			if (ModToRun == "RemoveSkylineGE") {
				return true;
			}
		}
		if (currentMenu == "ScoreAttack_Game" || currentMenu == "ScoreAttack_Pause" || currentMenu == "LAS_Game" || currentMenu == "LAS_Pause" || currentMenu == "LearnASong_Pause" || currentMenu == "LearnASong_Game") {
			if (ModToRun == "RainbowStrings") {
				return true;
			}
			if (ModToRun == "RemoveSkylineLAS_GC") {
				return true;
			}
		}
		return false;
	}


void cMemHelpers::ToggleLoft() {
	uintptr_t addr = MemUtil.FindDMAAddy(Offsets.baseHandle + Offsets.ptr_loft, Offsets.ptr_loftOffsets);

	if (*(float*)addr == 10)
		*(float*)addr = 10000;
	else
		*(float*)addr = 10;
}

void cMemHelpers::ToggleLoftWhenSongStarts() {
	uintptr_t addrTimer = MemUtil.FindDMAAddy(Offsets.baseHandle + Offsets.ptr_timer, Offsets.ptr_timerOffsets);
	uintptr_t addrLoft = MemUtil.FindDMAAddy(Offsets.baseHandle + Offsets.ptr_loft, Offsets.ptr_loftOffsets);

	if (addrLoft && *(float*)addrLoft != 10) { //it gets initialized at the login menu, so reading before crashes the game hence we use this as a sort-of check... not the best, but hey
		std::string currentMenu = GetCurrentMenu();
		if (currentMenu == "GuidedExperience_Game" || currentMenu == "GuidedExperience_Pause")
				*(float*)addrLoft = 10;
		
		return;
	}

	if (!addrTimer) { //don't try to read before a song started, otherwise crashes are inbound 
		if (addrLoft && *(float*)addrLoft != 10) { //if we are loaded far enough && loft is disabled
			*(float*)addrLoft = 10; //enable it
		}
		return;
	}

	std::string currentMenu = GetCurrentMenu(); //also by no means something that should be done (twice in a row), buuuuut... for now it will do
	if (currentMenu == "GuidedExperience_Game" || currentMenu == "GuidedExperience_Pause")
		return;
		
	if (*(float*)addrLoft != 10)
		return;

	if (*(float*)addrTimer >= 0.1)
		*(float*)addrLoft = 10000;
}

void cMemHelpers::ShowSongTimer() {
	uintptr_t addrTimer = MemUtil.FindDMAAddy(Offsets.baseHandle + Offsets.ptr_timer, Offsets.ptr_timerOffsets);

	if (!addrTimer)
		return;

	std::string valStr = std::to_string(*(float*)addrTimer);
	std::cout << "Current song time: " << valStr << std::endl;
}



void cMemHelpers::ShowCurrentTuning() {
    //98.432

	byte lowestStringTuning = getLowestStringTuning();
	if (lowestStringTuning == NULL)
		return;


	std::string valStr = std::to_string(lowestStringTuning);

	MessageBoxA(NULL, valStr.c_str(), "", 0);
}



void cMemHelpers::ToggleCB(bool enabled) {
	uintptr_t addrTimer = MemUtil.FindDMAAddy(Offsets.baseHandle + Offsets.ptr_timer, Offsets.ptr_timerOffsets);
	uintptr_t cbEnabled = MemUtil.FindDMAAddy(Offsets.baseHandle + Offsets.ptr_colorBlindMode, Offsets.ptr_colorBlindModeOffsets);

	if (!addrTimer)
		return;

	*(byte*)cbEnabled = enabled;
}

void cMemHelpers::PatchCDLCCheck() {
	uint8_t* VerifySignatureOffset = Offsets.cdlcCheckAdr;

	if (VerifySignatureOffset) {
		if (!MemUtil.PatchAdr(VerifySignatureOffset + 8, (UINT*)Offsets.patch_CDLCCheck, 2))
			printf("Failed to patch verify_signature!\n");
		else
			printf("Patch verify_signature success!\n");
	}
}
