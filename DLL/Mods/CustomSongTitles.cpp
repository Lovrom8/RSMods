#include "CustomSongTitles.hpp"
#pragma warning(disable: 4302) // Typecast truncated from const _Elem* -> char (Leaving this because the user shouldn't have a song list name over 255).

std::vector<std::string> songTitles(20);

/// <summary>
/// Hook to grab songlists. EAX = char* str "$[36969]SONG LIST". ESI = SongList Index (One-Indexed)
/// </summary>
void __declspec(naked) hook_fakeTitles() {
	__asm {
		mov ecx, dword ptr ds : [0x135FBFC]		// Store the contents of 0x135FBFC into the ECX register. This is the line we are "replacing" to inject this hook.
		pushad									// Save EAX, ECX, and EDX to the stack. 
		mov ebx, 0x2F							// Sets EBX to 0x2F / 47 / "/" in ASCII. This means that when we add the song index, we start at 0.
		add ebx, esi							// Add Song List Index to EBX. One-Indexed, but we store it as 0 indexed.

		// Defaults for normal songlist configs.
		mov byte ptr[eax + 0x6], bl				// "$[36969]SONG LIST" -> "$[3696X]SONG LIST" where "X" = SongList Index. Ex: Song list 1 would result in "$[36960]SONG LIST".
		mov byte ptr[eax + 0x2], 0x39			// "$[3696X]SONG LIST" -> "$[9696X]SONG LIST". This allows us to throw the localized string into an unknown value, which we can later hijack to put our "localized" version in.
		mov byte ptr[eax + 0x3], 0x30			// "$[9696X]SONG LIST" -> "$[9096X]SONG LIST".
		mov byte ptr[eax + 0x4], 0x30			// "$[9096X]SONG LIST" -> "$[9006X]SONG LIST".
		mov byte ptr[eax + 0x5], 0x30			// "$[9006X]SONG LIST" -> "$[9000X]SONG LIST".

		// Configs for users with more than 9 song lists.
		cmp bl, 0x3A							// If it is below 0x3A, aka is a number in ASCII, jump to the exit.
		jl ExitHookFakeTitle

		sub bl, 0xA								// Reset the index to 0.
		mov byte ptr[eax + 0x6], bl				// Set the new index of "X" in "$[9000X]SONG LIST".
		mov byte ptr[eax + 0x5], 0x31			// "$[9000X]SONG LIST" -> "$[9001X]SONG LIST".

	ExitHookFakeTitle:
		popad									// Return EAX, ECX, and EDX from the stack.
		jmp[Offsets::hookBackAddr_FakeTitles]	// Return to the original code.
	}
}

/// <summary>
/// Hijacks the format string for printf and discards the parameters and then returns our (kkomrade) versions of the names. Perfected by Koko.
/// </summary>
/// <param name="number"> - MainGame.csv string ID number [EAX]</param>
/// <param name="text"> - Song List Name [ESP + 0x10] **DISCARDED**</param>
/// <returns>New songlist name</returns>
const char* __stdcall missingLocalization(int localizationNumber, char* text) {
	text = "";
	sprintf_s(&string_buffer[0], buffer_size, "%d", localizationNumber);

	switch (localizationNumber)
	{
		case 90000:	// Song list 1
			return songTitles[0].c_str();
		case 90001: // Song list 2
			return songTitles[1].c_str();
		case 90002: // Song list 3
			return songTitles[2].c_str();
		case 90003: // Song list 4
			return songTitles[3].c_str();
		case 90004: // Song list 5
			return songTitles[4].c_str();
		case 90005: // Song list 6
			return songTitles[5].c_str();
		case 90006: // Song list 7
			return songTitles[6].c_str();
		case 90007: // Song list 8
			return songTitles[7].c_str();
		case 90008: // Song list 9
			return songTitles[8].c_str();
		case 90009: // Song list 10
			return songTitles[9].c_str();
		case 90010: // Song list 11
			return songTitles[10].c_str();
		case 90011: // Song list 12
			return songTitles[11].c_str();
		case 90012: // Song list 13
			return songTitles[12].c_str();
		case 90013: // Song list 14
			return songTitles[13].c_str();
		case 90014: // Song list 15
			return songTitles[14].c_str();
		case 90015: // Song list 16
			return songTitles[15].c_str();
		case 90016: // Song list 17
			return songTitles[16].c_str();
		case 90017: // Song list 18
			return songTitles[17].c_str();
		case 90018: // Song list 19
			return songTitles[18].c_str();
		case 90019: // Song list 20
			return songTitles[19].c_str();
		default:
			return string_buffer;
	}
}

/// <summary>
/// Hook to place new songlist names
/// </summary>
void __declspec(naked) missingLocalizationHookFunc() {
	__asm {
		push ecx // Push ECX to the stack. This is the line we are "replacing" to inject this hook.
		push edx // Push EDX to the stack. This is the line we are "replacing" to inject this hook.
		push esp // Push ESP to the stack. This is the line we are "replacing" to inject this hook.

		push[esp + 0x10]			// ESP+10 = Song List Name.
		push eax					// EAX = Localization Number. 46964 = Song list 1, 46965 = Song list 2, 46966, 46967, 46968, 46969.
		call missingLocalization	// Call our "localization" function that will replace the Song List name with the user's custom Song List name.

		pop esp // Return ESP from the stack.
		pop edx // Return EDX from the stack.
		pop ecx // Return ECX from the stack.
		
		add esp, 0x8									// Replace original instruction we were replacing
		push eax										// Replace original instruction we were replacing
		jmp[Offsets::hookBackAddr_missingLocalization]	// Jump back to the original instructions.
	}
}

/// <summary>
/// Remove spaces and numbers. "SONG LIST 1" -> "SONGLIST"
/// </summary>
void CustomSongTitles::PatchSongListAppendages() {
	MemUtil::PatchAdr((BYTE*)Offsets::patch_addedSpaces, (UINT*)Offsets::patch_ListSpaces, 5);		// Remove the space added between "SONG LIST" and the index.
	MemUtil::PatchAdr((BYTE*)Offsets::patch_addedNumbers, (UINT*)Offsets::patch_ListNumbers, 5);	// Remove the index from the Song List name.
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