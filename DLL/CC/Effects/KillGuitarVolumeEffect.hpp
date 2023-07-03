#pragma once
#include "../CCEffect.hpp"
#include "../CCEffectList.hpp"
#include "../../Wwise/Types.hpp"
#include "../../Wwise/SoundEngine.hpp"
#include "../../MemHelpers.hpp"

namespace CrowdControl::Effects {
	class KillGuitarVolumeEffect : public CCEffect
	{
	public:
		KillGuitarVolumeEffect(int64_t durationMilliseconds) {
			duration_ms = durationMilliseconds;
		}

		EffectStatus Test(Request request) override;
		EffectStatus Start(Request request) override;
		void Run() override;
		EffectStatus Stop() override;

	private:
		float oldVolume = 100.0f;
		bool ending = false;
	};
}
#pragma once
