#pragma once
#pragma once
#include "CCEffect.hpp"
#include "Settings.hpp"

namespace CrowdControl::Effects {
	class TransparentNotesEffect : public CCEffect
	{
	public:
		TransparentNotesEffect(unsigned int durationSeconds) {
			duration = durationSeconds;
		}

		EffectResult Test();
		EffectResult Start();
		void Run();
		EffectResult Stop();
	};
}
