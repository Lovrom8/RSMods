#pragma once

#include "../Framework/Framework.hpp"

class InputGainMod : public Framework::IMod {
public:
	MOD_ID(InputGainMod)
	Framework::SettingDefs Settings() const override;
	bool IsEnabled(const Framework::ModContext&) const override { return false; }
	static void OnProfileLoaded(unsigned int player);
};

namespace InputGain {
	// The getter takes the player in ECX and returns the float in ST0 with a plain RET.
	// Tail-call it so our cdecl caller owns the target argument on the stack.
	__declspec(naked) inline float __cdecl ReadPrimaryGain(uintptr_t) {
		__asm {
			xor ecx, ecx
			jmp dword ptr [esp + 4]
		}
	}
}
