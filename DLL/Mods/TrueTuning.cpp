#include "TrueTuning.hpp"

const float ForcedTrueTuning = 440.0f;

void __declspec(naked) disableTrueTuning()
{
	__asm
	{
		// We don't need to worry about replacing commands.
		// I placed this so we always call this code when doing true tuning calculations
		// Hence the change of 0x0149c5ee to 0xEB (JMP) from 0x74 (JE), so we can guarantee that we run without any issues.

		cmp [EBP + 0x8], -1200	// [EBP + 0x8] = CentOffset. If CentOffset == -1200, we need to use the true tuning or bass can break on some songs.
		jne trueTuningForceA440 // If this song does not have a CentOffset of -1200, then continue with forcing A440.

		jmp Offsets::ptr_disableTrueTuning_forceTT // Return to the code that sets the true tuning to the one specified in the arrangement. This is needed for some bass songs with a CentOffset of -1200.

		trueTuningForceA440:

		fld ForcedTrueTuning						// Set ST(0) to 440 | A440. This tells note detection that we want to use A440 as our true tuning.
		jmp Offsets::ptr_disableTrueTuning_jmpBck	// Return to the code we were running.
	}
}

/// <summary>
/// Disable true tuning by telling note detection that it should use A440 as the base-point.
/// </summary>
void TrueTuning::DisableTrueTuning()
{
	*(char*)0x0149c5ee = 0xEB; // Force a jump into our code, JMP.
	MemUtil::PlaceHook((void*)Offsets::ptr_disableTrueTuning, disableTrueTuning, 6);

	_LOG_INIT;
	_LOG("Disabled true tuning" << std::endl);
}

/// <summary>
/// Re-enable True Tuning
/// </summary>
void TrueTuning::EnableTrueTuning()
{
	*(char*)0x0149c5ee = 0x74; // Change the jump back to a conditional jump, JE.
	MemUtil::PatchAdr((void*)Offsets::ptr_disableTrueTuning, "\xD9\x05\x50\x19\x22\x01", 6);

	_LOG_INIT;
	_LOG("Enabled true tuning" << std::endl);
}