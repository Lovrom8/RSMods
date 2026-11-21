#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>

namespace Framework::InstructionPatch {
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
