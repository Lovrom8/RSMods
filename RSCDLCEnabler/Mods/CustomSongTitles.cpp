#include "CustomSongTitles.hpp"
#pragma warning(disable: 4302) // Typecast truncated from const _Elem* -> char (Leaving this because the user shouldn't have a song list name over 255).

// TODO: Fix the dark magic of ASM in here (if possible).

std::vector<std::string> songTitles(6);

/// <summary>
/// Hook to grab songlists. EAX = char* str "$[36969]SONG LIST". ESI = SongList Index
/// </summary>
void __declspec(naked) hook_fakeTitles() {
	__asm {
		mov ecx, dword ptr ds : [0x135CB7C]
		pushad
		mov ebx, 0x33 //sets the last number 
		add ebx, esi
		mov byte ptr[eax + 0x6], bl
		mov byte ptr[eax + 0x2], 0x34 //third char (that is, the first digit); 0x30 = 0, 0x31 = 1, ... | with 0x34, there's no string with key [4696x] - it returns the whole thing back
		//mov byte ptr[eax + 0x3], 0x34 //adapt to your needs
		popad
		jmp[Offsets::hookBackAddr_FakeTitles]
	}
}

/// <summary>
/// Hijacks the format string for printf and discards the parameters and then returns our (kkomrade) versions of the names. Perfected by Koko.
/// </summary>
/// <param name="number"> - MainGame.csv string ID number [EAX]</param>
/// <param name="text"> - Song List Name [ESP + 0x10] **DISCARDED**</param>
/// <returns>New songlist name</returns>
const char* __stdcall missingLocalization(int number, char* text) {
	text = "";
	sprintf_s(&string_buffer[0], buffer_size, "%d", number);
	//MsgBoxA(str, "Missing locale str");

	switch (number)
	{
	case 46964: //either start from 46964 or change mov ebx, 0x33 to mov ebx, 0x2F in hook_fakeTitles to start from 0
		return songTitles[0].c_str();
	case 46965:
		return songTitles[1].c_str();
	case 46966:
		return songTitles[2].c_str();
	case 46967:
		return songTitles[3].c_str();
	case 46968:
		return songTitles[4].c_str();
	case 46969:
		return songTitles[5].c_str();
	default:
		return string_buffer;
	}
}

/// <summary>
/// Hook to place new songlist names
/// </summary>
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
		jmp[Offsets::hookBackAddr_missingLocalization]
	}
}

/// <summary>
/// Remove spaces and numbers. "SONG LIST 1" -> "SONGLIST"
/// </summary>
void CustomSongTitles::PatchSongListAppendages() {
	MemUtil::PatchAdr((BYTE*)Offsets::patch_addedSpaces, (UINT*)Offsets::patch_ListSpaces, 5); //MemUtil out " "
	MemUtil::PatchAdr((BYTE*)Offsets::patch_addedNumbers, (UINT*)Offsets::patch_ListNumbers, 5); //MemUtil 1-6
}

/// <summary>
/// Place Songlist Remove Hooks
/// </summary>
void CustomSongTitles::SetFakeListNames() {
	PatchSongListAppendages();

	len = 6;

	Offsets::hookBackAddr_FakeTitles = Offsets::hookAddr_ModifyLocalized + len;
	MemUtil::PlaceHook((void*)Offsets::hookAddr_ModifyLocalized, hook_fakeTitles, len);
}

/// <summary>
/// Place Songlist Removal Hooks and New Songlist Name Hooks
/// </summary>
void CustomSongTitles::HookSongListsKoko() {
	SetFakeListNames();

	len = 5;
	Offsets::hookBackAddr_missingLocalization = Offsets::hookAddr_MissingLocalization + len;

	//Skip less printf parameters if those have been removed
	MemUtil::PatchAdr((BYTE*)Offsets::patch_sprintfArg, (BYTE*)Offsets::patch_SprintfArgs, 1);

	MemUtil::PlaceHook((void*)Offsets::hookAddr_MissingLocalization, missingLocalizationHookFunc, len);
}

/// <summary>
/// Load song list names from INI.
/// </summary>
void CustomSongTitles::LoadSettings() {
	songTitles = Settings::GetCustomSongTitles();
}