#pragma once
#include "../CCEffect.hpp"
#include "../../Mods/ExtendedRangeMode.hpp"
#include "../CCEffectList.hpp"

namespace CrowdControl::Effects {
	class RainbowStringsEffect : public CCEffect
	{
	public:
		RainbowStringsEffect(unsigned int durationSeconds) {
			duration = durationSeconds;
		}

		EffectResult Test(Request request);
		EffectResult Start(Request request);
		void Run();
		EffectResult Stop();
	};
}

