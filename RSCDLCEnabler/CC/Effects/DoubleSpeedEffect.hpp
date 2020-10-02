#pragma once
#include "../CCEffect.hpp"

namespace CrowdControl::Effects {
	class DoubleSpeedEffect : public CCEffect
	{
	public:
		DoubleSpeedEffect(unsigned int durationSeconds) {
			duration = durationSeconds;
		}

		EffectResult Test(Request request);
		EffectResult Start(Request request);
		void Run();
		EffectResult Stop();
	};
}

