#include "../../stdafx.h"
#include "DropPedalHooks.hpp"
#include "DropPedalState.hpp"

namespace
{
	constexpr LONG MAX_PENDING_EVENTS = 64;
	constexpr float SEMITONES_PER_OCTAVE = 12.0f;
	constexpr float TRUE_TUNING_COMPARISON_EPSILON = 0.0001f;

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
	typedef AKRESULT(__fastcall* tTermRaw)(void* self, void* unused, void* allocator);

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
	// the input signal is already retuned, so a MultiPitch override would shift it twice.
	std::atomic<bool> inputShifterActive{ false };
	bool hasReportedInputShifterUnavailable = false;
	bool hasLoggedSongTuning = false;
	std::atomic<unsigned long long> engineNoticeTick{ 0 };

	// The reference builder converts an arrangement's cent offset into the frequency
	// note detection expects, 440 * 2^(cents / 1200), and stamps it into the
	// detection object at song load. It is detoured so every consumer sees the
	// shifted reference, including the pre-song tuner, which snapshots its expected
	// pitches from that stamp immediately after it lands: writing the stamped value
	// afterwards always lost that race, because the game re-stamps on load an
	// instant before the tuner reads it.
	void* referenceBuilderTrampoline = nullptr;

	// Written by the game loop, read by the naked detour on the game's loading
	// thread. Aligned 32-bit loads and stores are atomic on x86.
	volatile LONG referenceCentsAdjustment = 0;
	volatile LONG authoredReferenceCents = 0;
	volatile LONG hasAuthoredReferenceCents = 0;

	std::mutex trueTuningMutex;
	uintptr_t trueTuningAddress = 0;
	float authoredTrueTuning = 0.0f;
	float appliedTrueTuning = 0.0f;
	bool hasCapturedTrueTuning = false;
	bool hasAppliedTrueTuning = false;
	bool hasReportedTrueTuningUnavailable = false;

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

	// The builder takes its cent offset as a single stack argument. Adjusting the
	// argument in place and running the original means the game computes the
	// shifted frequency with its own math: non-A440 offsets and the -1200
	// emulated-bass case compose naturally instead of needing special handling.
	// Runs on the game's loading thread, so no logging and no locks.
	__declspec(naked) void SpyReferenceBuilder()
	{
		__asm
		{
			push eax
			mov eax, dword ptr [esp + 8]
			mov authoredReferenceCents, eax
			mov hasAuthoredReferenceCents, 1
			add eax, referenceCentsAdjustment
			mov dword ptr [esp + 8], eax
			pop eax
			jmp referenceBuilderTrampoline
		}
	}

	/// <summary>
	/// Keep the cents adjustment in step with the pedal. Raising the reference makes
	/// detection expect the player's physical pitch: at a -2 target, 0 cents becomes
	/// +200 and A440 becomes ~A494. When the ASIO input shifter owns pitch the input
	/// itself is already retuned, so the reference stays authored.
	/// </summary>
	void UpdateReferenceCentsAdjustment()
	{
		const bool cableOwnsPitch = !inputShifterActive.load(std::memory_order_relaxed);
		const int targetSemitones = cableOwnsPitch && DropPedalState::IsEnabled()
			? DropPedalState::GetTargetSemitones()
			: 0;

		InterlockedExchange(&referenceCentsAdjustment, (LONG)(-targetSemitones * 100));
	}

	/// <summary>
	/// The authored reference for the current arrangement. With the builder hooked,
	/// the stamped value already carries the shift, so authored is reconstructed
	/// from the cents the builder was given rather than read back transposed.
	/// </summary>
	float DeriveAuthoredTrueTuning(float currentTrueTuning)
	{
		if (referenceBuilderTrampoline != nullptr
			&& InterlockedCompareExchange(&hasAuthoredReferenceCents, 1, 1) == 1)
		{
			return 440.0f * powf(2.0f, (float)authoredReferenceCents / 1200.0f);
		}

		return currentTrueTuning;
	}

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

		if (!DropPedalState::IsEnabled() || inputShifterActive.load(std::memory_order_relaxed))
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
	AKRESULT __fastcall SpyTerm(void* self, void* unused, void* allocator)
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

		return originalTerm(self, unused, allocator);
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

	void PushPitchToLiveShiftersWithShift(float shiftCents)
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

			const float cents = deliveredAuthoredCents[i] + shiftCents;

			originalSetParam(paramObject, nullptr, PITCH_PARAM_ID, &cents, sizeof(float));
		}
	}

	bool AreTrueTuningValuesEqual(float first, float second)
	{
		return fabsf(first - second) <= TRUE_TUNING_COMPARISON_EPSILON;
	}

	bool WriteTrueTuningLocked(float value)
	{
		if (!MemUtil::PatchAdr(
			reinterpret_cast<LPVOID>(trueTuningAddress),
			reinterpret_cast<LPVOID>(&value),
			sizeof(value)))
		{
			LOG_ERROR("Drop pedal failed to write true tuning at 0x"
				<< std::hex << trueTuningAddress << std::dec << std::endl);
			return false;
		}

		return true;
	}

	bool CaptureOrRefreshTrueTuningLocked(float& currentTrueTuning)
	{
		uintptr_t currentAddress = 0;
		if (!SongTuning::TryGetTrueTuning(currentTrueTuning, currentAddress))
		{
			if (!hasReportedTrueTuningUnavailable)
			{
				hasReportedTrueTuningUnavailable = true;
				LOG_ERROR("Drop pedal could not resolve the arrangement true-tuning value" << std::endl);
			}

			return false;
		}

		hasReportedTrueTuningUnavailable = false;

		if (!hasCapturedTrueTuning || currentAddress != trueTuningAddress)
		{
			trueTuningAddress = currentAddress;
			authoredTrueTuning = DeriveAuthoredTrueTuning(currentTrueTuning);
			appliedTrueTuning = currentTrueTuning;
			hasCapturedTrueTuning = true;
			hasAppliedTrueTuning = !AreTrueTuningValuesEqual(appliedTrueTuning, authoredTrueTuning);

			LOG_INFO("Drop pedal captured authored true tuning " << authoredTrueTuning
				<< " Hz at 0x" << std::hex << trueTuningAddress << std::dec << std::endl);
			return true;
		}

		// Rocksmith rewrites this location when an arrangement loads. A value other
		// than the one the drop pedal applied is therefore a new stamp from the
		// reference builder, not a result to compound on the next pedal update.
		if (hasAppliedTrueTuning && !AreTrueTuningValuesEqual(currentTrueTuning, appliedTrueTuning))
		{
			authoredTrueTuning = DeriveAuthoredTrueTuning(currentTrueTuning);
			appliedTrueTuning = currentTrueTuning;
			hasAppliedTrueTuning = !AreTrueTuningValuesEqual(appliedTrueTuning, authoredTrueTuning);
			LOG_INFO("Drop pedal observed a new authored true tuning "
				<< authoredTrueTuning << " Hz" << std::endl);
		}
		else if (!hasAppliedTrueTuning && !AreTrueTuningValuesEqual(currentTrueTuning, authoredTrueTuning))
		{
			authoredTrueTuning = DeriveAuthoredTrueTuning(currentTrueTuning);
			appliedTrueTuning = currentTrueTuning;
			hasAppliedTrueTuning = !AreTrueTuningValuesEqual(appliedTrueTuning, authoredTrueTuning);
			LOG_INFO("Drop pedal observed a new authored true tuning "
				<< authoredTrueTuning << " Hz" << std::endl);
		}

		return true;
	}

	void ApplyTrueTuningLocked()
	{
		float currentTrueTuning = 0.0f;
		if (!CaptureOrRefreshTrueTuningLocked(currentTrueTuning))
		{
			return;
		}

		const bool cableOwnsPitch = !inputShifterActive.load(std::memory_order_relaxed);
		const bool shouldTransposeDetection = cableOwnsPitch && DropPedalState::IsEnabled();
		const int targetSemitones = shouldTransposeDetection
			? DropPedalState::GetTargetSemitones()
			: 0;
		const float targetTrueTuning = authoredTrueTuning
			* powf(2.0f, -(float)targetSemitones / SEMITONES_PER_OCTAVE);
		const bool targetChanged = !AreTrueTuningValuesEqual(appliedTrueTuning, targetTrueTuning);
		bool wroteValue = false;

		if (!AreTrueTuningValuesEqual(currentTrueTuning, targetTrueTuning))
		{
			if (!WriteTrueTuningLocked(targetTrueTuning))
			{
				return;
			}

			wroteValue = true;
		}

		appliedTrueTuning = targetTrueTuning;
		hasAppliedTrueTuning = !AreTrueTuningValuesEqual(targetTrueTuning, authoredTrueTuning);

		if (targetChanged || wroteValue)
		{
			LOG_INFO("Drop pedal true tuning: authored " << authoredTrueTuning
				<< " Hz, applied " << targetTrueTuning << " Hz, target "
				<< targetSemitones << " semitone(s)" << std::endl);
		}
	}

	void ApplyCapturedTrueTuning()
	{
		std::lock_guard<std::mutex> lock(trueTuningMutex);
		if (!hasCapturedTrueTuning)
		{
			return;
		}

		ApplyTrueTuningLocked();
	}

	void RestoreTrueTuningLocked()
	{
		if (!hasCapturedTrueTuning)
		{
			return;
		}

		float currentTrueTuning = 0.0f;
		uintptr_t currentAddress = 0;
		if (SongTuning::TryGetTrueTuning(currentTrueTuning, currentAddress)
			&& currentAddress == trueTuningAddress
			&& !AreTrueTuningValuesEqual(currentTrueTuning, authoredTrueTuning))
		{
			if (WriteTrueTuningLocked(authoredTrueTuning))
			{
				LOG_INFO("Drop pedal restored authored true tuning "
					<< authoredTrueTuning << " Hz" << std::endl);
			}
		}

		appliedTrueTuning = authoredTrueTuning;
		hasAppliedTrueTuning = false;
	}
}

void DropPedalHooks::Install()
{
	// The engine notice starts counting here. Automatic starts game-side until the
	// ASIO chain proves itself; a forced asio engine claims pitch immediately so the
	// game-side MultiPitch path never runs, even if the chain later fails to appear.
	engineNoticeTick.store(GetTickCount64(), std::memory_order_relaxed);
	inputShifterActive.store(DropPedalState::IsAsioEngine(), std::memory_order_relaxed);

	LOG_INFO("Drop pedal engine: "
		<< (inputShifterActive.load(std::memory_order_relaxed) ? "ASIO Drop Pedal" : "Cable Drop Pedal") << std::endl);

	const uintptr_t target = Wwise::Exports::func_Wwise_Sound_RegisterPlugin.Get();
	originalRegisterPlugin = (tRegisterPlugin)DetourFunction((PBYTE)target, (PBYTE)SpyRegisterPlugin);

	if (originalRegisterPlugin == nullptr)
	{
		LOG_ERROR("Drop pedal failed to hook RegisterPlugin at 0x" << std::hex << target << std::dec << std::endl);
	}

	const uintptr_t referenceBuilder = Offsets::func_tuningReferenceBuilder.GetValue();
	if (referenceBuilder != 0)
	{
		const PBYTE trampoline = DetourFunction((PBYTE)referenceBuilder, (PBYTE)SpyReferenceBuilder);
		if (trampoline == nullptr)
		{
			LOG_ERROR("Drop pedal failed to hook the tuning reference builder at 0x"
				<< std::hex << referenceBuilder << std::dec << std::endl);
		}
		else
		{
			referenceBuilderTrampoline = trampoline;
		}
	}
	else
	{
		// Without the builder hook the live true-tuning writes still keep in-song
		// detection correct; only the tuner's load-time snapshot stays authored.
		LOG_INFO("Drop pedal has no reference builder address for this game version; "
			"the pre-song tuner will not follow the shift" << std::endl);
	}

	UpdateReferenceCentsAdjustment();
}

void DropPedalHooks::Poll()
{
	HookSetParamOnce();
	UpdateReferenceCentsAdjustment();
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
	// Disabled pushes the authored pitch itself, which is what disengages the pedal
	// immediately instead of waiting for the next tone load to re-deliver it.
	const float shiftCents = DropPedalState::IsEnabled() ? DropPedalState::GetTargetCents() : 0.0f;
	PushPitchToLiveShiftersWithShift(shiftCents);
	ApplyCapturedTrueTuning();
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

	if (inputShifterActive.load(std::memory_order_relaxed) == active) return;

	if (active)
	{
		// Remove the Cable shift before ASIO takes ownership, so the two engines
		// cannot apply the same target at once during automatic promotion.
		PushPitchToLiveShiftersWithShift(0.0f);
		inputShifterActive.store(true, std::memory_order_relaxed);
		ApplyCapturedTrueTuning();
	}
	else
	{
		// Publish Cable ownership before reapplying its target. Any concurrent
		// SetParam delivery will therefore apply the same shift rather than bypass it.
		inputShifterActive.store(false, std::memory_order_relaxed);
		const float shiftCents = DropPedalState::IsEnabled() ? DropPedalState::GetTargetCents() : 0.0f;
		PushPitchToLiveShiftersWithShift(shiftCents);
		ApplyCapturedTrueTuning();
	}

	engineNoticeTick.store(GetTickCount64(), std::memory_order_relaxed);
	LOG_INFO("Drop pedal engine: " << (active ? "ASIO Drop Pedal" : "Cable Drop Pedal") << std::endl);
}

bool DropPedalHooks::IsInputShifterActive()
{
	return inputShifterActive.load(std::memory_order_relaxed);
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
	return engineNoticeTick.load(std::memory_order_relaxed);
}

/// <summary>
/// Log the song's own tuning once per song, for diagnosing detection problems.
///
/// Writing this array was tried as a way to make detection expect the player's
/// tuning and had no effect, so it is left read only.
/// </summary>
void DropPedalHooks::HandleArrangementTuning()
{
	if (!hasLoggedSongTuning)
	{
		const uintptr_t addrTuning = MemUtil::FindDMAAddy(
			Offsets::baseHandle + Offsets::ptr_tuning,
			Offsets::ptr_tuningOffsets,
			true);
		if (addrTuning != 0)
		{
			const Tuning* tuning = reinterpret_cast<const Tuning*>(addrTuning);
			hasLoggedSongTuning = true;

			LOG_INFO("Drop pedal song tuning is "
				<< (int)(char)tuning->lowE << " " << (int)(char)tuning->strA << " "
				<< (int)(char)tuning->strD << " " << (int)(char)tuning->strG << " "
				<< (int)(char)tuning->strB << " " << (int)(char)tuning->highE
				<< ", drop pedal at " << DropPedalState::GetTargetSemitones() << std::endl);
		}
	}

	std::lock_guard<std::mutex> lock(trueTuningMutex);
	ApplyTrueTuningLocked();
}

void DropPedalHooks::ResetSongState()
{
	std::lock_guard<std::mutex> lock(trueTuningMutex);
	RestoreTrueTuningLocked();

	trueTuningAddress = 0;
	authoredTrueTuning = 0.0f;
	appliedTrueTuning = 0.0f;
	hasCapturedTrueTuning = false;
	hasAppliedTrueTuning = false;
	hasReportedTrueTuningUnavailable = false;
	hasLoggedSongTuning = false;
}
