#pragma once
#include "../CCEffect.hpp"
#include "../../Mods/ExtendedRangeMode.hpp"

namespace CrowdControl::Effects {
	class RainbowEffect : public CCEffect
	{
	public:
		RainbowEffect(unsigned int durationSeconds) {
			duration = durationSeconds;
		}

		EffectResult Test(Request request);
		EffectResult Start(Request request);
		void Run();
		EffectResult Stop();
	};
}

