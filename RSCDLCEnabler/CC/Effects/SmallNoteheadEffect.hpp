#pragma once
#include "../CCEffect.hpp"

namespace CrowdControl::Effects {
	class SmallNoteheadEffect : public CCEffect
	{
	public:
		SmallNoteheadEffect(unsigned int durationSeconds) {
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

#pragma once
#pragma once
