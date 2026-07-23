#include "../stdafx.h"
#include "RtpcProbe.hpp"

namespace
{
	constexpr LONG MAX_PENDING_EVENTS = 64;

	// Vtable slot indices, confirmed by disassembly rather than declaration order.
	// Slot 3 takes three arguments and tail-calls vtable[5] with the last two,
	// which is Init handing its params block to SetParamsBlock. Slot 2 takes one
	// argument and allocates through it, which is Clone. Slot 1 takes three and
	// null-checks the second, which is SetParam.
	constexpr int SET_PARAM_VTABLE_INDEX = 1;

	constexpr AkUInt32 PITCH_SHIFTER_PLUGIN_ID = 136;

	// The Pitch Shifter receives its knob values through SetParam, re-sent on every
	// tone load and tone switch. Param 6 is the pitch, in cents: the Tone Designer
	// MultiPitch pedal set to -12 semitones arrives as exactly -1200.
	constexpr AkUInt32 PITCH_PARAM_ID = 6;

	constexpr float CENTS_PER_KEYPRESS = 100.0f;
	constexpr float MIN_TARGET_CENTS = -2400.0f;
	constexpr float MAX_TARGET_CENTS = 2400.0f;

	typedef void* (__cdecl* tCreateParamRaw)(void* allocator);

	// Virtual member functions are __thiscall on x86, which passes this in ECX.
	// __fastcall matches that once the unused EDX argument is declared explicitly.
	typedef AKRESULT(__fastcall* tSetParamRaw)(void* self, void* unused, AkUInt32 paramId, const void* value, AkUInt32 size);

	struct PitchOverrideEvent
	{
		float originalCents;
		float appliedCents;
	};

	tCreateParamRaw originalCreateParam = nullptr;
	tSetParamRaw originalSetParam = nullptr;
	tRegisterPlugin originalRegisterPlugin = nullptr;

	bool isSetParamHooked = false;

	// SetParam runs on bank and audio threads, so it must not log or take a lock:
	// an earlier build deadlocked against Poll by doing both, and a later one
	// stalled the game by draining hundreds of verbose lines in a single frame.
	// It records a small event here, and Poll logs it.
	PitchOverrideEvent overrideEvents[MAX_PENDING_EVENTS] = {};
	volatile LONG overrideEventCount = 0;

	void* firstParamObject = nullptr;
	volatile LONG hasParamObject = 0;

	// Starts at zero so the pitch pedal is neutralised on the first tone load. A
	// MultiPitch pedal set to -12 semitones will sound completely unshifted, which
	// is an unmistakable result to listen for. Written by the game loop and read by
	// SetParam on other threads: a torn read of a float is impossible on x86, and a
	// briefly stale value is harmless.
	volatile float targetCents = 0.0f;

	bool wasLowerKeyDown = false;
	bool wasRaiseKeyDown = false;

	void* __cdecl SpyCreateParam(void* allocator)
	{
		void* paramObject = originalCreateParam(allocator);

		if (paramObject != nullptr && InterlockedCompareExchange(&hasParamObject, 1, 0) == 0)
		{
			firstParamObject = paramObject;
		}

		return paramObject;
	}

	/// <summary>
	/// Replace the pitch the game is delivering to a pitch pedal. The value is
	/// passed through a local so the caller's own buffer is never written, and
	/// only a pedal that is already shifting is touched, so tones without an
	/// active pitch pedal are left exactly as the player configured them.
	/// </summary>
	AKRESULT __fastcall SpySetParam(void* self, void* unused, AkUInt32 paramId, const void* value, AkUInt32 size)
	{
		if (paramId != PITCH_PARAM_ID || size != sizeof(float) || value == nullptr)
		{
			return originalSetParam(self, unused, paramId, value, size);
		}

		const float originalCents = *(const float*)value;
		if (originalCents == 0.0f)
		{
			return originalSetParam(self, unused, paramId, value, size);
		}

		float appliedCents = targetCents;

		const LONG index = InterlockedIncrement(&overrideEventCount) - 1;
		if (index < MAX_PENDING_EVENTS)
		{
			overrideEvents[index] = { originalCents, appliedCents };
		}

		return originalSetParam(self, unused, paramId, &appliedCents, size);
	}

	void HookSetParamOnce()
	{
		if (isSetParamHooked || InterlockedCompareExchange(&hasParamObject, 1, 1) == 0)
		{
			return;
		}

		if (MemUtil::IsBadReadPtr(firstParamObject))
		{
			return;
		}

		uintptr_t* vtable = *(uintptr_t**)firstParamObject;
		if (MemUtil::IsBadReadPtr(vtable))
		{
			return;
		}

		originalSetParam = (tSetParamRaw)vtable[SET_PARAM_VTABLE_INDEX];

		void* replacement = (void*)SpySetParam;
		if (!MemUtil::PatchAdr((LPVOID)&vtable[SET_PARAM_VTABLE_INDEX], (LPVOID)&replacement, sizeof(void*)))
		{
			LOG_ERROR("DROP PEDAL failed to patch SetParam into the pitch shifter param vtable" << std::endl);
			return;
		}

		isSetParamHooked = true;
		LOG_INFO("DROP PEDAL SetParam hooked, target pitch " << targetCents << " cents" << std::endl);
	}

	void LogPendingOverrides()
	{
		const LONG count = InterlockedExchange(&overrideEventCount, 0);
		const LONG usable = count < MAX_PENDING_EVENTS ? count : MAX_PENDING_EVENTS;

		for (LONG i = 0; i < usable; i++)
		{
			LOG_INFO("DROP PEDAL pedal asked for " << overrideEvents[i].originalCents
				<< " cents, applied " << overrideEvents[i].appliedCents << std::endl);
		}
	}

	void HandleHotkeys()
	{
		const bool isLowerKeyDown = (GetAsyncKeyState(VK_F9) & 0x8000) != 0;
		const bool isRaiseKeyDown = (GetAsyncKeyState(VK_F10) & 0x8000) != 0;

		bool didChange = false;

		if (isLowerKeyDown && !wasLowerKeyDown && targetCents > MIN_TARGET_CENTS)
		{
			targetCents = targetCents - CENTS_PER_KEYPRESS;
			didChange = true;
		}

		if (isRaiseKeyDown && !wasRaiseKeyDown && targetCents < MAX_TARGET_CENTS)
		{
			targetCents = targetCents + CENTS_PER_KEYPRESS;
			didChange = true;
		}

		if (didChange)
		{
			LOG_INFO("DROP PEDAL target now " << targetCents << " cents ("
				<< (targetCents / 100.0f) << " semitones), applies on the next tone load or tone switch" << std::endl);
		}

		wasLowerKeyDown = isLowerKeyDown;
		wasRaiseKeyDown = isRaiseKeyDown;
	}

	AKRESULT __cdecl SpyRegisterPlugin(AkPluginType type, AkUInt32 companyId, AkUInt32 pluginId, AkCreatePluginCallback createFunc, AkCreateParamCallback createParamFunc)
	{
		if (companyId == 0 && pluginId == PITCH_SHIFTER_PLUGIN_ID)
		{
			const PBYTE trampoline = DetourFunction((PBYTE)createParamFunc, (PBYTE)SpyCreateParam);
			if (trampoline == nullptr)
			{
				LOG_ERROR("DROP PEDAL failed to hook the pitch shifter create-param callback" << std::endl);
			}
			else
			{
				originalCreateParam = (tCreateParamRaw)trampoline;
			}
		}

		return originalRegisterPlugin(type, companyId, pluginId, createFunc, createParamFunc);
	}
}

void RtpcProbe::InstallHooks()
{
	const uintptr_t target = Wwise::Exports::func_Wwise_Sound_RegisterPlugin.Get();
	originalRegisterPlugin = (tRegisterPlugin)DetourFunction((PBYTE)target, (PBYTE)SpyRegisterPlugin);

	if (originalRegisterPlugin == nullptr)
	{
		LOG_ERROR("DROP PEDAL failed to hook RegisterPlugin at 0x" << std::hex << target << std::dec << std::endl);
		return;
	}

	LOG_INFO("=== Drop pedal build 13: pitch pedals forced to 0 cents, F9 and F10 adjust by a semitone ===" << std::endl);
}

void RtpcProbe::Poll()
{
	HookSetParamOnce();
	LogPendingOverrides();
	HandleHotkeys();
}
