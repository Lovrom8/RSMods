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

	template <std::size_t Size>
	SiteDescriptor<Size> MakeSite(const char* name, std::uintptr_t address, const VersionedPattern<Size>& expected,
		const std::array<std::uint8_t, Size>& replacement,
		typename SiteDescriptor<Size>::ReplacementBuilder buildReplacement = nullptr) {
		return Framework::InstructionPatch::MakeSite(name, address, ::GameVersion::Current(),
			expected, replacement, buildReplacement);
	}

	void LogOriginal(const char* name, std::uintptr_t address, const std::vector<std::uint8_t>& bytes) {
		std::ostringstream text;
		for (std::uint8_t byte : bytes)
			text << std::hex << static_cast<unsigned int>(byte) << ' ';
		LOG_INFO("(CUSTOM SONG TITLES) " << name << " original at 0x" << std::hex << address << ": " << text.str() << std::endl);
	}

	bool AppendSites(MemUtil::PreparedPatchSet& patches, const CaptureResult& captured) {
		return AppendCaptured(captured, [&](const CapturedSite& site) {
			MemUtil::PreparedPatchRequest request{};
			request.address = reinterpret_cast<LPVOID>(site.address);
			request.kind = MemUtil::PreparedPatchKind::Bytes;
			request.expectedBytes = site.original;
			request.replacementBytes = site.replacement;
			request.length = site.original.size();
			return patches.Append(request);
		});
	}

	void LogValidationFailure(const char* action, const CaptureResult& captured) {
		if (captured.failure != ValidationFailure::None)
			LOG_WARNING("(CUSTOM SONG TITLES) " << action << " left unchanged at " << captured.site
				<< ": " << FailureName(captured.failure) << std::endl);
	}

	void LogUnavailable(const char* action, const char* site) {
		LOG_WARNING("(CUSTOM SONG TITLES) " << action << " left unchanged at " << site << ": "
			<< FailureName(ValidationFailure::Unavailable) << std::endl);
	}

	void LogResult(const char* action, const CaptureResult& captured, MemUtil::PatchResult result,
		std::uintptr_t failureAddress) {
		const char* site = SiteNameAt(captured, failureAddress);
		if (result == MemUtil::PatchResult::Installed)
			LOG_INFO("(CUSTOM SONG TITLES) " << action << " installed" << std::endl);
		else if (result == MemUtil::PatchResult::RollbackIncomplete) {
			LOG_ERROR("(CUSTOM SONG TITLES) " << action << " outcome is indeterminate at "
				<< (site ? site : "patch group") << std::endl);
		}
		else {
			LOG_WARNING("(CUSTOM SONG TITLES) " << action << " left unchanged at "
				<< (site ? site : "patch group") << ": write was not applied" << std::endl);
		}
	}

	void InstallSites(const char* action, CaptureResult captured) {
		if (captured.failure != ValidationFailure::None) {
			LogValidationFailure(action, captured);
			return;
		}
		MemUtil::PreparedPatchSet patches;
		if (!AppendSites(patches, captured)) {
			LogResult(action, captured, MemUtil::PatchResult::Original, patches.FailureAddress());
			return;
		}
		for (const CapturedSite& site : captured.sites)
			LogOriginal(site.name, site.address, site.original);
		LogResult(action, captured, patches.Commit(), patches.FailureAddress());
	}

	void InstallTitleSites(bool includeMissingLocalization, const char* action) {
		CustomSongTitles::len = includeMissingLocalization ? 5 : 6;
		const auto spacesReplacement = MakeBytes(0x58, 0x58, 0x90, 0x90, 0x90);
		const auto numbersReplacement = MakeBytes(0x5A, 0x5A, 0x90, 0x90, 0x90);
		const auto spaces = MakeSite("added spaces call", Offsets::patch_addedSpaces.GetValue(), VersionedPattern<5>{
			ExactPattern(MakeBytes(0xE8, 0xD7, 0x64, 0xEB, 0xFF)), UnavailablePattern<5>()
		}, spacesReplacement);
		const auto numbers = MakeSite("added numbers call", Offsets::patch_addedNumbers.GetValue(), VersionedPattern<5>{
			ExactPattern(MakeBytes(0xE8, 0x96, 0x64, 0xEB, 0xFF)), UnavailablePattern<5>()
		}, numbersReplacement);
		const auto fakeTitlesAddress = Offsets::hookAddr_ModifyLocalized.GetValue();
		std::array<std::uint8_t, 6> fakeJump{};
		if (!BuildRelativeJump(fakeTitlesAddress, reinterpret_cast<std::uintptr_t>(&hook_fakeTitles), fakeJump)) {
			LogUnavailable(action, "localized-title hook");
			return;
		}
		const auto fakeTitles = MakeSite("localized-title hook", fakeTitlesAddress, VersionedPattern<6>{
			ExactPattern(MakeBytes(0x8B, 0x0D, 0xFC, 0xFB, 0x35, 0x01)), UnavailablePattern<6>()
		}, fakeJump);
		const std::uintptr_t sprintfImmediate = Offsets::patch_sprintfArg.GetValue();
		const auto sprintfArguments = MakeSite("localized-title argument count",
			sprintfImmediate ? sprintfImmediate - 2 : 0, VersionedPattern<3>{
				ExactPattern(MakeBytes(0x83, 0xC4, 0x0C)), UnavailablePattern<3>()
			}, MakeBytes(0x83, 0xC4, 0x04));
		const auto missingAddress = Offsets::hookAddr_MissingLocalization.GetValue();
		std::array<std::uint8_t, 5> missingJump{};
		if (includeMissingLocalization
			&& !BuildRelativeJump(missingAddress, reinterpret_cast<std::uintptr_t>(&missingLocalizationHookFunc), missingJump)) {
			LogUnavailable(action, "missing-localization hook");
			return;
		}
		const auto missingLocalization = MakeSite("missing-localization hook", missingAddress, VersionedPattern<5>{
			ExactPattern(MakeBytes(0x68, 0x58, 0x3B, 0x1D, 0x01)), UnavailablePattern<5>()
		}, missingJump);
		if (fakeTitlesAddress && ::GameVersion::Current() == Framework::GameVersion::Build::September2022)
			Offsets::hookBackAddr_FakeTitles.Get() = fakeTitlesAddress + 6;
		if (missingAddress && ::GameVersion::Current() == Framework::GameVersion::Build::September2022)
			Offsets::hookBackAddr_missingLocalization.Get() = missingAddress + 5;

		if (includeMissingLocalization)
			InstallSites(action, CaptureSites(readSite, spaces, numbers, fakeTitles, sprintfArguments, missingLocalization));
		else
			InstallSites(action, CaptureSites(readSite, spaces, numbers, fakeTitles));
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
	const auto spaces = MakeSite("added spaces call", Offsets::patch_addedSpaces.GetValue(), VersionedPattern<5>{
		ExactPattern(MakeBytes(0xE8, 0xD7, 0x64, 0xEB, 0xFF)), UnavailablePattern<5>()
	}, MakeBytes(0x58, 0x58, 0x90, 0x90, 0x90));
	const auto numbers = MakeSite("added numbers call", Offsets::patch_addedNumbers.GetValue(), VersionedPattern<5>{
		ExactPattern(MakeBytes(0xE8, 0x96, 0x64, 0xEB, 0xFF)), UnavailablePattern<5>()
	}, MakeBytes(0x5A, 0x5A, 0x90, 0x90, 0x90));
	InstallSites("Song-list appendage patches", CaptureSites(readSite, spaces, numbers));
}

/// <summary>
/// Place Songlist Remove Hooks
/// </summary>
void CustomSongTitles::SetFakeListNames() {
	InstallTitleSites(false, "Fake song-list names");
}

/// <summary>
/// Place Songlist Removal Hooks and New Songlist Name Hooks
/// </summary>
void CustomSongTitles::HookSongListsKoko() {
	InstallTitleSites(true, "Song-list localization hooks");
}

/// <summary>
/// Load song list names from INI.
/// </summary>
void CustomSongTitles::LoadSettings() {
	songTitles = Settings::GetCustomSongTitles();
}
