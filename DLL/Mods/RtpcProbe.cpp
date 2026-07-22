#include "../stdafx.h"
#include "RtpcProbe.hpp"

namespace
{
	constexpr int FRAMES_BEFORE_INSPECT = 30;
	constexpr int FLOATS_TO_DUMP = 24;
	constexpr int VTABLE_ENTRIES_TO_DUMP = 8;
	constexpr LONG MAX_PENDING_OBJECTS = 256;

	constexpr AkUInt32 HARMONIZER_PLUGIN_ID = 138;

	// Each Harmonizer voice stores its pitch as a frequency ratio, 2^(cents / 1200),
	// so 0.5 is an octave down and 2.0 an octave up.
	constexpr int VOICE_ONE_RATIO_INDEX = 6;
	constexpr int VOICE_TWO_RATIO_INDEX = 13;

	constexpr float OCTAVE_DOWN_RATIO = 0.5f;
	constexpr float RATIO_TOLERANCE = 0.0005f;
	constexpr float CENTS_PER_KEYPRESS = 100.0f;
	constexpr float MIN_TEST_CENTS = -2400.0f;
	constexpr float MAX_TEST_CENTS = 2400.0f;

	typedef void* (__cdecl* tCreateParamRaw)(void* allocator);

	struct PendingInspection
	{
		void* paramObject;
		int framesRemaining;
	};

	// A Harmonizer voice found at the emulated bass octave, and so a candidate for
	// retuning. The ratio is tracked rather than the object because one param object
	// holds two voices.
	struct RetunableVoice
	{
		float* ratio;
	};

	tCreateParamRaw originalCreateParam = nullptr;
	tRegisterPlugin originalRegisterPlugin = nullptr;

	bool wasVtableLogged = false;

	// Param objects are created on the bank and audio threads, so the hooks must not
	// log or take a lock: an earlier build deadlocked against Poll by doing both.
	// They only publish a pointer here, and Poll drains it.
	void* newParamObjects[MAX_PENDING_OBJECTS] = {};
	volatile LONG newParamObjectCount = 0;

	std::vector<PendingInspection> pendingInspections;
	std::vector<RetunableVoice> retunableVoices;
	std::mutex trackingMutex;

	float testCents = -1200.0f;
	bool wasLowerKeyDown = false;
	bool wasRaiseKeyDown = false;

	float CentsToRatio(float cents)
	{
		return powf(2.0f, cents / 1200.0f);
	}

	void PublishParamObject(void* paramObject)
	{
		if (paramObject == nullptr)
		{
			return;
		}

		const LONG index = InterlockedIncrement(&newParamObjectCount) - 1;
		if (index < MAX_PENDING_OBJECTS)
		{
			newParamObjects[index] = paramObject;
		}
	}

	void* __cdecl SpyCreateParam(void* allocator)
	{
		void* paramObject = originalCreateParam(allocator);
		PublishParamObject(paramObject);
		return paramObject;
	}

	void LogVtableOnce(uintptr_t* vtable)
	{
		if (wasVtableLogged)
		{
			return;
		}

		std::ostringstream entries;
		for (int i = 0; i < VTABLE_ENTRIES_TO_DUMP; i++)
		{
			entries << " [" << i << "]=0x" << std::hex << vtable[i] << std::dec;
		}

		LOG_INFO("SPY Harmonizer param vtable at 0x" << std::hex << (uintptr_t)vtable << std::dec << entries.str() << std::endl);
		wasVtableLogged = true;
	}

	void TrackOctaveDownVoices(float* floats)
	{
		const int voiceIndices[] = { VOICE_ONE_RATIO_INDEX, VOICE_TWO_RATIO_INDEX };

		for (const int voiceIndex : voiceIndices)
		{
			if (fabsf(floats[voiceIndex] - OCTAVE_DOWN_RATIO) > RATIO_TOLERANCE)
			{
				continue;
			}

			retunableVoices.push_back({ &floats[voiceIndex] });

			LOG_INFO("SPY tracking octave-down voice at 0x" << std::hex << (uintptr_t)&floats[voiceIndex] << std::dec
				<< " (index " << voiceIndex << "), now tracking " << retunableVoices.size() << std::endl);
		}
	}

	void InspectParamObject(void* paramObject)
	{
		if (MemUtil::IsBadReadPtr(paramObject))
		{
			LOG_WARNING("SPY param object 0x" << std::hex << (uintptr_t)paramObject << std::dec << " is no longer readable" << std::endl);
			return;
		}

		// Index 0 is the vtable pointer, so the parameter fields start at index 1.
		float* floats = (float*)paramObject;

		std::ostringstream values;
		for (int i = 1; i < FLOATS_TO_DUMP; i++)
		{
			values << " [" << i << "]=" << floats[i];
		}
		LOG_INFO("SPY params" << values.str() << std::endl);

		LogVtableOnce(*(uintptr_t**)paramObject);
		TrackOctaveDownVoices(floats);
	}

	void DrainNewParamObjects()
	{
		const LONG count = InterlockedExchange(&newParamObjectCount, 0);
		const LONG usable = count < MAX_PENDING_OBJECTS ? count : MAX_PENDING_OBJECTS;

		for (LONG i = 0; i < usable; i++)
		{
			pendingInspections.push_back({ newParamObjects[i], FRAMES_BEFORE_INSPECT });
			newParamObjects[i] = nullptr;
		}
	}

	/// <summary>
	/// Hold every tracked voice at the requested pitch. Re-applied every frame rather
	/// than written once, so the tone system re-applying its own parameters cannot
	/// quietly undo it.
	/// </summary>
	void EnforceTestCents()
	{
		const float ratio = CentsToRatio(testCents);

		for (const RetunableVoice& voice : retunableVoices)
		{
			if (MemUtil::IsBadReadPtr(voice.ratio))
			{
				continue;
			}

			*voice.ratio = ratio;
		}
	}

	void HandleRetuneHotkeys()
	{
		const bool isLowerKeyDown = (GetAsyncKeyState(VK_F9) & 0x8000) != 0;
		const bool isRaiseKeyDown = (GetAsyncKeyState(VK_F10) & 0x8000) != 0;

		bool didChange = false;

		if (isLowerKeyDown && !wasLowerKeyDown && testCents > MIN_TEST_CENTS)
		{
			testCents -= CENTS_PER_KEYPRESS;
			didChange = true;
		}

		if (isRaiseKeyDown && !wasRaiseKeyDown && testCents < MAX_TEST_CENTS)
		{
			testCents += CENTS_PER_KEYPRESS;
			didChange = true;
		}

		if (didChange)
		{
			LOG_INFO("SPY holding " << retunableVoices.size() << " voices at " << testCents << " cents (ratio " << CentsToRatio(testCents) << ")" << std::endl);
		}

		wasLowerKeyDown = isLowerKeyDown;
		wasRaiseKeyDown = isRaiseKeyDown;
	}

	AKRESULT __cdecl SpyRegisterPlugin(AkPluginType type, AkUInt32 companyId, AkUInt32 pluginId, AkCreatePluginCallback createFunc, AkCreateParamCallback createParamFunc)
	{
		if (companyId == 0 && pluginId == HARMONIZER_PLUGIN_ID)
		{
			const PBYTE trampoline = DetourFunction((PBYTE)createParamFunc, (PBYTE)SpyCreateParam);
			if (trampoline == nullptr)
			{
				LOG_ERROR("SPY failed to hook the Harmonizer create-param callback" << std::endl);
			}
			else
			{
				originalCreateParam = (tCreateParamRaw)trampoline;
				LOG_INFO("SPY Harmonizer create-param hooked at 0x" << std::hex << (uintptr_t)createParamFunc << std::dec << std::endl);
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
		LOG_ERROR("SPY failed to hook RegisterPlugin at 0x" << std::hex << target << std::dec << std::endl);
		return;
	}

	LOG_INFO("=== Drop pedal probe build 5, F9 lowers and F10 raises by 100 cents ===" << std::endl);
}

void RtpcProbe::Poll()
{
	std::lock_guard<std::mutex> lock(trackingMutex);

	DrainNewParamObjects();

	for (size_t i = 0; i < pendingInspections.size(); )
	{
		PendingInspection& inspection = pendingInspections[i];
		inspection.framesRemaining--;

		if (inspection.framesRemaining > 0)
		{
			i++;
			continue;
		}

		InspectParamObject(inspection.paramObject);
		pendingInspections.erase(pendingInspections.begin() + i);
	}

	HandleRetuneHotkeys();
	EnforceTestCents();
}
