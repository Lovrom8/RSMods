#pragma once
#pragma once
#pragma once
#include "../CCEffect.hpp"

namespace CrowdControl::Effects {
	class RemoveNotesEffect : public CCEffect
	{
	public:
		RemoveNotesEffect(unsigned int durationSeconds) {
			duration = durationSeconds;
		}

		EffectResult Test(Request request);
		EffectResult Start(Request request);
		void Run();
		EffectResult Stop();
	};
}
