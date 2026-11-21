#include "../InstructionPatch.hpp"

#include <array>
#include <cassert>
#include <cstdint>

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
}
