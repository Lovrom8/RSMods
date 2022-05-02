#pragma once
#include "../CCEffect.hpp"
#include "../CCEffectList.hpp"
#include "../../MemUtil.hpp"

namespace CrowdControl::Effects {
	class HighwayScrollSpeedEffect : public CCEffect
	{
	public:
		double multiplier = 5.0;

		HighwayScrollSpeedEffect(unsigned int durationSeconds, double speedMultiplier) {
			duration = durationSeconds;
			multiplier = speedMultiplier;
		}

		EffectResult Test(Request request);
		EffectResult Start(Request request);
		void Run();
		EffectResult Stop();

	private:
		std::vector<std::string> incompatibleEffects = { "halfscrollspeed", "doublescrollspeed", "triplescrollspeed" };

		void WriteScrollSpeedMultiplier(double val);
	};
}

#pragma once
