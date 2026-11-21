#include "../stdafx.h"
#include "BugPrevention.hpp"
#include "../MemUtil.hpp"
#include "../LnpSiteCheck.hpp"
#include "../Framework/InstructionPatch.hpp"
#include "../VersioningStruct.h"

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
		LOG_INFO("(BUG PREVENTION) " << name << " original at 0x" << std::hex << address << ": " << text.str() << std::endl);
	}

	void LogResult(const char* action, const Result& result) {
		if (result.outcome == Outcome::Installed) {
			if (result.changed)
				LOG_INFO("(BUG PREVENTION) " << action << " confirmed" << std::endl);
		}
		else if (result.outcome == Outcome::RollbackIncomplete) {
			LOG_ERROR("(BUG PREVENTION) " << action << " outcome is indeterminate at "
				<< (result.site ? result.site : "patch group") << std::endl);
		}
		else if (result.failure != Failure::None) {
			LOG_WARNING("(BUG PREVENTION) " << action << " left unchanged at "
				<< (result.site ? result.site : "patch group") << ": " << FailureName(result.failure) << std::endl);
		}
	}

	const auto logOriginal = [](const char* name, std::uintptr_t address, const auto& bytes) {
		LogOriginal(name, address, bytes);
	};

	PatchedSite<6>& OculusPnpFirstSite() {
		static auto site = MakeSite("Oculus/PnP first branch", Offsets::ptr_OculusCrashJmp.GetValue(), VersionedPattern<6>{
			MaskedPattern(MakeBytes(0x0F, 0x83, 0x18, 0x02, 0x00, 0x00), MakeBytes(0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00)),
			MaskedPattern(MakeBytes(0x0F, 0x83, 0x18, 0x02, 0x00, 0x00), MakeBytes(0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00))
		});
		return site;
	}
}

namespace BugPrevention {
	void DeferDlcOwnershipQueries() {
		// Leave each LocalDLC entry unqueried so IsOwned checks it on demand.
		constexpr Framework::GameVersion::Site site{ "Init installed query", 0x008DBB74, LnpSites::kDlcInstalledCheck.rva };
		constexpr unsigned char expected[] = {
			0x50, 0x8B, 0x42, 0x1C, 0xFF, 0xD0, 0x88, 0x44, 0x24,
			0x5D, 0x8B, 0x46, 0x1C, 0xC6, 0x44, 0x24, 0x5F, 0x01
		};
		constexpr unsigned char deferred[] = {
			0x90, 0x31, 0xC0, 0x90, 0x90, 0x90, 0x88, 0x44, 0x24,
			0x5D, 0x8B, 0x46, 0x1C, 0xC6, 0x44, 0x24, 0x5F, 0x00
		};
		const uintptr_t address = GameVersion::CheckedSite("DLC OWNERSHIP", site, expected, nullptr, sizeof(expected));
		if (!address) {
			MemUtil::FailBootPatchBundle();
			return;
		}
		LOG_INFO("(DLC OWNERSHIP) Init original bytes: 50 8B 42 1C FF D0 88 44 24 5D 8B 46 1C C6 44 24 5F 01" << std::endl);
		if (!MemUtil::PatchAdr(reinterpret_cast<LPVOID>(address), deferred, sizeof(deferred)) ||
			(!MemUtil::BootPatchStaged(address) && !MemUtil::MatchesBytes(address, deferred))) {
			MemUtil::PatchAdr(reinterpret_cast<LPVOID>(address), expected, sizeof(expected));
			LOG_ERROR("(DLC OWNERSHIP) Init patch failed; original bytes "
				<< (MemUtil::MatchesBytes(address, expected) ? "restored" : "NOT restored") << std::endl);
			return;
		}
		LOG_INFO("(DLC OWNERSHIP) Init defers installed checks to IsOwned" << std::endl);
	}

	void SkipDlcStorePrefetch() {
		// The store requests its DLC information again when it opens.
		constexpr Framework::GameVersion::Site site{ "Init store prefetch", 0x008DBBEA, LnpSites::kDlcStorePrefetch.rva };
		constexpr unsigned char expected[] = { 0x50, 0x56, 0xE8, 0x1F, 0x07, 0x00, 0x00 };
		constexpr unsigned char skipped[] = { 0x50, 0x56, 0x83, 0xC4, 0x08, 0x90, 0x90 };
		const uintptr_t address = GameVersion::CheckedSite("DLC STORE", site, expected, nullptr, sizeof(expected));
		if (!address) {
			MemUtil::FailBootPatchBundle();
			return;
		}
		LOG_INFO("(DLC STORE) Init original bytes: 50 56 E8 1F 07 00 00" << std::endl);
		if (!MemUtil::PatchAdr(reinterpret_cast<LPVOID>(address), skipped, sizeof(skipped)) ||
			(!MemUtil::BootPatchStaged(address) && !MemUtil::MatchesBytes(address, skipped))) {
			MemUtil::PatchAdr(reinterpret_cast<LPVOID>(address), expected, sizeof(expected));
			LOG_ERROR("(DLC STORE) Init patch failed; original bytes "
				<< (MemUtil::MatchesBytes(address, expected) ? "restored" : "NOT restored") << std::endl);
			return;
		}
		LOG_INFO("(DLC STORE) Init skips the store prefetch; the store asks when it opens" << std::endl);
	}

	/// <summary>
	/// When Rocksmith opens with a Oculus / Meta headset connected to the user computer, it can cause a crash.
	/// This is due to Rocksmith saying it owns memory that it doesn't have access to.
	/// In this fix, we jump over the interior of the for-loop (marked as a while with a break case) that writes to invalid memory.
	/// </summary>
	void PreventOculusCrash() {
		const auto result = OculusPnpFirstSite().Install(readSite, writeSite, MakeBytes(0xE9, 0x19, 0x02, 0x00, 0x00, 0x90), logOriginal);
		LogResult("Oculus crash guard", result);
	}

	/// <summary>
	/// When the user enters a song with a buggy tone, every tone after it will not work.
	/// This mod prevents that by changing a conditional jump to a jump that always happens.
	/// So when the user encounters a broken tone, all they need to do is change the tone and the tones should start working again.
	/// </summary>
	void PreventStuckTone() {
		static auto site = MakeSite("stuck-tone branch", Offsets::ptr_StuckToneJmp.GetValue(), VersionedPattern<2>{
			ExactPattern(MakeBytes(0x75, 0x13)), ExactPattern(MakeBytes(0x75, 0x13))
		}, 0, 1);
		const auto result = site.Install(readSite, writeSite, MakeBytes(0xEB, 0x13), logOriginal);
		LogResult("Stuck-tone guard", result);
	}

	/// <summary>
	/// When a user has a faulty PnP (Plug-n-Play) device connected Rocksmith can crash.
	/// It crashes with a memory access violation. The following code skips over the while loop that may eventually crash.
	/// </summary>
	void PreventPnPCrash() {
		static auto secondSite = MakeSite("PnP second branch", Offsets::ptr_PnpJmp_2.GetValue(), VersionedPattern<6>{
			MaskedPattern(MakeBytes(0x0F, 0x8C, 0x9A, 0xFD, 0xFF, 0xFF), MakeBytes(0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00)),
			MaskedPattern(MakeBytes(0x0F, 0x8C, 0x9A, 0xFD, 0xFF, 0xFF), MakeBytes(0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00))
		});
		auto& firstSite = OculusPnpFirstSite();
		firstSite.SetReplacement(MakeBytes(0xE9, 0x19, 0x02, 0x00, 0x00, 0x90));
		secondSite.SetReplacement(MakeBytes(0x90, 0x90, 0x90, 0x90, 0x90, 0x90));
		const auto result = InstallGroup(readSite, writeSite, logOriginal, firstSite, secondSite);
		LogResult("PnP crash guard", result);
	}

	/// <summary>
	/// Ubisoft lets you put almost any character in your Uplay password.
	/// However, Rocksmith does not allow some characters.
	/// This means that the user will have to change their password if they are using an invalid character, and they want to use leaderboards.
	/// Some of those invalid characters are as follows: " \ / and =
	/// This mod prevents the checks for those characters to allow the user to have more complex passwords.
	/// </summary>
	void AllowComplexPasswords() {
		static auto passwordSite = MakeSite("password limit branch", Offsets::ptr_Password_LimitCharacters.GetValue(), VersionedPattern<2>{
			ExactPattern(MakeBytes(0x74, 0x44)), ExactPattern(MakeBytes(0x74, 0x44))
		});
		static auto clipboardSite = MakeSite("password clipboard branch", Offsets::ptr_Password_LimitCharacters_Clipboard.GetValue(), VersionedPattern<2>{
			ExactPattern(MakeBytes(0x74, 0x1C)), ExactPattern(MakeBytes(0x74, 0x1C))
		});
		passwordSite.SetReplacement(MakeBytes(0x90, 0x90));
		clipboardSite.SetReplacement(MakeBytes(0x90, 0x90));
		const auto result = InstallGroup(readSite, writeSite, logOriginal, passwordSite, clipboardSite);
		LogResult("Complex password checks", result);
	}

	/// <summary>
	/// Skip profile save file Steam Account Id check to allow sharing profiles without needing external tooling.
	/// </summary>
	void BypassSaveFilePlatformIdCheck() {
		if (GameVersion::IsLearnAndPlay()
			&& (Offsets::ptr_SaveFilePlatformIdCheck.Get() != LnpSites::At(LnpSites::kProfileIdCheck) + 5
				|| !LnpSites::Matches(LnpSites::kProfileIdCheck))) {
			LOG_WARNING("(PROFILE ID) Learn & Play lead-in or branch mismatch; game check retained" << std::endl);
			return;
		}
		static auto site = MakeSite("profile platform-ID branch", Offsets::ptr_SaveFilePlatformIdCheck.GetValue(), VersionedPattern<2>{
			ExactPattern(MakeBytes(0x74, 0x2F)), ExactPattern(MakeBytes(0x74, 0x2F))
		}, 0, 1);
		const auto result = site.Install(readSite, writeSite, MakeBytes(0xEB, 0x2F), logOriginal);
		LogResult("Cross-platform profile-ID check", result);
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
		static auto site = MakeSite("advanced-display hook", Offsets::ptr_AdvancedDisplayCrash.GetValue(), VersionedPattern<7>{
			ExactPattern(MakeBytes(0x8A, 0x51, 0x04, 0x57, 0x8B, 0x7E, 0x0C)),
			UnavailablePattern<7>()
		});
		std::array<std::uint8_t, 7> replacement{};
		if (!BuildRelativeJump(site.Address(), reinterpret_cast<std::uintptr_t>(&advancedDisplayCrashHook), replacement)) {
			LogResult("Advanced-display guard", { Outcome::Original, Failure::Unavailable, site.Name() });
			return;
		}
		const auto result = site.Install(readSite, writeSite, replacement, logOriginal);
		LogResult("Advanced-display guard", result);
	}


	/// <summary>
	/// In extremely rare cases, the user may have an audio in device have a driver issue.
	/// This causes Rocksmith to crash when it reads all of their audio input devices.
	/// This just patches out those checks, so it won't crash when EBX is a nullptr.
	/// </summary>
	void PreventPortAudioInDeviceCrash() {
		static auto site = MakeSite("PortAudio input guard", Offsets::ptr_PortAudioInCrash.GetValue(), VersionedPattern<10>{
			ExactPattern(MakeBytes(0x83, 0x7B, 0x0C, 0x01, 0x0F, 0x8C, 0x75, 0x02, 0x00, 0x00)),
			UnavailablePattern<10>()
		});
		const auto result = site.Install(readSite, writeSite, MakeBytes(0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90), logOriginal);
		LogResult("PortAudio input guard", result);
	}

	/// <summary>
	/// Prevention for crash caused by certain audio devices (like Voicemeter virtual cables).
	/// </summary>
	void PreventExtraAudioDevicesCrash() {
		static auto site = MakeSite("additional audio devices guard", Offsets::ptr_AdditionalAudioDevicesCrash.GetValue(), VersionedPattern<9>{
			ExactPattern(MakeBytes(0x8B, 0x8D, 0xC4, 0xFD, 0xFF, 0xFF, 0x8B, 0x1C, 0x81)),
			ExactPattern(MakeBytes(0x8B, 0x8D, 0xC4, 0xFD, 0xFF, 0xFF, 0x8B, 0x1C, 0x81))
		});
		const auto result = site.Install(readSite, writeSite, MakeBytes(0x90, 0x90, 0x90, 0x90, 0x90, 0x31, 0xC9, 0x31, 0xDB), logOriginal);
		LogResult("Additional audio devices guard", result);
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
		static auto site = MakeSite("controller-axis hook", Offsets::ptr_ControllerAxisBounds.GetValue(), VersionedPattern<6>{
			ExactPattern(MakeBytes(0x55, 0x8B, 0xEC, 0x83, 0xEC, 0x08)),
			ExactPattern(MakeBytes(0x55, 0x8B, 0xEC, 0x83, 0xEC, 0x08))
		});
		std::array<std::uint8_t, 6> replacement{};
		if (!BuildRelativeJump(site.Address(), reinterpret_cast<std::uintptr_t>(&controllerAxisBoundsHook), replacement)) {
			LogResult("Controller-axis guard", { Outcome::Original, Failure::Unavailable, site.Name() });
			return;
		}
		const auto result = site.Install(readSite, writeSite, replacement, logOriginal);
		LogResult("Controller-axis guard", result);
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
		static auto site = MakeSite("invalid-input tree hook", Offsets::ptr_InvalidInputTreeRootCheck.GetValue(), VersionedPattern<hookLength>{
			MaskedPattern(MakeBytes(0x85, 0xDB, 0x0F, 0x84, 0x98, 0x00, 0x00, 0x00), MakeBytes(0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00)),
			UnavailablePattern<hookLength>()
		});
		std::array<std::uint8_t, hookLength> replacement{};
		if (!BuildRelativeJump(site.Address(), reinterpret_cast<std::uintptr_t>(&invalidInputTreeRootHook), replacement)) {
			LogResult("UI input guard", { Outcome::Original, Failure::Unavailable, site.Name() });
			return;
		}
		const auto result = site.Install(readSite, writeSite, replacement, logOriginal);
		LogResult("UI input guard", result);
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
		static auto site = MakeSite("calibration sample-count hook", Offsets::ptr_calibrationSampleCountClamp.GetValue(), VersionedPattern<10>{
			ExactPattern(MakeBytes(0x8B, 0x54, 0x24, 0x10, 0x89, 0x93, 0x88, 0x07, 0x00, 0x00)),
			UnavailablePattern<10>()
		});
		std::array<std::uint8_t, 10> replacement{};
		if (!BuildRelativeJump(site.Address(), reinterpret_cast<std::uintptr_t>(&calibrationSampleCountClampHook), replacement)) {
			LogResult("High-framerate calibration guard", { Outcome::Original, Failure::Unavailable, site.Name() });
			return;
		}
		const auto result = site.Install(readSite, writeSite, replacement, logOriginal);
		LogResult("High-framerate calibration guard", result);
	}

	/// <summary>
	/// Fixes crash when modifying functions in Rocksmith.
	/// </summary>
	void FixModifyingFunctions() {
		const uintptr_t rva = Offsets::ptr_ModdedPtrCrashFix.GetValue();
		const uintptr_t address = rva ? GameVersion::ModuleBase() + rva : 0;
		constexpr auto mask = MakeBytes(0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00,
			0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
			0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00);
		const auto lnp = ExpandLnpPattern<27>(
			"C7 46 30 @D9EAC4 53 33 DB 66 89 5E 34 89 46 3C 89 7E 38 C7 46 40 @D9EAB8", mask);
		static auto site = MakeSite("modifying-functions pointer", address, VersionedPattern<27>{
			MaskedPattern(MakeBytes(0xC7, 0x46, 0x30, 0xEC, 0xD2, 0x19, 0x01,
				0x53, 0x33, 0xDB, 0x66, 0x89, 0x5E, 0x34, 0x89, 0x46, 0x3C, 0x89, 0x7E, 0x38,
				0xC7, 0x46, 0x40, 0xE0, 0xD2, 0x19, 0x01), mask),
			lnp
		}, 3, 1);
		const auto result = site.InstallTransform(readSite, writeSite,
			[](const auto& original) {
				auto replacement = original;
				replacement[3] = original[23];
				return replacement;
			}, logOriginal);
		LogResult("Modifying-functions guard", result);
	}
}
