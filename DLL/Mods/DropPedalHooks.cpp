#include "../stdafx.h"
#include "DropPedalHooks.hpp"
#include "DropPedalState.hpp"
#include "TrueTuning.hpp"

namespace
{
	constexpr LONG MAX_PENDING_EVENTS = 64;
	constexpr int SET_PARAM_VTABLE_INDEX = 1;
	constexpr int TERM_VTABLE_INDEX = 4;
	constexpr AkUInt32 PITCH_SHIFTER_PLUGIN_ID = 136;
	constexpr AkUInt32 PITCH_PARAM_ID = 6;
	constexpr LONG MAX_PARAM_OBJECTS = 16;

	typedef void* (__cdecl* tCreateParamRaw)(void* allocator);
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
	bool inputShifterActive = false;
	bool hasCapturedSongTuning = false;
	unsigned long long engineNoticeTick = 0;

	PitchOverrideEvent overrideEvents[MAX_PENDING_EVENTS] = {};
	volatile LONG overrideEventCount = 0;

	void* firstParamObject = nullptr;
	volatile LONG hasParamObject = 0;
	volatile LONG paramObjectCount = 0;

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

	AKRESULT __fastcall SpySetParam(void* self, void* unused, AkUInt32 paramId, const void* value, AkUInt32 size)
	{
		if (paramId != PITCH_PARAM_ID || size != sizeof(float) || value == nullptr)
		{
			return originalSetParam(self, unused, paramId, value, size);
		}

		const float authoredCents = *(const float*)value;

		const LONG deliveredKnown = deliveredParamObjectCount;
		bool isAlreadyTracked = false;
		for (LONG i = 0; i < deliveredKnown && i < MAX_PARAM_OBJECTS; i++)
		{
			if (deliveredParamObjects[i] == self)
			{
				deliveredAuthoredCents[i] = authoredCents;
				isAlreadyTracked = true;
				break;
			}
		}

		if (!isAlreadyTracked)
		{
			const LONG slot = InterlockedIncrement(&deliveredParamObjectCount) - 1;
			if (slot < MAX_PARAM_OBJECTS)
			{
				deliveredParamObjects[slot] = self;
				deliveredAuthoredCents[slot] = authoredCents;
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
	engineNoticeTick = GetTickCount64();
	LOG_INFO("Drop pedal engine: Cable Drop Pedal" << std::endl);

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

void DropPedalHooks::PushPitchToLiveShifters()
{
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

		const float cents = deliveredAuthoredCents[i] + DropPedalState::GetTargetCents();

		originalSetParam(paramObject, nullptr, PITCH_PARAM_ID, &cents, sizeof(float));
	}
}

void DropPedalHooks::SetInputShifterActive(bool active)
{
	if (inputShifterActive == active) return;

	inputShifterActive = active;
	engineNoticeTick = GetTickCount64();
	LOG_INFO("Drop pedal engine: " << (active ? "ASIO Drop Pedal" : "Cable Drop Pedal") << std::endl);
}

bool DropPedalHooks::IsInputShifterActive()
{
	return inputShifterActive;
}

unsigned long long DropPedalHooks::GetEngineNoticeTick()
{
	return engineNoticeTick;
}

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
