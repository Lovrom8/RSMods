#pragma once
#include "../CCEffect.hpp"
#include "../../Mods/RiffRepeater.hpp"

namespace CrowdControl::Effects {
	class TurboSpeedEffect : public CCEffect
	{
	public:
		TurboSpeedEffect(unsigned int durationSeconds) {
			duration = durationSeconds;
		}

		EffectResult Test(Request request);
		EffectResult Start(Request request);
		void Run();
		EffectResult Stop();
	};
}
