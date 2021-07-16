#pragma once
#include "../CCEffect.hpp"
#include "../CCEffectList.hpp"

namespace CrowdControl::Effects {
	class BigNoteheadEffect : public CCEffect
	{
	public:
		BigNoteheadEffect(unsigned int durationSeconds) {
			duration = durationSeconds;
		}

		EffectResult Test(Request request);
		EffectResult Start(Request request);
		void Run();
		EffectResult Stop();

	private:
		void SetNoteHeadScale(float scale);

		std::vector<std::string> incompatibleEffects =
			{ "transparentnotes", "smallnoteheads", "removenotes" };
	};
}
