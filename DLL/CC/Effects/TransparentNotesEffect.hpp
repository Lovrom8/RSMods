#pragma once
#include "../CCEffect.hpp"
#include "../CCEffectList.hpp"

namespace CrowdControl::Effects {
	class TransparentNotesEffect : public CCEffect
	{
	public:
		TransparentNotesEffect(unsigned int durationSeconds) {
			duration = durationSeconds;
		}

		EffectResult Test(Request request);
		EffectResult Start(Request request);
		void Run();
		EffectResult Stop();

	private:
		std::vector<std::string> incompatibleEffects =
			{ "solidcustom", "solidrandom", "solidcustomrgb", "bignoteheads", "smallnoteheads", "removenotes" };
	};
}
