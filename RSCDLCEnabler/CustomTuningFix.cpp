#include "CustomTuningFix.h"


cCustomTuningFix CustomTuningFix;

/*
cache_psarc\cache7.7z\manifests \ tuningdatabase.json - full replacement with our file - "ideally" ability to add to it one entry at a time from the gui.... but low priority.

cache_psarc\cache4.7z\gfxassets\views \ introsequence.gfx - full replacement

cache_psarc\cache4.7z\localization \ maingame.csv - lines to be appended to existing file (base file is 8 mb), pipe dream is gui add entries to match tuningdatabase.json

*/


void __declspec(naked) customTuningHook() {
	__asm {
		push ecx
		push edx
		push esp

		push[esp + 0x10]
		push eax
		call cCustomTuningFix::customTuningFix

		pop esp
		pop edx
		pop ecx

		add esp, 0x8
		push eax
		jmp[Offsets.hookBackAddr_missingLocalization]
	}
}

char __stdcall cCustomTuningFix::customTuningFix(int numberInMainGameCSV, char* newValue) {
	const int buffer_size = 10;
	char str[buffer_size];
	sprintf_s(&str[0], buffer_size, "%d", numberInMainGameCSV);

	switch (numberInMainGameCSV)
	{
	/*
	Ex:
	case numberInMainGameCSV:
		return (char)variable;
	*/
	

	default:
		return (char)str;
	}
}

