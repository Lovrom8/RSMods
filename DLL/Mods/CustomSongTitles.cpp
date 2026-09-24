#include "../stdafx.h"
#include "CustomSongTitles.hpp"

#pragma warning(disable: 4302) // Typecast truncated from const _Elem* -> char (Leaving this because the user shouldn't have a song list name over 255).

std::vector<std::string> songTitles(20);

/// <summary>
/// Hook to grab songlists. EAX = char* str "$[36969]SONG LIST". ESI = SongList Index (One-Indexed)
/// </summary>
void __declspec(naked) hook_fakeTitles() {
	__asm {
		push eax
		push ecx

		lea ecx, Offsets::func_ecxAddress
		call VersioningStruct<uintptr_t>::GetValue

		pop ecx

		mov ecx, dword ptr ds : [eax] // Store the contents of the return value into the ECX register to replicate the original instruction.
		
		pop eax

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

		push offset Offsets::hookBackAddr_FakeTitles	// Return to the original code.
		jmp MemUtil::JumpToVersioned
	}
}

/// <summary>
/// Hijacks the format string for printf and discards the parameters and then returns our (kkomrade) versions of the names. Perfected by Koko.
/// </summary>
/// <param name="number"> - MainGame.csv string ID number [EAX]</param>
/// <param name="text"> - Song List Name [ESP + 0x10] **DISCARDED**</param>
/// <returns>New songlist name</returns>
const char* __stdcall missingLocalization(int localizationNumber, char*) {
	constexpr int SONG_LIST_START = 90000;
	constexpr int SONG_LIST_COUNT = 20;

	int index = localizationNumber - SONG_LIST_START;

	if (index >= 0 && index < SONG_LIST_COUNT && index < static_cast<int>(songTitles.size())) {
		return songTitles[index].c_str();
	}

	sprintf_s(&string_buffer[0], buffer_size, "%d", localizationNumber);
	return string_buffer;
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

		push offset Offsets::hookBackAddr_missingLocalization	// Jump back to the original instructions.
		jmp MemUtil::JumpToVersioned
	}
}

/// <summary>
/// Remove spaces and numbers. "SONG LIST 1" -> "SONGLIST"
/// </summary>
void CustomSongTitles::PatchSongListAppendages() {
	MemUtil::PatchAdr(Offsets::patch_addedSpaces, (UINT*)Offsets::patch_ListSpaces, 5);		// Remove the space added between "SONG LIST" and the index.
	MemUtil::PatchAdr(Offsets::patch_addedNumbers, (UINT*)Offsets::patch_ListNumbers, 5);	// Remove the index from the Song List name.
}

/// <summary>
/// Place Songlist Remove Hooks
/// </summary>
void CustomSongTitles::SetFakeListNames() {
	PatchSongListAppendages();

	len = 6;

	Offsets::hookBackAddr_FakeTitles.Get() = Offsets::hookAddr_ModifyLocalized + len;
	MemUtil::PlaceHook(Offsets::hookAddr_ModifyLocalized, hook_fakeTitles, len);
}

/// <summary>
/// Place Songlist Removal Hooks and New Songlist Name Hooks
/// </summary>
void CustomSongTitles::HookSongListsKoko() {
	SetFakeListNames();

	len = 5;
	Offsets::hookBackAddr_missingLocalization.Get() = Offsets::hookAddr_MissingLocalization + len;

	//Skip less printf parameters if those have been removed
	MemUtil::PatchAdr(Offsets::patch_sprintfArg, (BYTE*)Offsets::patch_SprintfArgs, 1);
	MemUtil::PlaceHook(Offsets::hookAddr_MissingLocalization, missingLocalizationHookFunc, len);
}

/// <summary>
/// Load song list names from INI.
/// </summary>
void CustomSongTitles::LoadSettings() {
	songTitles = Settings::GetCustomSongTitles();
}