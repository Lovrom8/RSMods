#pragma once
#include "../CCEffect.hpp"
#include "../CCEffectList.hpp"

namespace CrowdControl::Effects {
	class SmallNoteheadEffect : public CCEffect
	{
	public:
		SmallNoteheadEffect(int64_t durationMilliseconds) {
			duration_ms = durationMilliseconds;

			incompatibleEffects = { "transparentnotes", "bignoteheads", "removenotes" };
		}

		EffectStatus Test(Request request) override;
		EffectStatus Start(Request request) override;
		EffectStatus Stop() override;

	private:
		static void SetNoteHeadScale(float scale);
	};
}
