#pragma once
#include "../CCEffect.hpp"
#include "../../WwiseHijack.hpp"
#include "../../MemHelpers.hpp"

namespace CrowdControl::Effects {
	class KillGuitarVolumeEffect : public CCEffect
	{
	public:
		KillGuitarVolumeEffect(unsigned int durationSeconds) {
			duration = durationSeconds;
		}

		EffectResult Test(Request request);
		EffectResult Start(Request request);
		void Run();
		EffectResult Stop();

		float oldVolume = 100.0f;
	};
}
#pragma once
