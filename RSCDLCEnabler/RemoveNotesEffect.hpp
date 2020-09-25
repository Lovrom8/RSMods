#pragma once
#pragma once
#pragma once
#include "CCEffect.hpp"
#include "Settings.hpp"

namespace CrowdControl::Effects {
	class RemoveNotesEffect : public CCEffect
	{
	public:
		RemoveNotesEffect(unsigned int durationSeconds) {
			duration = durationSeconds;
		}

		EffectResult Test();
		EffectResult Start();
		void Run();
		EffectResult Stop();
	};
}
