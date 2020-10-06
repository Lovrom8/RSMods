#pragma once
#include "../CCEffect.hpp"

namespace CrowdControl::Effects {
	class ZoomInEffect : public CCEffect
	{
	public:
		ZoomInEffect(unsigned int durationSeconds) {
			duration = durationSeconds;
		}

		EffectResult Test(Request request);
		EffectResult Start(Request request);
		void Run();
		EffectResult Stop();
	};
}
