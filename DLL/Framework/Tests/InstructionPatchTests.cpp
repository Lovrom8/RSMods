#include "../InstructionPatch.hpp"

#include <algorithm>
#include <array>
#include <cassert>
#include <cstdint>
#include <map>
#include <vector>

int main() {
	using Framework::InstructionPatch::BuildRelativeJump;
	using Framework::InstructionPatch::CaptureOriginal;
	using Framework::InstructionPatch::HasShape;
	using Framework::InstructionPatch::IsOwnedReplacement;

	const std::array<std::uint8_t, 6> live{ 0xD9, 0x05, 0x00, 0x5D, 0xFF, 0x00 };
	const std::array<std::uint8_t, 2> shape{ 0xD9, 0x05 };
	std::array<std::uint8_t, 6> captured{};
	assert(HasShape(live, shape));
	assert(CaptureOriginal(live, shape, captured));
	assert(captured == live);

	const std::array<std::uint8_t, 6> invalid{ 0xD8, 0x05, 0x00, 0x5D, 0xFF, 0x00 };
	const std::array<std::uint8_t, 6> unchanged{ 1, 2, 3, 4, 5, 6 };
	captured = unchanged;
	assert(!CaptureOriginal(invalid, shape, captured));
	assert(captured == unchanged);

	const std::array<std::uint8_t, 6> replacement{ 0xE9, 1, 2, 3, 4, 0x90 };
	assert(IsOwnedReplacement(true, replacement, replacement));
	assert(!IsOwnedReplacement(false, replacement, replacement));
	assert(!IsOwnedReplacement(true, live, replacement));

	std::array<std::uint8_t, 6> jump{};
	assert(BuildRelativeJump(0x1000, 0x2000, jump));
	assert((jump == std::array<std::uint8_t, 6>{ 0xE9, 0xFB, 0x0F, 0x00, 0x00, 0x90 }));
	if constexpr (sizeof(std::uintptr_t) > sizeof(std::uint32_t))
		assert(!BuildRelativeJump(0x1000, 0x90000000, jump));

	using Framework::InstructionPatch::Failure;
	using Framework::InstructionPatch::InstallGroup;
	using Framework::InstructionPatch::MaskedPattern;
	using Framework::InstructionPatch::Outcome;
	using Framework::InstructionPatch::PatchedSite;
	using Framework::InstructionPatch::RestoreGroup;
	using Framework::InstructionPatch::VersionedPattern;

	using SiteBytes = std::array<std::uint8_t, 3>;
	const SiteBytes expected{ 0x10, 0x20, 0x30 };
	const SiteBytes mask{ 0xFF, 0x00, 0xFF };
	const SiteBytes siteReplacement{ 0x90, 0x90, 0x90 };
	const VersionedPattern<3> pattern{
		MaskedPattern(expected, mask),
		MaskedPattern(expected, mask)
	};
	std::map<std::uintptr_t, std::vector<std::uint8_t>> memory{
		{ 0x1000, { 0x10, 0x21, 0x30 } },
		{ 0x2000, { 0x10, 0x22, 0x30 } }
	};
	auto read = [&](std::uintptr_t address, auto& bytes) {
		const auto found = memory.find(address);
		if (found == memory.end() || found->second.size() != bytes.size())
			return false;
		std::copy(found->second.begin(), found->second.end(), bytes.begin());
		return true;
	};
	auto write = [&](std::uintptr_t address, const auto& bytes) {
		const auto found = memory.find(address);
		if (found == memory.end() || found->second.size() != bytes.size())
			return Outcome::RollbackIncomplete;
		std::copy(bytes.begin(), bytes.end(), found->second.begin());
		return Outcome::Installed;
	};
	const auto ignoreOriginal = [](const char*, std::uintptr_t, const auto&) {};
	PatchedSite<3> site("masked", 0x1000, Framework::GameVersion::Build::September2022, pattern);
	assert(site.Install(read, write, siteReplacement, ignoreOriginal).outcome == Outcome::Installed);
	assert(memory[0x1000] == std::vector<std::uint8_t>(siteReplacement.begin(), siteReplacement.end()));
	assert(site.Restore(read, write, ignoreOriginal).outcome == Outcome::Installed);
	assert((memory[0x1000] == std::vector<std::uint8_t>{ 0x10, 0x21, 0x30 }));

	memory[0x3000] = { 0x10, 0x23, 0x30 };
	PatchedSite<3> partialSite("single-byte write", 0x3000, Framework::GameVersion::Build::September2022, pattern, 1, 1);
	auto writeByte = [&](std::uintptr_t address, const auto& bytes) {
		if (address != 0x3001 || bytes.size() != 1)
			return Outcome::RollbackIncomplete;
		memory[0x3000][1] = bytes[0];
		return Outcome::Installed;
	};
	const SiteBytes partialReplacement{ 0x10, 0x99, 0x30 };
	assert(partialSite.Install(read, writeByte, partialReplacement, ignoreOriginal).outcome == Outcome::Installed);
	assert((memory[0x3000] == std::vector<std::uint8_t>{ 0x10, 0x99, 0x30 }));
	assert(partialSite.Restore(read, writeByte, ignoreOriginal).outcome == Outcome::Installed);
	assert((memory[0x3000] == std::vector<std::uint8_t>{ 0x10, 0x23, 0x30 }));

	PatchedSite<3> ownedSite("ownership", 0x1000, Framework::GameVersion::Build::September2022, pattern);
	assert(ownedSite.Install(read, write, siteReplacement, ignoreOriginal).outcome == Outcome::Installed);
	memory[0x1000][2] = 0x31;
	const auto changedBytes = memory[0x1000];
	const auto ownershipResult = ownedSite.Restore(read, write, ignoreOriginal);
	assert(ownershipResult.outcome == Outcome::Original);
	assert(ownershipResult.failure == Failure::NotOwned);
	assert(memory[0x1000] == changedBytes);
	memory[0x1000] = { 0x10, 0x21, 0x30 };

	PatchedSite<3> first("first", 0x1000, Framework::GameVersion::Build::September2022, pattern);
	PatchedSite<3> second("second", 0x2000, Framework::GameVersion::Build::September2022, pattern);
	first.SetReplacement(siteReplacement);
	second.SetReplacement(siteReplacement);
	memory[0x2000][0] = 0x11;
	int writes = 0;
	auto countWrite = [&](std::uintptr_t address, const auto& bytes) {
		++writes;
		return write(address, bytes);
	};
	const auto preflight = InstallGroup(read, countWrite, ignoreOriginal, first, second);
	assert(preflight.outcome == Outcome::Original);
	assert(preflight.failure == Failure::Mismatch);
	assert(writes == 0);
	memory[0x2000] = { 0x10, 0x22, 0x30 };

	first.SetReplacement(siteReplacement);
	second.SetReplacement(siteReplacement);
	auto failSecond = [&](std::uintptr_t address, const auto& bytes) {
		++writes;
		if (address == 0x2000)
			return Outcome::Original;
		return write(address, bytes);
	};
	writes = 0;
	const auto rolledBack = InstallGroup(read, failSecond, ignoreOriginal, first, second);
	assert(rolledBack.outcome == Outcome::Original);
	assert(rolledBack.failure == Failure::WriteRejected);
	assert((memory[0x1000] == std::vector<std::uint8_t>{ 0x10, 0x21, 0x30 }));
	assert((memory[0x2000] == std::vector<std::uint8_t>{ 0x10, 0x22, 0x30 }));

	memory[0x1000] = { 0x10, 0x21, 0x30 };
	first.SetReplacement(siteReplacement);
	second.SetReplacement(siteReplacement);
	auto failSecondRollback = [&](std::uintptr_t address, const auto& bytes) {
		++writes;
		if (address == 0x2000)
			return Outcome::Original;
		if (address == 0x1000 && bytes[0] == expected[0])
			return Outcome::RollbackIncomplete;
		return write(address, bytes);
	};
	const auto incomplete = InstallGroup(read, failSecondRollback, ignoreOriginal, first, second);
	assert(incomplete.outcome == Outcome::RollbackIncomplete);
	assert(incomplete.failure == Failure::RollbackFailed);

	first.SetReplacement(siteReplacement);
	second.SetReplacement(siteReplacement);
	assert(InstallGroup(read, write, ignoreOriginal, first, second).outcome == Outcome::Installed);
	memory[0x2000][0] = 0x91;
	writes = 0;
	const auto restorePreflight = RestoreGroup(read, countWrite, ignoreOriginal, first, second);
	assert(restorePreflight.outcome == Outcome::Original);
	assert(restorePreflight.failure == Failure::NotOwned);
	assert(writes == 0);
}
