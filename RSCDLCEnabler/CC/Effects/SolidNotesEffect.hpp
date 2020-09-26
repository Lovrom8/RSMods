#pragma once
#include "../CCEffect.hpp"
#include "../../Mods/ExtendedRangeMode.hpp"

namespace CrowdControl::Effects {
	class SolidNotesRandomEffect : public CCEffect
	{
	public:
		SolidNotesRandomEffect(unsigned int durationSeconds) {
			duration = durationSeconds;
		}

		EffectResult Test(Request request);
		EffectResult Start(Request request);
		void Run();
		EffectResult Stop();
	};

	class SolidNotesCustomEffect : public CCEffect
	{
	public:
		SolidNotesCustomEffect(unsigned int durationSeconds) {
			duration = durationSeconds;
		}

		EffectResult Test(Request request);
		EffectResult Start(Request request);
		void Run();
		EffectResult Stop();
	};

	class SolidNotesCustomRGBEffect : public CCEffect
	{
	public:
		SolidNotesCustomRGBEffect(unsigned int durationSeconds) {
			duration = durationSeconds;
		}

		EffectResult Test(Request request);
		EffectResult Start(Request request);
		void Run();
		EffectResult Stop();
	};
}
