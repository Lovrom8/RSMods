#pragma once
#include "../CCEffect.hpp"
#include "../CCEffectList.hpp"

namespace CrowdControl::Effects {
	class DoubleSpeedEffect : public CCEffect
	{
	public:
		DoubleSpeedEffect(unsigned int durationSeconds) {
			duration = durationSeconds;
		}

		EffectResult Test(Request request);
		EffectResult Start(Request request);
		void Run();
		EffectResult Stop();

	private:
		std::vector<std::string> incompatibleEffects =
			{ "halfsongspeed" };
	};
}

