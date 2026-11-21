#include "stdafx.h"
#include "QualityOfLife.hpp"
#include "EngineString.hpp"
#include "Framework/InstructionPatch.hpp"
#include "LnpSites.hpp"
#include "VersioningStruct.h"

#include <unordered_map>

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
	BytePattern<Size> ExpandLnpPattern(const char* text, const std::array<std::uint8_t, Size>& mask) {
		std::vector<unsigned char> expanded;
		if (!LnpSites::Expand(text, static_cast<std::uint32_t>(GameVersion::ModuleBase()), expanded)
			|| expanded.size() != Size)
			return UnavailablePattern<Size>();
		std::array<std::uint8_t, Size> bytes{};
		std::copy(expanded.begin(), expanded.end(), bytes.begin());
		return MaskedPattern(bytes, mask);
	}

	template <std::size_t Size>
	void LogOriginal(const char* name, std::uintptr_t address, const std::array<std::uint8_t, Size>& bytes) {
		std::ostringstream text;
		for (std::uint8_t byte : bytes)
			text << std::hex << static_cast<unsigned int>(byte) << ' ';
		LOG_INFO("(QOL) " << name << " original at 0x" << std::hex << address << ": " << text.str() << std::endl);
	}

	const auto logOriginal = [](const char* name, std::uintptr_t address, const auto& bytes) {
		LogOriginal(name, address, bytes);
	};

	void LogPatchResult(const char* action, const Result& result) {
		if (result.outcome == Outcome::Installed) {
			if (result.changed)
				LOG_INFO("(QOL) " << action << " confirmed" << std::endl);
		}
		else if (result.outcome == Outcome::RollbackIncomplete) {
			LOG_ERROR("(QOL) " << action << " outcome is indeterminate at "
				<< (result.site ? result.site : "patch group") << std::endl);
		}
		else if (result.failure != Failure::None) {
			LOG_WARNING("(QOL) " << action << " left unchanged at "
				<< (result.site ? result.site : "patch group") << ": " << FailureName(result.failure) << std::endl);
		}
	}

	PatchedSite<25>& TwoRtcSite() {
		constexpr auto mask = MakeBytes(0xFF, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0x00,
			0xFF, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
			0xFF, 0x00, 0x00, 0x00, 0x00);
		static auto site = MakeSite("two-RTC connection check", Offsets::ptr_twoRTCBypass.GetValue(), VersionedPattern<25>{
			MaskedPattern(MakeBytes(0xE8, 0x7C, 0x22, 0xE2, 0xFF, 0x84, 0xC0, 0x74, 0x6B, 0xBA,
				0x34, 0xFE, 0x1C, 0x01, 0x8D, 0xB3, 0x4C, 0x01, 0x00, 0x00, 0xE8, 0xD8, 0xD8, 0xC4, 0xFF), mask),
			ExpandLnpPattern<25>("E8 9C 24 E2 FF 84 C0 74 6B BA @DD165C 8D B3 4C 01 00 00 E8 18 CA C4 FF", mask)
		});
		return site;
	}

	// A menu only finishes loading once its object is ready AND its Flash movie exists. The movie is only made once the game
	// answers the menu's request for its movie file, and it only answers if it can find that file in its index of the game's
	// files. When the intro ends, the game (re)builds that index in the background, which takes a second or two (longer on slow
	// drives). With a very short intro (Fast Load) the title screen asks for its movie while the index is still being built, the
	// file isn't in it yet, no answer is ever sent, nothing asks again, and the game sits on the loading screen forever.
	// Asking again once the file has been indexed lets the game finish the menu the normal way.
	// A normal answer arrives within a frame or two (5-30 ms). Waiting on both time and frames between requests means a long
	// hitch can't trigger a retry while the real answer is still on its way, which would make the movie twice.
	constexpr ULONGLONG softLockRetryDelayMs = 250;
	constexpr unsigned softLockRetryDelayFrames = 3;
	constexpr ULONGLONG softLockGiveUpMs = 30000;		// Indexing finishes long before this, even on slow drives.
	constexpr int softLockLogEvery = 20;				// Log the first retry, then about every 5 seconds.
	unsigned menuLoadFrames = 0;

	struct StuckMenu {
		ULONGLONG firstStuck = 0;
		ULONGLONG since = 0;
		unsigned sinceFrame = 0;
		int retries = 0;
		bool gaveUp = false;
		bool seen = false;
		std::string name;
	};

	std::unordered_map<uintptr_t, StuckMenu> stuckMenus;		// By menu load state. Game thread only.
	std::unordered_map<uint64_t, uintptr_t> menuMovies;			// Owner id -> menu movie object. Game thread only.

	uint32_t ReadU32(uintptr_t address) {
		uint32_t value = 0;
		MemUtil::TryRead(address, value);
		return value;
	}

	uint8_t ReadU8(uintptr_t address) {
		uint8_t value = 0;
		MemUtil::TryRead(address, value);
		return value;
	}

	uint64_t OwnerIdOf(uintptr_t movie) {
		const uint32_t owner = ReadU32(movie + 0x0c);
		return (static_cast<uint64_t>(ReadU32(owner + 0x54)) << 32) | ReadU32(owner + 0x50);
	}

	bool IsLiveMenuMovie(uintptr_t movie, uint64_t ownerId) {
		return ReadU32(movie) == Offsets::ptr_menuMovieVTable.GetValue() && OwnerIdOf(movie) == ownerId;
	}

	/// The game's movie request: movie object in EAX, 8 byte request id by value on the stack, cleans its own stack (RET 8).
	void __declspec(naked) __cdecl SendMenuMovieRequest(uintptr_t /*function*/, uintptr_t /*movie*/, uint32_t /*requestIdHigh*/, uint32_t /*requestIdLow*/) {
		__asm {
			push ebp
			mov ebp, esp
			push ebx
			push esi
			push edi
			push dword ptr [ebp + 20]
			push dword ptr [ebp + 16]
			mov eax, dword ptr [ebp + 12]
			call dword ptr [ebp + 8]
			pop edi
			pop esi
			pop ebx
			pop ebp
			ret
		}
	}

	void __stdcall RememberMenuMovie(uintptr_t movie) {
		if (menuMovies.size() > 256) {
			for (auto it = menuMovies.begin(); it != menuMovies.end();)
				it = IsLiveMenuMovie(it->second, it->first) ? std::next(it) : menuMovies.erase(it);
		}
		menuMovies[OwnerIdOf(movie)] = movie;
	}

	/// Runs every frame, just before the game updates the menus it's loading.
	void __stdcall CheckForStuckMenus(uintptr_t menuLoader) {
		menuLoadFrames++;
		for (auto& [state, menu] : stuckMenus)
			menu.seen = false;

		// +0x144: menus still waiting on their object and movie.
		const uint32_t begin = ReadU32(menuLoader + 0x144), end = ReadU32(menuLoader + 0x148);
		if (begin && end >= begin && (end - begin) / 4 <= 256) {
			const ULONGLONG now = GetTickCount64();
			for (uint32_t slot = begin; slot < end; slot += 4) {
				const uint32_t handle = ReadU32(slot);
				const uint32_t record = ReadU32(ReadU32(menuLoader + 0x10 + ((handle >> 12) & 0xff) * 0x0c) + (handle & 0xfff) * 4);
				const uint32_t state = ReadU32(record + 0x0c);
				// Load state: +0x10 object ready, +0x11 movie made, +0x18 owner id.
				if (!state || !ReadU8(state + 0x10) || ReadU8(state + 0x11))
					continue;

				StuckMenu& menu = stuckMenus[state];
				menu.seen = true;
				if (!menu.firstStuck) {
					menu.firstStuck = menu.since = now;
					menu.sinceFrame = menuLoadFrames;
					menu.name = EngineStrings::SafeRead(ReadU32(record + 0x08) + 0x10, 128);
				}
				if (menu.gaveUp || now - menu.since < softLockRetryDelayMs || menuLoadFrames - menu.sinceFrame < softLockRetryDelayFrames)
					continue;

				if (now - menu.firstStuck > softLockGiveUpMs) {
					menu.gaveUp = true;
					LOG_ERROR("(QOL) Menu '" << menu.name << "' still hasn't got its movie after " << menu.retries << " requests, giving up" << std::endl);
					continue;
				}

				// Restart the wait, so each retry gets the same chance to be answered.
				menu.since = now;
				menu.sinceFrame = menuLoadFrames;
				menu.retries++;
				const bool logThisOne = menu.retries == 1 || menu.retries % softLockLogEvery == 0;
				const uint64_t ownerId = (static_cast<uint64_t>(ReadU32(state + 0x1c)) << 32) | ReadU32(state + 0x18);
				const auto movie = menuMovies.find(ownerId);
				if (movie == menuMovies.end() || !IsLiveMenuMovie(movie->second, ownerId) || ReadU32(movie->second + 0x58)) {
					if (logThisOne)
						LOG_WARNING("(QOL) Menu '" << menu.name << "' is stuck loading, but its movie can't be found to retry" << std::endl);
					continue;
				}

				if (logThisOne)
					LOG_WARNING("(QOL) Menu '" << menu.name << "' never got its movie, asking for it again (request " << menu.retries << ", " << (now - menu.firstStuck) << " ms)" << std::endl);
				// Same request id the movie is already listening for (+0x40), so the game's own answer handling takes it from here.
				SendMenuMovieRequest(Offsets::func_menuMovieRequest.GetValue(), movie->second, ReadU32(movie->second + 0x40), ReadU32(movie->second + 0x44));
			}
		}

		for (auto it = stuckMenus.begin(); it != stuckMenus.end();) {
			if (it->second.seen) {
				++it;
				continue;
			}
			if (it->second.retries)
				LOG_INFO("(QOL) Menu '" << it->second.name << "' got its movie after " << it->second.retries << " requests (" << (GetTickCount64() - it->second.firstStuck) << " ms)" << std::endl);
			it = stuckMenus.erase(it);
		}
	}

	void __declspec(naked) menuMovieSetupHook() {
		__asm {
			pushfd
			pushad
			push ecx // The menu movie object
			call RememberMenuMovie
			popad
			popfd
			push ebp
			mov ebp, esp
			and esp, 0xFFFFFFF8 // Original six-byte prologue
			push offset Offsets::ptr_menuMovieSetupJmpBck
			jmp MemUtil::JumpToVersioned
		}
	}

	void __declspec(naked) menuLoadUpdateHook() {
		__asm {
			pushfd
			pushad
			push dword ptr [esp + 40] // The menu loader (first argument, above 8 registers, the flags and the return address)
			call CheckForStuckMenus
			popad
			popfd
			push ebp
			mov ebp, esp
			and esp, 0xFFFFFFF8 // Original six-byte prologue
			push offset Offsets::ptr_menuLoadUpdateJmpBck
			jmp MemUtil::JumpToVersioned
		}
	}
}

namespace QualityOfLife {
	Framework::InstructionPatch::Result PatchTwoRTC(bool enable)
	{
		auto& site = TwoRtcSite();
		if (enable) {
			const auto result = site.Install(readSite, writeSite, MakeBytes(
				0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90,
				0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90,
				0x90, 0x90, 0x90, 0x90, 0x90), logOriginal);
			LogPatchResult("Two-RTC bypass", result);
			return result;
		}
		const auto result = site.Restore(readSite, writeSite, logOriginal);
		LogPatchResult("Two-RTC restore", result);
		return result;
	}

	/// <summary>
	/// RS spawns two processes, one of which complains about Steam not being active. 
	/// We find that one and close it.
	/// </summary>
	HANDLE GetMessageBoxProcess()
	{
		HWND hWnd = FindWindowW(L"#32770", L"Error."); // Dialog box class
		if (!hWnd) {
			return nullptr; 
		}

		DWORD dwProcessId = 0;
		GetWindowThreadProcessId(hWnd, &dwProcessId);
		if (dwProcessId == 0) {
			return nullptr;
		}
		
		return OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_TERMINATE, FALSE, dwProcessId);
	}

	/// <summary>
	/// Stops the game from starting two instances
	/// </summary>
	void StopTwoRSInstances() {
		HANDLE handle = GetMessageBoxProcess();

		if (!handle) {
			return;
		}

		std::vector<wchar_t> processNameBuffer(MAX_PATH);
		HMODULE hMod;

		if (DWORD cNeeded; EnumProcessModulesEx(handle, &hMod, sizeof(hMod), &cNeeded, LIST_MODULES_32BIT | LIST_MODULES_64BIT))
		{
			if (GetModuleBaseNameW(handle, hMod, processNameBuffer.data(), processNameBuffer.size()))
			{
				std::wstring processName(processNameBuffer.data());

				if (processName == L"Rocksmith2014.exe") {
					std::string narrowName;
					narrowName.resize(processName.size());
					std::transform(processName.begin(), processName.end(), narrowName.begin(),
						[](wchar_t ch) { return static_cast<char>(ch); });
					LOG_INFO("Found parasitic process '" << narrowName << "'. Terminating." << std::endl);

					if (!TerminateProcess(handle, 0)) {
						LOG_ERROR("Failed to terminate process. Error: " << GetLastError() << std::endl);
					}
				}
				else {
					std::string narrowName;
					narrowName.resize(processName.size());
					std::transform(processName.begin(), processName.end(), narrowName.begin(),
						[](wchar_t ch) { return static_cast<char>(ch); });
					LOG_INFO("Found dialog box, but process '" << narrowName << "' is not the target. Not terminating." << std::endl);
				}
			}
		}

		CloseHandle(handle);
	}

	/// <summary>
	/// Rocksmith ignores any note below MIDI 24 (C1, 32.7 Hz), so a bass low E tuned more than 4 semitones down at A440
	/// can never be detected. This lowers that floor to 18, about as low as the game can pick up a pitch at 48 kHz (~23.4 Hz).
	/// The game reads the floor when note detection starts, so this has to be patched early.
	/// </summary>
	void LowerNoteDetectionFloor() {
		const std::uintptr_t immediate = Offsets::ptr_noteDetectionFloor.GetValue();
		static auto site = MakeSite("note-detection floor", immediate ? immediate - 6 : 0, VersionedPattern<10>{
			ExactPattern(MakeBytes(0xC7, 0x83, 0xF4, 0x11, 0x00, 0x00, 0x18, 0x00, 0x00, 0x00)),
			ExactPattern(MakeBytes(0xC7, 0x83, 0xF4, 0x11, 0x00, 0x00, 0x18, 0x00, 0x00, 0x00))
		}, 6, 1);
		const auto result = site.Install(readSite, writeSite,
			MakeBytes(0xC7, 0x83, 0xF4, 0x11, 0x00, 0x00, 0x12, 0x00, 0x00, 0x00), logOriginal);
		LogPatchResult("Note-detection floor", result);
	}

	/// <summary>
	/// With Fast Load the title screen can get stuck on the loading screen forever: it asks for its Flash movie while the game is
	/// still indexing its files, gets no answer, and never asks again. While a menu is ready except for its movie, ask for the movie
	/// again every 250 ms (and 3 frames), for up to 30 seconds. Does nothing while menus load normally.
	/// </summary>
	void RetryFastLoadUponSoftLock() {
		const uintptr_t menuLoadUpdate = Offsets::ptr_menuLoadUpdate.GetValue();
		const uintptr_t menuMovieSetup = Offsets::ptr_menuMovieSetup.GetValue();
		if (!menuLoadUpdate || !menuMovieSetup || !Offsets::func_menuMovieRequest.GetValue()) {
			LOG_INFO("(QOL) Fast Load soft lock retry not supported on this game version" << std::endl);
			return;
		}

		static auto movieSite = MakeSite("menu movie setup", Offsets::ptr_menuMovieSetup.GetValue(), VersionedPattern<6>{
			ExactPattern(MakeBytes(0x55, 0x8B, 0xEC, 0x83, 0xE4, 0xF8)),
			ExactPattern(MakeBytes(0x55, 0x8B, 0xEC, 0x83, 0xE4, 0xF8))
		});
		static auto loadSite = MakeSite("menu load update", Offsets::ptr_menuLoadUpdate.GetValue(), VersionedPattern<6>{
			ExactPattern(MakeBytes(0x55, 0x8B, 0xEC, 0x83, 0xE4, 0xF8)),
			ExactPattern(MakeBytes(0x55, 0x8B, 0xEC, 0x83, 0xE4, 0xF8))
		});
		std::array<std::uint8_t, 6> movieJump{};
		std::array<std::uint8_t, 6> loadJump{};
		if (!BuildRelativeJump(movieSite.Address(), reinterpret_cast<std::uintptr_t>(&menuMovieSetupHook), movieJump)
			|| !BuildRelativeJump(loadSite.Address(), reinterpret_cast<std::uintptr_t>(&menuLoadUpdateHook), loadJump)) {
			LogPatchResult("Fast Load soft-lock retry", { Outcome::Original, Failure::Unavailable, "menu hooks" });
			return;
		}
		movieSite.SetReplacement(movieJump);
		loadSite.SetReplacement(loadJump);
		LogPatchResult("Fast Load soft-lock retry", InstallGroup(readSite, writeSite, logOriginal, movieSite, loadSite));
	}
}
