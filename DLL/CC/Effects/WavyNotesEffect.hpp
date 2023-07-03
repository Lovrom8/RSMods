#pragma once
#include "../CCEffect.hpp"
#include "../CCEffectList.hpp"

namespace CrowdControl::Effects {
	struct Vec3 {
		float z;
		float x;
		float y;
	};

	inline Vec3* vec;
	inline uintptr_t return_ptr = 0x007AE0D6;
	inline bool wavy_notes_enabled = false;

	inline void __declspec(naked) NotePositionHook()
	{
		/*
			007AE0AD | D900			| fld st(0),dword ptr ds:[eax]		| load z
			007AE0AF | D95C24 24	| fstp dword ptr ss:[esp+24],st(0)	| store z to tmp
			007AE0B3 | D940 04		| fld st(0),dword ptr ds:[eax+4]	| load x
			007AE0B6 | D95C24 28	| fstp dword ptr ss:[esp+28],st(0)	| store x to tmp
			007AE0BA | D940 08		| fld st(0),dword ptr ds:[eax+8]	| load y
			007AE0BD | D95C24 2C	| fstp dword ptr ss:[esp+2C],st(0)	| store y to tmp
			007AE0C1 | D94424 24	| fld st(0),dword ptr ss:[esp+24]	| load z from tmp
			007AE0C5 | D95B 70		| fstp dword ptr ds:[ebx+70],st(0)	| store z to object
			007AE0C8 | D94424 28	| fld st(0),dword ptr ss:[esp+28]	| load x from tmp
			007AE0CC | D95B 74		| fstp dword ptr ds:[ebx+74],st(0)	| store x to object
			007AE0CF | D94424 2C	| fld st(0),dword ptr ss:[esp+2C]	| load y from tmp
			007AE0D3 | D95B 78		| fstp dword ptr ds:[ebx+78],st(0)	| store y to object
		*/
		// Get the vector
		__asm {
			mov vec, eax
			pushad // save all registers
		}

		// Mess with the vector
		if(wavy_notes_enabled)
		{
			if (vec->y > -5.0f) {
				vec->y = vec->y + (sin(vec->z / 40.0f) * 4.0f);
			}
		}

		// Carry on as usual
		__asm {
			popad // restore all registers
			fld dword ptr[eax]
			fstp dword ptr[esp + 0x24]
			fld dword ptr[eax + 0x04]
			fstp dword ptr[esp + 0x28]
			fld dword ptr[eax + 0x08]
			fstp dword ptr[esp + 0x2C]
			fld dword ptr[esp + 0x24]
			fstp dword ptr[ebx + 0x70]
			fld dword ptr[esp + 0x28]
			fstp dword ptr[ebx + 0x74]
			fld dword ptr[esp + 0x2C]
			fstp dword ptr[ebx + 0x78]
			jmp[return_ptr]
		}
	}

	class WavyNotesEffect : public CCEffect
	{
	public:
		WavyNotesEffect(int64_t durationMilliseconds)
		{
			duration_ms = durationMilliseconds;

			incompatibleEffects = { "removenotes" };

			MemUtil::PlaceHook((void*)0x007AE0AD, NotePositionHook, 41);
		}

		EffectStatus Test(Request request) override;
		EffectStatus Start(Request request) override;
		EffectStatus Stop() override;
	};
}
