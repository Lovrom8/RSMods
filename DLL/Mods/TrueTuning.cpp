#include "../stdafx.h"
#include "TrueTuning.hpp"

const float ForcedTrueTuning = 440.0f;

/// <summary>
/// If the user is in some menus, we should not trigger this mod or some issues may arise.
/// </summary>
bool __cdecl CanDisableTrueTuning()
{
	if (GameState::currentMenu.size() < 1)
	{
		return false;
	}

	return GameState::currentMenu[0] != 83;
}

void __declspec(naked) disableTrueTuning()
{
	__asm
	{
		// We don't need to worry about replacing commands.
		// I placed this so we always call this code when doing true tuning calculations
		// Hence the change of 0x004DCCBF to 0xEB (JMP) from 0x74 (JE), so we can guarantee that we run without any issues.

		push EAX // Save EAX to the stack

		call CanDisableTrueTuning	// Check if we should disable true tuning on this song.
		je forceUserToTrueTune		// If we can't disable true tuning, then force the user to true tune.

		cmp [EBP + 0x8], -1200	// [EBP + 0x8] = CentOffset. If CentOffset == -1200, we need to use the true tuning or bass can break on some songs.
		jne trueTuningForceA440 // If this song does not have a CentOffset of -1200, then continue with forcing A440.

		forceUserToTrueTune:
		pop EAX										// Restore EAX from the stack

		pushad

		lea ecx, Offsets::ptr_disableTrueTuning_forceTT
		call VersioningStruct<uintptr_t>::GetValue
		mov Offsets::runtimeVersionStructValue, eax

		popad

		jmp Offsets::runtimeVersionStructValue

		trueTuningForceA440:
		pop EAX										// Restore EAX from the stack
		fld ForcedTrueTuning						// Set ST(0) to 440 | A440. This tells note detection that we want to use A440 as our true tuning.

		pushad

		lea ecx, Offsets::ptr_disableTrueTuning_jmpBck
		call VersioningStruct<uintptr_t>::GetValue
		mov Offsets::runtimeVersionStructValue, eax

		popad

		jmp Offsets::runtimeVersionStructValue
	}
}

/// <summary>
/// Disable true tuning by telling note detection that it should use A440 as the base-point.
/// </summary>
void TrueTuning::DisableTrueTuning()
{
	MemUtil::PatchAdr(Offsets::ptr_disableTrueTuningGate, "\xEB", 1); // Force a jump into our code, JMP.
	MemUtil::PlaceHook(Offsets::ptr_disableTrueTuning, disableTrueTuning, 6);

	LOG_INFO("Disabled true tuning" << std::endl);
}

/// <summary>
/// Re-enable True Tuning
/// </summary>
void TrueTuning::EnableTrueTuning()
{
	MemUtil::PatchAdr(Offsets::ptr_disableTrueTuningGate, "\x74", 1); // Change the jump back to a conditional jump, JE.
	MemUtil::PatchAdr(Offsets::ptr_disableTrueTuning, "\xD9\x05\x68\x44\x22\x01", 6);

	LOG_INFO("Enabled true tuning" << std::endl);
}