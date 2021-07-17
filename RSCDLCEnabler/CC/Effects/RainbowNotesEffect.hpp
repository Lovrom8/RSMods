#pragma once
#include "../CCEffect.hpp"
#include "../../Mods/ExtendedRangeMode.hpp"
#include "../CCEffectList.hpp"

namespace CrowdControl::Effects {
	class RainbowNotesEffect : public CCEffect
	{
	public:
		RainbowNotesEffect(unsigned int durationSeconds) {
			duration = durationSeconds;
		}

		EffectResult Test(Request request);
		EffectResult Start(Request request);
		void Run();
		EffectResult Stop();

	private:
		std::vector<std::string> incompatibleEffects =
		{ "removenotes", "transparentnotes", "solidrandom",  "solidcustom", "solidcustomrgb" };
	};
}
