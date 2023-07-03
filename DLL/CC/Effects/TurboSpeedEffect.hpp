#pragma once
#include "../CCEffect.hpp"
#include "../CCEffectList.hpp"
#include "../../Mods/RiffRepeater.hpp"

namespace CrowdControl::Effects {
	class TurboSpeedEffect : public CCEffect
	{
	public:
		TurboSpeedEffect(int64_t durationMilliseconds) {
			duration_ms = durationMilliseconds;
		}

		EffectStatus Test(Request request) override;
		EffectStatus Start(Request request) override;
		EffectStatus Stop() override;
	};
}
