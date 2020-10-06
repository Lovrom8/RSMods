#pragma once
#include "../CCEffect.hpp"
#include "../CCEffectList.hpp"

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

	private:
		void ScaleNotes(float scale);

		std::vector<std::string> incompatibleEffects =
			{ "transparentnotes" };
	};
}
