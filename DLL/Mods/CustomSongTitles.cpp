#include "../stdafx.h"
#include "CustomSongTitles.hpp"
#include "../Framework/InstructionPatch.hpp"
#include "../VersioningStruct.h"

#pragma warning(disable: 4302) // Typecast truncated from const _Elem* -> char (Leaving this because the user shouldn't have a song list name over 255).

void hook_fakeTitles();
void missingLocalizationHookFunc();

namespace {
	using namespace Framework::InstructionPatch;

	const auto readSite = [](std::uintptr_t address, auto& bytes) {
		return MemUtil::TryRead(address, bytes);
	};
	const auto writeSite = [](std::uintptr_t address, const auto& bytes) {
		return FromWriteResult(MemUtil::WritePatch(reinterpret_cast<LPVOID>(address), bytes.data(), bytes.size()),
			MemUtil::PatchResult::Installed, MemUtil::PatchResult::Original, MemUtil::PatchResult::RollbackIncomplete);
	};

	template <std::size_t Size>
	PatchedSite<Size> MakeSite(const char* name, std::uintptr_t address, const VersionedPattern<Size>& patterns,
		std::size_t writeOffset = 0, std::size_t writeSize = Size) {
		return { name, address, GameVersion::Current(), patterns, writeOffset, writeSize };
	}

	template <std::size_t Size>
	void LogOriginal(const char* name, std::uintptr_t address, const std::array<std::uint8_t, Size>& bytes) {
		std::ostringstream text;
		for (std::uint8_t byte : bytes)
			text << std::hex << static_cast<unsigned int>(byte) << ' ';
		LOG_INFO("(CUSTOM SONG TITLES) " << name << " original at 0x" << std::hex << address << ": " << text.str() << std::endl);
	}

	const auto logOriginal = [](const char* name, std::uintptr_t address, const auto& bytes) {
		LogOriginal(name, address, bytes);
	};

	void LogResult(const char* action, const Result& result) {
		if (result.outcome == Outcome::Installed) {
			if (result.changed)
				LOG_INFO("(CUSTOM SONG TITLES) " << action << " installed" << std::endl);
		}
		else if (result.outcome == Outcome::RollbackIncomplete) {
			LOG_ERROR("(CUSTOM SONG TITLES) " << action << " outcome is indeterminate at "
				<< (result.site ? result.site : "patch group") << std::endl);
		}
		else if (result.failure != Failure::None) {
			LOG_WARNING("(CUSTOM SONG TITLES) " << action << " left unchanged at "
				<< (result.site ? result.site : "patch group") << ": " << FailureName(result.failure) << std::endl);
		}
	}

	PatchedSite<5>& AddedSpacesSite() {
		static auto site = MakeSite("added spaces call", Offsets::patch_addedSpaces.GetValue(), VersionedPattern<5>{
			MaskedPattern(MakeBytes(0xE8, 0xD7, 0x64, 0xEB, 0xFF), MakeBytes(0xFF, 0x00, 0x00, 0x00, 0x00)),
			UnavailablePattern<5>()
		});
		return site;
	}

	PatchedSite<5>& AddedNumbersSite() {
		static auto site = MakeSite("added numbers call", Offsets::patch_addedNumbers.GetValue(), VersionedPattern<5>{
			MaskedPattern(MakeBytes(0xE8, 0x96, 0x64, 0xEB, 0xFF), MakeBytes(0xFF, 0x00, 0x00, 0x00, 0x00)),
			UnavailablePattern<5>()
		});
		return site;
	}

	PatchedSite<6>& FakeTitlesSite() {
		static auto site = MakeSite("localized-title hook", Offsets::hookAddr_ModifyLocalized.GetValue(), VersionedPattern<6>{
			MaskedPattern(MakeBytes(0x8B, 0x0D, 0xFC, 0xFB, 0x35, 0x01), MakeBytes(0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00)),
			UnavailablePattern<6>()
		});
		return site;
	}

	PatchedSite<3>& SprintfArgumentsSite() {
		const std::uintptr_t immediate = Offsets::patch_sprintfArg.GetValue();
		static auto site = MakeSite("localized-title argument count", immediate ? immediate - 2 : 0, VersionedPattern<3>{
			ExactPattern(MakeBytes(0x83, 0xC4, 0x0C)),
			UnavailablePattern<3>()
		}, 2, 1);
		return site;
	}

	PatchedSite<5>& MissingLocalizationSite() {
		static auto site = MakeSite("missing-localization hook", Offsets::hookAddr_MissingLocalization.GetValue(), VersionedPattern<5>{
			MaskedPattern(MakeBytes(0x68, 0x58, 0x3B, 0x1D, 0x01), MakeBytes(0xFF, 0x00, 0x00, 0x00, 0x00)),
			UnavailablePattern<5>()
		});
		return site;
	}

	Result InstallTitleSites(bool includeMissingLocalization) {
		auto& spaces = AddedSpacesSite();
		auto& numbers = AddedNumbersSite();
		auto& fakeTitles = FakeTitlesSite();
		auto& sprintfArguments = SprintfArgumentsSite();
		auto& missingLocalization = MissingLocalizationSite();
		CustomSongTitles::len = includeMissingLocalization ? 5 : 6;
		if (fakeTitles.Address())
			Offsets::hookBackAddr_FakeTitles.Get() = fakeTitles.Address() + 6;
		if (missingLocalization.Address())
			Offsets::hookBackAddr_missingLocalization.Get() = missingLocalization.Address() + 5;
		spaces.SetReplacement(MakeBytes(0x58, 0x58, 0x90, 0x90, 0x90));
		numbers.SetReplacement(MakeBytes(0x5A, 0x5A, 0x90, 0x90, 0x90));
		sprintfArguments.SetReplacement(MakeBytes(0x83, 0xC4, 0x04));

		std::array<std::uint8_t, 6> fakeJump{};
		if (!BuildRelativeJump(fakeTitles.Address(), reinterpret_cast<std::uintptr_t>(&hook_fakeTitles), fakeJump))
			return { Outcome::Original, Failure::Unavailable, fakeTitles.Name() };
		fakeTitles.SetReplacement(fakeJump);

		std::array<std::uint8_t, 5> missingJump{};
		if (!BuildRelativeJump(missingLocalization.Address(), reinterpret_cast<std::uintptr_t>(&missingLocalizationHookFunc), missingJump))
			return { Outcome::Original, Failure::Unavailable, missingLocalization.Name() };
		missingLocalization.SetReplacement(missingJump);

		const auto read = readSite;
		const auto write = writeSite;
		if (includeMissingLocalization)
			return InstallGroup(read, write, logOriginal, spaces, numbers, fakeTitles, sprintfArguments, missingLocalization);
		return InstallGroup(read, write, logOriginal, spaces, numbers, fakeTitles);
	}
}

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
	auto& spaces = AddedSpacesSite();
	auto& numbers = AddedNumbersSite();
	spaces.SetReplacement(MakeBytes(0x58, 0x58, 0x90, 0x90, 0x90));
	numbers.SetReplacement(MakeBytes(0x5A, 0x5A, 0x90, 0x90, 0x90));
	LogResult("Song-list appendage patches", InstallGroup(readSite, writeSite, logOriginal, spaces, numbers));
}

/// <summary>
/// Place Songlist Remove Hooks
/// </summary>
void CustomSongTitles::SetFakeListNames() {
	LogResult("Fake song-list names", InstallTitleSites(false));
}

/// <summary>
/// Place Songlist Removal Hooks and New Songlist Name Hooks
/// </summary>
void CustomSongTitles::HookSongListsKoko() {
	LogResult("Song-list localization hooks", InstallTitleSites(true));
}

/// <summary>
/// Load song list names from INI.
/// </summary>
void CustomSongTitles::LoadSettings() {
	songTitles = Settings::GetCustomSongTitles();
}
