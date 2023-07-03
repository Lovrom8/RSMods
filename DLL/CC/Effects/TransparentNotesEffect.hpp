#pragma once
#include "../CCEffect.hpp"
#include "../CCEffectList.hpp"

namespace CrowdControl::Effects {
	class TransparentNotesEffect : public CCEffect
	{
	public:
		TransparentNotesEffect(int64_t durationMilliseconds) {
			duration_ms = durationMilliseconds;

			incompatibleEffects = { "solidcustom", "solidrandom", "solidcustomrgb", "bignoteheads", "smallnoteheads", "removenotes" };
		}

		EffectStatus Test(Request request) override;
		EffectStatus Start(Request request) override;
		EffectStatus Stop() override;
	};
}
