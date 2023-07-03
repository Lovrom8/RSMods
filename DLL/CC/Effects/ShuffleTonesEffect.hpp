#pragma once
#include "../CCEffect.hpp"
#include "../CCEffectList.hpp"
#include "../../D3D/D3D.hpp"

namespace CrowdControl::Effects {
	class ShuffleTonesEffect : public CCEffect
	{
	public:
		ShuffleTonesEffect(int64_t durationMilliseconds) {
			duration_ms = durationMilliseconds;
		}

		uint32_t tickIntervalMilliseconds = 2000;
		std::chrono::steady_clock::time_point nextTickTime;

		EffectStatus Test(Request request) override;
		EffectStatus Start(Request request) override;
		void Run() override;
		EffectStatus Stop() override;
	};
}