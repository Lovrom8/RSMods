#pragma once

#include <cstddef>
#include <cstdint>
#include <limits>

namespace Framework::Trampoline {
	// Longest span a gateway copies; every reviewed hook site is shorter.
	constexpr size_t kMaxSpan = 32;
	// Bytes read past a span, so its last instruction (at most 15 bytes) decodes from the snapshot.
	constexpr size_t kDecodeSlack = 14;

	enum class Verdict { Copy, RelocateLeading, Refuse };

	// What the decoder reported for one instruction of the stolen span.
	struct Instruction {
		size_t offset = 0;
		size_t length = 0;
		unsigned char opcode = 0;
		bool hasTarget = false;
		bool dynamicTarget = false;
		long extra = 0;
	};

	constexpr bool SpanLengthAllowed(size_t length) noexcept {
		return length >= 5 && length <= kMaxSpan;
	}

	constexpr bool InstructionFits(size_t sourceOffset, size_t instructionLength,
		size_t limit) noexcept {
		return instructionLength != 0 && sourceOffset <= limit
			&& instructionLength <= limit - sourceOffset;
	}

	// An instruction with no target, or a dynamic one (indirect jump or call, return), has no
	// operand relative to its own address and is copied as is. The only relocation is a leading
	// E8/E9 rel32; every other relative branch, call or operand is refused.
	constexpr Verdict Classify(const Instruction& instruction) noexcept {
		if (instruction.length == 0 || instruction.length > 15)
			return Verdict::Refuse;
		if (!instruction.hasTarget || instruction.dynamicTarget)
			return instruction.extra == 0 ? Verdict::Copy : Verdict::Refuse;
		if (instruction.offset == 0 && instruction.length == 5 && instruction.extra == 0
			&& (instruction.opcode == 0xE8 || instruction.opcode == 0xE9))
			return Verdict::RelocateLeading;
		return Verdict::Refuse;
	}

	// A leading jump may not land in the bytes it replaces. A leading call may not land in them or
	// at their end: a call to the next instruction reads its own return address, which the gateway moves.
	constexpr bool LeadingTargetAllowed(unsigned char opcode, std::uintptr_t target,
		std::uintptr_t source, size_t spanLength) noexcept {
		if (spanLength > (std::numeric_limits<std::uintptr_t>::max)() - source)
			return false;
		const std::uintptr_t end = source + spanLength;
		if (opcode == 0xE8)
			return target < source || target > end;
		if (opcode == 0xE9)
			return target < source || target >= end;
		return false;
	}

	constexpr bool OutputFits(size_t outputOffset, size_t outputLength, size_t capacity,
		size_t trailingLength) noexcept {
		return outputOffset <= capacity && trailingLength <= capacity - outputOffset
			&& outputLength <= capacity - outputOffset - trailingLength;
	}
}
