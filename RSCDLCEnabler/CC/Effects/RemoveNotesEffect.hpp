#pragma once
#include "../CCEffect.hpp"

namespace CrowdControl::Effects {
	class RemoveNotesEffect : public CCEffect
	{
	public:
		RemoveNotesEffect(unsigned int durationSeconds) {
			duration = durationSeconds;
		}

		unsigned int tickIntervalMilliseconds = 1000;
		std::chrono::steady_clock::time_point nextTickTime;

		EffectResult Test(Request request);
		EffectResult Start(Request request);
		void Run();
		EffectResult Stop();
	};
}
