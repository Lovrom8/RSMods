#include "../stdafx.h"
#include "UltrawideRRDim.hpp"

namespace {
	// Decoded from the September 2022 remaster; the state id offset is the one the game reads
	// in LoftManager::PushLoftState (0x00775550) at +0x14.
	constexpr unsigned int off_state_id = 0x14;         // GRLoftState +0x14: state ID
	constexpr unsigned int off_manager_fromState = 0x0C; // LoftPostEffectsManager +0x0C: the state a transition starts from
	constexpr unsigned int riffRepeaterStateId = 0x13;

	constexpr unsigned char opcode_callRel32 = 0xE8;
	constexpr size_t callLength = 5;

	// The gate the naked thunk reads. A plain aligned LONG published with Interlocked
	// operations rather than std::atomic, because inline assembly must not depend on the
	// layout of a library type. Nonzero: substitution allowed.
	__declspec(align(4)) volatile LONG gateActive = 0;

	// Resolved once by Install(). The thunk tail-jumps through this so the original
	// function runs with the same stack it was called with and returns straight to the
	// game's call site.
	uintptr_t originalTarget = 0;

	bool installed = false;
	unsigned char originalBytes[callLength] = {};

	#ifdef _DEBUG
	// One line per SetLoftState call, which runs once per transition.
	void Report(const char* outcome, void* requested, unsigned int requestedId, void* substitute, unsigned int substituteId) {
		LOG_INFO("(ULTRAWIDE RR) " << outcome
			<< " requested=" << std::hex << reinterpret_cast<uintptr_t>(requested) << " id=" << std::dec << requestedId
			<< " substitute=" << std::hex << reinterpret_cast<uintptr_t>(substitute) << " id=" << std::dec << substituteId
			<< std::endl);
	}
	#else
	inline void Report(const char*, void*, unsigned int, void*, unsigned int) {}
	#endif

	// Return the state the original function should receive. Any failed read preserves the
	// game's requested state.
	void* __cdecl Decide(void* requested, void* manager) {
		if (InterlockedCompareExchange(&gateActive, 0, 0) == 0)
			return requested;

		unsigned int requestedId = ~0u;
		if (!MemUtil::TryRead(reinterpret_cast<uintptr_t>(requested) + off_state_id, requestedId))
			return requested;
		if (requestedId != riffRepeaterStateId)
			return requested;

		void* previous = nullptr;
		if (!MemUtil::TryRead(reinterpret_cast<uintptr_t>(manager) + off_manager_fromState, previous) || !previous) {
			Report("passthrough, no previous state", requested, requestedId, previous, ~0u);
			return requested;
		}

		unsigned int previousId = ~0u;
		if (!MemUtil::TryRead(reinterpret_cast<uintptr_t>(previous) + off_state_id, previousId)) {
			Report("passthrough, previous state unreadable", requested, requestedId, previous, ~0u);
			return requested;
		}
		if (previous == requested || previousId > 0xFF || previousId == riffRepeaterStateId) {
			Report("passthrough, previous state is already RR", requested, requestedId, previous, previousId);
			return requested;
		}

		Report("substituted", requested, requestedId, previous, previousId);
		return previous;
	}

	// Preserve the original custom ABI. After pushfd and pushad, the manager is at
	// [ESP+0x28]; writing Decide's result into saved EAX supplies the substituted state.
	void __declspec(naked) Thunk() {
		__asm {
			pushfd
			pushad
			mov ecx, [esp + 0x28]
			push ecx
			push eax
			call Decide
			add esp, 8
			mov [esp + 0x1C], eax
			popad
			popfd
			jmp dword ptr [originalTarget]
		}
	}

	// Require a CALL rel32 that resolves to the expected target.
	bool ValidateSite(uintptr_t site, uintptr_t expectedTarget) {
		unsigned char opcode = 0;
		int32_t rel = 0;
		if (!MemUtil::TryRead(site, opcode) || !MemUtil::TryRead(site + 1, rel))
			return false;
		if (opcode != opcode_callRel32)
			return false;
		const intptr_t destination = static_cast<intptr_t>(site + callLength)
			+ static_cast<intptr_t>(rel);
		return destination == static_cast<intptr_t>(expectedTarget);
	}

	bool ValidateOriginal(uintptr_t site, uintptr_t target) {
		static constexpr unsigned char expectedTarget[] = {
			0x55, 0x8B, 0xEC, 0x53, 0x8B, 0x5D, 0x08, 0x83,
			0x7B, 0x0C, 0x00, 0x56, 0x57, 0x8B, 0xF0
		};
		static constexpr unsigned char expectedReturn[] = { 0xC2, 0x08, 0x00 };

		return ValidateSite(site, target)
			&& MemUtil::MatchesBytes(target, expectedTarget)
			&& MemUtil::MatchesBytes(target + 0x2A9, expectedReturn);
	}
}

void UltrawideRRDim::Install() {
	if (installed)
		return;

	const uintptr_t site = Offsets::hook_loftPostFxSetState.GetValue();
	const uintptr_t target = Offsets::func_loftPostFxSetState.GetValue();
	if (!site || !target) {
		LOG_WARNING("(ULTRAWIDE RR) not installed: offsets not ported for this version" << std::endl);
		return;
	}

	if (!ValidateOriginal(site, target)) {
		LOG_WARNING("(ULTRAWIDE RR) not installed: bytes at 0x" << std::hex << site
			<< " are not a CALL to 0x" << target << std::dec << ", site already modified or wrong build" << std::endl);
		return;
	}

	const intptr_t displacement = static_cast<intptr_t>(reinterpret_cast<uintptr_t>(&Thunk)) - static_cast<intptr_t>(site + callLength);
	if (displacement != static_cast<int32_t>(displacement)) {
		LOG_WARNING("(ULTRAWIDE RR) not installed: thunk out of rel32 range" << std::endl);
		return;
	}
	const int32_t rel = static_cast<int32_t>(displacement);

	memcpy(originalBytes, reinterpret_cast<const void*>(site), callLength);
	originalTarget = target;
	if (!MemUtil::PatchAdr(site + 1, &rel, sizeof(rel), false)) {
		LOG_ERROR("(ULTRAWIDE RR) not installed: patch failed at 0x" << std::hex << site << std::dec << std::endl);
		return;
	}
	if (!ValidateSite(site, reinterpret_cast<uintptr_t>(&Thunk))) {
		LOG_ERROR("(ULTRAWIDE RR) not installed: redirected CALL did not verify, restoring original bytes"
			<< std::endl);
		MemUtil::PatchAdr(site, originalBytes, callLength, false);
		return;
	}

	installed = true;
	LOG_INFO("(ULTRAWIDE RR) installed: CALL at 0x" << std::hex << site << " redirected from 0x" << target
		<< " to thunk 0x" << reinterpret_cast<uintptr_t>(&Thunk) << std::dec << std::endl);
}

void UltrawideRRDim::Uninstall() {
	if (!installed)
		return;
	InterlockedExchange(&gateActive, 0);
	const uintptr_t site = Offsets::hook_loftPostFxSetState.GetValue();
	if (MemUtil::MatchesBytes(site, originalBytes)) {
		installed = false;
		return;
	}
	if (!ValidateSite(site, reinterpret_cast<uintptr_t>(&Thunk))) {
		LOG_ERROR("(ULTRAWIDE RR) restore refused at 0x" << std::hex << site << std::dec
			<< ": CALL is no longer owned by this mod" << std::endl);
		return;
	}
	if (MemUtil::PatchAdr(site, originalBytes, callLength, false)) {
		installed = false;
	}
	else
		LOG_ERROR("(ULTRAWIDE RR) restore failed at 0x" << std::hex << site << std::dec << std::endl);
}

void UltrawideRRDim::SetActive(bool active) {
	InterlockedExchange(&gateActive, active ? 1 : 0);
}
