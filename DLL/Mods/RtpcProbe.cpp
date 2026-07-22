#include "../stdafx.h"
#include "RtpcProbe.hpp"

// Spy hooks on the game's Wwise setter functions, using the vendored Detours 1.5
// API: DetourFunction(target, detour) patches the target and returns a trampoline
// that runs the original. Every spy logs its arguments and calls the original, so
// this is pure observation. Code patching only, no sound engine calls, so it is
// safe to install at startup and catches every call the game makes.

// Trampolines returned by DetourFunction. Calling these runs the real function.
static tSetRTPCValue_Char originalSetRtpcValueChar = nullptr;
static tSetRTPCValue_RTPCID originalSetRtpcValueId = nullptr;
static tSetBusEffect_Char originalSetBusEffectChar = nullptr;
static tSetBusEffect_UniqueID originalSetBusEffectId = nullptr;
static tSetActorMixerEffect originalSetActorMixerEffect = nullptr;

// SetEffectParam's real signature is an unverified RE guess (AkUInt32, AkUInt16,
// void*). To survive the guess being short, the spy over-declares as six DWORDs
// and forwards all of them - harmless for __cdecl since the caller cleans the
// stack, but it means the real function gets every argument it actually expects.
typedef AKRESULT(__cdecl* tSetEffectParamRaw)(uint32_t a, uint32_t b, uint32_t c, uint32_t d, uint32_t e, uint32_t f);
static tSetEffectParamRaw originalSetEffectParam = nullptr;

static AKRESULT __cdecl SpySetRtpcValueChar(const char* rtpcName, AkRtpcValue value, AkGameObjectID gameObjectId, AkTimeMs valueChangeDuration, AkCurveInterpolation fadeCurve)
{
	LOG_INFO("SPY SetRTPCValue(char) name=" << (rtpcName ? rtpcName : "<null>") << " value=" << value << " obj=" << gameObjectId << std::endl);
	return originalSetRtpcValueChar(rtpcName, value, gameObjectId, valueChangeDuration, fadeCurve);
}

static AKRESULT __cdecl SpySetRtpcValueId(AkRtpcID rtpcId, AkRtpcValue value, AkGameObjectID gameObjectId, AkTimeMs valueChangeDuration, AkCurveInterpolation fadeCurve)
{
	LOG_INFO("SPY SetRTPCValue(id) id=" << rtpcId << " value=" << value << " obj=" << gameObjectId << std::endl);
	return originalSetRtpcValueId(rtpcId, value, gameObjectId, valueChangeDuration, fadeCurve);
}

static AKRESULT __cdecl SpySetBusEffectChar(const char* busName, AkUInt32 fxIndex, AkUniqueID shareSetId)
{
	LOG_INFO("SPY SetBusEffect(char) bus=" << (busName ? busName : "<null>") << " fxIndex=" << fxIndex << " shareSet=" << shareSetId << std::endl);
	return originalSetBusEffectChar(busName, fxIndex, shareSetId);
}

static AKRESULT __cdecl SpySetBusEffectId(AkUniqueID audioNodeId, AkUInt32 fxIndex, AkUniqueID shareSetId)
{
	LOG_INFO("SPY SetBusEffect(id) node=" << audioNodeId << " fxIndex=" << fxIndex << " shareSet=" << shareSetId << std::endl);
	return originalSetBusEffectId(audioNodeId, fxIndex, shareSetId);
}

static AKRESULT __cdecl SpySetActorMixerEffect(AkUniqueID audioNodeId, AkUInt32 fxIndex, AkUniqueID shareSetId)
{
	LOG_INFO("SPY SetActorMixerEffect node=" << audioNodeId << " fxIndex=" << fxIndex << " shareSet=" << shareSetId << std::endl);
	return originalSetActorMixerEffect(audioNodeId, fxIndex, shareSetId);
}

static AKRESULT __cdecl SpySetEffectParam(uint32_t a, uint32_t b, uint32_t c, uint32_t d, uint32_t e, uint32_t f)
{
	LOG_INFO("SPY SetEffectParam a=0x" << std::hex << a << " b=0x" << b << " c=0x" << c << std::dec << std::endl);

	// The third argument is believed to be a param-block pointer. If it reads as
	// valid memory, dump the first floats - a value like 1200.0 or -1200.0 here
	// would be the pitch cents we are hunting.
	if (c != 0 && !MemUtil::IsBadReadPtr((void*)c))
	{
		const float* floats = (const float*)c;
		LOG_INFO("SPY SetEffectParam floats: " << floats[0] << ", " << floats[1] << ", " << floats[2] << ", " << floats[3] << std::endl);
	}

	return originalSetEffectParam(a, b, c, d, e, f);
}

static bool HookOne(const char* name, uintptr_t target, PBYTE spyFunction, PBYTE* outOriginal)
{
	*outOriginal = DetourFunction((PBYTE)target, spyFunction);

	if (*outOriginal == nullptr)
	{
		LOG_ERROR("SPY failed to hook " << name << " at 0x" << std::hex << target << std::dec << std::endl);
		return false;
	}

	return true;
}

void RtpcProbe::InstallHooks()
{
	int installed = 0;

	installed += HookOne("SetRTPCValue(char)", Wwise::Exports::func_Wwise_Sound_SetRTPCValue_Char.Get(), (PBYTE)SpySetRtpcValueChar, (PBYTE*)&originalSetRtpcValueChar);
	installed += HookOne("SetRTPCValue(id)", Wwise::Exports::func_Wwise_Sound_SetRTPCValue_RTPCID.Get(), (PBYTE)SpySetRtpcValueId, (PBYTE*)&originalSetRtpcValueId);
	installed += HookOne("SetBusEffect(char)", Wwise::Exports::func_Wwise_Sound_SetBusEffect_Char.Get(), (PBYTE)SpySetBusEffectChar, (PBYTE*)&originalSetBusEffectChar);
	installed += HookOne("SetBusEffect(id)", Wwise::Exports::func_Wwise_Sound_SetBusEffect_UniqueID.Get(), (PBYTE)SpySetBusEffectId, (PBYTE*)&originalSetBusEffectId);
	installed += HookOne("SetActorMixerEffect", Wwise::Exports::func_Wwise_Sound_SetActorMixerEffect.Get(), (PBYTE)SpySetActorMixerEffect, (PBYTE*)&originalSetActorMixerEffect);
	installed += HookOne("SetEffectParam", Wwise::Exports::func_Wwise_Sound_SetEffectParam.Get(), (PBYTE)SpySetEffectParam, (PBYTE*)&originalSetEffectParam);

	LOG_INFO("=== Wwise spy: " << installed << "/6 hooks installed ===" << std::endl);
}
