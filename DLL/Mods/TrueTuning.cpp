#include "../stdafx.h"
#include "TrueTuning.hpp"

// The reference frequency note detection is told to expect. Transposition is
// multiplication on a frequency, so scaling this by 2^(semitones/12) makes the game
// expect a signal that many semitones away from the song's written tuning.
//
// This is the same mechanism CDLC charters use to place notes outside Rocksmith's
// normal range: setting an arrangement's tuning pitch to 311.2 Hz instead of 440
// moves every expected note down six semitones (440 / 2^(6/12) = 311.13).
float ForcedTrueTuning = 440.0f;

// Both branches of the hook resolve to a fixed address for the life of the process,
// so they are looked up once at install time. Resolving them inside the hook meant a
// call and a single shared scratch global that both branches wrote before jumping
// through it - two threads in the hook at once and the loser jumps to the other
// branch's target.
uintptr_t forceTrueTuningAddress = 0;
uintptr_t jumpBackAddress = 0;

void TrueTuning::SetReferenceSemitones(int semitones)
{
	const float updated = 440.0f * powf(2.0f, (float)semitones / 12.0f);
	if (updated == ForcedTrueTuning)
	{
		return;
	}

	ForcedTrueTuning = updated;

	LOG_INFO("True tuning reference set to " << ForcedTrueTuning << " Hz for "
		<< semitones << " semitone(s)" << std::endl);
}

// Upstream additionally gated this hook behind CanDisableTrueTuning(), a check on
// GameState::currentMenu. That call was removed deliberately, not lost in the rework:
// it read a std::string the mod thread rewrites, from the game's tuning thread - a
// data race - and calling it from naked asm meant saving EAX/ECX/EDX around a __cdecl
// call whose bool return sets no flags, which produced a real branch-on-stale-flags
// bug in testing. No issue was reproducible without the menu check; the one concrete
// protection in this area, the CentOffset == -1200 bass case, is preserved below.
void __declspec(naked) disableTrueTuning()
{
	__asm
	{
		// We don't need to worry about replacing commands.
		// I placed this so we always call this code when doing true tuning calculations
		// Hence the change of 0x004DCCBF to 0xEB (JMP) from 0x74 (JE), so we can guarantee that we run without any issues.
		//
		// Nothing is saved or restored here because nothing is clobbered: EFLAGS is the
		// only thing this touches, and both jump targets begin by setting it themselves.

		cmp dword ptr [EBP + 0x8], -1200	// [EBP + 0x8] = CentOffset. If CentOffset == -1200, we need to use the true tuning or bass can break on some songs.
		jne trueTuningForceA440				// If this song does not have a CentOffset of -1200, then continue with forcing our reference.

		jmp forceTrueTuningAddress

		trueTuningForceA440:
		fld ForcedTrueTuning				// Set ST(0) to our reference. This tells note detection which tuning we want it to expect.
		jmp jumpBackAddress
	}
}

/// <summary>
/// Disable true tuning by telling note detection that it should use our reference as the base-point.
/// </summary>
void TrueTuning::DisableTrueTuning()
{
	forceTrueTuningAddress = Offsets::ptr_disableTrueTuning_forceTT.GetValue();
	jumpBackAddress = Offsets::ptr_disableTrueTuning_jmpBck.GetValue();

	if (forceTrueTuningAddress == 0 || jumpBackAddress == 0)
	{
		LOG_ERROR("True tuning not hooked because this Rocksmith version has no branch targets: forceTT 0x"
			<< std::hex << forceTrueTuningAddress << ", jmpBck 0x" << jumpBackAddress << std::dec << std::endl);
		return;
	}

	MemUtil::PatchAdr(Offsets::ptr_disableTrueTuningGate, "\xEB", 1); // Force a jump into our code, JMP.
	MemUtil::PlaceHook(Offsets::ptr_disableTrueTuning, disableTrueTuning, 6);

	LOG_INFO("Disabled true tuning, forceTT 0x" << std::hex << forceTrueTuningAddress
		<< ", jmpBck 0x" << jumpBackAddress << std::dec << std::endl);
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
