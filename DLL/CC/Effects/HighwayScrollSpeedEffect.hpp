#pragma once
#include "../CCEffect.hpp"
#include "../CCEffectList.hpp"
#include "../../MemUtil.hpp"

namespace CrowdControl::Effects {
	class HighwayScrollSpeedEffect : public CCEffect
	{
	public:
		double multiplier = 5.0;

		HighwayScrollSpeedEffect(int64_t durationMilliseconds, double speedMultiplier) : multiplier(speedMultiplier)
		{
			duration_ms = durationMilliseconds;

			incompatibleEffects = { "halfscrollspeed", "doublescrollspeed", "triplescrollspeed" };
		}

		EffectStatus Test(Request request);
		EffectStatus Start(Request request);
		EffectStatus Stop();

	private:
		static void WriteScrollSpeedMultiplier(double val);
	};
}

#pragma once
