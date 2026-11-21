#include "../TrampolinePolicy.hpp"
#include <cassert>
#include <cstdio>

using namespace Framework::Trampoline;

namespace {
	Instruction Plain(size_t offset, size_t length, unsigned char opcode = 0x8B, long extra = 0) {
		Instruction instruction{};
		instruction.offset = offset;
		instruction.length = length;
		instruction.opcode = opcode;
		instruction.extra = extra;
		return instruction;
	}

	Instruction Relative(size_t offset, size_t length, unsigned char opcode, long extra = 0) {
		Instruction instruction = Plain(offset, length, opcode, extra);
		instruction.hasTarget = true;
		return instruction;
	}
}

int main() {
	// Span length: at least a JMP rel32, at most the snapshot size.
	assert(!SpanLengthAllowed(4));
	assert(SpanLengthAllowed(5) && SpanLengthAllowed(kMaxSpan));
	assert(!SpanLengthAllowed(kMaxSpan + 1));

	// Boundary: an instruction that runs past the requested length is refused.
	assert(InstructionFits(0, 5, 5));
	assert(InstructionFits(4, 1, 5));
	assert(!InstructionFits(4, 2, 5));
	assert(!InstructionFits(6, 1, 5));
	assert(!InstructionFits(0, 0, 5));

	// Instructions with no relative operand are copied.
	assert(Classify(Plain(0, 1, 0x55)) == Verdict::Copy);
	assert(Classify(Plain(1, 2, 0x8B)) == Verdict::Copy);
	Instruction indirect = Relative(3, 6, 0xFF);
	indirect.dynamicTarget = true;
	assert(Classify(indirect) == Verdict::Copy);

	// Only a leading E8/E9 rel32 is relocated.
	assert(Classify(Relative(0, 5, 0xE8)) == Verdict::RelocateLeading);
	assert(Classify(Relative(0, 5, 0xE9)) == Verdict::RelocateLeading);
	// Relative call or jump after the first instruction.
	assert(Classify(Relative(1, 5, 0xE8)) == Verdict::Refuse);
	assert(Classify(Relative(2, 5, 0xE9)) == Verdict::Refuse);
	// Short jump and Jcc, which the decoder would widen, and Jcc rel32.
	assert(Classify(Relative(0, 2, 0xEB, 3)) == Verdict::Refuse);
	assert(Classify(Relative(0, 2, 0x74, 4)) == Verdict::Refuse);
	assert(Classify(Relative(0, 6, 0x0F)) == Verdict::Refuse);
	// Operand-size prefixed call (rel16).
	assert(Classify(Relative(0, 4, 0x66)) == Verdict::Refuse);
	// Any growth of a copied instruction.
	assert(Classify(Plain(0, 3, 0x8B, 1)) == Verdict::Refuse);
	indirect.extra = 2;
	assert(Classify(indirect) == Verdict::Refuse);
	assert(Classify(Plain(0, 0)) == Verdict::Refuse);
	assert(Classify(Plain(0, 16)) == Verdict::Refuse);

	// Leading targets.
	const std::uintptr_t source = 0x1000;
	assert(LeadingTargetAllowed(0xE8, 0x2000, source, 5));
	assert(LeadingTargetAllowed(0xE8, 0x0F00, source, 5));
	assert(!LeadingTargetAllowed(0xE8, source, source, 5));
	assert(!LeadingTargetAllowed(0xE8, source + 3, source, 5));
	// call $+5; pop reg
	assert(!LeadingTargetAllowed(0xE8, source + 5, source, 5));
	assert(!LeadingTargetAllowed(0xE8, source + 7, source, 7));
	assert(LeadingTargetAllowed(0xE8, source + 6, source, 5));
	assert(LeadingTargetAllowed(0xE9, source + 5, source, 5));
	assert(!LeadingTargetAllowed(0xE9, source + 4, source, 5));
	assert(!LeadingTargetAllowed(0xEB, 0x2000, source, 5));
	assert(!LeadingTargetAllowed(0xE8, 0, (std::numeric_limits<std::uintptr_t>::max)() - 2, 5));

	// Output bounds.
	assert(OutputFits(10, 8, 23, 5));
	assert(!OutputFits(10, 9, 23, 5));
	assert(!OutputFits(24, 0, 23, 0));
	puts("Trampoline policy ALL PASS");
}
