#include "../stdafx.h"
#include "BugPrevention.hpp"
#include "../MemUtil.hpp"
#include <unordered_map>

namespace BugPrevention {

	/// <summary>
	/// When Rocksmith opens with a Oculus / Meta headset connected to the user computer, it can cause a crash.
	/// This is due to Rocksmith saying it owns memory that it doesn't have access to.
	/// In this fix, we jump over the interior of the for-loop (marked as a while with a break case) that writes to invalid memory.
	/// </summary>
	void PreventOculusCrash() {
		MemUtil::PatchAdr(Offsets::ptr_OculusCrashJmp, "\xE9\x19\x02\x00\x00\x90", 6);

		LOG_INFO("(BUG PREVENTION) Prevented Oculus Crash" << std::endl);
	}

	/// <summary>
	/// When the user enters a song with a buggy tone, every tone after it will not work.
	/// This mod prevents that by changing a conditional jump to a jump that always happens.
	/// So when the user encounters a broken tone, all they need to do is change the tone and the tones should start working again.
	/// </summary>
	void PreventStuckTone() {
		MemUtil::PatchAdr(Offsets::ptr_StuckToneJmp, "\xEB", 1);

		LOG_INFO("(BUG PREVENTION) Prevented Tone Bug" << std::endl);
	}

	/// <summary>
	/// When a user has a faulty PnP (Plug-n-Play) device connected Rocksmith can crash.
	/// It crashes with a memory access violation. The following code skips over the while loop that may eventually crash.
	/// </summary>
	void PreventPnPCrash() {
		MemUtil::PatchAdr(Offsets::ptr_PnpJmp_1, "\xE9\x19\x02\x00\x00\x90", 6);
		MemUtil::PatchAdr(Offsets::ptr_PnpJmp_2, "\x90\x90\x90\x90\x90\x90", 6);

		LOG_INFO("(BUG PREVENTION) Prevented PnP Crash" << std::endl);
	}

	/// <summary>
	/// Ubisoft lets you put almost any character in your Uplay password.
	/// However, Rocksmith does not allow some characters.
	/// This means that the user will have to change their password if they are using an invalid character, and they want to use leaderboards.
	/// Some of those invalid characters are as follows: " \ / and =
	/// This mod prevents the checks for those characters to allow the user to have more complex passwords.
	/// </summary>
	void AllowComplexPasswords() {
		MemUtil::PatchAdr(Offsets::ptr_Password_LimitCharacters, "\x90\x90", 2);
		MemUtil::PatchAdr(Offsets::ptr_Password_LimitCharacters_Clipboard, "\x90\x90", 2);

		LOG_INFO("(BUG PREVENTION) Allowed Complex Uplay Passwords" << std::endl);
	}

	/// <summary>
	/// Skip profile save file Steam Account Id check to allow sharing profiles without needing external tooling.
	/// </summary>
	void BypassSaveFilePlatformIdCheck() {
		constexpr byte alwaysContinue = 0xEB;
		MemUtil::PatchAdr(Offsets::ptr_SaveFilePlatformIdCheck, &alwaysContinue, sizeof(alwaysContinue));
		LOG_INFO("(BUG PREVENTION) Allowed cross-platform-ID PRFLDB and JSON reads" << std::endl);
	}

	void __declspec(naked) advancedDisplayCrashHook()
	{
		__asm {

			cmp ECX, 0 // ECX == NULL?

			je prevAdvancedDisplayCrash // If ECX == NULL, then we need to jump to prevAdvancedDisplayCrash

			mov DL, BYTE PTR DS : [ECX + 0x4]	// The code we are overwriting to place this hook
			push EDI						// The code we are overwriting to place this hook
			MOV EDI, DWORD PTR DS : [ESI + 0xC] // The code we are overwriting to place this hook

			push offset Offsets::ptr_AdvancedDisplayCrashJmpBck
			jmp MemUtil::JumpToVersioned

			prevAdvancedDisplayCrash :
			ret							// ECX is NULL, so we need to leave this function or we will crash.
		}
	}

	/// <summary>
	/// When a user enters their Advanced Display settings, there is a tendency for Rocksmith 2014 to crash.
	/// This mod tries to prevent that by exiting the function if ECX (the memory address it is reading from) is NULL.
	/// </summary>
	void PreventAdvancedDisplayCrash() {
		MemUtil::PlaceHook(Offsets::ptr_AdvancedDisplayCrash, advancedDisplayCrashHook, 7);

		FlushInstructionCache(GetCurrentProcess(), (void*)Offsets::ptr_AdvancedDisplayCrash.Get(), 7);

		LOG_INFO("(BUG PREVENTION) Prevented Advanced Display Crash" << std::endl);
	}


	/// <summary>
	/// In extremely rare cases, the user may have an audio in device have a driver issue.
	/// This causes Rocksmith to crash when it reads all of their audio input devices.
	/// This just patches out those checks, so it won't crash when EBX is a nullptr.
	/// </summary>
	void PreventPortAudioInDeviceCrash() {
		// Overwrite some code that doesn't do null checks with NOP.
		// Be very careful in this code. If you overwrite the next instruction, then you end up breaking audio input.
		// NB: JZ has been replaced by JL, now it's 10 bytes in total
		MemUtil::PatchAdr(Offsets::ptr_PortAudioInCrash, "\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90", 10);

		LOG_INFO("(BUG PREVENTION) Prevented Port Audio In Device Crash" << std::endl);
	}

	/// <summary>
	/// Prevention for crash caused by certain audio devices (like Voicemeter virtual cables).
	/// </summary>
	void PreventExtraAudioDevicesCrash() {
		MemUtil::PatchAdr(Offsets::ptr_AdditionalAudioDevicesCrash, "\x90\x90\x90\x90\x90\x31\xC9\x31\xDB", 9);

		LOG_INFO("(BUG PREVENTION) Prevented Additional Audio Devices Crash" << std::endl);
	}

	void __declspec(naked) controllerAxisBoundsHook() {
		__asm {
			pushfd
			pushad
			mov eax, dword ptr [esp + 40] // First argument.
			cmp dword ptr [eax + 8], -1
			jne validControllerAxisRecord
			cmp dword ptr [eax + 4], 4
			jb validControllerAxisRecord // Unsigned comparison also rejects negative indices.
			popad
			popfd
			ret 4

		validControllerAxisRecord:
			popad
			popfd
			push ebp
			mov ebp, esp
			sub esp, 8 // Original six-byte prologue
			push offset Offsets::ptr_ControllerAxisBoundsJmpBck
			jmp MemUtil::JumpToVersioned
		}
	}

	/// <summary>
	/// Ignore controller-axis updates with indices outside the game's supported range.
	/// Button input remains available.
	/// </summary>
	void PreventControllerAxisOverflow() {
		if (!MemUtil::PlaceHook(Offsets::ptr_ControllerAxisBounds, controllerAxisBoundsHook, 6)) {
			LOG_ERROR("(BUG PREVENTION) Failed controller input crash guard" << std::endl);
			return;
		}
		FlushInstructionCache(GetCurrentProcess(), (void*)Offsets::ptr_ControllerAxisBounds.Get(), 6);
		LOG_INFO("(BUG PREVENTION) Prevented controller input crash" << std::endl);
	}

	void __stdcall LogInvalidUIInputCrash() {
		LOG_WARNING("(BUG PREVENTION) Prevented crash from invalid UI input" << std::endl);
	}

	void __declspec(naked) invalidInputTreeRootHook() {
		__asm {
			test ebx, ebx
			jz emptyInputTree
			js invalidInputTree
			cmp ebx, 0x10000
			jb invalidInputTree
			test ebx, ebx // Preserve the original TEST flags on the normal lookup path.
			push offset Offsets::ptr_InvalidInputTreeRootJmpBck
			jmp MemUtil::JumpToVersioned

		invalidInputTree:
			pushfd
			pushad
			call LogInvalidUIInputCrash
			popad
			popfd
		emptyInputTree:
			push offset Offsets::ptr_InvalidInputTreeRootEmptyJmpBck
			jmp MemUtil::JumpToVersioned
		}
	}

	/// <summary>
	/// Sign-in UI can access an invalid input root while looking up an action.
	/// Treat negative or low addresses like empty input to avoid a crash.
	/// Original report: https://discord.com/channels/238233332511539200/305406306821472257/1552076343632728065
	/// </summary>
	void PreventInvalidInputTreeRootCrash() {
		constexpr int hookLength = 8; // Replaces the original input check.
		if (!MemUtil::PlaceHook(Offsets::ptr_InvalidInputTreeRootCheck, invalidInputTreeRootHook, hookLength)) {
			LOG_ERROR("(BUG PREVENTION) Failed UI input crash guard" << std::endl);
			return;
		}

		FlushInstructionCache(GetCurrentProcess(), (void*)Offsets::ptr_InvalidInputTreeRootCheck.Get(), hookLength);
		LOG_INFO("(BUG PREVENTION) Installed UI input crash guard" << std::endl);
	}

	/// <summary>
	/// Clamps the sample count to the buffer's real capacity before it is stored.
	/// </summary>
	void __declspec(naked) calibrationSampleCountClampHook() {
		__asm {
			mov edx, dword ptr [esp + 0x10]		// The code we are overwriting to place this hook
			cmp edx, 100						// Capacity of the ring buffer, per player
			jbe keepCalibrationSampleCount
			mov edx, 100
		keepCalibrationSampleCount:
			mov dword ptr [ebx + 0x788], edx	// The code we are overwriting to place this hook

			push offset Offsets::ptr_calibrationSampleCountClampJmpBck
			jmp MemUtil::JumpToVersioned
		}
	}

	/// <summary>
	/// The input calibration screen sizes its volume averaging buffer to the current framerate (1.0 / delta time),
	/// but the buffer is a fixed 100 floats per player. Above ~100 FPS the sampler writes past the end of it, and
	/// the mean is then taken over more floats than the array holds, reading neighbouring members as if they were
	/// samples. The mean never settles in the acceptance window, so the meter never fills and calibration cannot
	/// be completed - which is why players on high refresh rate displays have to cap their framerate first.
	/// Clamping that count to the real capacity fixes both the writes and the reads. Nothing at or below 100 FPS
	/// changes; above it the averaging window just covers less time.
	/// </summary>
	void FixCalibrationSampleCount() {
		MemUtil::PlaceHook(Offsets::ptr_calibrationSampleCountClamp, calibrationSampleCountClampHook, 10);

		FlushInstructionCache(GetCurrentProcess(), (void*)Offsets::ptr_calibrationSampleCountClamp.Get(), 10);

		LOG_INFO("(BUG PREVENTION) Fixed Calibration At High Framerates" << std::endl);
	}

	/// <summary>
	/// Rocksmith turns an index into a JSON key ("0", "1", ...) by reading a static table of 101 strings, "0" to "100".
	/// Nothing checks the index against that size. The song options menu, manifest loading, mastery and phrase
	/// accuracy saving all index it by phrase iteration, so a song with more than 101 phrase iterations reads past the end.
	/// From 101 to 458 it reads unrelated strings, so lookups quietly use the wrong key. At 459 it reads an integer
	/// and crashes in strchr as soon as the song is picked in Learn a Song (song options menu).
	/// This builds a larger table (the original 101 pointers followed by our own strings) and points every
	/// instruction that reads the old table at the new one.
	/// </summary>
	bool ExtendIndexKeyStringTable() {
		constexpr size_t originalEntries = 101;
		constexpr size_t extendedEntries = 10000;
		static std::vector<std::string> extraKeys;
		static std::vector<const char*> extendedTable;

		const uintptr_t oldTable = Offsets::ptr_indexKeyStringTable.Get();
		const char* const* oldEntries = reinterpret_cast<const char* const*>(oldTable);

		// Make sure the address really is the table, so we never patch the wrong build.
		for (size_t i = 0; i < originalEntries; i++) {
			if (!oldEntries[i] || std::to_string(i) != oldEntries[i]) {
				LOG_ERROR("(BUG PREVENTION) Index key table not found" << std::endl);
				return false;
			}
		}

		// Find every instruction in .text that has the table address as its displacement / immediate.
		BYTE* textStart = reinterpret_cast<BYTE*>(Offsets::baseHandle) + 0x1000;
		const size_t textLength = MemUtil::GetTextSectionLength();
		std::vector<BYTE*> references;
		for (size_t i = 3; i + sizeof(uintptr_t) <= textLength; i++) {
			BYTE* hit = textStart + i;
			if (*reinterpret_cast<uintptr_t*>(hit) != oldTable)
				continue;

			const bool isIndexedMov = hit[-3] == 0x8B && (hit[-2] & 0xC7) == 0x04 && (hit[-1] & 0x07) == 0x05;	// mov r32, [index*scale + table]
			const bool isOffsetMov = hit[-2] == 0x8B && (hit[-1] & 0xC0) == 0x80 && (hit[-1] & 0x07) != 0x04;		// mov r32, [reg + table]
			const bool isTableAddress = hit[-1] == 0xA1 || hit[-1] == 0xBE;										// mov eax, [table] / mov esi, table
			if (isIndexedMov || isOffsetMov || isTableAddress)
				references.push_back(hit);
		}

		constexpr size_t expectedReferences = 82; // Same count on both builds
		if (references.size() != expectedReferences) {
			LOG_ERROR("(BUG PREVENTION) Expected " << expectedReferences << " index key table references, found " << references.size() << std::endl);
			return false;
		}

		// Keep the game's own pointers for the first 101, so nothing that compares them changes.
		extendedTable.assign(oldEntries, oldEntries + originalEntries);
		extraKeys.reserve(extendedEntries - originalEntries);
		for (size_t i = originalEntries; i < extendedEntries; i++)
			extraKeys.push_back(std::to_string(i));
		for (const std::string& key : extraKeys)
			extendedTable.push_back(key.c_str());

		const uintptr_t newTable = reinterpret_cast<uintptr_t>(extendedTable.data());
		size_t moved = 0;
		for (BYTE* reference : references)
			moved += MemUtil::PatchAdr(reference, &newTable, sizeof(newTable)) ? 1 : 0;

		if (moved != references.size()) {
			LOG_ERROR("(BUG PREVENTION) Only moved " << moved << " of " << references.size() << " index key table references" << std::endl);
			return false;
		}

		LOG_INFO("(BUG PREVENTION) Fixed index key table overflow (" << moved << " references moved)" << std::endl);
		return true;
	}

	/// <summary>
	/// Is this JSON key something strchr can read? Out of range index keys can be small integers or random values.
	/// </summary>
	bool __stdcall IsReadableJsonKey(const char* key) {
		if (reinterpret_cast<uintptr_t>(key) < 0x10000)
			return false;

		__try {
			volatile char firstCharacter = *key;
			(void)firstCharacter;
			return true;
		}
		__except (EXCEPTION_EXECUTE_HANDLER) {
			return false;
		}
	}

	void __stdcall LogInvalidJsonKey() {
		static int timesLogged = 0;
		if (timesLogged++ < 5)
			LOG_WARNING("(BUG PREVENTION) Skipped JSON lookup with an invalid key (song has too many phrase iterations?)" << std::endl);
	}

	void __declspec(naked) jsonFindByPathHook() {
		__asm {
			push ecx							// this
			push dword ptr [esp + 8]			// key
			call IsReadableJsonKey
			pop ecx
			test al, al
			jz invalidKey

			push ebp							// The code we are overwriting to place this hook
			mov ebp, esp						// The code we are overwriting to place this hook
			sub esp, 0x14						// The code we are overwriting to place this hook
			push offset Offsets::ptr_jsonFindByPathJmpBck
			jmp MemUtil::JumpToVersioned

		invalidKey:
			call LogInvalidJsonKey
			xor eax, eax						// Not found
			ret 4
		}
	}

	void __declspec(naked) jsonResolveByPathHook() {
		__asm {
			push ecx							// this
			push dword ptr [esp + 8]			// key
			call IsReadableJsonKey
			pop ecx
			test al, al
			jz invalidKey

			push ebp							// The code we are overwriting to place this hook
			mov ebp, esp						// The code we are overwriting to place this hook
			sub esp, 0x14						// The code we are overwriting to place this hook
			push offset Offsets::ptr_jsonResolveByPathJmpBck
			jmp MemUtil::JumpToVersioned

		invalidKey:
			call LogInvalidJsonKey
			xor eax, eax						// Not found
			ret 4
		}
	}

	/// <summary>
	/// Fallback for when the index key table can't be moved (unknown build, code changed, patch failed).
	/// Guards the two JSON path lookups the out of range keys end up in, so an unreadable key counts as "not found"
	/// instead of crashing in strchr. Songs past 101 phrase iterations still get wrong lookups, but won't crash here.
	/// </summary>
	void GuardJsonPathLookups() {
		constexpr int hookLength = 6; // push ebp / mov ebp, esp / sub esp, 0x14
		const bool findHooked = MemUtil::PlaceHook(Offsets::ptr_jsonFindByPath, jsonFindByPathHook, hookLength);
		const bool resolveHooked = MemUtil::PlaceHook(Offsets::ptr_jsonResolveByPath, jsonResolveByPathHook, hookLength);
		FlushInstructionCache(GetCurrentProcess(), (void*)Offsets::ptr_jsonFindByPath.Get(), hookLength);
		FlushInstructionCache(GetCurrentProcess(), (void*)Offsets::ptr_jsonResolveByPath.Get(), hookLength);

		if (findHooked && resolveHooked)
			LOG_WARNING("(BUG PREVENTION) Index key table not moved, using JSON key guard instead" << std::endl);
		else
			LOG_ERROR("(BUG PREVENTION) Failed index key table fix and JSON key guard" << std::endl);
	}

	/// <summary>
	/// Rocksmith turns an index into a JSON key ("0", "1", ...) with a static table that only goes up to "100".
	/// Move that table somewhere bigger. If we can't, guard the JSON lookups so bad keys don't crash the game.
	/// </summary>
	void FixIndexKeyTableOverflow() {
		if (!ExtendIndexKeyStringTable())
			GuardJsonPathLookups();
	}

	/// <summary>
	/// The lyrics renderer keeps its total glyph count in 16 bits (lyrics renderer + 0x9C), then sizes the lyrics mesh
	/// from it (4 vertices and 6 indices per glyph). A song with more than 65,535 lyric glyphs wraps the count, gets a
	/// far too small mesh, and crashes writing past it when the song starts.
	/// The next field starts at + 0xA0, so the count has room to be 32 bits. The mesh already uses 32 bit indices.
	/// This turns every 16 bit access of the count into the 32 bit one. Each is the same instruction minus the 0x66
	/// prefix (or MOVZX to MOV), with a NOP to keep the length. Every register written to it already has its top half
	/// zeroed or holds the full count, and every read is only used as a 32 bit number.
	/// </summary>
	void FixLyricsGlyphCountOverflow() {
		MemUtil::PatchAdr(Offsets::ptr_lyricsGlyphCountConstructor, "\x89\x90\x9C\x00\x00\x00\x90", 7);		// mov [eax+9C], edx
		MemUtil::PatchAdr(Offsets::ptr_lyricsGlyphCountReset, "\x89\x87\x9C\x00\x00\x00\x90", 7);			// mov [edi+9C], eax
		MemUtil::PatchAdr(Offsets::ptr_lyricsGlyphCountAdd, "\x01\x90\x9C\x00\x00\x00\x90", 7);			// add [eax+9C], edx
		MemUtil::PatchAdr(Offsets::ptr_lyricsGlyphCountPositions, "\x8B\x8B\x9C\x00\x00\x00\x90", 7);		// mov ecx, [ebx+9C]
		MemUtil::PatchAdr(Offsets::ptr_lyricsGlyphCountTexCoords0, "\x8B\x83\x9C\x00\x00\x00\x90", 7);	// mov eax, [ebx+9C]
		MemUtil::PatchAdr(Offsets::ptr_lyricsGlyphCountTexCoords1, "\x8B\x83\x9C\x00\x00\x00\x90", 7);	// mov eax, [ebx+9C]
		MemUtil::PatchAdr(Offsets::ptr_lyricsGlyphCountIndices, "\x8B\x83\x9C\x00\x00\x00\x90", 7);		// mov eax, [ebx+9C]
		MemUtil::PatchAdr(Offsets::ptr_lyricsGlyphCountIndexLoopStart, "\x3B\x8B\x9C\x00\x00\x00\x90", 7);	// cmp ecx, [ebx+9C]
		MemUtil::PatchAdr(Offsets::ptr_lyricsGlyphCountIndexLoop, "\x8B\xBB\x9C\x00\x00\x00\x90", 7);		// mov edi, [ebx+9C]

		LOG_INFO("(BUG PREVENTION) Fixed lyrics glyph count overflow" << std::endl);
	}

	// Per phrase iteration difficulty caches. The dynamic difficulty component (+ 0x38) and the score attack difficulty
	// component (+ 0x18) each keep a fixed array of 256 ints, but the game fills one per phrase iteration with no limit. A song with more than
	// 256 phrase iterations writes over the objects after it on the heap and crashes once the song starts.
	constexpr int phraseDifficultyCapacity = 256;
	constexpr int ddPhraseDifficulties = 0x38;
	constexpr int saPhraseDifficulties = 0x18;

	/// Every phrase iteration's difficulty, for components whose song has more than fit in the object.
	std::unordered_map<uintptr_t, std::vector<int>> phraseDifficulties;

	/// 0x0043C660: the difficulty of one phrase iteration. Component in EAX, phrase iteration in ECX.
	int ComputePhraseDifficulty(uintptr_t component, int phraseIteration) {
		const uintptr_t function = Offsets::ptr_phraseDifficultyCompute.Get();
		int result;
		__asm {
			mov eax, component
			mov ecx, phraseIteration
			call function
			mov result, eax
		}
		return result;
	}

	/// Fills the cache, keeping the first 256 in the object like before and all of them on the side.
	void __stdcall FillPhraseDifficulties(uintptr_t component, int arrayOffset) {
		if (!*reinterpret_cast<uint8_t*>(component + 0x14) || !*reinterpret_cast<uint8_t*>(component + 0x15))
			return;
		const uintptr_t song = *reinterpret_cast<uintptr_t*>(*reinterpret_cast<uintptr_t*>(component + 0xC) + 0x78);
		if (!song)
			return;
		const int count = static_cast<int>(*reinterpret_cast<uintptr_t*>(song + 0x68) - *reinterpret_cast<uintptr_t*>(song + 0x64)) / 0x18;

		std::vector<int>& all = phraseDifficulties[component];
		all.assign(count > 0 ? count : 0, -1);
		for (int i = 0; i < count; i++) {
			const int difficulty = ComputePhraseDifficulty(component, i);
			all[i] = difficulty;
			if (i < phraseDifficultyCapacity)
				*reinterpret_cast<int*>(component + arrayOffset + i * 4) = difficulty;
		}
	}

	/// The cached difficulty of a phrase iteration, or -1 when there isn't one.
	int CachedPhraseDifficulty(uintptr_t component, int arrayOffset, int phraseIteration) {
		if (static_cast<uint32_t>(phraseIteration) < phraseDifficultyCapacity)
			return *reinterpret_cast<int*>(component + arrayOffset + phraseIteration * 4);
		const auto found = phraseDifficulties.find(component);
		if (phraseIteration < 0 || found == phraseDifficulties.end() || static_cast<size_t>(phraseIteration) >= found->second.size())
			return -1;
		return found->second[phraseIteration];
	}

	void __stdcall FillDDPhraseDifficulties(uintptr_t component) {
		FillPhraseDifficulties(component, ddPhraseDifficulties);
	}

	void __stdcall FillSAPhraseDifficulties(uintptr_t component) {
		FillPhraseDifficulties(component, saPhraseDifficulties);
	}

	/// Dynamic difficulty: one phrase iteration's cached difficulty.
	int __stdcall GetDDPhraseDifficulty(uintptr_t component, int phraseIteration) {
		if (!*reinterpret_cast<uint8_t*>(component + 0x14) || phraseIteration == -1)
			return 0;
		const int difficulty = CachedPhraseDifficulty(component, ddPhraseDifficulties, phraseIteration);
		return difficulty != -1 ? difficulty : 0;
	}

	/// Dynamic difficulty: one phrase iteration's cached difficulty, if there is one.
	int __stdcall ResolveDDPhraseDifficulty(uintptr_t component, int phraseIteration, int* difficulty) {
		if (!*reinterpret_cast<uint8_t*>(component + 0x14) || phraseIteration == -1 || !*reinterpret_cast<uint8_t*>(component + 0x15))
			return 0;
		const int cached = CachedPhraseDifficulty(component, ddPhraseDifficulties, phraseIteration);
		if (cached == -1)
			return 0;
		*difficulty = cached;
		return 1;
	}

	/// Score attack: one phrase iteration's cached difficulty.
	int __stdcall GetSAPhraseDifficulty(uintptr_t component, int phraseIteration) {
		if (!*reinterpret_cast<uint8_t*>(component + 0x14) || phraseIteration == -1)
			return 0;
		const int difficulty = CachedPhraseDifficulty(component, saPhraseDifficulties, phraseIteration);
		return difficulty < 0 ? 0 : difficulty;
	}

	/// A new component may reuse a freed one's address, so drop anything we kept for that address.
	void __stdcall ForgetPhraseDifficulties(uintptr_t component) {
		phraseDifficulties.erase(component);
	}

	void __declspec(naked) ddFillPhraseDifficultiesHook() {
		__asm {
			push dword ptr [esp + 4]			// Component
			call FillDDPhraseDifficulties
			ret 4
		}
	}

	void __declspec(naked) saFillPhraseDifficultiesHook() {
		__asm {
			push dword ptr [esp + 4]			// Component
			call FillSAPhraseDifficulties
			ret 4
		}
	}

	void __declspec(naked) ddGetPhraseDifficultyHook() {
		__asm {
			push dword ptr [esp + 4]			// Phrase iteration
			push ecx							// Component
			call GetDDPhraseDifficulty
			ret 4
		}
	}

	void __declspec(naked) ddResolvePhraseDifficultyHook() {
		__asm {
			push dword ptr [esp + 8]			// Difficulty out
			push dword ptr [esp + 8]			// Phrase iteration
			push ecx							// Component
			call ResolveDDPhraseDifficulty
			ret 8
		}
	}

	void __declspec(naked) saGetPhraseDifficultyHook() {
		__asm {
			push dword ptr [esp + 4]			// Phrase iteration
			push ecx							// Component
			call GetSAPhraseDifficulty
			ret 4
		}
	}

	void __declspec(naked) ddConstructedHook() {
		__asm {
			pushad
			push esi							// The new dynamic difficulty component
			call ForgetPhraseDifficulties
			popad
			pop edi								// The code we are overwriting to place this hook
			mov eax, esi						// The code we are overwriting to place this hook
			pop ebx								// The code we are overwriting to place this hook
			pop ecx								// The code we are overwriting to place this hook
			pop ebp								// The code we are overwriting to place this hook
			push offset Offsets::ptr_ddConstructedJmpBck
			jmp MemUtil::JumpToVersioned
		}
	}

	void __declspec(naked) saConstructedHook() {
		__asm {
			pushad
			push esi							// The new score attack difficulty component
			call ForgetPhraseDifficulties
			popad
			mov edi, dword ptr [esp + 0x10]		// The code we are overwriting to place this hook
			push offset Offsets::ptr_saConstructedJmpBck
			jmp MemUtil::JumpToVersioned
		}
	}

	/// <summary>
	/// The dynamic difficulty and score attack difficulty components cache every phrase
	/// iteration's difficulty in a fixed array of 256 ints inside the component. The fill writes one entry per
	/// phrase iteration without checking, so a song with more than 256 phrase iterations overwrites the next objects
	/// on the heap, and the game crashes once the song starts (usually in the gameplay update, on an object whose
	/// vtable was overwritten with difficulty numbers).
	/// This keeps the first 256 in the component as before and the rest on the side, and makes the cached difficulty
	/// lookups read from there, so every phrase iteration still gets its real difficulty.
	/// </summary>
	void FixPhraseDifficultyCacheOverflow() {
		MemUtil::PlaceHook(Offsets::ptr_ddFillPhraseDifficulties, ddFillPhraseDifficultiesHook, 7);
		MemUtil::PlaceHook(Offsets::ptr_ddGetPhraseDifficulty, ddGetPhraseDifficultyHook, 7);
		MemUtil::PlaceHook(Offsets::ptr_ddResolvePhraseDifficulty, ddResolvePhraseDifficultyHook, 7);
		MemUtil::PlaceHook(Offsets::ptr_ddConstructed, ddConstructedHook, 6);
		MemUtil::PlaceHook(Offsets::ptr_saFillPhraseDifficulties, saFillPhraseDifficultiesHook, 7);
		MemUtil::PlaceHook(Offsets::ptr_saGetPhraseDifficulty, saGetPhraseDifficultyHook, 7);
		MemUtil::PlaceHook(Offsets::ptr_saConstructed, saConstructedHook, 6);

		LOG_INFO("(BUG PREVENTION) Fixed phrase difficulty cache overflow" << std::endl);
	}

	/// <summary>
	/// Fixes crash when modifying functions in Rocksmith.
	/// </summary>
	void FixModifyingFunctions() {
		uintptr_t forceSuccessLSBOffset = Offsets::ptr_ModdedPtrCrashFix.Get() + 0x3; // LSB of the MOV is what we are replacing
		byte forceFailedLSB = MemUtil::ReadValue<byte>(forceSuccessLSBOffset + 0x14, true); // We are replacing it with ForceFailed LSB, which is 0x14 away
		MemUtil::PatchAdr(forceSuccessLSBOffset, (LPVOID)&forceFailedLSB, 1, true);
	}
}