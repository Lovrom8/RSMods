#pragma once
#include "CCEffect.hpp"
#include "Settings.hpp"
#include "Mods/ExtendedRangeMode.hpp"

namespace CrowdControl::Effects {
	class SolidNotesRandomEffect : public CCEffect
	{
	public:
		SolidNotesRandomEffect(unsigned int durationSeconds) {
			duration = durationSeconds;
		}

		EffectResult Test();
		EffectResult Start();
		void Run();
		EffectResult Stop();
	};

	class SolidNotesCustomEffect : public CCEffect
	{
	public:
		SolidNotesCustomEffect(unsigned int durationSeconds) {
			duration = durationSeconds;
		}

		EffectResult Test();
		EffectResult Start();
		void Run();
		EffectResult Stop();
	};
}
