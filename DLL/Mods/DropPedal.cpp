#include "../stdafx.h"
#include "DropPedal.hpp"
#include "TrueTuning.hpp"

extern "C" void* _ReturnAddress(void);
#pragma intrinsic(_ReturnAddress)

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

	constexpr uintptr_t FX_WALKER_ADDRESS = 0xEF5750;
	constexpr size_t FX_RECORD_SIZE = 0x1C;
	constexpr LONG MAX_FX_SNAPSHOTS = 64;
	constexpr LONG MAX_FX_RECORDS_PER_SNAPSHOT = 128;
	// Word indices into a 28-byte FX record. +00 repeats within a single chain
	// while +08 never does, so +00 reads as a parent/bus id and +08 as the effect
	// id. Diffs must be done on +08: removing one effect from a tone changed every
	// +00 in that chain, so +00 is position-dependent and useless for identity.
	// +04 was zero in every record captured, so it is the high half of a 64-bit +00.
	constexpr int FX_BUS_WORD = 0;
	constexpr int FX_EFFECT_WORD = 2;
	constexpr int FX_KIND_WORD = 5;

	// Effect ids from the Emulated Bass chain, captured while the shifter was sent
	// -1200 cents. Whichever of these is absent once the MultiPitch is removed is
	// the pitch node.
	constexpr AkUInt32 WATCHED_FX_EFFECTS[] = { 0x2DE350C2, 0x0FA365D3, 0x0891E5AF, 0x0DEC9B6F };

	// The node deserializer reads a plugin id straight out of bank data and builds
	// the effect unless the id is the "no plugin" sentinel. It is the only gate on
	// instantiation, so what a stock tone puts in that field decides everything.
	//
	// Wwise packs the id as (pluginId << 16) | (companyId << 4) | pluginType, with
	// type 3 meaning effect. The shifter is plugin 136 from company 0, so bank data
	// carries 0x880003 rather than the bare 136 that RegisterPlugin reports.
	constexpr uintptr_t NODE_DESERIALIZE_ADDRESS = 0xEF5B10;
	constexpr AkUInt32 PITCH_SHIFTER_ENCODED_PLUGIN_ID = 0x880003;
	constexpr AkUInt32 NODE_NO_PLUGIN_ID = 0xFFFFFFFF;
	constexpr size_t NODE_PLUGIN_ID_OFFSET = 4;
	constexpr size_t NODE_BLOB_SIZE_OFFSET = 8;
	constexpr size_t NODE_BLOB_OFFSET = 0xC;
	constexpr LONG MAX_NODE_SNAPSHOTS = 128;

	// Only the pitch shifter's parameter block is worth keeping: it is the template
	// a synthetic shifter would have to supply to Init.
	constexpr AkUInt32 MAX_CAPTURED_BLOB_BYTES = 64;

	const unsigned char NODE_DESERIALIZE_PROLOGUE[] = { 0x55, 0x8B, 0xEC, 0x83, 0xEC, 0x0C, 0x53, 0x56 };

	// The executable is packed, so game code exists only in the running process.
	// This span covers the two SetParam delivery sites (0xEC87D5, 0xEC94B7) and the
	// plugin dispatch at 0xECA520, dumped together so cross-references resolve.
	constexpr uintptr_t TEXT_DUMP_BASE = 0xEC0000;
	constexpr size_t TEXT_DUMP_SIZE = 0x1C000;
	constexpr size_t TEXT_DUMP_PAGE_SIZE = 0x1000;
	const char TEXT_DUMP_FILE_NAME[] = "RSMods_text_dump.bin";

	const unsigned char FX_WALKER_PROLOGUE[] = { 0x55, 0x8B, 0xEC, 0x56, 0x57 };

	typedef void* (__cdecl* tCreateParamRaw)(void* allocator);
	typedef uintptr_t(__thiscall* tFxWalkerRaw)(void* self);

	// 0xEF5B10 is __thiscall with two stack arguments: derived from ret 8 at
	// 0xEF5BEF and mov edi, ecx at 0xEF5B22, not assumed.
	typedef AKRESULT(__fastcall* tNodeDeserializeRaw)(void* self, void* unused, const void* record, AkUInt32 context);

	// Virtual member functions are __thiscall on x86, which passes this in ECX.
	// __fastcall matches that once the unused EDX argument is declared explicitly.
	typedef AKRESULT(__fastcall* tSetParamRaw)(void* self, void* unused, AkUInt32 paramId, const void* value, AkUInt32 size);
	typedef void(__fastcall* tTermRaw)(void* self, void* unused, void* allocator);

	struct PitchOverrideEvent
	{
		float originalCents;
		float appliedCents;
	};

	struct NodeSnapshot
	{
		volatile LONG state;
		AkUInt32 pluginId;
		AkUInt32 blobSize;
		AkUInt32 capturedBlobBytes;
		unsigned char blob[MAX_CAPTURED_BLOB_BYTES];
	};

	enum FxSnapshotStatus
	{
		FX_SNAPSHOT_VALID,
		FX_SNAPSHOT_INVALID_OWNER,
		FX_SNAPSHOT_INVALID_BOUNDS,
		FX_SNAPSHOT_INVALID_RECORD
	};

	struct FxRecordSnapshot
	{
		AkUInt32 words[FX_RECORD_SIZE / sizeof(AkUInt32)];
	};

	struct FxWalkerSnapshot
	{
		volatile LONG state;
		FxSnapshotStatus status;
		uintptr_t owner;
		uintptr_t begin;
		uintptr_t end;
		LONG recordCount;
		LONG capturedCount;
		LONG pitchCallsBefore;
		bool hasWatchedKey;
		FxRecordSnapshot records[MAX_FX_RECORDS_PER_SNAPSHOT];
	};

	tCreateParamRaw originalCreateParam = nullptr;
	tFxWalkerRaw originalFxWalker = nullptr;
	tNodeDeserializeRaw originalNodeDeserialize = nullptr;
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

	FxWalkerSnapshot fxSnapshots[MAX_FX_SNAPSHOTS] = {};
	volatile LONG nextFxSnapshot = -1;
	volatile LONG droppedFxSnapshots = 0;

	NodeSnapshot nodeSnapshots[MAX_NODE_SNAPSHOTS] = {};
	volatile LONG droppedNodeSnapshots = 0;

	// A bank load walks well over a thousand nodes. Only shifters and empty slots
	// are worth a line each; the rest are counted so the volume is still visible.
	volatile LONG otherNodeCount = 0;
	volatile LONG unreadableNodeCount = 0;

	// Counts every pitch parameter the engine delivers, so a snapshot can say how
	// many had arrived when it was taken. The Emulated Bass group followed the
	// -1200 immediately; that ordering is what identifies the chain.
	volatile LONG pitchParamCallCount = 0;

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

	// Records who asked for a pitch shifter. SpyCreateParam is a detour of the
	// plugin's own create-param callback, so its return address is the engine code
	// that decided to build one. Comparing that between a tone that has a
	// MultiPitch and one that does not is the shortest route to the decision point
	// the mod needs to reach for stock tones.
	constexpr LONG MAX_CREATE_PARAM_EVENTS = 32;
	constexpr LONG MAX_CREATE_PARAM_CALLERS = 8;
	constexpr LONG CREATE_PARAM_STACK_SCAN_WORDS = 192;

	// The executable is a little over 16 MB at a standard base, so anything in this
	// window is plausibly a return address and anything outside it certainly is not.
	constexpr uintptr_t CODE_RANGE_LOW = 0x00400000;
	constexpr uintptr_t CODE_RANGE_HIGH = 0x01500000;

	struct CreateParamEvent
	{
		volatile LONG state;
		LONG shifterIndex;
		uintptr_t returnAddress;
		LONG callerCount;
		uintptr_t callers[MAX_CREATE_PARAM_CALLERS];
	};

	CreateParamEvent createParamEvents[MAX_CREATE_PARAM_EVENTS] = {};
	volatile LONG droppedCreateParamEvents = 0;

	void CaptureCreateParamCallers(LONG shifterIndex, uintptr_t returnAddress, const uintptr_t* stackCursor)
	{
		CreateParamEvent* event = nullptr;
		for (LONG index = 0; index < MAX_CREATE_PARAM_EVENTS; index++)
		{
			if (InterlockedCompareExchange(&createParamEvents[index].state, 1, 0) == 0)
			{
				event = &createParamEvents[index];
				break;
			}
		}

		if (event == nullptr)
		{
			InterlockedIncrement(&droppedCreateParamEvents);
			return;
		}

		event->shifterIndex = shifterIndex;
		event->returnAddress = returnAddress;
		event->callerCount = 0;

		// Walking the frame pointers would need the engine to keep them, which an
		// optimised build does not. Scanning a bounded window of the stack for
		// values that fall inside the image gives a usable trace without that
		// assumption, at the cost of occasional false positives.
		for (LONG word = 0; word < CREATE_PARAM_STACK_SCAN_WORDS; word++)
		{
			if (event->callerCount >= MAX_CREATE_PARAM_CALLERS)
			{
				break;
			}

			const uintptr_t* slot = stackCursor + word;
			if (MemUtil::IsBadReadPtr((void*)slot))
			{
				break;
			}

			const uintptr_t value = *slot;
			if (value >= CODE_RANGE_LOW && value < CODE_RANGE_HIGH)
			{
				event->callers[event->callerCount] = value;
				event->callerCount++;
			}
		}

		InterlockedExchange(&event->state, 2);
	}

	void LogCreateParamEvents()
	{
		for (LONG index = 0; index < MAX_CREATE_PARAM_EVENTS; index++)
		{
			CreateParamEvent* event = &createParamEvents[index];
			if (InterlockedCompareExchange(&event->state, 3, 2) != 2)
			{
				continue;
			}

			std::ostringstream callers;
			for (LONG callerIndex = 0; callerIndex < event->callerCount; callerIndex++)
			{
				callers << " 0x" << std::hex << event->callers[callerIndex] << std::dec;
			}

			LOG_INFO("Drop pedal shifter built index=" << event->shifterIndex
				<< " calledFrom=0x" << std::hex << event->returnAddress << std::dec
				<< " stack" << callers.str() << std::endl);

			InterlockedExchange(&event->state, 0);
		}

		const LONG dropped = InterlockedExchange(&droppedCreateParamEvents, 0);
		if (dropped > 0)
		{
			LOG_ERROR("Drop pedal dropped " << dropped << " create-param event(s)" << std::endl);
		}
	}

	void* __cdecl SpyCreateParam(void* allocator)
	{
		const uintptr_t returnAddress = (uintptr_t)_ReturnAddress();
		uintptr_t stackAnchor = 0;

		void* paramObject = originalCreateParam(allocator);

		if (paramObject != nullptr)
		{
			const LONG index = InterlockedIncrement(&paramObjectCount) - 1;

			if (InterlockedCompareExchange(&hasParamObject, 1, 0) == 0)
			{
				firstParamObject = paramObject;
			}

			CaptureCreateParamCallers(index, returnAddress, (const uintptr_t*)&stackAnchor);
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

		InterlockedIncrement(&pitchParamCallCount);

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

	FxWalkerSnapshot* ReserveFxSnapshot()
	{
		const LONG firstIndex = InterlockedIncrement(&nextFxSnapshot);
		for (LONG offset = 0; offset < MAX_FX_SNAPSHOTS; offset++)
		{
			const ULONG index = ((ULONG)firstIndex + (ULONG)offset) & (MAX_FX_SNAPSHOTS - 1);
			FxWalkerSnapshot* snapshot = &fxSnapshots[index];
			if (InterlockedCompareExchange(&snapshot->state, 1, 0) == 0)
			{
				return snapshot;
			}
		}

		InterlockedIncrement(&droppedFxSnapshots);
		return nullptr;
	}

	void CaptureFxWalkerSnapshot(void* owner)
	{
		FxWalkerSnapshot* snapshot = ReserveFxSnapshot();
		if (snapshot == nullptr)
		{
			return;
		}

		snapshot->status = FX_SNAPSHOT_VALID;
		snapshot->owner = (uintptr_t)owner;
		snapshot->begin = 0;
		snapshot->end = 0;
		snapshot->recordCount = 0;
		snapshot->capturedCount = 0;
		snapshot->pitchCallsBefore = pitchParamCallCount;
		snapshot->hasWatchedKey = false;

		if (owner == nullptr || MemUtil::IsBadReadPtr(owner)
			|| MemUtil::IsBadReadPtr((unsigned char*)owner + 0x28 + sizeof(uintptr_t) - 1))
		{
			snapshot->status = FX_SNAPSHOT_INVALID_OWNER;
			InterlockedExchange(&snapshot->state, 2);
			return;
		}

		snapshot->begin = *(uintptr_t*)((unsigned char*)owner + 0x24);
		snapshot->end = *(uintptr_t*)((unsigned char*)owner + 0x28);
		if (snapshot->end < snapshot->begin
			|| (snapshot->end - snapshot->begin) % FX_RECORD_SIZE != 0)
		{
			snapshot->status = FX_SNAPSHOT_INVALID_BOUNDS;
			InterlockedExchange(&snapshot->state, 2);
			return;
		}

		const uintptr_t recordCount = (snapshot->end - snapshot->begin) / FX_RECORD_SIZE;
		if (recordCount > LONG_MAX)
		{
			snapshot->status = FX_SNAPSHOT_INVALID_BOUNDS;
			InterlockedExchange(&snapshot->state, 2);
			return;
		}

		snapshot->recordCount = (LONG)recordCount;
		snapshot->capturedCount = snapshot->recordCount < MAX_FX_RECORDS_PER_SNAPSHOT
			? snapshot->recordCount
			: MAX_FX_RECORDS_PER_SNAPSHOT;

		for (LONG index = 0; index < snapshot->capturedCount; index++)
		{
			const uintptr_t recordAddress = snapshot->begin + index * FX_RECORD_SIZE;
			if (MemUtil::IsBadReadPtr((void*)recordAddress)
				|| MemUtil::IsBadReadPtr((void*)(recordAddress + FX_RECORD_SIZE - 1)))
			{
				snapshot->status = FX_SNAPSHOT_INVALID_RECORD;
				snapshot->capturedCount = index;
				break;
			}

			memcpy(snapshot->records[index].words, (void*)recordAddress, FX_RECORD_SIZE);

			const AkUInt32 effect = snapshot->records[index].words[FX_EFFECT_WORD];
			for (size_t watchIndex = 0; watchIndex < _countof(WATCHED_FX_EFFECTS); watchIndex++)
			{
				if (effect == WATCHED_FX_EFFECTS[watchIndex])
				{
					snapshot->hasWatchedKey = true;
					break;
				}
			}
		}

		InterlockedExchange(&snapshot->state, 2);
	}

	uintptr_t __fastcall SpyFxWalker(void* self, void* unused)
	{
		CaptureFxWalkerSnapshot(self);
		return originalFxWalker(self);
	}

	const char* GetFxSnapshotStatusName(FxSnapshotStatus status)
	{
		switch (status)
		{
		case FX_SNAPSHOT_VALID:
			return "valid";
		case FX_SNAPSHOT_INVALID_OWNER:
			return "invalid-owner";
		case FX_SNAPSHOT_INVALID_BOUNDS:
			return "invalid-bounds";
		case FX_SNAPSHOT_INVALID_RECORD:
			return "invalid-record";
		default:
			return "unknown";
		}
	}

	void LogFxWalkerSnapshots()
	{
		for (LONG snapshotIndex = 0; snapshotIndex < MAX_FX_SNAPSHOTS; snapshotIndex++)
		{
			FxWalkerSnapshot* snapshot = &fxSnapshots[snapshotIndex];
			if (InterlockedCompareExchange(&snapshot->state, 3, 2) != 2)
			{
				continue;
			}

			if (snapshot->recordCount == 0)
			{
				InterlockedExchange(&snapshot->state, 0);
				continue;
			}

			std::ostringstream keys;
			for (LONG recordIndex = 0; recordIndex < snapshot->capturedCount; recordIndex++)
			{
				const AkUInt32* words = snapshot->records[recordIndex].words;
				keys << " " << std::hex << words[FX_BUS_WORD] << "/" << words[FX_EFFECT_WORD]
					<< ":" << words[FX_KIND_WORD] << std::dec;
			}

			LOG_INFO("Drop pedal FX probe owner=0x" << std::hex << snapshot->owner << std::dec
				<< " records=" << snapshot->recordCount
				<< " captured=" << snapshot->capturedCount
				<< " pitchCalls=" << snapshot->pitchCallsBefore
				<< " status=" << GetFxSnapshotStatusName(snapshot->status)
				<< (snapshot->hasWatchedKey ? " WATCHED" : "")
				<< " keys" << keys.str() << std::endl);

			if (snapshot->hasWatchedKey)
			{
				for (LONG recordIndex = 0; recordIndex < snapshot->capturedCount; recordIndex++)
				{
					const AkUInt32* words = snapshot->records[recordIndex].words;
					LOG_INFO("Drop pedal FX probe record[" << recordIndex << "]"
						<< " +00=0x" << std::hex << words[0]
						<< " +04=0x" << words[1]
						<< " +08=0x" << words[2]
						<< " +0c=0x" << words[3]
						<< " +10=0x" << words[4]
						<< " +14=0x" << words[5]
						<< " +18=0x" << words[6] << std::dec << std::endl);
				}
			}

			if (snapshot->capturedCount < snapshot->recordCount)
			{
				LOG_INFO("Drop pedal FX probe omitted "
					<< snapshot->recordCount - snapshot->capturedCount << " record(s)" << std::endl);
			}

			InterlockedExchange(&snapshot->state, 0);
		}

		const LONG dropped = InterlockedExchange(&droppedFxSnapshots, 0);
		if (dropped > 0)
		{
			LOG_ERROR("Drop pedal FX probe dropped " << dropped << " walker snapshot(s)" << std::endl);
		}
	}

	NodeSnapshot* ReserveNodeSnapshot()
	{
		for (LONG index = 0; index < MAX_NODE_SNAPSHOTS; index++)
		{
			NodeSnapshot* snapshot = &nodeSnapshots[index];
			if (InterlockedCompareExchange(&snapshot->state, 1, 0) == 0)
			{
				return snapshot;
			}
		}

		InterlockedIncrement(&droppedNodeSnapshots);
		return nullptr;
	}

	void CaptureNodeSnapshot(const void* record)
	{
		const unsigned char* bytes = (const unsigned char*)record;
		if (record == nullptr || MemUtil::IsBadReadPtr((void*)record)
			|| MemUtil::IsBadReadPtr((void*)(bytes + NODE_BLOB_OFFSET - 1)))
		{
			InterlockedIncrement(&unreadableNodeCount);
			return;
		}

		const AkUInt32 pluginId = *(const AkUInt32*)(bytes + NODE_PLUGIN_ID_OFFSET);
		const AkUInt32 blobSize = *(const AkUInt32*)(bytes + NODE_BLOB_SIZE_OFFSET);

		if (pluginId != PITCH_SHIFTER_ENCODED_PLUGIN_ID && pluginId != NODE_NO_PLUGIN_ID)
		{
			InterlockedIncrement(&otherNodeCount);
			return;
		}

		NodeSnapshot* snapshot = ReserveNodeSnapshot();
		if (snapshot == nullptr)
		{
			return;
		}

		snapshot->pluginId = pluginId;
		snapshot->blobSize = blobSize;
		snapshot->capturedBlobBytes = 0;

		if (pluginId == PITCH_SHIFTER_ENCODED_PLUGIN_ID && blobSize > 0)
		{
			const AkUInt32 wanted = blobSize < MAX_CAPTURED_BLOB_BYTES
				? blobSize
				: MAX_CAPTURED_BLOB_BYTES;

			if (!MemUtil::IsBadReadPtr((void*)(bytes + NODE_BLOB_OFFSET))
				&& !MemUtil::IsBadReadPtr((void*)(bytes + NODE_BLOB_OFFSET + wanted - 1)))
			{
				memcpy(snapshot->blob, bytes + NODE_BLOB_OFFSET, wanted);
				snapshot->capturedBlobBytes = wanted;
			}
		}

		InterlockedExchange(&snapshot->state, 2);
	}

	AKRESULT __fastcall SpyNodeDeserialize(void* self, void* unused, const void* record, AkUInt32 context)
	{
		CaptureNodeSnapshot(record);
		return originalNodeDeserialize(self, unused, record, context);
	}

	void LogNodeSnapshots()
	{
		for (LONG index = 0; index < MAX_NODE_SNAPSHOTS; index++)
		{
			NodeSnapshot* snapshot = &nodeSnapshots[index];
			if (InterlockedCompareExchange(&snapshot->state, 3, 2) != 2)
			{
				continue;
			}

			const bool isEmptySlot = snapshot->pluginId == NODE_NO_PLUGIN_ID;
			LOG_INFO("Drop pedal node pluginId=0x" << std::hex << snapshot->pluginId << std::dec
				<< " blobSize=" << snapshot->blobSize
				<< (isEmptySlot ? " EMPTY" : " SHIFTER") << std::endl);

			if (snapshot->capturedBlobBytes > 0)
			{
				std::ostringstream blob;
				for (AkUInt32 byteIndex = 0; byteIndex < snapshot->capturedBlobBytes; byteIndex++)
				{
					static const char HEX_DIGITS[] = "0123456789abcdef";
					const unsigned char value = snapshot->blob[byteIndex];
					blob << " " << HEX_DIGITS[value >> 4] << HEX_DIGITS[value & 0xF];
				}

				LOG_INFO("Drop pedal node shifter blob" << blob.str() << std::endl);
			}

			InterlockedExchange(&snapshot->state, 0);
		}

		const LONG dropped = InterlockedExchange(&droppedNodeSnapshots, 0);
		if (dropped > 0)
		{
			LOG_ERROR("Drop pedal node probe dropped " << dropped << " node(s)" << std::endl);
		}

		const LONG others = InterlockedExchange(&otherNodeCount, 0);
		if (others > 0)
		{
			LOG_INFO("Drop pedal node probe saw " << others << " node(s) with other plugin ids" << std::endl);
		}

		const LONG unreadable = InterlockedExchange(&unreadableNodeCount, 0);
		if (unreadable > 0)
		{
			LOG_ERROR("Drop pedal node probe could not read " << unreadable << " node record(s)" << std::endl);
		}
	}

	/// <summary>
	/// Write the unpacked code region around the SetParam delivery sites to a file
	/// beside the game executable, for offline disassembly.
	/// </summary>
	void WriteTextRegionDump()
	{
		char executablePath[MAX_PATH] = {};
		if (GetModuleFileNameA(nullptr, executablePath, MAX_PATH) == 0)
		{
			LOG_ERROR("Drop pedal could not resolve the game executable path for the text dump, error "
				<< GetLastError() << std::endl);
			return;
		}

		std::string dumpPath(executablePath);
		const size_t lastSeparator = dumpPath.find_last_of('\\');
		if (lastSeparator == std::string::npos)
		{
			LOG_ERROR("Drop pedal could not derive a dump directory from " << executablePath << std::endl);
			return;
		}

		dumpPath.erase(lastSeparator + 1);
		dumpPath += TEXT_DUMP_FILE_NAME;

		for (size_t offset = 0; offset < TEXT_DUMP_SIZE; offset += TEXT_DUMP_PAGE_SIZE)
		{
			if (MemUtil::IsBadReadPtr((void*)(TEXT_DUMP_BASE + offset)))
			{
				LOG_ERROR("Drop pedal skipped the text dump because 0x" << std::hex
					<< TEXT_DUMP_BASE + offset << " is unreadable" << std::dec << std::endl);
				return;
			}
		}

		const HANDLE dumpFile = CreateFileA(dumpPath.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
		if (dumpFile == INVALID_HANDLE_VALUE)
		{
			LOG_ERROR("Drop pedal could not create " << dumpPath << ", error " << GetLastError() << std::endl);
			return;
		}

		DWORD bytesWritten = 0;
		const BOOL didWrite = WriteFile(dumpFile, (const void*)TEXT_DUMP_BASE, (DWORD)TEXT_DUMP_SIZE, &bytesWritten, nullptr);
		const DWORD writeError = GetLastError();
		CloseHandle(dumpFile);

		if (didWrite == FALSE || bytesWritten != TEXT_DUMP_SIZE)
		{
			LOG_ERROR("Drop pedal wrote " << bytesWritten << " of " << TEXT_DUMP_SIZE
				<< " bytes to " << dumpPath << ", error " << writeError << std::endl);
			return;
		}

		LOG_INFO("Drop pedal dumped text region base=0x" << std::hex << TEXT_DUMP_BASE
			<< " length=0x" << TEXT_DUMP_SIZE << std::dec << " to " << dumpPath << std::endl);
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

	if (MemUtil::IsBadReadPtr((void*)FX_WALKER_ADDRESS)
		|| MemUtil::IsBadReadPtr((void*)(FX_WALKER_ADDRESS + sizeof(FX_WALKER_PROLOGUE) - 1))
		|| memcmp((void*)FX_WALKER_ADDRESS, FX_WALKER_PROLOGUE, sizeof(FX_WALKER_PROLOGUE)) != 0)
	{
		LOG_ERROR("Drop pedal did not hook the FX walker because the prologue at 0x" << std::hex
			<< FX_WALKER_ADDRESS << " does not match this Rocksmith build" << std::dec << std::endl);
		return;
	}

	originalFxWalker = (tFxWalkerRaw)DetourFunction((PBYTE)FX_WALKER_ADDRESS, (PBYTE)SpyFxWalker);
	if (originalFxWalker == nullptr)
	{
		LOG_ERROR("Drop pedal failed to hook the FX walker at 0x" << std::hex
			<< FX_WALKER_ADDRESS << std::dec << std::endl);
	}
	else
	{
		LOG_INFO("Drop pedal hooked the FX walker at 0x" << std::hex
			<< FX_WALKER_ADDRESS << std::dec << std::endl);
	}

	if (MemUtil::IsBadReadPtr((void*)NODE_DESERIALIZE_ADDRESS)
		|| MemUtil::IsBadReadPtr((void*)(NODE_DESERIALIZE_ADDRESS + sizeof(NODE_DESERIALIZE_PROLOGUE) - 1))
		|| memcmp((void*)NODE_DESERIALIZE_ADDRESS, NODE_DESERIALIZE_PROLOGUE, sizeof(NODE_DESERIALIZE_PROLOGUE)) != 0)
	{
		LOG_ERROR("Drop pedal did not hook the node deserializer because the prologue at 0x" << std::hex
			<< NODE_DESERIALIZE_ADDRESS << " does not match this Rocksmith build" << std::dec << std::endl);
		return;
	}

	originalNodeDeserialize = (tNodeDeserializeRaw)DetourFunction((PBYTE)NODE_DESERIALIZE_ADDRESS, (PBYTE)SpyNodeDeserialize);
	if (originalNodeDeserialize == nullptr)
	{
		LOG_ERROR("Drop pedal failed to hook the node deserializer at 0x" << std::hex
			<< NODE_DESERIALIZE_ADDRESS << std::dec << std::endl);
	}
	else
	{
		LOG_INFO("Drop pedal hooked the node deserializer at 0x" << std::hex
			<< NODE_DESERIALIZE_ADDRESS << std::dec << std::endl);
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

		// Diagnostics for the game-side MultiPitch machinery, which is suppressed while
		// the input shifter owns pitch, so their output would only describe a dormant
		// subsystem.
		LogPendingOverrides();
		LogCreateParamEvents();
		LogFxWalkerSnapshots();
		LogNodeSnapshots();
	}
}
