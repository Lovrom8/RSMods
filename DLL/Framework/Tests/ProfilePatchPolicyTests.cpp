#include "../ProfilePatchPolicy.hpp"

#include <cassert>

int main() {
	using Framework::ProfilePatchPolicy::CanRetrySuspension;
	using Framework::ProfilePatchPolicy::BuildLearnAndPlayLoadTickExpected;
	using Framework::ProfilePatchPolicy::ModeLatch;
	using Framework::ProfilePatchPolicy::kMaxSuspensionPasses;
	using Framework::ProfilePatchPolicy::kLoadTickInstructionSize;

	for (unsigned pass = 1; pass < kMaxSuspensionPasses; ++pass)
		assert(CanRetrySuspension(pass));
	assert(!CanRetrySuspension(0));
	assert(!CanRetrySuspension(kMaxSuspensionPasses));
	assert(!CanRetrySuspension(kMaxSuspensionPasses + 1));

	ModeLatch mode;
	assert(!mode.Begin(false));
	assert(!mode.Enabled());
	assert(!mode.Enabled());
	mode.End();
	assert(mode.Begin(true));
	assert(mode.Enabled());
	mode.End();
	assert(!mode.Enabled());

	std::array<unsigned char, kLoadTickInstructionSize> expected{};
	assert(BuildLearnAndPlayLoadTickExpected(0x001D0000, expected));
	assert((expected == std::array<unsigned char, kLoadTickInstructionSize>{ 0x83, 0x3D, 0x94, 0x5E, 0x14, 0x01, 0x00 }));
	assert(BuildLearnAndPlayLoadTickExpected(0x2A000000, expected));
	assert((expected == std::array<unsigned char, kLoadTickInstructionSize>{ 0x83, 0x3D, 0x94, 0x5E, 0xF7, 0x2A, 0x00 }));
}
