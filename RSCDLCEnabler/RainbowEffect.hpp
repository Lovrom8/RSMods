#pragma once
#include "CCEffect.hpp"

namespace CrowdControl::Effects {
	class RainbowEffect : public CCEffect
	{
	public:
		RainbowEffect(unsigned int durationSeconds) {
			duration = durationSeconds;
		}

		EffectResult Test();
		EffectResult Start();
		void Run();
		EffectResult Stop();
	};
}

