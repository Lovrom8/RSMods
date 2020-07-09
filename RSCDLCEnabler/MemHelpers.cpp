#include "MemHelpers.h"
#include <fstream>
#include <iostream>

int len = 0;
bool rainbowEnabled = false;

cMemHelpers MemHelpers;

byte cMemHelpers::getLowestStringTuning() {
	uintptr_t addrTuning = MemUtil.FindDMAAddy(Offsets.baseHandle + Offsets.ptr_tuning, Offsets.ptr_tuningOffsets);

	if (!addrTuning)
		return NULL;

	return *(BYTE*)addrTuning;
}

bool cMemHelpers::IsExtendedRangeSong() {
	uintptr_t addrTimer = MemUtil.FindDMAAddy(Offsets.baseHandle + Offsets.ptr_timer, Offsets.ptr_timerOffsets);

	if (!addrTimer) 
		return false;

	byte currTuning = cMemHelpers::getLowestStringTuning();
	if (currTuning == 0 || currTuning > (256 + Settings.GetModSetting("ExtendedRangeMode"))) //tunings are in negative values*, so things go backwards ;) 
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

void cMemHelpers::ToggleLoft() {
	uintptr_t addr = MemUtil.FindDMAAddy(Offsets.baseHandle + Offsets.ptr_loft, Offsets.ptr_loftOffsets);

	if (*(float*)addr == 10)
		*(float*)addr = 10000;
	else
		*(float*)addr = 10;
}

void cMemHelpers::ToggleSkyline() { 



}
void cMemHelpers::ShowSongTimer() {
	uintptr_t addrTimer = MemUtil.FindDMAAddy(Offsets.baseHandle + Offsets.ptr_timer, Offsets.ptr_timerOffsets);

	if (!addrTimer)
		return;

	std::string valStr = std::to_string(*(float*)addrTimer);
	std::cout << "Current song time: " << valStr << std::endl;
}


void cMemHelpers::ShowCurrentTuning() {
	byte lowestStringTuning = cMemHelpers::getLowestStringTuning();
	if (lowestStringTuning == NULL)
		return;

	std::string valStr = std::to_string(lowestStringTuning);
	std::cout << valStr << std::endl;
}

void cMemHelpers::ToggleCB(bool enabled) {
	uintptr_t addrTimer = MemUtil.FindDMAAddy(Offsets.baseHandle + Offsets.ptr_timer, Offsets.ptr_timerOffsets);
	uintptr_t cbEnabled = MemUtil.FindDMAAddy(Offsets.baseHandle + Offsets.ptr_colorBlindMode, Offsets.ptr_colorBlindModeOffsets);

	if (!addrTimer)
		return;

	if (*(byte*)cbEnabled != enabled) //JIC, no need to write the same value constantly
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
