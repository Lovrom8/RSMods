#include "../stdafx.h"
#include "BugPrevention.hpp"
#include "../MemUtil.hpp"
#include "../LnpSiteCheck.hpp"
#include "../SitePatchGroup.hpp"
#include "../VersioningStruct.h"

namespace {
	using namespace Framework::InstructionPatch;
	using namespace SitePatch;
	// These startup groups are installed from MainThread and remain owned for the process lifetime.
	std::vector<std::unique_ptr<PatchedSiteGroup>> retainedSiteGroups;

	void InstallSites(const char* action, CaptureResult captured) {
		SitePatch::InstallRetained(retainedSiteGroups, "BUG PREVENTION", action, std::move(captured));
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
		const auto replacement = MakeBytes(0xE9, 0x19, 0x02, 0x00, 0x00, 0x90);
		const auto site = MakeSite("Oculus/PnP first branch", Offsets::ptr_OculusCrashJmp.GetValue(), VersionedPattern<6>{
			ExactPattern(MakeBytes(0x0F, 0x83, 0x18, 0x02, 0x00, 0x00)),
			ExactPattern(MakeBytes(0x0F, 0x83, 0x18, 0x02, 0x00, 0x00))
		}, replacement);
		InstallSites("Oculus crash guard", CaptureSites(ReadSite, site));
	}

	/// <summary>
	/// When the user enters a song with a buggy tone, every tone after it will not work.
	/// This mod prevents that by changing a conditional jump to a jump that always happens.
	/// So when the user encounters a broken tone, all they need to do is change the tone and the tones should start working again.
	/// </summary>
	void PreventStuckTone() {
		const auto replacement = MakeBytes(0xEB, 0x13);
		const auto site = MakeSite("stuck-tone branch", Offsets::ptr_StuckToneJmp.GetValue(), VersionedPattern<2>{
			ExactPattern(MakeBytes(0x75, 0x13)), ExactPattern(MakeBytes(0x75, 0x13))
		}, replacement);
		InstallSites("Stuck-tone guard", CaptureSites(ReadSite, site));
	}

	/// <summary>
	/// When a user has a faulty PnP (Plug-n-Play) device connected Rocksmith can crash.
	/// It crashes with a memory access violation. The following code skips over the while loop that may eventually crash.
	/// </summary>
	void PreventPnPCrash() {
		const auto firstReplacement = MakeBytes(0xE9, 0x19, 0x02, 0x00, 0x00, 0x90);
		const auto firstSite = MakeSite("Oculus/PnP first branch", Offsets::ptr_OculusCrashJmp.GetValue(), VersionedPattern<6>{
			ExactPattern(MakeBytes(0x0F, 0x83, 0x18, 0x02, 0x00, 0x00)),
			ExactPattern(MakeBytes(0x0F, 0x83, 0x18, 0x02, 0x00, 0x00))
		}, firstReplacement);
		const auto secondReplacement = MakeBytes(0x90, 0x90, 0x90, 0x90, 0x90, 0x90);
		const auto secondSite = MakeSite("PnP second branch", Offsets::ptr_PnpJmp_2.GetValue(), VersionedPattern<6>{
			ExactPattern(MakeBytes(0x0F, 0x8C, 0x9A, 0xFD, 0xFF, 0xFF)),
			ExactPattern(MakeBytes(0x0F, 0x8C, 0x9A, 0xFD, 0xFF, 0xFF))
		}, secondReplacement);
		InstallSites("PnP crash guard", CaptureSites(ReadSite, firstSite, secondSite));
	}

	/// <summary>
	/// Ubisoft lets you put almost any character in your Uplay password.
	/// However, Rocksmith does not allow some characters.
	/// This means that the user will have to change their password if they are using an invalid character, and they want to use leaderboards.
	/// Some of those invalid characters are as follows: " \ / and =
	/// This mod prevents the checks for those characters to allow the user to have more complex passwords.
	/// </summary>
	void AllowComplexPasswords() {
		const auto replacement = MakeBytes(0x90, 0x90);
		const auto passwordSite = MakeSite("password limit branch", Offsets::ptr_Password_LimitCharacters.GetValue(), VersionedPattern<2>{
			ExactPattern(MakeBytes(0x74, 0x44)), ExactPattern(MakeBytes(0x74, 0x44))
		}, replacement);
		const auto clipboardSite = MakeSite("password clipboard branch", Offsets::ptr_Password_LimitCharacters_Clipboard.GetValue(), VersionedPattern<2>{
			ExactPattern(MakeBytes(0x74, 0x1C)), ExactPattern(MakeBytes(0x74, 0x1C))
		}, replacement);
		InstallSites("Complex password checks", CaptureSites(ReadSite, passwordSite, clipboardSite));
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
		const auto replacement = MakeBytes(0xEB, 0x2F);
		const auto site = MakeSite("profile platform-ID branch", Offsets::ptr_SaveFilePlatformIdCheck.GetValue(), VersionedPattern<2>{
			ExactPattern(MakeBytes(0x74, 0x2F)), ExactPattern(MakeBytes(0x74, 0x2F))
		}, replacement);
		InstallSites("Cross-platform profile-ID check", CaptureSites(ReadSite, site));
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
		std::array<std::uint8_t, 7> replacement{};
		const auto address = Offsets::ptr_AdvancedDisplayCrash.GetValue();
		if (!BuildRelativeJump(address, reinterpret_cast<std::uintptr_t>(&advancedDisplayCrashHook), replacement)) {
			SitePatch::LogUnavailable("BUG PREVENTION", "Advanced-display guard", "advanced-display hook");
			return;
		}
		const auto site = MakeSite("advanced-display hook", address, VersionedPattern<7>{
			ExactPattern(MakeBytes(0x8A, 0x51, 0x04, 0x57, 0x8B, 0x7E, 0x0C)),
			ExactPattern(MakeBytes(0x8A, 0x51, 0x04, 0x57, 0x8B, 0x7E, 0x0C))
		}, replacement);
		InstallSites("Advanced-display guard", CaptureSites(ReadSite, site));
	}


	/// <summary>
	/// In extremely rare cases, the user may have an audio in device have a driver issue.
	/// This causes Rocksmith to crash when it reads all of their audio input devices.
	/// This just patches out those checks, so it won't crash when EBX is a nullptr.
	/// </summary>
	void PreventPortAudioInDeviceCrash() {
		const auto replacement = MakeBytes(0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90);
		const auto site = MakeSite("PortAudio input guard", Offsets::ptr_PortAudioInCrash.GetValue(), VersionedPattern<10>{
			ExactPattern(MakeBytes(0x83, 0x7B, 0x0C, 0x01, 0x0F, 0x8C, 0x75, 0x02, 0x00, 0x00)),
			ExactPattern(MakeBytes(0x83, 0x7B, 0x0C, 0x01, 0x0F, 0x8C, 0x75, 0x02, 0x00, 0x00))
		}, replacement);
		InstallSites("PortAudio input guard", CaptureSites(ReadSite, site));
	}

	/// <summary>
	/// Prevention for crash caused by certain audio devices (like Voicemeter virtual cables).
	/// </summary>
	void PreventExtraAudioDevicesCrash() {
		const auto replacement = MakeBytes(0x90, 0x90, 0x90, 0x90, 0x90, 0x31, 0xC9, 0x31, 0xDB);
		const auto site = MakeSite("additional audio devices guard", Offsets::ptr_AdditionalAudioDevicesCrash.GetValue(), VersionedPattern<9>{
			ExactPattern(MakeBytes(0x8B, 0x8D, 0xC4, 0xFD, 0xFF, 0xFF, 0x8B, 0x1C, 0x81)),
			ExactPattern(MakeBytes(0x8B, 0x8D, 0xC4, 0xFD, 0xFF, 0xFF, 0x8B, 0x1C, 0x81))
		}, replacement);
		InstallSites("Additional audio devices guard", CaptureSites(ReadSite, site));
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
		const auto address = Offsets::ptr_ControllerAxisBounds.GetValue();
		std::array<std::uint8_t, 6> replacement{};
		if (!BuildRelativeJump(address, reinterpret_cast<std::uintptr_t>(&controllerAxisBoundsHook), replacement)) {
			SitePatch::LogUnavailable("BUG PREVENTION", "Controller-axis guard", "controller-axis hook");
			return;
		}
		const auto site = MakeSite("controller-axis hook", address, VersionedPattern<6>{
			ExactPattern(MakeBytes(0x55, 0x8B, 0xEC, 0x83, 0xEC, 0x08)),
			ExactPattern(MakeBytes(0x55, 0x8B, 0xEC, 0x83, 0xEC, 0x08))
		}, replacement);
		InstallSites("Controller-axis guard", CaptureSites(ReadSite, site));
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
		const auto address = Offsets::ptr_InvalidInputTreeRootCheck.GetValue();
		std::array<std::uint8_t, hookLength> replacement{};
		if (!BuildRelativeJump(address, reinterpret_cast<std::uintptr_t>(&invalidInputTreeRootHook), replacement)) {
			SitePatch::LogUnavailable("BUG PREVENTION", "UI input guard", "invalid-input tree hook");
			return;
		}
		const auto site = MakeSite("invalid-input tree hook", address, VersionedPattern<hookLength>{
			ExactPattern(MakeBytes(0x85, 0xDB, 0x0F, 0x84, 0x98, 0x00, 0x00, 0x00)),
			ExactPattern(MakeBytes(0x85, 0xDB, 0x0F, 0x84, 0x98, 0x00, 0x00, 0x00))
		}, replacement);
		InstallSites("UI input guard", CaptureSites(ReadSite, site));
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
		const auto address = Offsets::ptr_calibrationSampleCountClamp.GetValue();
		std::array<std::uint8_t, 10> replacement{};
		if (!BuildRelativeJump(address, reinterpret_cast<std::uintptr_t>(&calibrationSampleCountClampHook), replacement)) {
			SitePatch::LogUnavailable("BUG PREVENTION", "High-framerate calibration guard",
				"calibration sample-count hook");
			return;
		}
		const auto site = MakeSite("calibration sample-count hook", address, VersionedPattern<10>{
			ExactPattern(MakeBytes(0x8B, 0x54, 0x24, 0x10, 0x89, 0x93, 0x88, 0x07, 0x00, 0x00)),
			ExactPattern(MakeBytes(0x8B, 0x54, 0x24, 0x10, 0x89, 0x93, 0x88, 0x07, 0x00, 0x00))
		}, replacement);
		InstallSites("High-framerate calibration guard", CaptureSites(ReadSite, site));
	}

	/// <summary>
	/// Fixes crash when modifying functions in Rocksmith.
	/// </summary>
	void FixModifyingFunctions() {
		const uintptr_t rva = Offsets::ptr_ModdedPtrCrashFix.GetValue();
		const uintptr_t address = rva ? GameVersion::ModuleBase() + rva : 0;
		const auto lnp = ExpandLnpPattern<27>(
			"C7 46 30 @D9EAC4 53 33 DB 66 89 5E 34 89 46 3C 89 7E 38 C7 46 40 @D9EAB8");
		const auto buildReplacement = +[](const std::array<std::uint8_t, 27>& original,
			std::array<std::uint8_t, 27>& replacement) noexcept {
			replacement = original;
			replacement[3] = original[23];
			return true;
		};
		const auto site = MakeSite("modifying-functions pointer", address, VersionedPattern<27>{
			ExactPattern(MakeBytes(0xC7, 0x46, 0x30, 0xEC, 0xD2, 0x19, 0x01,
				0x53, 0x33, 0xDB, 0x66, 0x89, 0x5E, 0x34, 0x89, 0x46, 0x3C, 0x89, 0x7E, 0x38,
				0xC7, 0x46, 0x40, 0xE0, 0xD2, 0x19, 0x01)),
			lnp
		}, std::array<std::uint8_t, 27>{}, buildReplacement);
		InstallSites("Modifying-functions guard", CaptureSites(ReadSite, site));
	}
}
