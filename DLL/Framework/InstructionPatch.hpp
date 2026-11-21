#pragma once

#include "GameVersion.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <utility>
#include <vector>

namespace Framework::InstructionPatch {
	enum class ValidationFailure {
		None,
		Unavailable,
		Unreadable,
		Mismatch,
	};

	constexpr const char* FailureName(ValidationFailure failure) noexcept {
		switch (failure) {
		case ValidationFailure::Unavailable: return "site unavailable on this build";
		case ValidationFailure::Unreadable: return "site could not be read";
		case ValidationFailure::Mismatch: return "original bytes did not match";
		default: return "no failure";
		}
	}

	template <typename... Bytes>
	constexpr auto MakeBytes(Bytes... bytes) noexcept {
		return std::array<std::uint8_t, sizeof...(Bytes)>{ static_cast<std::uint8_t>(bytes)... };
	}

	template <std::size_t Size>
	struct BytePattern {
		std::array<std::uint8_t, Size> bytes{};
		std::array<std::uint8_t, Size> mask{};
		bool available = true;
	};

	template <std::size_t Size>
	struct VersionedPattern {
		BytePattern<Size> september2022;
		BytePattern<Size> learnAndPlay;
	};

	template <std::size_t Size>
	constexpr BytePattern<Size> ExactPattern(const std::array<std::uint8_t, Size>& bytes) noexcept {
		BytePattern<Size> pattern{ bytes, {}, true };
		pattern.mask.fill(0xFF);
		return pattern;
	}

	template <std::size_t Size>
	constexpr BytePattern<Size> MaskedPattern(const std::array<std::uint8_t, Size>& bytes,
		const std::array<std::uint8_t, Size>& mask) noexcept {
		return { bytes, mask, true };
	}

	template <std::size_t Size>
	constexpr BytePattern<Size> UnavailablePattern() noexcept {
		return { {}, {}, false };
	}

	template <std::size_t Size>
	constexpr bool MatchesMasked(const std::array<std::uint8_t, Size>& actual,
		const std::array<std::uint8_t, Size>& expected, const std::array<std::uint8_t, Size>& mask) noexcept {
		for (std::size_t i = 0; i < Size; ++i)
			if ((actual[i] & mask[i]) != (expected[i] & mask[i]))
				return false;
		return true;
	}

	template <std::size_t Size>
	struct SiteDescriptor {
		using Bytes = std::array<std::uint8_t, Size>;
		using ReplacementBuilder = bool (*)(const Bytes&, Bytes&) noexcept;

		const char* name = nullptr;
		std::uintptr_t address = 0;
		GameVersion::Build build = GameVersion::Build::Unknown;
		VersionedPattern<Size> expected{};
		Bytes replacement{};
		ReplacementBuilder buildReplacement = nullptr;
	};

	template <std::size_t Size>
	SiteDescriptor<Size> MakeSite(const char* name, std::uintptr_t address, GameVersion::Build build,
		const VersionedPattern<Size>& expected, const std::array<std::uint8_t, Size>& replacement,
		typename SiteDescriptor<Size>::ReplacementBuilder buildReplacement = nullptr) noexcept {
		return { name, address, build, expected, replacement, buildReplacement };
	}

	struct CapturedSite {
		const char* name = nullptr;
		std::uintptr_t address = 0;
		std::vector<std::uint8_t> original;
		std::vector<std::uint8_t> replacement;
	};

	struct CaptureResult {
		std::vector<CapturedSite> sites;
		ValidationFailure failure = ValidationFailure::None;
		const char* site = nullptr;
	};

	template <std::size_t Size, typename Read>
	ValidationFailure CaptureSite(const SiteDescriptor<Size>& site, Read&& read, CapturedSite& captured) {
		if (!site.address)
			return ValidationFailure::Unavailable;

		const BytePattern<Size>* expected = nullptr;
		switch (site.build) {
		case GameVersion::Build::September2022:
			expected = &site.expected.september2022;
			break;
		case GameVersion::Build::LearnAndPlayDec2024:
			expected = &site.expected.learnAndPlay;
			break;
		default:
			break;
		}
		if (!expected || !expected->available)
			return ValidationFailure::Unavailable;

		typename SiteDescriptor<Size>::Bytes original{};
		if (!read(site.address, original))
			return ValidationFailure::Unreadable;
		if (!MatchesMasked(original, expected->bytes, expected->mask))
			return ValidationFailure::Mismatch;

		auto replacement = site.replacement;
		if (site.buildReplacement && !site.buildReplacement(original, replacement))
			return ValidationFailure::Unavailable;

		captured.name = site.name;
		captured.address = site.address;
		captured.original.assign(original.begin(), original.end());
		captured.replacement.assign(replacement.begin(), replacement.end());
		return ValidationFailure::None;
	}

	template <typename Read, typename... Sites>
	CaptureResult CaptureSites(Read&& read, const Sites&... sites) {
		CaptureResult captured;
		const auto captureOne = [&](const auto& site) {
			if (captured.failure != ValidationFailure::None)
				return;
			CapturedSite bytes;
			captured.failure = CaptureSite(site, read, bytes);
			if (captured.failure == ValidationFailure::None)
				captured.sites.push_back(std::move(bytes));
			else
				captured.site = site.name;
		};
		(captureOne(sites), ...);
		return captured;
	}

	template <typename Append>
	bool AppendCaptured(const CaptureResult& captured, Append&& append) {
		if (captured.failure != ValidationFailure::None)
			return false;
		for (const CapturedSite& site : captured.sites)
			if (!append(site))
				return false;
		return true;
	}

	inline const char* SiteNameAt(const CaptureResult& captured, std::uintptr_t address) noexcept {
		for (const CapturedSite& site : captured.sites)
			if (site.address == address)
				return site.name;
		return nullptr;
	}

	template <std::size_t Size>
	bool BuildRelativeJump(std::uintptr_t site, std::uintptr_t target,
		std::array<std::uint8_t, Size>& patch) noexcept {
		static_assert(Size >= 5);
		std::uint32_t encoded = 0;
		if constexpr (sizeof(std::uintptr_t) == sizeof(std::uint32_t))
			encoded = static_cast<std::uint32_t>(target - site - 5);
		else {
			const std::uint64_t next = static_cast<std::uint64_t>(site) + 5;
			const std::uint64_t destination = static_cast<std::uint64_t>(target);
			std::int64_t displacement = 0;
			if (destination >= next) {
				const std::uint64_t distance = destination - next;
				if (distance > static_cast<std::uint64_t>(std::numeric_limits<std::int32_t>::max()))
					return false;
				displacement = static_cast<std::int64_t>(distance);
			}
			else {
				const std::uint64_t distance = next - destination;
				if (distance > static_cast<std::uint64_t>(std::numeric_limits<std::int32_t>::max()) + 1)
					return false;
				displacement = -static_cast<std::int64_t>(distance);
			}
			encoded = static_cast<std::uint32_t>(static_cast<std::int32_t>(displacement));
		}

		patch.fill(0x90);
		patch[0] = 0xE9;
		for (std::size_t i = 0; i < sizeof(encoded); ++i)
			patch[i + 1] = static_cast<std::uint8_t>(encoded >> (i * 8));
		return true;
	}
}
