#pragma once
#include "CCEffect.hpp"
#include "DrunkModeEffect.hpp"
#include "MemHelpers.hpp"

namespace CrowdControl::Effects {
	class DrunkModeEffect : public CCEffect
	{
	public:
		DrunkModeEffect(unsigned int durationSeconds) {
			duration = durationSeconds;
		}

		EffectResult Test();
		EffectResult Start();
		void Run();
		EffectResult Stop();
	};
}

