#pragma once
#include "../CCEffect.hpp"
#include "../CCEffectList.hpp"

namespace CrowdControl::Effects {
	class RemoveInstrumentEffect : public CCEffect
	{
	public:
		RemoveInstrumentEffect(unsigned int durationSeconds) {
			duration = durationSeconds;
		}

		EffectResult Test(Request request);
		EffectResult Start(Request request);
		void Run();
		EffectResult Stop();

	private:
		void SetInstrumentScale(float scale);

		std::vector<std::string> incompatibleEffects =
			{ "invertedstrings" };
	};
}