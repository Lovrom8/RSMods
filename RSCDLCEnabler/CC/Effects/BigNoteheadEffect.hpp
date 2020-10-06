#pragma once
#include "../CCEffect.hpp"

namespace CrowdControl::Effects {
	class BigNoteheadEffect : public CCEffect
	{
	public:
		BigNoteheadEffect(unsigned int durationSeconds) {
			duration = durationSeconds;
		}

		EffectResult Test(Request request);
		EffectResult Start(Request request);
		void Run();
		EffectResult Stop();

	private:
		void SetNoteHeadScale(float scale);
	};
}
