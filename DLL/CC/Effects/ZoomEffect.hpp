#pragma once
#include "../CCEffect.hpp"
#include "../CCEffectList.hpp"

namespace CrowdControl::Effects {
	class ZoomEffect : public CCEffect
	{
	public:
		float factor = 1.0;

		ZoomEffect(int64_t durationMilliseconds, float zoomFactor) : factor(zoomFactor)
		{
			duration_ms = durationMilliseconds;

			incompatibleEffects = { "zoomin", "zoomout" };
		}

		EffectStatus Test(Request request) override;
		EffectStatus Start(Request request) override;
		EffectStatus Stop() override;
	};
}
