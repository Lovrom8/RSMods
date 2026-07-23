#include "../stdafx.h"
#include "DropPedal.hpp"

namespace
{
	constexpr LONG MAX_PENDING_EVENTS = 64;

	// Vtable slot indices, confirmed by disassembly rather than declaration order.
	// Slot 1 takes three arguments and null-checks the second, which is SetParam.
	constexpr int SET_PARAM_VTABLE_INDEX = 1;

	constexpr AkUInt32 PITCH_SHIFTER_PLUGIN_ID = 136;

	// The pitch shifter receives its knob values through SetParam, sent while the
	// effect is being built and again on every tone load and tone switch. Param 6
	// is the pitch, in cents: a MultiPitch pedal at -12 semitones arrives as -1200.
	constexpr AkUInt32 PITCH_PARAM_ID = 6;

	constexpr float CENTS_PER_SEMITONE = 100.0f;
	constexpr int MIN_TARGET_SEMITONES = -24;
	constexpr int MAX_TARGET_SEMITONES = 24;
	constexpr int SEMITONES_PER_OCTAVE = 12;

	// Saving to the ini and writing a log line are both disk work, and a player
	// holding a hotkey can step through tunings faster than that can keep up. The
	// displayed value moves immediately; the disk only sees the value they settled
	// on, once they have stopped for this long.
	constexpr ULONGLONG PERSIST_DELAY_MILLISECONDS = 400;

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
	// an earlier build deadlocked against the game loop by doing both, and another
	// stalled the game by draining hundreds of verbose lines in a single frame.
	// It records a small event here, and Poll logs it.
	PitchOverrideEvent overrideEvents[MAX_PENDING_EVENTS] = {};
	volatile LONG overrideEventCount = 0;

	void* firstParamObject = nullptr;
	volatile LONG hasParamObject = 0;

	// Every pitch shifter the engine builds passes through SpyCreateParam. Keeping
	// the objects lets a pitch change be written straight into the live effects
	// mid-song, without waiting for a tone load to deliver it.
	constexpr LONG MAX_PARAM_OBJECTS = 16;
	void* paramObjects[MAX_PARAM_OBJECTS] = {};
	volatile LONG paramObjectCount = 0;

	// Semitones the guitar has to move. Written by the game loop and read by
	// SetParam on other threads: a torn read of a float is impossible on x86, and
	// a briefly stale value is harmless.
	volatile float targetCents = 0.0f;

	// Enabled state lives in the session rather than in the settings map, because
	// the settings reload during boot and would wipe it. Starts on, so a session
	// never silently begins with the pedal off; F8 still turns it off for the
	// session. Read by SetParam on other threads.
	volatile bool isEnabledSession = true;

	bool wasLowerKeyDown = false;
	bool wasRaiseKeyDown = false;
	bool wasToggleKeyDown = false;

	bool isPersistPending = false;
	ULONGLONG lastTargetChangeTime = 0;

	void* __cdecl SpyCreateParam(void* allocator)
	{
		void* paramObject = originalCreateParam(allocator);

		if (paramObject != nullptr)
		{
			const LONG index = InterlockedIncrement(&paramObjectCount) - 1;
			if (index < MAX_PARAM_OBJECTS)
			{
				paramObjects[index] = paramObject;
			}

			if (InterlockedCompareExchange(&hasParamObject, 1, 0) == 0)
			{
				firstParamObject = paramObject;
			}
		}

		return paramObject;
	}

	/// <summary>
	/// Replace the pitch the game delivers to a pitch shifter. The value is passed
	/// through a local, so the caller's own buffer is never written.
	/// </summary>
	AKRESULT __fastcall SpySetParam(void* self, void* unused, AkUInt32 paramId, const void* value, AkUInt32 size)
	{
		if (paramId != PITCH_PARAM_ID || size != sizeof(float) || value == nullptr || !DropPedal::IsEnabled())
		{
			return originalSetParam(self, unused, paramId, value, size);
		}

		float appliedCents = targetCents;

		const LONG index = InterlockedIncrement(&overrideEventCount) - 1;
		if (index < MAX_PENDING_EVENTS)
		{
			overrideEvents[index] = { *(const float*)value, appliedCents };
		}

		return originalSetParam(self, unused, paramId, &appliedCents, size);
	}

	/// <summary>
	/// Write the current pitch straight into every live pitch shifter, the same way
	/// the engine delivers it when one is built. Objects the engine has torn down
	/// cannot be tracked, so each is sanity-checked before the call; a switch of
	/// tone can still leave a brief window where one is gone, which is accepted.
	/// </summary>
	void PushPitchToLiveShifters()
	{
		if (originalSetParam == nullptr)
		{
			return;
		}

		const float cents = targetCents;
		const LONG known = paramObjectCount < MAX_PARAM_OBJECTS ? paramObjectCount : MAX_PARAM_OBJECTS;
		LONG pushed = 0;

		for (LONG i = 0; i < known; i++)
		{
			void* paramObject = paramObjects[i];
			if (paramObject == nullptr || MemUtil::IsBadReadPtr(paramObject))
			{
				continue;
			}

			uintptr_t* vtable = *(uintptr_t**)paramObject;
			if (MemUtil::IsBadReadPtr(vtable))
			{
				continue;
			}

			originalSetParam(paramObject, nullptr, PITCH_PARAM_ID, &cents, sizeof(float));
			pushed++;
		}

		LOG_INFO("Drop pedal pushed " << cents << " cents into " << pushed << " live pitch shifter(s)" << std::endl);
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
		LOG_INFO("Drop pedal hooked SetParam and is driving the pitch shifter, target "
			<< DropPedal::GetTuningName() << std::endl);
	}

	void LogPendingOverrides()
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
	/// Move the target immediately, so the on-screen tuning tracks the player's key
	/// presses without lag, and mark it to be saved once they stop.
	/// </summary>
	void AdjustTarget(int semitoneDelta)
	{
		const int adjusted = DropPedal::GetTargetSemitones() + semitoneDelta;
		if (adjusted < MIN_TARGET_SEMITONES || adjusted > MAX_TARGET_SEMITONES)
		{
			return;
		}

		targetCents = adjusted * CENTS_PER_SEMITONE;
		isPersistPending = true;
		lastTargetChangeTime = GetTickCount64();
	}

	void PersistTargetWhenSettled()
	{
		if (!isPersistPending || GetTickCount64() - lastTargetChangeTime < PERSIST_DELAY_MILLISECONDS)
		{
			return;
		}

		isPersistPending = false;
		Settings::UpdateCustomSetting("DropPedalSemitones", DropPedal::GetTargetSemitones());

		LOG_INFO("Drop pedal target now " << DropPedal::GetTuningName() << std::endl);
		PushPitchToLiveShifters();
	}

	void ToggleEnabled()
	{
		isEnabledSession = !isEnabledSession;
		Settings::UpdateModSetting("DropPedalEnabled", isEnabledSession ? "on" : "off");

		if (isEnabledSession)
		{
			LOG_INFO("Drop pedal enabled, target " << DropPedal::GetTuningName() << std::endl);
			PushPitchToLiveShifters();
		}
		else
		{
			LOG_INFO("Drop pedal disabled, tones return to how the player configured them"
				<< " on the next tone load or tone switch" << std::endl);
		}
	}

	void HandleHotkeys()
	{
		const unsigned int toggleKey = Settings::GetKeyBind("DropPedalToggleKey");
		const bool isToggleKeyDown = toggleKey != 0 && (GetAsyncKeyState(toggleKey) & 0x8000) != 0;

		if (isToggleKeyDown && !wasToggleKeyDown)
		{
			ToggleEnabled();
		}

		wasToggleKeyDown = isToggleKeyDown;

		const unsigned int lowerKey = Settings::GetKeyBind("DropPedalDownKey");
		const unsigned int raiseKey = Settings::GetKeyBind("DropPedalUpKey");

		const bool isLowerKeyDown = lowerKey != 0 && (GetAsyncKeyState(lowerKey) & 0x8000) != 0;
		const bool isRaiseKeyDown = raiseKey != 0 && (GetAsyncKeyState(raiseKey) & 0x8000) != 0;

		if (isLowerKeyDown && !wasLowerKeyDown)
		{
			AdjustTarget(-1);
		}

		if (isRaiseKeyDown && !wasRaiseKeyDown)
		{
			AdjustTarget(1);
		}

		wasLowerKeyDown = isLowerKeyDown;
		wasRaiseKeyDown = isRaiseKeyDown;
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

bool DropPedal::IsEnabled()
{
	return isEnabledSession;
}

int DropPedal::GetTargetSemitones()
{
	return (int)(targetCents / CENTS_PER_SEMITONE);
}

/// <summary>
/// Name the tuning the current shift lands on, assuming the player is in E
/// standard, in the form a tuner would show it.
/// </summary>
std::string DropPedal::GetTuningName()
{
	static const char* tuningNames[SEMITONES_PER_OCTAVE] = {
		"E", "Eb", "D", "C#", "C", "B", "Bb", "A", "Ab", "G", "F#", "F"
	};

	const int semitones = GetTargetSemitones();
	if (semitones == 0)
	{
		return "E standard";
	}

	int index = semitones % SEMITONES_PER_OCTAVE;
	if (index < 0)
	{
		index = -index;
	}

	// The names are listed downwards from E, so a downward shift indexes straight
	// into them and an upward shift walks the same list backwards.
	const int nameIndex = semitones < 0 ? index : (SEMITONES_PER_OCTAVE - index) % SEMITONES_PER_OCTAVE;

	std::ostringstream name;
	name << tuningNames[nameIndex] << " standard (" << (semitones > 0 ? "+" : "") << semitones << ")";
	return name.str();
}

void DropPedal::InstallHooks()
{
	Settings::UpdateModSetting("DropPedalEnabled", "on");
	targetCents = (float)Settings::GetModSetting("DropPedalSemitones") * CENTS_PER_SEMITONE;

	const uintptr_t target = Wwise::Exports::func_Wwise_Sound_RegisterPlugin.Get();
	originalRegisterPlugin = (tRegisterPlugin)DetourFunction((PBYTE)target, (PBYTE)SpyRegisterPlugin);

	if (originalRegisterPlugin == nullptr)
	{
		LOG_ERROR("Drop pedal failed to hook RegisterPlugin at 0x" << std::hex << target << std::dec << std::endl);
	}
}

void DropPedal::Poll()
{
	HookSetParamOnce();
	HandleHotkeys();
	PersistTargetWhenSettled();
	LogPendingOverrides();
}
