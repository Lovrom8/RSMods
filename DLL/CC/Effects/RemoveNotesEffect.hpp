#pragma once
#include "../CCEffect.hpp"
#include "../CCEffectList.hpp"

namespace CrowdControl::Effects {
	// Prevents the game from drawing note head meshes
	class RemoveNotesEffect : public CCEffect
	{
	public:
		RemoveNotesEffect(unsigned int durationMilliseconds) {
			duration_ms = durationMilliseconds;

			incompatibleEffects = { "transparentnotes", "bignoteheads", "smallnoteheads", "wavynotes"};
		}

		EffectStatus Test(Request request) override;
		EffectStatus Start(Request request) override;
		EffectStatus Stop() override;

	private:
		static void ScaleNotes(float scale);
	};
}
