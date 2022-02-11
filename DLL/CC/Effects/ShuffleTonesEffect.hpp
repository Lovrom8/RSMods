#pragma once
#include "../CCEffect.hpp"
#include "../../D3D/D3D.hpp"

namespace CrowdControl::Effects {
	class ShuffleTonesEffect : public CCEffect
	{
	public:
		ShuffleTonesEffect(unsigned int durationSeconds) {
			duration = durationSeconds;
		}

		unsigned int tickIntervalMilliseconds = 2000;
		std::chrono::steady_clock::time_point nextTickTime;

		EffectResult Test(Request request);
		EffectResult Start(Request request);
		void Run();
		EffectResult Stop();
	};
}