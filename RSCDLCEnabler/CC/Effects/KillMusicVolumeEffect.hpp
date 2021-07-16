#pragma once
#include "../CCEffect.hpp"
#include "../../WwiseHijack.hpp"
#include "../CCEffectList.hpp"

namespace CrowdControl::Effects {
	class KillMusicVolumeEffect : public CCEffect
	{
	public:
		KillMusicVolumeEffect(unsigned int durationSeconds) {
			duration = durationSeconds;
		}

		EffectResult Test(Request request);
		EffectResult Start(Request request);
		void Run();
		EffectResult Stop();

		float oldVolume = 100.0f;
		bool ending = false;
	};
}