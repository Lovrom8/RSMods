#pragma once
#include "../CCEffect.hpp"
#include "../CCEffectList.hpp"

namespace CrowdControl::Effects {
	// Scales notes in a song to unusually big size
	class BigNoteheadEffect : public CCEffect
	{
	public:
		BigNoteheadEffect(int64_t durationMilliseconds) {
			duration_ms = durationMilliseconds;

			incompatibleEffects = { "transparentnotes", "smallnoteheads", "removenotes" };
		}

		EffectStatus Test(Request request) override;
		EffectStatus Start(Request request) override;
		EffectStatus Stop() override;

	private:
		static void SetNoteHeadScale(float scale);
	};
}
