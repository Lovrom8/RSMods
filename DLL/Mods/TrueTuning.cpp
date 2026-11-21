#include "../stdafx.h"
#include "../Framework/InstructionPatch.hpp"
#include "../LnpSites.hpp"
#include "../VersioningStruct.h"
#include "TrueTuning.hpp"

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
		LOG_INFO("(TRUE TUNING) " << name << " original at 0x" << std::hex << address << ": " << text.str() << std::endl);
	}

	const auto logOriginal = [](const char* name, std::uintptr_t address, const auto& bytes) {
		LogOriginal(name, address, bytes);
	};

	void LogResult(const char* action, const Result& result) {
		if (result.outcome == Outcome::Installed) {
			if (result.changed)
				LOG_INFO("(TRUE TUNING) " << action << " confirmed" << std::endl);
		}
		else if (result.outcome == Outcome::RollbackIncomplete) {
			LOG_ERROR("(TRUE TUNING) " << action << " outcome is indeterminate at "
				<< (result.site ? result.site : "patch group") << std::endl);
		}
		else if (result.failure != Failure::None) {
			LOG_WARNING("(TRUE TUNING) " << action << " left unchanged at "
				<< (result.site ? result.site : "patch group") << ": " << FailureName(result.failure) << std::endl);
		}
	}

	PatchedSite<2>& TuningGateSite() {
		constexpr auto mask = MakeBytes(0xFF, 0x00);
		static auto site = MakeSite("tuning gate", Offsets::ptr_disableTrueTuningGate.GetValue(), VersionedPattern<2>{
			MaskedPattern(MakeBytes(0x74, 0x31), mask),
			MaskedPattern(MakeBytes(0x74, 0x31), mask)
		}, 0, 1);
		return site;
	}

	PatchedSite<6>& TuningInstructionSite() {
		constexpr auto mask = MakeBytes(0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00);
		static auto site = MakeSite("tuning instruction", Offsets::ptr_disableTrueTuning.GetValue(), VersionedPattern<6>{
			MaskedPattern(MakeBytes(0xD9, 0x05, 0x68, 0x44, 0x22, 0x01), mask),
			ExpandLnpPattern<6>("D9 05 @E05D00", mask)
		});
		return site;
	}
}

const float ForcedTrueTuning = 440.0f;

/// <summary>
/// If the user is in some menus, we should not trigger this mod or some issues may arise.
/// </summary>
bool __cdecl CanDisableTrueTuning()
{
	if (GameState::currentMenu.size() < 1)
	{
		return false;
	}

	return GameState::currentMenu[0] != 83;
}

void __declspec(naked) disableTrueTuning()
{
	__asm
	{
		// We don't need to worry about replacing commands.
		// I placed this so we always call this code when doing true tuning calculations
		// Hence the change of 0x004DCCBF to 0xEB (JMP) from 0x74 (JE), so we can guarantee that we run without any issues.

		push EAX // Save EAX to the stack

		call CanDisableTrueTuning	// Check if we should disable true tuning on this song.
		je forceUserToTrueTune		// If we can't disable true tuning, then force the user to true tune.

		cmp [EBP + 0x8], -1200	// [EBP + 0x8] = CentOffset. If CentOffset == -1200, we need to use the true tuning or bass can break on some songs.
		jne trueTuningForceA440 // If this song does not have a CentOffset of -1200, then continue with forcing A440.

		forceUserToTrueTune:
		pop EAX										// Restore EAX from the stack

		push offset Offsets::ptr_disableTrueTuning_forceTT
		jmp MemUtil::JumpToVersioned

		trueTuningForceA440:
		pop EAX										// Restore EAX from the stack
		fld ForcedTrueTuning						// Set ST(0) to 440 | A440. This tells note detection that we want to use A440 as our true tuning.

		push offset Offsets::ptr_disableTrueTuning_jmpBck
		jmp MemUtil::JumpToVersioned
	}
}

/// <summary>
/// Disable true tuning by telling note detection that it should use A440 as the base-point.
/// </summary>
void TrueTuning::DisableTrueTuning()
{
	auto& instruction = TuningInstructionSite();
	auto& gate = TuningGateSite();
	std::array<std::uint8_t, 6> hookBytes{};
	if (!BuildRelativeJump(instruction.Address(), reinterpret_cast<std::uintptr_t>(&disableTrueTuning), hookBytes)) {
		LogResult("Disable", { Outcome::Original, Failure::Unavailable, instruction.Name() });
		return;
	}
	instruction.SetReplacement(hookBytes);
	gate.SetReplacement(MakeBytes(0xEB, 0x31));
	LogResult("Disable", InstallGroup(readSite, writeSite, logOriginal, instruction, gate));
}

/// <summary>
/// Re-enable True Tuning
/// </summary>
void TrueTuning::EnableTrueTuning()
{
	auto& instruction = TuningInstructionSite();
	auto& gate = TuningGateSite();
	std::array<std::uint8_t, 6> hookBytes{};
	if (!BuildRelativeJump(instruction.Address(), reinterpret_cast<std::uintptr_t>(&disableTrueTuning), hookBytes)) {
		LogResult("Enable", { Outcome::Original, Failure::Unavailable, instruction.Name() });
		return;
	}
	instruction.SetReplacement(hookBytes);
	gate.SetReplacement(MakeBytes(0xEB, 0x31));
	LogResult("Enable", RestoreGroup(readSite, writeSite, logOriginal, instruction, gate));
}
