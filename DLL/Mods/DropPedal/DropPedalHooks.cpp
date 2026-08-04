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

	// Effect class slot 6 is Init(allocator, context, param, format): the only slot
	// reading four stack arguments and copying the 10-dword AkAudioFormat.
	constexpr int EFFECT_INIT_VTABLE_INDEX = 6;

	constexpr AkUInt32 PITCH_SHIFTER_PLUGIN_ID = 136;

	// The pitch shifter receives its knob values through SetParam, sent while the
	// effect is being built and again on every tone load and tone switch. Param 6
	// is the pitch, in cents: a MultiPitch pedal at -12 semitones arrives as -1200.
	constexpr AkUInt32 PITCH_PARAM_ID = 6;

	constexpr LONG MAX_PARAM_OBJECTS = 16;
	void* const RESERVED_PARAM_OBJECT = reinterpret_cast<void*>(1);

	enum class PitchOwner
	{
		Cable,
		Transitioning,
		Asio
	};

	typedef void* (__cdecl* tCreateParamRaw)(void* allocator);
	typedef void* (__cdecl* tCreateEffectRaw)(void* allocator);

	// Virtual member functions are __thiscall on x86, which passes this in ECX.
	// __fastcall matches that once the unused EDX argument is declared explicitly.
	typedef AKRESULT(__fastcall* tSetParamRaw)(void* self, void* unused, AkUInt32 paramId, const void* value, AkUInt32 size);
	typedef AKRESULT(__fastcall* tTermRaw)(void* self, void* unused, void* allocator);
	typedef AKRESULT(__fastcall* tInitRaw)(
		void* self, void* unused, void* allocator, void* context, void* param, void* format);

	struct PitchOverrideEvent
	{
		float originalCents;
		float appliedCents;
	};

	tCreateParamRaw originalCreateParam = nullptr;
	tCreateEffectRaw originalCreateEffect = nullptr;
	tSetParamRaw originalSetParam = nullptr;
	tTermRaw originalTerm = nullptr;
	tInitRaw originalInit = nullptr;
	tRegisterPlugin originalRegisterPlugin = nullptr;

	bool isSetParamHooked = false;

	// Transitioning suppresses Cable SetParam overrides while the audio-thread callback
	// restores every live MultiPitch object. ASIO ownership is published only afterwards.
	std::atomic<PitchOwner> pitchOwner{ PitchOwner::Cable };
	std::atomic<bool> pitchPushPending{ false };
	std::atomic<bool> isGlobalCallbackRegistered{ false };
	std::atomic<bool> inputShifterTransitionFailed{ false };
	bool hasReportedInputShifterUnavailable = false;
	std::atomic<unsigned long long> engineNoticeTick{ 0 };

	// The reference builder converts an arrangement's cent offset into the frequency
	// note detection expects, 440 * 2^(cents / 1200), and stamps it into the
	// detection object at song load. It is detoured so every consumer sees the
	// shifted reference, including the pre-song tuner, which snapshots its expected
	// pitches from that stamp the instant it lands.
	void* referenceBuilderTrampoline = nullptr;

	// Written by the game loop, read by the naked detour on the game's loading
	// thread. Aligned 32-bit loads and stores are atomic on x86.
	volatile LONG referenceCentsAdjustment = 0;
	volatile LONG playerTwoReferenceCentsAdjustment = 0;
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
	// an earlier build deadlocked against the game loop by doing both. It records
	// a small event here, and Poll logs it.
	PitchOverrideEvent overrideEvents[MAX_PENDING_EVENTS] = {};
	volatile LONG overrideEventCount = 0;

	void* firstParamObject = nullptr;
	volatile LONG hasParamObject = 0;
	volatile LONG paramObjectCount = 0;

	volatile LONG hasSeenEffectInstance = 0;
	volatile LONG initHookState = 0; // 0 not attempted, 1 hooked, -1 failed
	bool hasReportedInitHookFailure = false;            // Game loop only.

	// Objects the engine itself delivers pitch to. The create-param callback's return
	// value is not necessarily what a running effect reads: IAkPluginParam has Clone at
	// vtable slot 2, and writing to the original moved nothing. These are the objects
	// the engine drives, so they are the ones worth writing to.
	PVOID volatile deliveredParamObjects[MAX_PARAM_OBJECTS] = {};
	std::atomic<float> deliveredAuthoredCents[MAX_PARAM_OBJECTS] = {};
	volatile LONG deliveredParamObjectCount = 0;
	volatile LONG deliveredParamObjectPlayerIndices[MAX_PARAM_OBJECTS] = {};

	// Player attribution: Init's context holds, at +0x18, the mixer pipeline node
	// the effect renders into, and the node's ID at +0xC is per-player and stable
	// across sessions. The IDs are game-version specific; zero marks an
	// unresolved version, which resolves every object to Player One and logs the
	// IDs it saw so they can be added.
	constexpr uintptr_t CONTEXT_PIPELINE_NODE_OFFSET = 0x18;
	constexpr uintptr_t PIPELINE_NODE_ID_OFFSET = 0xC;
	VersioningStruct<uintptr_t> versionedPlayerOnePipelineNodeId{ { 0x5e22c1ab, 0 } };
	VersioningStruct<uintptr_t> versionedPlayerTwoPipelineNodeId{ { 0x5e22c1a8, 0 } };

	// Resolved once at Install so the bank-thread lookups read plain values.
	uintptr_t playerOnePipelineNodeId = 0;
	uintptr_t playerTwoPipelineNodeId = 0;

	constexpr LONG MAX_UNKNOWN_NODE_IDS = 4;
	volatile LONG unknownPipelineNodeIds[MAX_UNKNOWN_NODE_IDS] = {};
	LONG loggedUnknownNodeIdFlags = 0;                  // Game loop only.

	// Init-time pairings for param objects that are not in the delivered table
	// yet: the engine delivers a tone's params before the effect's Init runs, and
	// objects are reused across both players' tone loads, so the pairing is
	// per-delivery state, not per-object-lifetime.
	PVOID volatile pairedParamObjects[MAX_PARAM_OBJECTS] = {};
	volatile LONG pairedPlayerIndices[MAX_PARAM_OBJECTS] = {};
	volatile LONG pairedParamCursor = 0;

	// (detection, cents) pairs captured by the builder detour, identified on the
	// game loop where locks and logging are allowed.
	constexpr LONG BUILDER_CAPTURE_SLOTS = 8; // Power of two; the detour masks with 7.
	PVOID volatile builderCapturedDetections[BUILDER_CAPTURE_SLOTS] = {};
	volatile LONG builderCapturedCents[BUILDER_CAPTURE_SLOTS] = {};
	volatile LONG builderCaptureReserveCount = 0;
	volatile LONG builderCapturePublishCount = 0;
	LONG builderCaptureConsumedCount = 0;               // Game loop, under trueTuningMutex.

	void* identifiedPlayerOneDetection = nullptr;       // Game loop, under trueTuningMutex.
	PVOID volatile identifiedPlayerTwoDetection = nullptr; // Compared, never dereferenced, by the detour.
	LONG playerTwoAuthoredCents = 0;                    // Under trueTuningMutex.
	bool hasPlayerTwoAuthoredCents = false;
	bool sawPlayerOneBuilderCapture = false;
	void* pendingUnmatchedDetection = nullptr;
	LONG pendingUnmatchedCents = 0;
	bool hasPendingUnmatchedCapture = false;

	void HandleGlobalAudioCallback(bool isLastCall);

	bool CableOwnsPitch()
	{
		return pitchOwner.load(std::memory_order_acquire) == PitchOwner::Cable;
	}

	DropPedal::Player PlayerFromTag(LONG playerIndex)
	{
		return playerIndex == 1 ? DropPedal::Player::Two : DropPedal::Player::One;
	}

	void* LoadDeliveredParamObject(LONG index)
	{
		return InterlockedCompareExchangePointer(
			&deliveredParamObjects[index],
			nullptr,
			nullptr);
	}

	LONG LoadDeliveredParamObjectCount()
	{
		const LONG count = InterlockedCompareExchange(&deliveredParamObjectCount, 0, 0);
		return count < MAX_PARAM_OBJECTS ? count : MAX_PARAM_OBJECTS;
	}

	// The push carries no value: the audio-thread callback derives each object's
	// shift from the owner and pedal state at push time, so a hotkey change
	// queued mid-transition cannot overwrite a restore.
	void QueuePitchPush()
	{
		pitchPushPending.store(true, std::memory_order_release);
	}

	// Runs on the bank thread: no logging and no locks.
	void RecordUnknownPipelineNodeId(uintptr_t nodeId)
	{
		if (nodeId == 0) return;

		for (LONG i = 0; i < MAX_UNKNOWN_NODE_IDS; i++)
		{
			const LONG existing = InterlockedCompareExchange(
				&unknownPipelineNodeIds[i], (LONG)nodeId, 0);
			if (existing == 0 || existing == (LONG)nodeId) return;
		}
	}

	void LogUnknownPipelineNodeIds()
	{
		for (LONG i = 0; i < MAX_UNKNOWN_NODE_IDS; i++)
		{
			const LONG nodeId = InterlockedCompareExchange(&unknownPipelineNodeIds[i], 0, 0);
			if (nodeId == 0 || (loggedUnknownNodeIdFlags & (1 << i)) != 0) continue;

			loggedUnknownNodeIdFlags |= 1 << i;
			LOG_INFO("Drop pedal: unrecognized mixer node ID 0x"
				<< std::hex << (uintptr_t)(ULONG)nodeId << std::dec
				<< ". Cable multiplayer attribution does not know this game version; "
				"please report this ID with your game edition." << std::endl);
		}
	}

	// Runs on the bank thread: no logging and no locks.
	LONG ResolvePlayerIndexFromInitContext(void* context)
	{
		if (context == nullptr || MemUtil::IsBadReadPtr(context)) return 0;

		const uintptr_t node = *reinterpret_cast<const uintptr_t*>(
			reinterpret_cast<uintptr_t>(context) + CONTEXT_PIPELINE_NODE_OFFSET);
		if (node == 0
			|| MemUtil::IsBadReadPtr(reinterpret_cast<void*>(node + PIPELINE_NODE_ID_OFFSET)))
		{
			return 0;
		}

		const uintptr_t nodeId = *reinterpret_cast<const uintptr_t*>(node + PIPELINE_NODE_ID_OFFSET);
		if (playerOnePipelineNodeId != 0 && nodeId == playerOnePipelineNodeId) return 0;
		if (playerTwoPipelineNodeId != 0 && nodeId == playerTwoPipelineNodeId) return 1;

		RecordUnknownPipelineNodeId(nodeId);
		return 0;
	}

	// Runs on bank and audio threads: no logging and no locks.
	LONG LookupParamPlayerPairing(void* param)
	{
		for (LONG i = 0; i < MAX_PARAM_OBJECTS; i++)
		{
			if (InterlockedCompareExchangePointer(&pairedParamObjects[i], param, param) == param)
			{
				return InterlockedCompareExchange(&pairedPlayerIndices[i], 0, 0);
			}
		}

		return 0;
	}

	// Runs on the bank thread: no logging and no locks.
	void OnEffectInitialized(void* context, void* param)
	{
		if (param == nullptr) return;

		const LONG playerIndex = ResolvePlayerIndexFromInitContext(context);

		const LONG known = LoadDeliveredParamObjectCount();
		for (LONG i = 0; i < known; i++)
		{
			if (LoadDeliveredParamObject(i) == param)
			{
				InterlockedExchange(&deliveredParamObjectPlayerIndices[i], playerIndex);
				break;
			}
		}

		bool isStored = false;
		for (LONG i = 0; i < MAX_PARAM_OBJECTS; i++)
		{
			if (InterlockedCompareExchangePointer(&pairedParamObjects[i], param, param) == param)
			{
				InterlockedExchange(&pairedPlayerIndices[i], playerIndex);
				isStored = true;
				break;
			}
		}

		if (!isStored)
		{
			const LONG slot =
				(InterlockedIncrement(&pairedParamCursor) - 1) & (MAX_PARAM_OBJECTS - 1);
			InterlockedExchange(&pairedPlayerIndices[slot], playerIndex);
			InterlockedExchangePointer(
				const_cast<PVOID*>(&pairedParamObjects[slot]), param);
		}

		// Deliveries that landed before this pairing carried the default tag, so
		// the audio thread re-applies every object's owner-correct pitch.
		QueuePitchPush();
	}

	// Runs on the bank thread: no logging and no locks.
	AKRESULT __fastcall SpyInit(
		void* self, void* unused, void* allocator, void* context, void* param, void* format)
	{
		OnEffectInitialized(context, param);
		return originalInit(self, unused, allocator, context, param, format);
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

	// Patching Init here rather than from Poll catches even the first instance's
	// own Init, which runs right after creation. Runs on the bank thread: no
	// logging and no locks; Poll reports a failed patch.
	void* __cdecl SpyCreateEffect(void* allocator)
	{
		void* effect = originalCreateEffect(allocator);

		if (effect != nullptr
			&& InterlockedCompareExchange(&hasSeenEffectInstance, 1, 0) == 0)
		{
			uintptr_t* vtable = *(uintptr_t**)effect;
			if (!MemUtil::IsBadReadPtr(vtable))
			{
				originalInit = (tInitRaw)vtable[EFFECT_INIT_VTABLE_INDEX];

				void* replacement = (void*)SpyInit;
				if (MemUtil::PatchAdr(
					(LPVOID)&vtable[EFFECT_INIT_VTABLE_INDEX],
					(LPVOID)&replacement,
					sizeof(void*)))
				{
					InterlockedExchange(&initHookState, 1);
				}
				else
				{
					originalInit = nullptr;
					InterlockedExchange(&initHookState, -1);
				}
			}
		}

		return effect;
	}

	// Replace the pitch the game delivers to a pitch shifter. The value is passed
	// through a local, so the caller's own buffer is never written. Runs on bank and
	// audio threads, so no logging and no locks.
	AKRESULT __fastcall SpySetParam(void* self, void* unused, AkUInt32 paramId, const void* value, AkUInt32 size)
	{
		if (paramId != PITCH_PARAM_ID || size != sizeof(float) || value == nullptr)
		{
			return originalSetParam(self, unused, paramId, value, size);
		}

		const float authoredCents = *(const float*)value;

		const LONG deliveredKnown = LoadDeliveredParamObjectCount();
		bool isAlreadyTracked = false;
		LONG trackedSlot = -1;
		for (LONG i = 0; i < deliveredKnown && i < MAX_PARAM_OBJECTS; i++)
		{
			if (LoadDeliveredParamObject(i) == self)
			{
				// Objects are reused across tone loads, so the baseline has to follow
				// the tone currently loaded into this one rather than the first tone
				// ever seen through it.
				deliveredAuthoredCents[i].store(authoredCents, std::memory_order_relaxed);
				isAlreadyTracked = true;
				trackedSlot = i;
				break;
			}
		}

		if (!isAlreadyTracked)
		{
			const LONG playerIndex = LookupParamPlayerPairing(self);
			bool isStored = false;

			// Slots freed by Term are reclaimed first, so the table tracks the objects
			// currently alive rather than the first MAX_PARAM_OBJECTS ever created.
			for (LONG i = 0; i < deliveredKnown && i < MAX_PARAM_OBJECTS; i++)
			{
				if (InterlockedCompareExchangePointer(
					&deliveredParamObjects[i],
					RESERVED_PARAM_OBJECT,
					nullptr) == nullptr)
				{
					deliveredAuthoredCents[i].store(authoredCents, std::memory_order_relaxed);
					InterlockedExchange(&deliveredParamObjectPlayerIndices[i], playerIndex);
					InterlockedCompareExchangePointer(
						&deliveredParamObjects[i],
						self,
						RESERVED_PARAM_OBJECT);
					isStored = true;
					trackedSlot = i;
					break;
				}
			}

			if (!isStored)
			{
				const LONG slot = InterlockedIncrement(&deliveredParamObjectCount) - 1;
				if (slot < MAX_PARAM_OBJECTS)
				{
					deliveredAuthoredCents[slot].store(authoredCents, std::memory_order_relaxed);
					InterlockedExchange(&deliveredParamObjectPlayerIndices[slot], playerIndex);
					InterlockedCompareExchangePointer(
						&deliveredParamObjects[slot],
						self,
						nullptr);
					trackedSlot = slot;
				}
				else
				{
					// Keep the counter at the table size so reclaim keeps working
					// instead of the counter drifting past every future null slot.
					InterlockedDecrement(&deliveredParamObjectCount);
				}
			}
		}

		if (!DropPedalState::IsEnabled() || !CableOwnsPitch())
		{
			return originalSetParam(self, unused, paramId, value, size);
		}

		const LONG playerTag = trackedSlot >= 0
			? InterlockedCompareExchange(&deliveredParamObjectPlayerIndices[trackedSlot], 0, 0)
			: 0;
		float appliedCents = authoredCents + DropPedalState::GetTargetCents(PlayerFromTag(playerTag));

		const LONG index = InterlockedIncrement(&overrideEventCount) - 1;
		if (index < MAX_PENDING_EVENTS)
		{
			overrideEvents[index] = { authoredCents, appliedCents };
		}

		return originalSetParam(self, unused, paramId, &appliedCents, size);
	}

	// Forget a param object as the engine tears it down, so a later live push cannot
	// write into freed memory. Runs on whichever thread destroys it, so no logging
	// and no locks.
	AKRESULT __fastcall SpyTerm(void* self, void* unused, void* allocator)
	{
		const LONG known = LoadDeliveredParamObjectCount();

		for (LONG i = 0; i < known; i++)
		{
			if (LoadDeliveredParamObject(i) == self)
			{
				InterlockedCompareExchangePointer(
					&deliveredParamObjects[i],
					nullptr,
					self);
				deliveredAuthoredCents[i].store(0.0f, std::memory_order_relaxed);
				InterlockedExchange(&deliveredParamObjectPlayerIndices[i], 0);

				// The engine reuses freed param objects for either player's next
				// tone load, so a stale pairing must not outlive the object.
				for (LONG pair = 0; pair < MAX_PARAM_OBJECTS; pair++)
				{
					if (InterlockedCompareExchangePointer(
						&pairedParamObjects[pair], nullptr, self) == self)
					{
						InterlockedExchange(&pairedPlayerIndices[pair], 0);
						break;
					}
				}
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
			<< DropPedalState::GetTuningName(DropPedal::Player::One) << std::endl);
	}

	AKRESULT __cdecl SpyRegisterPlugin(AkPluginType type, AkUInt32 companyId, AkUInt32 pluginId, AkCreatePluginCallback createFunc, AkCreateParamCallback createParamFunc)
	{
		const bool isPitchShifter = companyId == 0 && pluginId == PITCH_SHIFTER_PLUGIN_ID;
		if (isPitchShifter)
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

			const PBYTE effectTrampoline = DetourFunction((PBYTE)createFunc, (PBYTE)SpyCreateEffect);
			if (effectTrampoline == nullptr)
			{
				LOG_ERROR("Drop pedal failed to hook the pitch shifter create-effect callback" << std::endl);
			}
			else
			{
				originalCreateEffect = (tCreateEffectRaw)effectTrampoline;
			}
		}

		const AKRESULT pluginResult = originalRegisterPlugin(type, companyId, pluginId, createFunc, createParamFunc);
		if (!isPitchShifter || pluginResult != AK_Success
			|| isGlobalCallbackRegistered.load(std::memory_order_acquire)) return pluginResult;

		const AKRESULT callbackResult = Wwise::SoundEngine::RegisterGlobalCallback(HandleGlobalAudioCallback);
		if (callbackResult != AK_Success)
		{
			LOG_ERROR("Drop pedal failed to register the Wwise audio-thread callback, result "
				<< callbackResult << ". Live pitch pushes and automatic ASIO ownership are "
				"unavailable; hotkey changes take effect on the next tone load." << std::endl);
			return pluginResult;
		}

		isGlobalCallbackRegistered.store(true, std::memory_order_release);
		return pluginResult;
	}

	// The builder takes its cent offset as a single stack argument and carries the
	// detection object it stamps in ESI. Adjusting the argument in place and
	// running the original means the game computes the shifted frequency with its
	// own math: non-A440 offsets and the -1200 emulated-bass case compose
	// naturally. Player Two's stamps take Player Two's adjustment once its
	// detection object is identified; unknown objects take Player One's, and the
	// guarded live writes correct such a stamp within a poll. Each (detection,
	// cents) pair is published for the game loop, which owns identification and
	// the authored-cents globals. Runs on the game's loading thread, so no
	// logging and no locks.
	__declspec(naked) void SpyReferenceBuilder()
	{
		__asm
		{
			push eax
			push ebx

			// Reserve a ring slot, fill it, then publish, so the game loop never
			// reads a slot that is still being written.
			mov eax, 1
			lock xadd dword ptr [builderCaptureReserveCount], eax
			and eax, 7 // BUILDER_CAPTURE_SLOTS - 1
			mov dword ptr [builderCapturedDetections + eax * 4], esi
			mov ebx, dword ptr [esp + 12]
			mov dword ptr [builderCapturedCents + eax * 4], ebx
			lock inc dword ptr [builderCapturePublishCount]

			mov eax, dword ptr [esp + 12]
			cmp esi, identifiedPlayerTwoDetection
			jne adjustAsPlayerOne
			add eax, dword ptr [playerTwoReferenceCentsAdjustment]
			jmp writeAdjustedCents

		adjustAsPlayerOne:
			add eax, dword ptr [referenceCentsAdjustment]

		writeAdjustedCents:
			mov dword ptr [esp + 12], eax
			pop ebx
			pop eax
			jmp referenceBuilderTrampoline
		}
	}

	/// <summary>
	/// Keep the cents adjustments in step with the pedal. Raising the reference makes
	/// detection expect the player's physical pitch: at a -2 target, 0 cents becomes
	/// +200 and A440 becomes ~A494. When the ASIO input shifter owns pitch the input
	/// itself is already retuned, so the reference stays authored.
	/// </summary>
	void UpdateReferenceCentsAdjustment()
	{
		const bool shouldTranspose = CableOwnsPitch() && DropPedalState::IsEnabled();
		const int playerOneSemitones = shouldTranspose
			? DropPedalState::GetTargetSemitones(DropPedal::Player::One)
			: 0;
		const int playerTwoSemitones = shouldTranspose
			? DropPedalState::GetTargetSemitones(DropPedal::Player::Two)
			: 0;

		InterlockedExchange(&referenceCentsAdjustment, (LONG)(-playerOneSemitones * 100));
		InterlockedExchange(&playerTwoReferenceCentsAdjustment, (LONG)(-playerTwoSemitones * 100));
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

	bool PushPitchToLiveShiftersOnAudioThread()
	{
		const LONG known = LoadDeliveredParamObjectCount();
		if (known == 0) return true;

		if (originalSetParam == nullptr || originalTerm == nullptr)
		{
			for (LONG i = 0; i < known; i++)
			{
				void* paramObject = LoadDeliveredParamObject(i);
				if (paramObject != nullptr && paramObject != RESERVED_PARAM_OBJECT) return false;
			}

			return true;
		}

		// During Transitioning the only correct push is the authored pitch, so the
		// owner decides the shift; otherwise each object follows its tagged
		// player's target. State reads are atomics: no locks on the audio thread.
		const bool restoreAuthoredPitch =
			pitchOwner.load(std::memory_order_acquire) == PitchOwner::Transitioning
			|| !DropPedalState::IsEnabled();

		bool allPitchesApplied = true;

		for (LONG i = 0; i < known; i++)
		{
			void* paramObject = LoadDeliveredParamObject(i);
			if (paramObject == nullptr || paramObject == RESERVED_PARAM_OBJECT)
			{
				continue;
			}

			const float shiftCents = restoreAuthoredPitch
				? 0.0f
				: DropPedalState::GetTargetCents(PlayerFromTag(
					InterlockedCompareExchange(&deliveredParamObjectPlayerIndices[i], 0, 0)));
			const float cents = deliveredAuthoredCents[i].load(std::memory_order_relaxed) + shiftCents;

			if (originalSetParam(paramObject, nullptr, PITCH_PARAM_ID, &cents, sizeof(float)) != AK_Success)
			{
				allPitchesApplied = false;
			}
		}

		return allPitchesApplied;
	}

	void HandleGlobalAudioCallback(bool isLastCall)
	{
		if (isLastCall)
		{
			isGlobalCallbackRegistered.store(false, std::memory_order_release);
			pitchPushPending.store(false, std::memory_order_release);
			return;
		}

		if (!pitchPushPending.exchange(false, std::memory_order_acq_rel)) return;

		if (!PushPitchToLiveShiftersOnAudioThread())
		{
			if (pitchOwner.load(std::memory_order_acquire) == PitchOwner::Transitioning)
			{
				inputShifterTransitionFailed.store(true, std::memory_order_release);
			}

			return;
		}

		PitchOwner expectedOwner = PitchOwner::Transitioning;
		pitchOwner.compare_exchange_strong(
			expectedOwner,
			PitchOwner::Asio,
			std::memory_order_release,
			std::memory_order_relaxed);
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

	void ClassifyPlayerTwoDetectionLocked(void* detection, LONG cents)
	{
		identifiedPlayerTwoDetection = detection;
		playerTwoAuthoredCents = cents;
		hasPlayerTwoAuthoredCents = true;
	}

	/// <summary>
	/// Identify players from the (detection, cents) pairs the builder detour
	/// captured. Player One's detection object is the one the ptr_trueTuning
	/// chain resolves into; any other captured object is Player Two's. Until a
	/// capture matches Player One, authored cents keep the pre-multiplayer
	/// last-wins behaviour and the unmatched pair is held for retroactive
	/// classification, so an unexpected game version cannot regress single player.
	/// </summary>
	void ProcessBuilderCapturesLocked()
	{
		const LONG published = InterlockedCompareExchange(&builderCapturePublishCount, 0, 0);
		if (builderCaptureConsumedCount == published) return;

		float currentTrueTuning = 0.0f;
		uintptr_t currentAddress = 0;
		if (!SongTuning::TryGetTrueTuning(currentTrueTuning, currentAddress))
		{
			// The chain is not up yet; captures stay pending for the next poll.
			return;
		}

		// The ring holds the newest BUILDER_CAPTURE_SLOTS captures; older ones
		// are already overwritten, so skip their sequence numbers.
		if (published - builderCaptureConsumedCount > BUILDER_CAPTURE_SLOTS)
		{
			builderCaptureConsumedCount = published - BUILDER_CAPTURE_SLOTS;
		}

		void* playerOneDetection = reinterpret_cast<void*>(
			currentAddress - Offsets::ptr_trueTuningOffsets.back());
		identifiedPlayerOneDetection = playerOneDetection;

		// The Player Two identity kept from the previous song is stale if the
		// allocator reused its address for this song's Player One detection.
		if (identifiedPlayerTwoDetection == playerOneDetection)
		{
			identifiedPlayerTwoDetection = nullptr;
		}

		while (builderCaptureConsumedCount < published)
		{
			const LONG slot = builderCaptureConsumedCount & (BUILDER_CAPTURE_SLOTS - 1);
			void* detection = builderCapturedDetections[slot];
			const LONG cents = builderCapturedCents[slot];
			builderCaptureConsumedCount++;

			if (detection == playerOneDetection)
			{
				sawPlayerOneBuilderCapture = true;
				InterlockedExchange(&authoredReferenceCents, cents);
				InterlockedExchange(&hasAuthoredReferenceCents, 1);

				if (hasPendingUnmatchedCapture
					&& pendingUnmatchedDetection != playerOneDetection)
				{
					ClassifyPlayerTwoDetectionLocked(pendingUnmatchedDetection, pendingUnmatchedCents);
				}

				hasPendingUnmatchedCapture = false;
			}
			else if (sawPlayerOneBuilderCapture)
			{
				ClassifyPlayerTwoDetectionLocked(detection, cents);
			}
			else
			{
				InterlockedExchange(&authoredReferenceCents, cents);
				InterlockedExchange(&hasAuthoredReferenceCents, 1);
				pendingUnmatchedDetection = detection;
				pendingUnmatchedCents = cents;
				hasPendingUnmatchedCapture = true;
			}
		}
	}

	/// <summary>
	/// Keep Player Two's detection reference in step with the pedal. Unlike
	/// Player One there is no pointer chain to re-resolve, so writes are guarded
	/// by a readability check and a plausibility window on the value already
	/// stamped there; a misidentified pointer fails those checks and is skipped.
	/// </summary>
	void ApplyPlayerTwoTrueTuningLocked()
	{
		void* detection = identifiedPlayerTwoDetection;
		if (detection == nullptr || !hasPlayerTwoAuthoredCents) return;

		const uintptr_t address =
			reinterpret_cast<uintptr_t>(detection) + Offsets::ptr_trueTuningOffsets.back();
		if (MemUtil::IsBadReadPtr(reinterpret_cast<void*>(address))) return;

		const float stampedValue = *reinterpret_cast<volatile float*>(address);
		if (!std::isfinite(stampedValue) || stampedValue < 100.0f || stampedValue > 1000.0f) return;

		const float authored = 440.0f * powf(2.0f, (float)playerTwoAuthoredCents / 1200.0f);
		const bool shouldTranspose = CableOwnsPitch() && DropPedalState::IsEnabled();
		const int targetSemitones = shouldTranspose
			? DropPedalState::GetTargetSemitones(DropPedal::Player::Two)
			: 0;
		const float targetTrueTuning = authored
			* powf(2.0f, -(float)targetSemitones / SEMITONES_PER_OCTAVE);

		if (AreTrueTuningValuesEqual(stampedValue, targetTrueTuning)) return;

		if (!MemUtil::PatchAdr(
			reinterpret_cast<LPVOID>(address),
			reinterpret_cast<LPVOID>(const_cast<float*>(&targetTrueTuning)),
			sizeof(targetTrueTuning)))
		{
			LOG_ERROR("Drop pedal failed to write player 2 true tuning at 0x"
				<< std::hex << address << std::dec << std::endl);
			return;
		}

		LOG_INFO("Drop pedal player 2 true tuning: authored " << authored
			<< " Hz, applied " << targetTrueTuning << " Hz, target "
			<< targetSemitones << " semitone(s)" << std::endl);
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
		const float comparisonValue = hasAppliedTrueTuning ? appliedTrueTuning : authoredTrueTuning;
		if (!AreTrueTuningValuesEqual(currentTrueTuning, comparisonValue))
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
		ProcessBuilderCapturesLocked();

		float currentTrueTuning = 0.0f;
		if (!CaptureOrRefreshTrueTuningLocked(currentTrueTuning))
		{
			return;
		}

		const bool shouldTransposeDetection = CableOwnsPitch() && DropPedalState::IsEnabled();
		const int targetSemitones = shouldTransposeDetection
			? DropPedalState::GetTargetSemitones(DropPedal::Player::One)
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

		ApplyPlayerTwoTrueTuningLocked();
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
	playerOnePipelineNodeId = versionedPlayerOnePipelineNodeId.GetValue();
	playerTwoPipelineNodeId = versionedPlayerTwoPipelineNodeId.GetValue();
	if (playerOnePipelineNodeId == 0)
	{
		LOG_INFO("Drop pedal: Cable multiplayer attribution has no mixer node IDs for "
			"this game version; Cable multiplayer uses one shared target." << std::endl);
	}

	// The engine notice starts counting here. Automatic starts game-side until the
	// ASIO chain proves itself; a forced asio engine claims pitch immediately so the
	// game-side MultiPitch path never runs, even if the chain later fails to appear.
	engineNoticeTick.store(GetTickCount64(), std::memory_order_relaxed);
	pitchOwner.store(
		DropPedalState::IsAsioEngine() ? PitchOwner::Asio : PitchOwner::Cable,
		std::memory_order_release);

	LOG_INFO("Drop pedal engine: "
		<< (DropPedalHooks::IsInputShifterActive() ? "ASIO Drop Pedal" : "Cable Drop Pedal") << std::endl);

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

	{
		std::lock_guard<std::mutex> lock(trueTuningMutex);
		ProcessBuilderCapturesLocked();
	}

	LogUnknownPipelineNodeIds();

	if (InterlockedCompareExchange(&initHookState, 0, 0) == -1 && !hasReportedInitHookFailure)
	{
		hasReportedInitHookFailure = true;
		LOG_ERROR("Drop pedal failed to hook the pitch shifter effect Init; Cable "
			"multiplayer attribution is unavailable." << std::endl);
	}
}

void DropPedalHooks::LogPendingOverrides()
{
	const LONG count = InterlockedExchange(&overrideEventCount, 0);
	const LONG usable = count < MAX_PENDING_EVENTS ? count : MAX_PENDING_EVENTS;

	// Tone loads and the game's own pitch glides deliver in bursts; only a change
	// in the applied shift is worth a line.
	static float lastLoggedShiftCents = 0.0f;
	static bool hasLoggedShift = false;

	for (LONG i = 0; i < usable; i++)
	{
		const float shiftCents = overrideEvents[i].appliedCents - overrideEvents[i].originalCents;
		if (hasLoggedShift && shiftCents == lastLoggedShiftCents) continue;

		lastLoggedShiftCents = shiftCents;
		hasLoggedShift = true;
		LOG_INFO("Drop pedal applied " << overrideEvents[i].appliedCents
			<< " cents in place of " << overrideEvents[i].originalCents << std::endl);
	}
}

void DropPedalHooks::PushPitchToLiveShifters()
{
	QueuePitchPush();
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

	const PitchOwner currentOwner = pitchOwner.load(std::memory_order_acquire);
	if (active && currentOwner != PitchOwner::Cable) return;
	if (!active && currentOwner == PitchOwner::Cable) return;

	if (active)
	{
		if (!isGlobalCallbackRegistered.load(std::memory_order_acquire))
		{
			inputShifterTransitionFailed.store(true, std::memory_order_release);
			return;
		}

		pitchOwner.store(PitchOwner::Transitioning, std::memory_order_release);
		UpdateReferenceCentsAdjustment();
		ApplyCapturedTrueTuning();
		QueuePitchPush();
	}
	else
	{
		pitchOwner.store(PitchOwner::Cable, std::memory_order_release);
		UpdateReferenceCentsAdjustment();
		QueuePitchPush();
		ApplyCapturedTrueTuning();
	}

	engineNoticeTick.store(GetTickCount64(), std::memory_order_relaxed);
	LOG_INFO("Drop pedal engine: " << (active ? "ASIO Drop Pedal" : "Cable Drop Pedal") << std::endl);
}

bool DropPedalHooks::IsInputShifterActive()
{
	return pitchOwner.load(std::memory_order_acquire) != PitchOwner::Cable;
}

bool DropPedalHooks::IsCableAttributionActive()
{
	return playerOnePipelineNodeId != 0;
}

bool DropPedalHooks::HasLivePlayerPedalTone(DropPedal::Player player)
{
	const LONG wantedIndex = player == DropPedal::Player::Two ? 1 : 0;
	const LONG known = LoadDeliveredParamObjectCount();

	for (LONG i = 0; i < known; i++)
	{
		void* paramObject = LoadDeliveredParamObject(i);
		if (paramObject == nullptr || paramObject == RESERVED_PARAM_OBJECT) continue;

		if (InterlockedCompareExchange(&deliveredParamObjectPlayerIndices[i], 0, 0) == wantedIndex)
		{
			return true;
		}
	}

	return false;
}

bool DropPedalHooks::ConsumeInputShifterTransitionFailure()
{
	return inputShifterTransitionFailed.exchange(false, std::memory_order_acq_rel);
}

bool DropPedalHooks::TryGetAuthoredTrueTuning(float& trueTuning)
{
	std::lock_guard<std::mutex> lock(trueTuningMutex);
	if (hasCapturedTrueTuning && hasAppliedTrueTuning)
	{
		trueTuning = authoredTrueTuning;
		return true;
	}

	if (!CableOwnsPitch()
		|| !DropPedalState::IsEnabled()
		|| referenceBuilderTrampoline == nullptr
		|| InterlockedCompareExchange(&hasAuthoredReferenceCents, 1, 1) != 1) return false;

	trueTuning = 440.0f * powf(2.0f, (float)authoredReferenceCents / 1200.0f);
	return true;
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

void DropPedalHooks::HandleArrangementTuning()
{
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
	InterlockedExchange(&authoredReferenceCents, 0);
	InterlockedExchange(&hasAuthoredReferenceCents, 0);

	// Player Two's identified detection pointer survives on purpose: the detour
	// only compares against it, never dereferences it, and keeping it lets the
	// next load's stamps take Player Two's adjustment before the tuner snapshots
	// them. ProcessBuilderCapturesLocked clears it if the address is reused.
	// Player Two's stamp is not restored: the builder re-stamps it on the next
	// load, and a write into a freed object would be worse than a stale value.
	identifiedPlayerOneDetection = nullptr;
	playerTwoAuthoredCents = 0;
	hasPlayerTwoAuthoredCents = false;
	sawPlayerOneBuilderCapture = false;
	hasPendingUnmatchedCapture = false;
	pendingUnmatchedCents = 0;
	builderCaptureConsumedCount = InterlockedCompareExchange(&builderCapturePublishCount, 0, 0);
}
