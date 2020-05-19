#include "Offsets.h"
#include "MemUtil.h"

cOffsets Offsets;

void cOffsets::Initialize() {
	cdlcCheckAdr = MemUtil.FindPattern(cdlcCheckdwAdr, cdlcCheckSearchLen, (uint8_t*)sig_CDLCCheck, sig_CDLCCheckMask);
	baseHandle = (uintptr_t)GetModuleHandle(NULL);
}

