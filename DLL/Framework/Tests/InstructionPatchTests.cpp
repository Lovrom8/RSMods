#include "../InstructionPatch.hpp"
#include "../../LnpSites.hpp"

#include <algorithm>
#include <array>
#include <cassert>
#include <cstdint>
#include <map>
#include <vector>

namespace {
	using namespace Framework::InstructionPatch;
	enum class EngineResult { Installed, Original, RollbackIncomplete };

	class InjectedEngine {
	public:
		explicit InjectedEngine(std::map<std::uintptr_t, std::vector<std::uint8_t>>& memory) : memory_(memory) {}

		bool Append(const CapturedSite& site) {
			requests_.push_back({ site.address, site.original, site.replacement });
			return true;
		}

		EngineResult Commit() {
			if (attempted_)
				return EngineResult::Original;
			for (const auto& request : requests_)
				if (memory_[request.address] != request.original)
					return EngineResult::Original;
			for (const auto& request : requests_)
				memory_[request.address] = request.replacement;
			attempted_ = true;
			return EngineResult::Installed;
		}

		EngineResult Rollback() {
			if (!attempted_)
				return EngineResult::Original;
			for (const auto& request : requests_)
				if (memory_[request.address] != request.replacement)
					return EngineResult::RollbackIncomplete;
			for (const auto& request : requests_)
				memory_[request.address] = request.original;
			return EngineResult::Original;
		}

	private:
		struct Request {
			std::uintptr_t address;
			std::vector<std::uint8_t> original;
			std::vector<std::uint8_t> replacement;
		};
		std::map<std::uintptr_t, std::vector<std::uint8_t>>& memory_;
		std::vector<Request> requests_;
		bool attempted_ = false;
	};

	auto ReadMemory(std::map<std::uintptr_t, std::vector<std::uint8_t>>& memory) {
		return [&memory](std::uintptr_t address, auto& bytes) {
			const auto found = memory.find(address);
			if (found == memory.end() || found->second.size() != bytes.size())
				return false;
			std::copy(found->second.begin(), found->second.end(), bytes.begin());
			return true;
		};
	}
}

int main() {
	using Bytes3 = std::array<std::uint8_t, 3>;
	using Bytes6 = std::array<std::uint8_t, 6>;
	using Bytes8 = std::array<std::uint8_t, 8>;
	using Build = Framework::GameVersion::Build;

	const Bytes3 expected{ 0xE8, 0x10, 0x20 };
	const Bytes3 replacement{ 0x90, 0x90, 0x90 };
	const VersionedPattern<3> exactPattern{ ExactPattern(expected), ExactPattern(expected) };
	std::map<std::uintptr_t, std::vector<std::uint8_t>> memory{
		{ 0x1000, std::vector<std::uint8_t>(expected.begin(), expected.end()) },
		{ 0x2000, { 0x11, 0x22, 0x33 } },
	};
	auto read = ReadMemory(memory);

	const auto exactSite = MakeSite("exact", 0x1000, Build::September2022, exactPattern, replacement);
	auto capturedExact = CaptureSites(read, exactSite);
	assert(capturedExact.failure == ValidationFailure::None);
	assert(capturedExact.sites[0].original == std::vector<std::uint8_t>(expected.begin(), expected.end()));
	memory[0x1000][1] ^= 1;
	assert(CaptureSites(read, exactSite).failure == ValidationFailure::Mismatch);
	memory[0x1000] = std::vector<std::uint8_t>(expected.begin(), expected.end());

	std::vector<unsigned char> expanded;
	assert(LnpSites::Expand("DD 05 @E25C50", 0x1D0000, expanded));
	assert((expanded == std::vector<unsigned char>{ 0xDD, 0x05, 0x50, 0x5C, 0xFF, 0x00 }));
	Bytes6 lpExpected{};
	std::copy(expanded.begin(), expanded.end(), lpExpected.begin());
	const VersionedPattern<6> lpPattern{ UnavailablePattern<6>(), ExactPattern(lpExpected) };
	const auto lpSite = MakeSite("LP absolute operand", 0x3000, Build::LearnAndPlayDec2024,
		lpPattern, lpExpected);
	memory[0x3000] = std::vector<std::uint8_t>(lpExpected.begin(), lpExpected.end());
	assert(CaptureSites(read, lpSite).failure == ValidationFailure::None);
	assert(LnpSites::Expand("DD 05 @E25C50", 0x1E0000, expanded));
	assert(!std::equal(expanded.begin(), expanded.end(), lpExpected.begin()));

	const Bytes8 focusZero{ 0, 0, 0, 0, 4, 0, 0, 0 };
	const Bytes8 focusOne{ 1, 0, 0, 0, 4, 0, 0, 0 };
	const auto focusMask = MakeBytes(0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF);
	const VersionedPattern<8> focusPattern{
		MaskedPattern(focusZero, focusMask),
		MaskedPattern(focusOne, focusMask),
	};
	const auto focusSite = MakeSite("focus flag", 0x4000, Build::September2022, focusPattern, focusOne);
	memory[0x4000] = std::vector<std::uint8_t>(focusZero.begin(), focusZero.end());
	assert(CaptureSites(read, focusSite).failure == ValidationFailure::None);
	memory[0x4000] = std::vector<std::uint8_t>(focusOne.begin(), focusOne.end());
	assert(CaptureSites(read, focusSite).failure == ValidationFailure::None);
	memory[0x4000][4] = 3;
	assert(CaptureSites(read, focusSite).failure == ValidationFailure::Mismatch);
	memory[0x4000] = std::vector<std::uint8_t>(focusZero.begin(), focusZero.end());

	const Bytes3 secondExpected{ 0x11, 0x22, 0x33 };
	const Bytes3 secondReplacement{ 0x91, 0x92, 0x93 };
	const VersionedPattern<3> secondPattern{ ExactPattern(secondExpected), ExactPattern(secondExpected) };
	const auto firstSite = MakeSite("first", 0x1000, Build::September2022, exactPattern, replacement);
	const auto secondSite = MakeSite("second", 0x2000, Build::September2022, secondPattern, secondReplacement);
	for (int toggle = 0; toggle < 2; ++toggle) {
		const auto captured = CaptureSites(read, firstSite, secondSite);
		assert(captured.failure == ValidationFailure::None && captured.sites.size() == 2);
		InjectedEngine engine(memory);
		assert(AppendCaptured(captured, [&](const CapturedSite& site) { return engine.Append(site); }));
		assert(engine.Commit() == EngineResult::Installed);
		assert(memory[0x1000] == std::vector<std::uint8_t>(replacement.begin(), replacement.end()));
		assert(memory[0x2000] == std::vector<std::uint8_t>(secondReplacement.begin(), secondReplacement.end()));
		assert(engine.Rollback() == EngineResult::Original);
		assert(engine.Commit() == EngineResult::Original);
	}

	const auto capturedOwned = CaptureSites(read, firstSite, secondSite);
	InjectedEngine owner(memory);
	assert(AppendCaptured(capturedOwned, [&](const CapturedSite& site) { return owner.Append(site); }));
	assert(owner.Commit() == EngineResult::Installed);
	memory[0x2000][0] = 0xFF;
	const auto changedSecond = memory[0x2000];
	assert(owner.Rollback() == EngineResult::RollbackIncomplete);
	assert(memory[0x1000] == std::vector<std::uint8_t>(replacement.begin(), replacement.end()));
	assert(memory[0x2000] == changedSecond);

	std::array<std::uint8_t, 6> jump{};
	assert(BuildRelativeJump(0x1000, 0x2000, jump));
	assert((jump == Bytes6{ 0xE9, 0xFB, 0x0F, 0x00, 0x00, 0x90 }));
	if constexpr (sizeof(std::uintptr_t) > sizeof(std::uint32_t))
		assert(!BuildRelativeJump(0x1000, 0x90000000, jump));
}
