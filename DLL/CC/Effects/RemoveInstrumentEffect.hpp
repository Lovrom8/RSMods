#pragma once
#include "../CCEffect.hpp"
#include "../CCEffectList.hpp"

namespace CrowdControl::Effects {
	class RemoveInstrumentEffect : public CCEffect
	{
	public:
		RemoveInstrumentEffect(int64_t durationMilliseconds) {
			duration_ms = durationMilliseconds;

			incompatibleEffects = { "invertedstrings", "rainbowstrings" };
		}

		EffectStatus Test(Request request) override;
		EffectStatus Start(Request request) override;
		EffectStatus Stop() override;

	private:
		static void SetInstrumentScale(float scale);
	};
}