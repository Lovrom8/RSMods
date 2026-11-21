#pragma once

#include "../LnpSites.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <exception>

namespace Framework::ProfilePatchPolicy {
	inline constexpr unsigned kMaxSuspensionPasses = 4;
	enum class OperationMode : unsigned char { Inactive, Native, Replacement };

	constexpr bool CanRetrySuspension(unsigned completedPass) noexcept {
		return completedPass != 0 && completedPass < kMaxSuspensionPasses;
	}

	constexpr OperationMode CaptureOperationMode(bool enabled) noexcept {
		return enabled ? OperationMode::Replacement : OperationMode::Native;
	}

	class ModeLatch {
		OperationMode mode_ = OperationMode::Inactive;

	public:
		constexpr bool Begin(bool enabled) noexcept {
			mode_ = CaptureOperationMode(enabled);
			return Enabled();
		}

		constexpr bool Enabled() const noexcept {
			return mode_ == OperationMode::Replacement;
		}

		constexpr void End() noexcept {
			mode_ = OperationMode::Inactive;
		}
	};

	inline constexpr size_t kLoadTickInstructionSize = 7;

	inline bool BuildLearnAndPlayLoadTickExpected(uint32_t liveBase,
		std::array<unsigned char, kLoadTickInstructionSize>& expected) noexcept {
		try {
			std::vector<unsigned char> expanded;
			if (!LnpSites::Expand("83 3D @F75E94 00", liveBase, expanded)
				|| expanded.size() != expected.size())
				return false;
			std::copy(expanded.begin(), expanded.end(), expected.begin());
			return true;
		}
		catch (const std::exception&) {
			return false;
		}
	}
}
