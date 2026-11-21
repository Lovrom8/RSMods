#include "../stdafx.h"
#include "../Framework/InstructionPatch.hpp"
#include "../LnpSites.hpp"
#include "../VersioningStruct.h"
#include "TrueTuning.hpp"

#include <memory>

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

	template <std::size_t Size>
	BytePattern<Size> ExpandLnpPattern(const char* text) {
		std::vector<unsigned char> expanded;
		if (!LnpSites::Expand(text, static_cast<std::uint32_t>(GameVersion::ModuleBase()), expanded)
			|| expanded.size() != Size)
			return UnavailablePattern<Size>();
		std::array<std::uint8_t, Size> bytes{};
		std::copy(expanded.begin(), expanded.end(), bytes.begin());
		return ExactPattern(bytes);
	}

	void LogOriginal(const char* name, std::uintptr_t address, const std::vector<std::uint8_t>& bytes) {
		std::ostringstream text;
		for (std::uint8_t byte : bytes)
			text << std::hex << static_cast<unsigned int>(byte) << ' ';
		LOG_INFO("(TRUE TUNING) " << name << " original at 0x" << std::hex << address << ": " << text.str() << std::endl);
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
			LOG_WARNING("(TRUE TUNING) " << action << " left unchanged at " << captured.site
				<< ": " << FailureName(captured.failure) << std::endl);
	}

	void LogUnavailable(const char* action, const char* site) {
		LOG_WARNING("(TRUE TUNING) " << action << " left unchanged at " << site << ": "
			<< FailureName(ValidationFailure::Unavailable) << std::endl);
	}

	void LogResult(const char* action, const CaptureResult& captured, MemUtil::PatchResult result,
		std::uintptr_t failureAddress, bool restoring) {
		const char* site = SiteNameAt(captured, failureAddress);
		if ((!restoring && result == MemUtil::PatchResult::Installed)
			|| (restoring && result == MemUtil::PatchResult::Original))
			LOG_INFO("(TRUE TUNING) " << action << " confirmed" << std::endl);
		else if (result == MemUtil::PatchResult::RollbackIncomplete) {
			LOG_ERROR("(TRUE TUNING) " << action << " outcome is indeterminate at "
				<< (site ? site : "patch group") << std::endl);
		}
		else {
			LOG_WARNING("(TRUE TUNING) " << action << " left unchanged at "
				<< (site ? site : "patch group") << ": write was not applied" << std::endl);
		}
	}

	std::unique_ptr<MemUtil::PreparedPatchSet> tuningPatch;
	CaptureResult tuningSites;

	SiteDescriptor<2> TuningGateSite() {
		return MakeSite("tuning gate", Offsets::ptr_disableTrueTuningGate.GetValue(), VersionedPattern<2>{
			ExactPattern(MakeBytes(0x74, 0x31)), ExactPattern(MakeBytes(0x74, 0x31))
		}, MakeBytes(0xEB, 0x31));
	}

	SiteDescriptor<6> TuningInstructionSite(const std::array<std::uint8_t, 6>& replacement) {
		return MakeSite("tuning instruction", Offsets::ptr_disableTrueTuning.GetValue(), VersionedPattern<6>{
			ExactPattern(MakeBytes(0xD9, 0x05, 0x68, 0x44, 0x22, 0x01)),
			ExpandLnpPattern<6>("D9 05 @E25D00")
		}, replacement);
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
	if (tuningPatch) {
		const auto result = MemUtil::ToPatchResult(tuningPatch->Outcome());
		if (result != MemUtil::PatchResult::Installed)
			LogResult("Disable", tuningSites, result, tuningPatch->FailureAddress(), false);
		return;
	}
	const auto address = Offsets::ptr_disableTrueTuning.GetValue();
	std::array<std::uint8_t, 6> hookBytes{};
	if (!BuildRelativeJump(address, reinterpret_cast<std::uintptr_t>(&disableTrueTuning), hookBytes)) {
		LogUnavailable("Disable", "tuning instruction");
		return;
	}
	const auto instruction = TuningInstructionSite(hookBytes);
	const auto gate = TuningGateSite();
	auto captured = CaptureSites(readSite, instruction, gate);
	if (captured.failure != ValidationFailure::None) {
		LogValidationFailure("Disable", captured);
		return;
	}
	auto patches = std::make_unique<MemUtil::PreparedPatchSet>();
	if (!AppendSites(*patches, captured)) {
		LogResult("Disable", captured, MemUtil::PatchResult::Original, patches->FailureAddress(), false);
		return;
	}
	for (const CapturedSite& site : captured.sites)
		LogOriginal(site.name, site.address, site.original);
	const auto result = patches->Commit();
	const auto failureAddress = patches->FailureAddress();
	if (result == MemUtil::PatchResult::Original)
		LogResult("Disable", captured, result, failureAddress, false);
	else {
		tuningSites = std::move(captured);
		tuningPatch = std::move(patches);
		LogResult("Disable", tuningSites, result, failureAddress, false);
	}
}

/// <summary>
/// Re-enable True Tuning
/// </summary>
void TrueTuning::EnableTrueTuning()
{
	if (!tuningPatch)
		return;
	for (const CapturedSite& site : tuningSites.sites)
		LogOriginal(site.name, site.address, site.original);
	const auto result = tuningPatch->Rollback();
	const auto failureAddress = tuningPatch->FailureAddress();
	LogResult("Enable", tuningSites, result, failureAddress, true);
	if (result == MemUtil::PatchResult::Original) {
		tuningPatch.reset();
		tuningSites = {};
	}
}
