#include "../stdafx.h"
#include "../Framework/InstructionPatch.hpp"
#include "TrueTuning.hpp"

namespace {
	using TuningInstruction = std::array<std::uint8_t, 6>;
	using TuningGate = std::array<std::uint8_t, 1>;
	constexpr std::array<std::uint8_t, 2> kTuningInstructionShape{ 0xD9, 0x05 };
	constexpr TuningGate kTuningGateShape{ 0x74 };
	constexpr TuningGate kTuningGatePatch{ 0xEB };
	TuningInstruction originalTuningInstruction{};
	TuningGate originalTuningGate{};
	bool tuningOriginalCaptured = false;

	void LogTuningPatchResult(const char* action, const char* site, MemUtil::PatchResult result) {
		switch (result) {
		case MemUtil::PatchResult::Installed:
			LOG_INFO("(TRUE TUNING) " << action << " " << site << " bytes confirmed" << std::endl);
			break;
		case MemUtil::PatchResult::Original:
			LOG_WARNING("(TRUE TUNING) " << action << " " << site << " write not applied; previous bytes confirmed" << std::endl);
			break;
		case MemUtil::PatchResult::RollbackIncomplete:
			LOG_ERROR("(TRUE TUNING) " << action << " " << site << " outcome is indeterminate" << std::endl);
			break;
		}
	}

	template <std::size_t Size>
	void RollbackTuningPatch(uintptr_t site, const std::array<std::uint8_t, Size>& expected,
		const std::array<std::uint8_t, Size>& previous, const char* name) {
		std::array<std::uint8_t, Size> current{};
		if (!MemUtil::TryRead(site, current)) {
			LOG_ERROR("(TRUE TUNING) Could not read " << name << " while rolling back" << std::endl);
			return;
		}
		if (current == previous)
			return;
		if (current != expected) {
			LOG_ERROR("(TRUE TUNING) " << name << " changed during rollback; left untouched" << std::endl);
			return;
		}
		const auto result = MemUtil::WritePatch(reinterpret_cast<LPVOID>(site), previous.data(), previous.size());
		LogTuningPatchResult("rollback", name, result);
	}

	void LogTuningBytes(const char* site, const std::uint8_t* bytes, std::size_t size) {
		std::ostringstream text;
		for (std::size_t i = 0; i < size; ++i)
			text << std::hex << static_cast<unsigned int>(bytes[i]) << ' ';
		LOG_INFO("(TRUE TUNING) " << site << " original: " << text.str() << std::endl);
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
	const uintptr_t gate = Offsets::ptr_disableTrueTuningGate.Get();
	const uintptr_t instruction = Offsets::ptr_disableTrueTuning.Get();
	TuningGate gateBytes{};
	TuningInstruction instructionBytes{};
	TuningInstruction hookBytes{};
	if (!gate || !instruction || !MemUtil::TryRead(gate, gateBytes) || !MemUtil::TryRead(instruction, instructionBytes)
		|| !Framework::InstructionPatch::BuildRelativeJump(instruction,
			reinterpret_cast<uintptr_t>(&disableTrueTuning), hookBytes)) {
		LOG_WARNING("(TRUE TUNING) Disable sites could not be read or mapped; no changes made" << std::endl);
		return;
	}

	if (!tuningOriginalCaptured) {
		if (!Framework::InstructionPatch::CaptureOriginal(instructionBytes, kTuningInstructionShape, originalTuningInstruction)
			|| !Framework::InstructionPatch::CaptureOriginal(gateBytes, kTuningGateShape, originalTuningGate)) {
			LOG_WARNING("(TRUE TUNING) Disable instruction shape mismatch; no changes made" << std::endl);
			return;
		}
		tuningOriginalCaptured = true;
	}

	const bool instructionIsOriginal = instructionBytes == originalTuningInstruction;
	const bool gateIsOriginal = gateBytes == originalTuningGate;
	const bool instructionIsHooked = Framework::InstructionPatch::IsOwnedReplacement(
		tuningOriginalCaptured, instructionBytes, hookBytes);
	const bool gateIsHooked = Framework::InstructionPatch::IsOwnedReplacement(
		tuningOriginalCaptured, gateBytes, kTuningGatePatch);
	if ((!instructionIsOriginal && !instructionIsHooked) || (!gateIsOriginal && !gateIsHooked)) {
		LOG_WARNING("(TRUE TUNING) Disable sites changed; no changes made" << std::endl);
		return;
	}
	if (instructionIsHooked && gateIsHooked) {
		LOG_INFO("(TRUE TUNING) Already disabled" << std::endl);
		return;
	}

	LogTuningBytes("disable instruction", originalTuningInstruction.data(), originalTuningInstruction.size());
	LogTuningBytes("disable gate", originalTuningGate.data(), originalTuningGate.size());
	if (!instructionIsHooked) {
		const auto result = MemUtil::WritePatch(reinterpret_cast<LPVOID>(instruction), hookBytes.data(), hookBytes.size());
		LogTuningPatchResult("disable", "instruction", result);
		if (result != MemUtil::PatchResult::Installed) {
			RollbackTuningPatch(instruction, hookBytes, originalTuningInstruction, "instruction");
			return;
		}
	}

	if (!gateIsHooked) {
		const auto result = MemUtil::WritePatch(reinterpret_cast<LPVOID>(gate), kTuningGatePatch.data(), kTuningGatePatch.size());
		LogTuningPatchResult("disable", "gate", result);
		if (result != MemUtil::PatchResult::Installed) {
			RollbackTuningPatch(gate, kTuningGatePatch, originalTuningGate, "gate");
			RollbackTuningPatch(instruction, hookBytes, originalTuningInstruction, "instruction");
			return;
		}
	}

	LOG_INFO("Disabled true tuning" << std::endl);
}

/// <summary>
/// Re-enable True Tuning
/// </summary>
void TrueTuning::EnableTrueTuning()
{
	if (!tuningOriginalCaptured) {
		LOG_WARNING("(TRUE TUNING) No captured original bytes; sites were not restored" << std::endl);
		return;
	}

	const uintptr_t gate = Offsets::ptr_disableTrueTuningGate.Get();
	const uintptr_t instruction = Offsets::ptr_disableTrueTuning.Get();
	TuningGate gateBytes{};
	TuningInstruction instructionBytes{};
	TuningInstruction hookBytes{};
	if (!gate || !instruction || !MemUtil::TryRead(gate, gateBytes) || !MemUtil::TryRead(instruction, instructionBytes)
		|| !Framework::InstructionPatch::BuildRelativeJump(instruction,
			reinterpret_cast<uintptr_t>(&disableTrueTuning), hookBytes)) {
		LOG_WARNING("(TRUE TUNING) Enable sites could not be read or mapped; no changes made" << std::endl);
		return;
	}

	if (gateBytes == originalTuningGate && instructionBytes == originalTuningInstruction) {
		LOG_INFO("(TRUE TUNING) Already enabled" << std::endl);
		return;
	}
	if (!Framework::InstructionPatch::IsOwnedReplacement(tuningOriginalCaptured, gateBytes, kTuningGatePatch)
		|| !Framework::InstructionPatch::IsOwnedReplacement(tuningOriginalCaptured, instructionBytes, hookBytes)) {
		LOG_WARNING("(TRUE TUNING) Enable sites no longer hold our replacements; no changes made" << std::endl);
		return;
	}

	const auto instructionResult = MemUtil::WritePatch(reinterpret_cast<LPVOID>(instruction),
		originalTuningInstruction.data(), originalTuningInstruction.size());
	LogTuningPatchResult("enable", "instruction", instructionResult);
	if (instructionResult != MemUtil::PatchResult::Installed) {
		RollbackTuningPatch(instruction, originalTuningInstruction, hookBytes, "instruction");
		return;
	}

	const auto gateResult = MemUtil::WritePatch(reinterpret_cast<LPVOID>(gate),
		originalTuningGate.data(), originalTuningGate.size());
	LogTuningPatchResult("enable", "gate", gateResult);
	if (gateResult != MemUtil::PatchResult::Installed) {
		RollbackTuningPatch(gate, originalTuningGate, kTuningGatePatch, "gate");
		RollbackTuningPatch(instruction, originalTuningInstruction, hookBytes, "instruction");
		return;
	}

	LOG_INFO("Enabled true tuning" << std::endl);
}