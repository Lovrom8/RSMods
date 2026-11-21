#pragma once

#include "GameVersion.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <span>
#include <tuple>

namespace Framework::InstructionPatch {
	enum class Outcome {
		Installed,
		Original,
		RollbackIncomplete,
	};

	enum class Failure {
		None,
		Unavailable,
		Unreadable,
		Mismatch,
		NotOwned,
		NoCapturedOriginal,
		WriteRejected,
		WriteIndeterminate,
		RollbackFailed,
	};

	constexpr const char* FailureName(Failure failure) noexcept {
		switch (failure) {
		case Failure::Unavailable: return "site unavailable on this build";
		case Failure::Unreadable: return "site could not be read";
		case Failure::Mismatch: return "original bytes did not match";
		case Failure::NotOwned: return "site no longer holds our replacement";
		case Failure::NoCapturedOriginal: return "original bytes were not captured";
		case Failure::WriteRejected: return "write was not applied";
		case Failure::WriteIndeterminate: return "write outcome is indeterminate";
		case Failure::RollbackFailed: return "rollback outcome is indeterminate";
		default: return "no failure";
		}
	}

	struct Result {
		Outcome outcome = Outcome::Original;
		Failure failure = Failure::None;
		const char* site = nullptr;
		bool changed = false;
	};

	template <typename NativeOutcome>
	constexpr Outcome FromWriteResult(NativeOutcome result, NativeOutcome installed,
		NativeOutcome original, NativeOutcome rollbackIncomplete) noexcept {
		if (result == installed)
			return Outcome::Installed;
		if (result == original)
			return Outcome::Original;
		if (result == rollbackIncomplete)
			return Outcome::RollbackIncomplete;
		return Outcome::RollbackIncomplete;
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

	// A site's state changes only after the injected writer confirms the requested bytes.
	template <std::size_t Size>
	class PatchedSite {
	public:
		using Bytes = std::array<std::uint8_t, Size>;

		PatchedSite(const char* name, std::uintptr_t address, GameVersion::Build build,
			const VersionedPattern<Size>& patterns, std::size_t writeOffset = 0, std::size_t writeSize = Size) noexcept
			: name_(name), address_(address), writeOffset_(writeOffset), writeSize_(writeSize) {
			if (writeSize_ == 0 || writeOffset_ > Size || writeSize_ > Size - writeOffset_)
				address_ = 0;
			const BytePattern<Size>* selected = nullptr;
			switch (build) {
			case GameVersion::Build::September2022:
				selected = &patterns.september2022;
				break;
			case GameVersion::Build::LearnAndPlayDec2024:
				selected = &patterns.learnAndPlay;
				break;
			default:
				break;
			}
			if (!selected || !selected->available) {
				address_ = 0;
				return;
			}
			expected_ = selected->bytes;
			mask_ = selected->mask;
		}

		std::uintptr_t Address() const noexcept { return address_; }
		const char* Name() const noexcept { return name_; }
		const Result& LastResult() const noexcept { return lastResult_; }

		void SetReplacement(const Bytes& replacement) noexcept {
			replacement_ = replacement;
		}

		template <typename Read, typename Write, typename BeforeWrite>
		Result Install(Read&& read, Write&& write, const Bytes& replacement, BeforeWrite&& beforeWrite) {
			SetReplacement(replacement);
			if (!PrepareInstall(read))
				return lastResult_;
			return InstallPrepared(read, write, beforeWrite);
		}

		template <typename Read, typename Write, typename Transform, typename BeforeWrite>
		Result InstallTransform(Read&& read, Write&& write, Transform&& transform, BeforeWrite&& beforeWrite) {
			if (!PrepareInstall(read))
				return lastResult_;
			if (preparedAlreadyInstalled_)
				return lastResult_;
			SetReplacement(transform(preparedOriginal_));
			return InstallPrepared(read, write, beforeWrite);
		}

		template <typename Read, typename Write, typename BeforeWrite>
		Result Restore(Read&& read, Write&& write, BeforeWrite&& beforeWrite) {
			if (!PrepareRestore(read))
				return lastResult_;
			return RestorePrepared(read, write, beforeWrite);
		}

		template <typename Read>
		bool PrepareInstall(Read&& read) {
			prepared_ = false;
			lastChanged_ = false;
			lastRestored_ = false;
			if (!address_)
				return Fail(Outcome::Original, Failure::Unavailable);

			Bytes current{};
			if (!read(address_, current))
				return Fail(Outcome::Original, Failure::Unreadable);

			if (captured_ && current == replacement_) {
				preparedOriginal_ = original_;
				preparedAlreadyInstalled_ = true;
				prepared_ = true;
				lastResult_ = { Outcome::Installed, Failure::None, name_ };
				return true;
			}
			if (!MatchesMasked(current, expected_, mask_))
				return Fail(Outcome::Original, Failure::Mismatch);

			preparedOriginal_ = current;
			preparedAlreadyInstalled_ = false;
			prepared_ = true;
			lastResult_ = { Outcome::Original, Failure::None, name_ };
			return true;
		}

		template <typename Read>
		bool PrepareRestore(Read&& read) {
			prepared_ = false;
			lastChanged_ = false;
			lastRestored_ = false;
			if (!address_)
				return Fail(Outcome::Original, Failure::Unavailable);
			if (!captured_)
				return Fail(Outcome::Original, Failure::NoCapturedOriginal);

			Bytes current{};
			if (!read(address_, current))
				return Fail(Outcome::RollbackIncomplete, Failure::Unreadable);
			if (current != original_ && current != replacement_)
				return Fail(Outcome::Original, Failure::NotOwned);

			preparedOriginal_ = current;
			preparedAlreadyOriginal_ = current == original_;
			prepared_ = true;
			lastResult_ = { Outcome::Original, Failure::None, name_ };
			return true;
		}

		template <typename Read, typename Write, typename BeforeWrite>
		Result InstallPrepared(Read&& read, Write&& write, BeforeWrite&& beforeWrite) {
			if (!prepared_ || preparedAlreadyInstalled_)
				return lastResult_;

			Bytes current{};
			if (!read(address_, current))
				return FailResult(Outcome::Original, Failure::Unreadable);
			if (current != preparedOriginal_)
				return FailResult(Outcome::Original, Failure::Mismatch);

			original_ = preparedOriginal_;
			captured_ = true;
			beforeWrite(name_, address_, original_);
			const Outcome outcome = write(address_ + writeOffset_,
				std::span<const std::uint8_t>(replacement_.data() + writeOffset_, writeSize_));
			lastChanged_ = outcome != Outcome::Original;
			if (outcome == Outcome::Installed) {
				lastResult_ = { Outcome::Installed, Failure::None, name_, true };
				return lastResult_;
			}
			return FailResult(outcome, outcome == Outcome::Original ? Failure::WriteRejected : Failure::WriteIndeterminate);
		}

		template <typename Read, typename Write, typename BeforeWrite>
		Result RestorePrepared(Read&& read, Write&& write, BeforeWrite&& beforeWrite) {
			if (!prepared_)
				return lastResult_;
			if (preparedAlreadyOriginal_) {
				lastResult_ = { Outcome::Installed, Failure::None, name_ };
				return lastResult_;
			}

			Bytes current{};
			if (!read(address_, current))
				return FailResult(Outcome::RollbackIncomplete, Failure::Unreadable);
			if (current != replacement_)
				return FailResult(Outcome::Original, Failure::NotOwned);

			beforeWrite(name_, address_, original_);
			const Outcome outcome = write(address_ + writeOffset_,
				std::span<const std::uint8_t>(original_.data() + writeOffset_, writeSize_));
			lastChanged_ = outcome != Outcome::Installed;
			lastRestored_ = outcome == Outcome::Installed;
			if (outcome == Outcome::Installed) {
				lastResult_ = { Outcome::Installed, Failure::None, name_, true };
				return lastResult_;
			}
			return FailResult(outcome, outcome == Outcome::Original ? Failure::WriteRejected : Failure::WriteIndeterminate);
		}

		template <typename Read, typename Write>
		bool RollbackInstall(Read&& read, Write&& write) {
			if (!lastChanged_)
				return true;
			Bytes current{};
			if (!read(address_, current))
				return false;
			if (current == original_) {
				lastChanged_ = false;
				return true;
			}
			if (current != replacement_)
				return false;
			if (write(address_ + writeOffset_,
				std::span<const std::uint8_t>(original_.data() + writeOffset_, writeSize_)) != Outcome::Installed)
				return false;
			lastChanged_ = false;
			return true;
		}

		template <typename Read, typename Write>
		bool RollbackRestore(Read&& read, Write&& write) {
			if (!lastRestored_)
				return true;
			Bytes current{};
			if (!read(address_, current) || current != original_)
				return false;
			const Outcome outcome = write(address_ + writeOffset_,
				std::span<const std::uint8_t>(replacement_.data() + writeOffset_, writeSize_));
			if (outcome == Outcome::Installed) {
				lastRestored_ = false;
				return true;
			}
			return false;
		}

	private:
		Result FailResult(Outcome outcome, Failure failure) noexcept {
			prepared_ = false;
			lastResult_ = { outcome, failure, name_ };
			return lastResult_;
		}

		bool Fail(Outcome outcome, Failure failure) noexcept {
			prepared_ = false;
			lastResult_ = { outcome, failure, name_ };
			return false;
		}

		const char* name_ = nullptr;
		std::uintptr_t address_ = 0;
		std::size_t writeOffset_ = 0;
		std::size_t writeSize_ = Size;
		Bytes expected_{};
		Bytes mask_{};
		Bytes original_{};
		Bytes replacement_{};
		Bytes preparedOriginal_{};
		bool captured_ = false;
		bool prepared_ = false;
		bool preparedAlreadyInstalled_ = false;
		bool preparedAlreadyOriginal_ = false;
		bool lastChanged_ = false;
		bool lastRestored_ = false;
		Result lastResult_{};
	};

	namespace Detail {
		inline const auto NoopBeforeWrite = [](const char*, std::uintptr_t, const auto&) {};

		template <std::size_t Index = 0, typename Tuple>
		Result FirstFailure(Tuple& sites) {
			if constexpr (Index == std::tuple_size_v<Tuple>) {
				return { Outcome::Original, Failure::None, nullptr };
			}
			else {
				auto& site = std::get<Index>(sites);
				if (site.LastResult().failure != Failure::None)
					return site.LastResult();
				return FirstFailure<Index + 1>(sites);
			}
		}
	}

	template <typename Read, typename Write, typename BeforeWrite, typename... Sites>
	Result InstallGroup(Read&& read, Write&& write, BeforeWrite&& beforeWrite, Sites&... sites) {
		static_assert(sizeof...(Sites) > 0);
		std::tuple<Sites&...> group(sites...);
		const std::array<bool, sizeof...(Sites)> prepared{ sites.PrepareInstall(read)... };
		if (!std::all_of(prepared.begin(), prepared.end(), [](bool value) { return value; }))
			return Detail::FirstFailure(group);

		Result failure{};
		bool failed = false;
		bool changed = false;
		std::apply([&](auto&... site) {
			const auto applyOne = [&](auto& current) {
				if (failed)
					return;
				const Result result = current.InstallPrepared(read, write, beforeWrite);
				if (result.outcome != Outcome::Installed) {
					failure = result;
					failed = true;
				}
				changed = changed || result.changed;
			};
			(applyOne(site), ...);
		}, group);
		if (!failed)
			return { Outcome::Installed, Failure::None, nullptr, changed };

		bool rollbackComplete = true;
		std::apply([&](auto&... site) { ((rollbackComplete = site.RollbackInstall(read, write) && rollbackComplete), ...); }, group);
		return rollbackComplete
			? Result{ Outcome::Original, failure.failure, failure.site }
			: Result{ Outcome::RollbackIncomplete, Failure::RollbackFailed, failure.site, true };
	}

	template <typename Read, typename Write, typename BeforeWrite, typename... Sites>
	Result RestoreGroup(Read&& read, Write&& write, BeforeWrite&& beforeWrite, Sites&... sites) {
		static_assert(sizeof...(Sites) > 0);
		std::tuple<Sites&...> group(sites...);
		const std::array<bool, sizeof...(Sites)> prepared{ sites.PrepareRestore(read)... };
		if (!std::all_of(prepared.begin(), prepared.end(), [](bool value) { return value; }))
			return Detail::FirstFailure(group);

		Result failure{};
		bool failed = false;
		bool changed = false;
		std::apply([&](auto&... site) {
			const auto applyOne = [&](auto& current) {
				if (failed)
					return;
				const Result result = current.RestorePrepared(read, write, beforeWrite);
				if (result.outcome != Outcome::Installed) {
					failure = result;
					failed = true;
				}
				changed = changed || result.changed;
			};
			(applyOne(site), ...);
		}, group);
		if (!failed)
			return { Outcome::Installed, Failure::None, nullptr, changed };

		bool rollbackComplete = true;
		std::apply([&](auto&... site) { ((rollbackComplete = site.RollbackRestore(read, write) && rollbackComplete), ...); }, group);
		return rollbackComplete
			? Result{ Outcome::Original, failure.failure, failure.site }
			: Result{ Outcome::RollbackIncomplete, Failure::RollbackFailed, failure.site, true };
	}

	template <std::size_t Size, std::size_t ShapeSize>
	constexpr bool HasShape(const std::array<std::uint8_t, Size>& bytes,
		const std::array<std::uint8_t, ShapeSize>& shape) noexcept {
		static_assert(ShapeSize <= Size);
		for (std::size_t i = 0; i < ShapeSize; ++i)
			if (bytes[i] != shape[i])
				return false;
		return true;
	}

	template <std::size_t Size, std::size_t ShapeSize>
	constexpr bool CaptureOriginal(const std::array<std::uint8_t, Size>& bytes,
		const std::array<std::uint8_t, ShapeSize>& shape, std::array<std::uint8_t, Size>& original) noexcept {
		if (!HasShape(bytes, shape))
			return false;
		original = bytes;
		return true;
	}

	template <std::size_t Size>
	constexpr bool IsOwnedReplacement(bool captured, const std::array<std::uint8_t, Size>& current,
		const std::array<std::uint8_t, Size>& replacement) noexcept {
		return captured && current == replacement;
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
