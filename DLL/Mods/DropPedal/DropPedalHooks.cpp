#include "../../stdafx.h"
#include "DropPedalHooks.hpp"
#include "DropPedalState.hpp"
#include "../TrueTuning.hpp"

namespace
{
	constexpr LONG MAX_PENDING_EVENTS = 64;

	// Vtable slot indices, confirmed by disassembly rather than declaration order.
	// Slot 1 takes three arguments and null-checks the second, which is SetParam.
	constexpr int SET_PARAM_VTABLE_INDEX = 1;

	// IAkPluginParam: 0 destructor, 1 SetParam, 2 Clone, 3 Init, 4 Term. Term is the
	// only notification we get that a param object is going away, and without it the
	// tracked pointers go stale on a tone switch and a live push writes into freed
	// memory. IsBadReadPtr does not catch that: freed memory usually stays mapped.
	constexpr int TERM_VTABLE_INDEX = 4;

	constexpr AkUInt32 PITCH_SHIFTER_PLUGIN_ID = 136;

	// The pitch shifter receives its knob values through SetParam, sent while the
	// effect is being built and again on every tone load and tone switch. Param 6
	// is the pitch, in cents: a MultiPitch pedal at -12 semitones arrives as -1200.
	constexpr AkUInt32 PITCH_PARAM_ID = 6;

	constexpr LONG MAX_PARAM_OBJECTS = 16;

	typedef void* (__cdecl* tCreateParamRaw)(void* allocator);

	// Virtual member functions are __thiscall on x86, which passes this in ECX.
	// __fastcall matches that once the unused EDX argument is declared explicitly.
	typedef AKRESULT(__fastcall* tSetParamRaw)(void* self, void* unused, AkUInt32 paramId, const void* value, AkUInt32 size);
	typedef void(__fastcall* tTermRaw)(void* self, void* unused, void* allocator);

	struct PitchOverrideEvent
	{
		float originalCents;
		float appliedCents;
	};

	tCreateParamRaw originalCreateParam = nullptr;
	tSetParamRaw originalSetParam = nullptr;
	tTermRaw originalTerm = nullptr;
	tRegisterPlugin originalRegisterPlugin = nullptr;

	bool isSetParamHooked = false;

	// When the ASIO input shifter owns pitch, all game-side application is suppressed:
	// the input signal is already retuned, so detection hears the shifted notes and the
	// tuner reference must stay at 440.
	bool inputShifterActive = false;
	bool hasReportedInputShifterUnavailable = false;
	bool hasCapturedSongTuning = false;
	unsigned long long engineNoticeTick = 0;

	// SetParam runs on bank and audio threads, so it must not log or take a lock:
	// an earlier build deadlocked against the game loop by doing both, and another
	// stalled the game by draining hundreds of verbose lines in a single frame.
	// It records a small event here, and Poll logs it.
	PitchOverrideEvent overrideEvents[MAX_PENDING_EVENTS] = {};
	volatile LONG overrideEventCount = 0;

	void* firstParamObject = nullptr;
	volatile LONG hasParamObject = 0;
	volatile LONG paramObjectCount = 0;

	// Objects the engine itself delivers pitch to. The create-param callback's return
	// value is not necessarily what a running effect reads: IAkPluginParam has Clone at
	// vtable slot 2, and writing to the original moved nothing. These are the objects
	// the engine drives, so they are the ones worth writing to.
	void* deliveredParamObjects[MAX_PARAM_OBJECTS] = {};
	float deliveredAuthoredCents[MAX_PARAM_OBJECTS] = {};
	volatile LONG deliveredParamObjectCount = 0;

	void* __cdecl SpyCreateParam(void* allocator)
	{
		void* paramObject = originalCreateParam(allocator);

		if (paramObject != nullptr)
		{
			InterlockedIncrement(&paramObjectCount);

			if (InterlockedCompareExchange(&hasParamObject, 1, 0) == 0)
			{
				firstParamObject = paramObject;
			}
		}

		return paramObject;
	}

	/// <summary>
	/// Replace the pitch the game delivers to a pitch shifter. The value is passed
	/// through a local, so the caller's own buffer is never written. Runs on bank and
	/// audio threads, so no logging and no locks.
	/// </summary>
	AKRESULT __fastcall SpySetParam(void* self, void* unused, AkUInt32 paramId, const void* value, AkUInt32 size)
	{
		if (paramId != PITCH_PARAM_ID || size != sizeof(float) || value == nullptr)
		{
			return originalSetParam(self, unused, paramId, value, size);
		}

		// Record the object the engine delivered to, so live pushes can target it.
		const float authoredCents = *(const float*)value;

		const LONG deliveredKnown = deliveredParamObjectCount;
		bool isAlreadyTracked = false;
		for (LONG i = 0; i < deliveredKnown && i < MAX_PARAM_OBJECTS; i++)
		{
			if (deliveredParamObjects[i] == self)
			{
				// Objects are reused across tone loads, so the baseline has to follow
				// the tone currently loaded into this one rather than the first tone
				// ever seen through it.
				deliveredAuthoredCents[i] = authoredCents;
				isAlreadyTracked = true;
				break;
			}
		}

		// The tone's own pitch is the baseline the player's shift moves from, so a tone
		// authored as an octave-down emulated bass stays a bass when it is dropped a
		// semitone. Tones authored at 0, which the setup instructions ask for, are
		// unaffected: their baseline is 0 and the shift is the whole value.
		if (!isAlreadyTracked)
		{
			bool isStored = false;

			// Slots freed by Term are reclaimed first, so the table tracks the objects
			// currently alive rather than the first MAX_PARAM_OBJECTS ever created.
			for (LONG i = 0; i < deliveredKnown && i < MAX_PARAM_OBJECTS; i++)
			{
				if (InterlockedCompareExchangePointer(&deliveredParamObjects[i], self, nullptr) == nullptr)
				{
					deliveredAuthoredCents[i] = authoredCents;
					isStored = true;
					break;
				}
			}

			if (!isStored)
			{
				const LONG slot = InterlockedIncrement(&deliveredParamObjectCount) - 1;
				if (slot < MAX_PARAM_OBJECTS)
				{
					deliveredParamObjects[slot] = self;
					deliveredAuthoredCents[slot] = authoredCents;
				}
				else
				{
					// Keep the counter at the table size so reclaim keeps working
					// instead of the counter drifting past every future null slot.
					InterlockedDecrement(&deliveredParamObjectCount);
				}
			}
		}

		if (!DropPedalState::IsEnabled() || inputShifterActive)
		{
			return originalSetParam(self, unused, paramId, value, size);
		}

		float appliedCents = authoredCents + DropPedalState::GetTargetCents();

		const LONG index = InterlockedIncrement(&overrideEventCount) - 1;
		if (index < MAX_PENDING_EVENTS)
		{
			overrideEvents[index] = { authoredCents, appliedCents };
		}

		return originalSetParam(self, unused, paramId, &appliedCents, size);
	}

	/// <summary>
	/// Forget a param object as the engine tears it down, so a later live push cannot
	/// write into freed memory. Runs on whichever thread destroys it, so no logging
	/// and no locks.
	/// </summary>
	void __fastcall SpyTerm(void* self, void* unused, void* allocator)
	{
		const LONG known = deliveredParamObjectCount < MAX_PARAM_OBJECTS
			? deliveredParamObjectCount
			: MAX_PARAM_OBJECTS;

		for (LONG i = 0; i < known; i++)
		{
			if (deliveredParamObjects[i] == self)
			{
				deliveredParamObjects[i] = nullptr;
				deliveredAuthoredCents[i] = 0.0f;
				break;
			}
		}

		originalTerm(self, unused, allocator);
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
			LOG_ERROR("Drop pedal failed to patch SetParam into the pitch shifter param vtable" << std::endl);
			return;
		}

		isSetParamHooked = true;

		originalTerm = (tTermRaw)vtable[TERM_VTABLE_INDEX];

		void* termReplacement = (void*)SpyTerm;
		if (!MemUtil::PatchAdr((LPVOID)&vtable[TERM_VTABLE_INDEX], (LPVOID)&termReplacement, sizeof(void*)))
		{
			originalTerm = nullptr;
			LOG_ERROR("Drop pedal failed to patch Term, so live pushes are disabled to "
				"avoid writing into torn down shifters" << std::endl);
		}

		LOG_INFO("Drop pedal hooked SetParam and is driving the pitch shifter, target "
			<< DropPedalState::GetTuningName() << std::endl);
	}

	AKRESULT __cdecl SpyRegisterPlugin(AkPluginType type, AkUInt32 companyId, AkUInt32 pluginId, AkCreatePluginCallback createFunc, AkCreateParamCallback createParamFunc)
	{
		if (companyId == 0 && pluginId == PITCH_SHIFTER_PLUGIN_ID)
		{
			const PBYTE paramTrampoline = DetourFunction((PBYTE)createParamFunc, (PBYTE)SpyCreateParam);
			if (paramTrampoline == nullptr)
			{
				LOG_ERROR("Drop pedal failed to hook the pitch shifter create-param callback" << std::endl);
			}
			else
			{
				originalCreateParam = (tCreateParamRaw)paramTrampoline;
			}
		}

		return originalRegisterPlugin(type, companyId, pluginId, createFunc, createParamFunc);
	}
}

void DropPedalHooks::Install()
{
	// The engine notice starts counting here. Automatic starts game-side until the
	// ASIO chain proves itself; a forced asio engine claims pitch immediately so the
	// game-side MultiPitch path never runs, even if the chain later fails to appear.
	engineNoticeTick = GetTickCount64();
	inputShifterActive = DropPedalState::IsAsioEngine();

	LOG_INFO("Drop pedal engine: "
		<< (inputShifterActive ? "ASIO Drop Pedal" : "Cable Drop Pedal") << std::endl);

	// Note detection reads the raw guitar signal, so the pitch shifter is invisible to
	// it. The game derives expected pitch from a reference frequency instead, which is
	// the same value CDLC charters set as an arrangement's tuning pitch. Redirecting it
	// is what keeps scoring in agreement with the strings the player is holding.
	TrueTuning::DisableTrueTuning();
	TrueTuning::SetReferenceSemitones(-DropPedalState::GetTargetSemitones());

	const uintptr_t target = Wwise::Exports::func_Wwise_Sound_RegisterPlugin.Get();
	originalRegisterPlugin = (tRegisterPlugin)DetourFunction((PBYTE)target, (PBYTE)SpyRegisterPlugin);

	if (originalRegisterPlugin == nullptr)
	{
		LOG_ERROR("Drop pedal failed to hook RegisterPlugin at 0x" << std::hex << target << std::dec << std::endl);
	}
}

void DropPedalHooks::Poll()
{
	HookSetParamOnce();

	// Kept in step every tick rather than only when the pitch changes, so the value
	// is already correct when a song loads. Detection appears to take its reference
	// at load time, which is why setting it mid-song has no effect.
	TrueTuning::SetReferenceSemitones((DropPedalState::IsEnabled() && !inputShifterActive)
		? -DropPedalState::GetTargetSemitones()
		: 0);
}

void DropPedalHooks::LogPendingOverrides()
{
	const LONG count = InterlockedExchange(&overrideEventCount, 0);
	const LONG usable = count < MAX_PENDING_EVENTS ? count : MAX_PENDING_EVENTS;

	for (LONG i = 0; i < usable; i++)
	{
		LOG_INFO("Drop pedal applied " << overrideEvents[i].appliedCents
			<< " cents in place of " << overrideEvents[i].originalCents << std::endl);
	}
}

/// <summary>
/// Write the current pitch straight into every live pitch shifter, the same way
/// the engine delivers it when one is built. Objects the engine has torn down
/// cannot be tracked, so each is sanity-checked before the call; a switch of
/// tone can still leave a brief window where one is gone, which is accepted.
/// </summary>
void DropPedalHooks::PushPitchToLiveShifters()
{
	// Without the Term hook there is no way to know an object has been freed, and
	// a push after a tone switch would write into released memory.
	if (originalSetParam == nullptr || originalTerm == nullptr)
	{
		return;
	}

	const LONG known = deliveredParamObjectCount < MAX_PARAM_OBJECTS
		? deliveredParamObjectCount
		: MAX_PARAM_OBJECTS;

	// Disabled pushes the authored pitch itself, which is what disengages the pedal
	// immediately instead of waiting for the next tone load to re-deliver it.
	const float shiftCents = DropPedalState::IsEnabled() ? DropPedalState::GetTargetCents() : 0.0f;

	for (LONG i = 0; i < known; i++)
	{
		void* paramObject = deliveredParamObjects[i];
		if (paramObject == nullptr || MemUtil::IsBadReadPtr(paramObject))
		{
			continue;
		}

		uintptr_t* vtable = *(uintptr_t**)paramObject;
		if (MemUtil::IsBadReadPtr(vtable))
		{
			continue;
		}

		// Each shifter moves from its own tone's authored pitch, the same baseline
		// the engine's own delivery is given.
		const float cents = deliveredAuthoredCents[i] + shiftCents;

		originalSetParam(paramObject, nullptr, PITCH_PARAM_ID, &cents, sizeof(float));
	}
}

void DropPedalHooks::SetInputShifterActive(bool active)
{
	// A forced engine wins over runtime arbitration: cable never hands pitch to the
	// input shifter, asio never hands it back to the game-side path.
	if (DropPedalState::IsCableEngine())
	{
		active = false;
	}
	else if (DropPedalState::IsAsioEngine())
	{
		active = true;
	}

	if (inputShifterActive == active) return;

	inputShifterActive = active;
	engineNoticeTick = GetTickCount64();
	LOG_INFO("Drop pedal engine: " << (active ? "ASIO Drop Pedal" : "Cable Drop Pedal") << std::endl);
}

bool DropPedalHooks::IsInputShifterActive()
{
	return inputShifterActive;
}

void DropPedalHooks::ReportInputShifterUnavailable()
{
	if (hasReportedInputShifterUnavailable) return;

	hasReportedInputShifterUnavailable = true;
	LOG_ERROR("Drop pedal Engine=asio but ASIO processing is not active. "
		"The drop pedal remains inactive until the ASIO input chain appears." << std::endl);
}

unsigned long long DropPedalHooks::GetEngineNoticeTick()
{
	return engineNoticeTick;
}

/// <summary>
/// Log the song's own tuning once per song, for diagnosing detection problems.
///
/// Writing this array was tried as a way to make detection expect the player's
/// tuning and had no effect, so it is left read only. The reference frequency in
/// TrueTuning is the mechanism that actually drives detection.
/// </summary>
void DropPedalHooks::HandleTuningInSong()
{
	if (hasCapturedSongTuning)
	{
		return;
	}

	const uintptr_t addrTuning = MemUtil::FindDMAAddy(Offsets::baseHandle + Offsets::ptr_tuning, Offsets::ptr_tuningOffsets, true);
	if (addrTuning == 0)
	{
		return;
	}

	const Tuning* tuning = reinterpret_cast<const Tuning*>(addrTuning);
	hasCapturedSongTuning = true;

	LOG_INFO("Drop pedal song tuning is "
		<< (int)(char)tuning->lowE << " " << (int)(char)tuning->strA << " "
		<< (int)(char)tuning->strD << " " << (int)(char)tuning->strG << " "
		<< (int)(char)tuning->strB << " " << (int)(char)tuning->highE
		<< ", drop pedal at " << DropPedalState::GetTargetSemitones() << std::endl);
}

void DropPedalHooks::ResetSongState()
{
	hasCapturedSongTuning = false;
}
