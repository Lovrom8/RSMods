#include "../stdafx.h"
#include "DropPedal.hpp"
#include "TrueTuning.hpp"


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

	constexpr float CENTS_PER_SEMITONE = 100.0f;
	constexpr int MIN_TARGET_SEMITONES = -24;
	constexpr int MAX_TARGET_SEMITONES = 24;
	constexpr int MIN_BASE_TUNING_SEMITONES = -11;
	constexpr int MAX_BASE_TUNING_SEMITONES = 11;
	constexpr int SEMITONES_PER_OCTAVE = 12;

	// V1 does not support rebinding. The settings app rebuilds RSMods.ini from its own
	// table of known keys and truncates the file first, so a hand-added binding is
	// discarded the next time the player saves anything from it — silently, and back to
	// the default rather than to nothing, which is the worse of the two failures. Fixed
	// bindings are the only ones that cannot quietly change under the player. Move these
	// into [Keybinds] once the settings app carries them.
	constexpr int PITCH_DOWN_KEY = VK_OEM_COMMA;
	constexpr int PITCH_UP_KEY = VK_OEM_PERIOD;
	constexpr int TOGGLE_KEY = VK_F8;
	constexpr int BASE_TUNING_DOWN_KEY = VK_F9;
	constexpr int BASE_TUNING_UP_KEY = VK_F10;

	// Holding a hotkey can step through tunings faster than the engine can absorb
	// parameter writes: pushing on every press floods the audio path from the game
	// thread and briefly locks the game. The displayed tuning still moves on every
	// press; only the write to the shifters waits for the player to settle.
	constexpr ULONGLONG PITCH_PUSH_DELAY_MILLISECONDS = 150;

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

	// SetParam runs on bank and audio threads, so it must not log or take a lock:
	// an earlier build deadlocked against the game loop by doing both, and another
	// stalled the game by draining hundreds of verbose lines in a single frame.
	// It records a small event here, and Poll logs it.
	PitchOverrideEvent overrideEvents[MAX_PENDING_EVENTS] = {};
	volatile LONG overrideEventCount = 0;

	void* firstParamObject = nullptr;
	volatile LONG hasParamObject = 0;

	// Bounds the tracked shifter arrays, and numbers each shifter as it is built so a
	// create-param event can say which one it came from.
	constexpr LONG MAX_PARAM_OBJECTS = 16;
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

	// When the ASIO input shifter owns pitch, all game-side application is suppressed:
	// the input signal is already retuned, so detection hears the shifted notes and the
	// tuner reference must stay at 440.
	bool inputShifterActive = false;
	unsigned long long engineNoticeTick = 0;

	bool wasLowerKeyDown = false;
	bool wasRaiseKeyDown = false;
	bool wasToggleKeyDown = false;
	bool wasBaseDownKeyDown = false;
	bool wasBaseUpKeyDown = false;

	// The tuning the player's guitar is physically in, as semitones from E standard.
	// Everything the mod shows is relative to this, so a player who lives in Eb sees
	// tunings named from Eb rather than being told to do the arithmetic themselves.
	// Session state like the shift itself: a player in Eb sets it once per launch.
	int baseTuningSemitones = 0;

	bool isPitchPushPending = false;

	bool hasCapturedSongTuning = false;

	// Objects the engine itself delivers pitch to. The create-param callback's return
	// value is not necessarily what a running effect reads: IAkPluginParam has Clone at
	// vtable slot 2, and writing to the original moved nothing. These are the objects
	// the engine drives, so they are the ones worth writing to.
	void* deliveredParamObjects[MAX_PARAM_OBJECTS] = {};
	float deliveredAuthoredCents[MAX_PARAM_OBJECTS] = {};
	volatile LONG deliveredParamObjectCount = 0;
	ULONGLONG lastTargetChangeTime = 0;

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
	/// through a local, so the caller's own buffer is never written.
	/// </summary>
	AKRESULT __fastcall SpySetParam(void* self, void* unused, AkUInt32 paramId, const void* value, AkUInt32 size)
	{
		if (paramId != PITCH_PARAM_ID || size != sizeof(float) || value == nullptr)
		{
			return originalSetParam(self, unused, paramId, value, size);
		}


		// Record the object the engine delivered to, so live pushes can target it.
		// Runs on the audio thread, so no logging and no locks.
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
			const LONG slot = InterlockedIncrement(&deliveredParamObjectCount) - 1;
			if (slot < MAX_PARAM_OBJECTS)
			{
				deliveredParamObjects[slot] = self;
				deliveredAuthoredCents[slot] = authoredCents;
			}
		}

		if (!DropPedal::IsEnabled() || inputShifterActive)
		{
			return originalSetParam(self, unused, paramId, value, size);
		}

		float appliedCents = authoredCents + targetCents;

		const LONG index = InterlockedIncrement(&overrideEventCount) - 1;
		if (index < MAX_PENDING_EVENTS)
		{
			overrideEvents[index] = { authoredCents, appliedCents };
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
		// Without the Term hook there is no way to know an object has been freed, and
		// a push after a tone switch would write into released memory.
		if (originalSetParam == nullptr || originalTerm == nullptr)
		{
			return;
		}

		const LONG known = deliveredParamObjectCount < MAX_PARAM_OBJECTS
			? deliveredParamObjectCount
			: MAX_PARAM_OBJECTS;
		LONG pushed = 0;

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
			const float cents = deliveredAuthoredCents[i] + targetCents;

			originalSetParam(paramObject, nullptr, PITCH_PARAM_ID, &cents, sizeof(float));
			pushed++;
		}
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
	/// presses without lag. The shifters are updated once they stop.
	/// Does nothing while the mod is off, so the pitch keys are inert until toggled on.
	/// </summary>
	void AdjustTarget(int semitoneDelta)
	{
		if (!isEnabledSession)
		{
			return;
		}

		const int adjusted = DropPedal::GetTargetSemitones() + semitoneDelta;
		if (adjusted < MIN_TARGET_SEMITONES || adjusted > MAX_TARGET_SEMITONES)
		{
			return;
		}

		targetCents = adjusted * CENTS_PER_SEMITONE;
		isPitchPushPending = true;
		lastTargetChangeTime = GetTickCount64();
	}

	void PushPitchWhenSettled()
	{
		if (!isPitchPushPending || GetTickCount64() - lastTargetChangeTime < PITCH_PUSH_DELAY_MILLISECONDS)
		{
			return;
		}

		isPitchPushPending = false;
		PushPitchToLiveShifters();

		LOG_INFO("Drop pedal target now " << DropPedal::GetTuningName() << std::endl);
	}

	void ToggleEnabled()
	{
		isEnabledSession = !isEnabledSession;

		if (isEnabledSession)
		{
			LOG_INFO("Drop pedal enabled, target " << DropPedal::GetTuningName() << std::endl);

			if (!inputShifterActive)
			{
				PushPitchToLiveShifters();
			}
		}
		else
		{
			LOG_INFO("Drop pedal disabled, tones return to how the player configured them"
				<< " on the next tone load or tone switch" << std::endl);
		}
	}

	void AdjustBaseTuning(int semitoneDelta)
	{
		const int adjusted = baseTuningSemitones + semitoneDelta;
		if (adjusted < MIN_BASE_TUNING_SEMITONES || adjusted > MAX_BASE_TUNING_SEMITONES)
		{
			return;
		}

		baseTuningSemitones = adjusted;

		LOG_INFO("Drop pedal base tuning now " << DropPedal::GetBaseTuningName() << std::endl);
	}

	void HandleHotkeys()
	{
		// GetAsyncKeyState reads global keyboard state, so without this guard the pedal
		// retunes while the player is typing in another window.
		DWORD foregroundProcessId = 0;
		GetWindowThreadProcessId(GetForegroundWindow(), &foregroundProcessId);
		if (foregroundProcessId != GetCurrentProcessId()) return;

		const bool isToggleKeyDown = (GetAsyncKeyState(TOGGLE_KEY) & 0x8000) != 0;

		if (isToggleKeyDown && !wasToggleKeyDown)
		{
			ToggleEnabled();
		}

		wasToggleKeyDown = isToggleKeyDown;

		// Disabled means disabled: the game ignores the pedal's output, so no key besides
		// the toggle may change its state either. The latches still update below so a key
		// held across re-enabling does not fire on the first enabled poll.
		const bool acceptAdjustments = DropPedal::IsEnabled();

		const bool isBaseDownKeyDown = (GetAsyncKeyState(BASE_TUNING_DOWN_KEY) & 0x8000) != 0;
		const bool isBaseUpKeyDown = (GetAsyncKeyState(BASE_TUNING_UP_KEY) & 0x8000) != 0;

		if (acceptAdjustments && isBaseDownKeyDown && !wasBaseDownKeyDown)
		{
			AdjustBaseTuning(-1);
		}

		if (acceptAdjustments && isBaseUpKeyDown && !wasBaseUpKeyDown)
		{
			AdjustBaseTuning(1);
		}

		wasBaseDownKeyDown = isBaseDownKeyDown;
		wasBaseUpKeyDown = isBaseUpKeyDown;

		const bool isLowerKeyDown = (GetAsyncKeyState(PITCH_DOWN_KEY) & 0x8000) != 0;
		const bool isRaiseKeyDown = (GetAsyncKeyState(PITCH_UP_KEY) & 0x8000) != 0;

		if (acceptAdjustments && isLowerKeyDown && !wasLowerKeyDown)
		{
			AdjustTarget(-1);
		}

		if (acceptAdjustments && isRaiseKeyDown && !wasRaiseKeyDown)
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

/// <summary>
/// Log the song's own tuning once per song, for diagnosing detection problems.
///
/// Writing this array was tried as a way to make detection expect the player's
/// tuning and had no effect, so it is left read only. The reference frequency in
/// TrueTuning is the mechanism that actually drives detection.
/// </summary>
void DropPedal::HandleTuningInSong()
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
		<< ", drop pedal at " << DropPedal::GetTargetSemitones() << std::endl);
}

void DropPedal::ResetSongState()
{
	hasCapturedSongTuning = false;
}

int DropPedal::GetTargetSemitones()
{
	return (int)(targetCents / CENTS_PER_SEMITONE);
}

/// <summary>
/// Name the tuning the player's guitar is heard in: their physical tuning moved by
/// the current shift, in the form a tuner would show it.
/// </summary>
std::string DropPedal::GetTuningName()
{
	static const char* tuningNames[SEMITONES_PER_OCTAVE] = {
		"E", "Eb", "D", "C#", "C", "B", "Bb", "A", "Ab", "G", "F#", "F"
	};

	const int semitones = GetTargetSemitones();

	// The names read downwards from E, so a downward step indexes straight into them.
	int stepsBelowE = (-(baseTuningSemitones + semitones)) % SEMITONES_PER_OCTAVE;
	if (stepsBelowE < 0)
	{
		stepsBelowE += SEMITONES_PER_OCTAVE;
	}

	std::ostringstream name;
	name << tuningNames[stepsBelowE] << " standard";

	if (semitones != 0)
	{
		name << " (" << (semitones > 0 ? "+" : "") << semitones << ")";
	}

	return name.str();
}

/// <summary>
/// Name the tuning the player's guitar is physically in, with no shift applied.
/// </summary>
std::string DropPedal::GetBaseTuningName()
{
	static const char* tuningNames[SEMITONES_PER_OCTAVE] = {
		"E", "Eb", "D", "C#", "C", "B", "Bb", "A", "Ab", "G", "F#", "F"
	};

	int stepsBelowE = (-baseTuningSemitones) % SEMITONES_PER_OCTAVE;
	if (stepsBelowE < 0)
	{
		stepsBelowE += SEMITONES_PER_OCTAVE;
	}

	return std::string(tuningNames[stepsBelowE]) + " standard";
}

int DropPedal::GetShiftDirection()
{
	const int semitones = GetTargetSemitones();
	if (semitones < 0)
	{
		return -1;
	}

	return semitones > 0 ? 1 : 0;
}

void DropPedal::InstallHooks()
{
	// The engine notice starts counting here: game-side until the ASIO chain proves
	// itself, at which point SetInputShifterActive restarts it.
	engineNoticeTick = GetTickCount64();
	LOG_INFO("Drop pedal engine: Cable Drop Pedal" << std::endl);

	// Note detection reads the raw guitar signal, so the pitch shifter is invisible to
	// it. The game derives expected pitch from a reference frequency instead, which is
	// the same value CDLC charters set as an arrangement's tuning pitch. Redirecting it
	// is what keeps scoring in agreement with the strings the player is holding.
	TrueTuning::DisableTrueTuning();
	TrueTuning::SetReferenceSemitones(-DropPedal::GetTargetSemitones());

	const uintptr_t target = Wwise::Exports::func_Wwise_Sound_RegisterPlugin.Get();
	originalRegisterPlugin = (tRegisterPlugin)DetourFunction((PBYTE)target, (PBYTE)SpyRegisterPlugin);

	if (originalRegisterPlugin == nullptr)
	{
		LOG_ERROR("Drop pedal failed to hook RegisterPlugin at 0x" << std::hex << target << std::dec << std::endl);
	}
}

void DropPedal::SetInputShifterActive(bool active)
{
	if (inputShifterActive == active) return;

	inputShifterActive = active;
	engineNoticeTick = GetTickCount64();
	LOG_INFO("Drop pedal engine: " << (active ? "ASIO Drop Pedal" : "Cable Drop Pedal") << std::endl);
}

bool DropPedal::IsInputShifterActive()
{
	return inputShifterActive;
}

unsigned long long DropPedal::GetEngineNoticeTick()
{
	return engineNoticeTick;
}

void DropPedal::PollHotkeys()
{
	HandleHotkeys();
}

void DropPedal::Poll()
{
	HookSetParamOnce();

	// Kept in step every tick rather than only when the pitch changes, so the value
	// is already correct when a song loads. Detection appears to take its reference
	// at load time, which is why setting it mid-song has no effect.
	TrueTuning::SetReferenceSemitones((isEnabledSession && !inputShifterActive) ? -DropPedal::GetTargetSemitones() : 0);

	if (!inputShifterActive)
	{
		PushPitchWhenSettled();

		LogPendingOverrides();
	}
}
