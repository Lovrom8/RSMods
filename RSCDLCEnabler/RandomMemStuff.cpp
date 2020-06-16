#include "RandomMemStuff.h"
#include <fstream>
int len = 0;

#include <iostream>


std::vector<std::string> songTitles(6);

byte getLowestStringTuning() {
	uintptr_t addrTuning = MemUtil.FindDMAAddy(Offsets.baseHandle + Offsets.ptr_tuning, Offsets.ptr_tuningOffsets);

	if (!addrTuning)
		return NULL;

	return *(BYTE*)addrTuning;
}

uintptr_t GetStringColor(uintptr_t stringnum, string_state state) {
	uintptr_t edx = stringnum;
	uintptr_t eax = 0;
	uintptr_t magic1 = Offsets.ptr_stringColor;

	uintptr_t ecx = MemUtil.ReadPtr(magic1);

	if (!ecx)
		return NULL;

	eax = MemUtil.ReadPtr(ecx + eax * 0x4 + 0x348);

	if (eax >= 2) {
		return NULL;
	}

	eax = eax * 0xA8;
	eax += edx;

	eax = MemUtil.ReadPtr(ecx + eax * 0x4 + state);

	return eax;
}

void RandomMemStuff::Toggle7StringMode() {
	uintptr_t string1 = GetStringColor(0, normal);
	uintptr_t string2 = GetStringColor(1, normal);

	if (!string1)
		return;

	uintptr_t addrTimer = MemUtil.FindDMAAddy(Offsets.baseHandle + Offsets.ptr_timer, Offsets.ptr_timerOffsets);
	uintptr_t addrLoft = MemUtil.FindDMAAddy(Offsets.baseHandle + Offsets.ptr_loft, Offsets.ptr_loftOffsets);

	if (!addrTimer) {
		return;
	}

	byte currTuning = getLowestStringTuning();
	if (currTuning == 0 || currTuning > (255-Settings.GetModSetting("ExtendedRangeMode"))) { //tunings are in negative values*, so things go backwards ;) 
		/*Color c = Color();
		c.r = 1.f;
		c.g = 0.f;
		c.b = 0.f;
		*(Color*)string1 = c

		*/

		Is7StringSong = false;
	}
	else {
		/*Color c = Color();
		c.r = 1.f;
		c.g = 0.f;
		c.b = 0.f;

		float h = 50.f;
		c.setH(h);

		*(Color*)string1 = c;
		*/

		/*h = 75.0f;
		c.setH(h);

		*(Color*)string2 = c; */
	
		Is7StringSong = true;
	}
}

std::string GetCurrentMenu() {
	uintptr_t currentMenuAdr = MemUtil.FindDMAAddy(Offsets.baseHandle + Offsets.ptr_currentMenu, Offsets.ptr_currentMenuOffsets);

	if (!currentMenuAdr)
		return "";

	std::string currentMenu((char*)currentMenuAdr);

	return currentMenu;
}


bool RandomMemStuff::LoadModsWhenSongsLoad(std::string ModToRun) {
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
		return false;
	}
	if (currentMenu == "ScoreAttack_Game" || currentMenu == "ScoreAttack_Pause" || currentMenu == "LAS_Game" || currentMenu == "LAS_Pause" || currentMenu == "LearnASong_Pause" || currentMenu == "LearnASong_Game") {
		if (ModToRun == "RainbowStrings") {
			return true;
		}
	}
	return false;
}

void RandomMemStuff::DoRainbow() {
	std::vector<uintptr_t> stringsNormal;
	std::vector<uintptr_t> stringsHigh;
	std::vector<uintptr_t> stringsDisabled;

	for (int i = 0; i < 6; i++) {
		stringsNormal.push_back(GetStringColor(i, normal));
		stringsHigh.push_back(GetStringColor(i, highlight));
		stringsDisabled.push_back(GetStringColor(i, disabled));
	}

	Color c = Color();
	c.r = 1.f;
	c.g = 0.f;
	c.b = 0.f;

	float h = 0.f;
	float speed = 2.f;
	float stringOffset = 20.f;

	while (true) {

		h += speed;
		if (h >= 360.f) { h = 0.f; }

		for (int i = 0; i < 6; i++) {
			c.setH(h + (stringOffset*i));

			*(Color*)stringsNormal[i] = c;
			*(Color*)stringsHigh[i] = c;
			*(Color*)stringsDisabled[i] = c;
		}

		Sleep(16);
	}

}

void RandomMemStuff::AddVolume(float add) {
	uintptr_t addr = MemUtil.FindDMAAddy(Offsets.baseHandle + Offsets.ptr_volume, Offsets.ptr_volumeOffsets);

	float val = *(float*)addr;

	if (val + add >= 100.0f)
		return;

	*(float*)addr = val + add;
}

void RandomMemStuff::DecreaseVolume(float remove) {
	uintptr_t addr = MemUtil.FindDMAAddy(Offsets.baseHandle + Offsets.ptr_volume, Offsets.ptr_volumeOffsets);

	float val = *(float*)addr;

	if (val + remove <= 0.0f)
		return;

	*(float*)addr = val + remove;
}

void RandomMemStuff::ToggleLoft() {
	uintptr_t addr = MemUtil.FindDMAAddy(Offsets.baseHandle + Offsets.ptr_loft, Offsets.ptr_loftOffsets);

	if (*(float*)addr == 10)
		*(float*)addr = 10000;
	else
		*(float*)addr = 10;
}

void RandomMemStuff::ToggleLoftWhenSongStarts() {
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

void RandomMemStuff::ShowSongTimer() {
	uintptr_t addrTimer = MemUtil.FindDMAAddy(Offsets.baseHandle + Offsets.ptr_timer, Offsets.ptr_timerOffsets);

	if (!addrTimer)
		return;

	std::string valStr = std::to_string(*(float*)addrTimer);
	MessageBoxA(NULL, valStr.c_str(), "", 0);
}



void RandomMemStuff::ShowCurrentTuning() {
    //98.432

	byte lowestStringTuning = getLowestStringTuning();
	if (lowestStringTuning == NULL)
		return;


	std::string valStr = std::to_string(lowestStringTuning);
	MessageBoxA(NULL, valStr.c_str(), "", 0);
}

void __declspec(naked) hook_fakeTitles() {
	//ESI = INDEX
	//EAX = char* str "$[36969]SONG LIST"
	__asm {
		mov ecx, dword ptr ds : [0x135CB7C]
		pushad
		mov ebx, 0x33 //sets the last number 
		add ebx, esi
		mov byte ptr[eax + 0x6], bl
		mov byte ptr[eax + 0x2], 0x34 //third char (that is, the first digit); 0x30 = 0, 0x31 = 1, ... | with 0x34, there's no string with key [4696x] - it returns the whole thing back
		//mov byte ptr[eax + 0x3], 0x34 //adapt to your needs
		popad
		jmp[Offsets.hookBackAddr_FakeTitles]
	}
}

/*Koko's version - hijacks the format string for printf & discards the parameters and then returns our (kkomrade) versions of the names
Quite likely, this one is better in our use case, since we want to grab titles from a file, which is done more conveniently in a regular CPP function without the ASM
*/
char __stdcall missingLocalization(int number, char* text) {
	const int buffer_size = 10;
	char str[buffer_size];
	sprintf_s(&str[0], buffer_size, "%d", number);
	//MsgBoxA(str, "Missing locale str");

	switch (number)
	{
	case 46964: //either start from 46964 or change mov ebx, 0x33 to mov ebx, 0x2F in hook_fakeTitles to start from 0
		return (char)songTitles[0].c_str();
	case 46965:
		return (char)songTitles[1].c_str();
	case 46966:
		return (char)songTitles[2].c_str();
	case 46967:
		return (char)songTitles[3].c_str();
	case 46968:
		return (char)songTitles[4].c_str();
	case 46969:
		return (char)songTitles[5].c_str();
	default:
		return (char)str;
	}
}

void __declspec(naked) missingLocalizationHookFunc() {
	__asm {
		push ecx
		push edx
		push esp

		push[esp + 0x10]
		push eax
		call missingLocalization

		pop esp
		pop edx
		pop ecx

		add esp, 0x8
		push eax
		jmp[Offsets.hookBackAddr_missingLocalization]
	}
}


void RandomMemStuff::PatchSongListAppendages() {
	MemUtil.PatchAdr((BYTE*)Offsets.patch_addedSpaces, (UINT*)Offsets.patch_ListSpaces, 5); //MemUtil out " "
	MemUtil.PatchAdr((BYTE*)Offsets.patch_addedNumbers, (UINT*)Offsets.patch_ListNumbers, 5); //MemUtil 1-6
}

void RandomMemStuff::SetFakeListNames() {
	PatchSongListAppendages();

	len = 6;

	Offsets.hookBackAddr_FakeTitles = Offsets.hookAddr_ModifyLocalized + len;
	MemUtil.PlaceHook((void*)Offsets.hookAddr_ModifyLocalized, hook_fakeTitles, len);
}

void RandomMemStuff::HookSongListsKoko() {
	SetFakeListNames();

	len = 5;
	Offsets.hookBackAddr_missingLocalization = Offsets.hookAddr_MissingLocalization + len;

	//Skip less printf parameters if those have been removed
	MemUtil.PatchAdr((BYTE*)Offsets.patch_sprintfArg, (BYTE*)Offsets.patch_SprintfArgs, 1);

	MemUtil.PlaceHook((void*)Offsets.hookAddr_MissingLocalization, missingLocalizationHookFunc, len);
}

void RandomMemStuff::LoadSettings() {
	songTitles = Settings.GetCustomSongTitles();
}

void RandomMemStuff::ToggleCB(bool enabled) {
	uintptr_t addrTimer = MemUtil.FindDMAAddy(Offsets.baseHandle + Offsets.ptr_timer, Offsets.ptr_timerOffsets);
	uintptr_t cbEnabled = MemUtil.FindDMAAddy(Offsets.baseHandle + Offsets.ptr_colorBlindMode, Offsets.ptr_colorBlindModeOffsets);

	if (!addrTimer)
		return;

	*(byte*)cbEnabled = enabled;
}