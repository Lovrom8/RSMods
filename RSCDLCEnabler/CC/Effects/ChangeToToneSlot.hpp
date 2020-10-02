#pragma once
#include "../CCEffect.hpp"

namespace CrowdControl::Effects {
	class ChangeToToneSlot : public CCEffect
	{
	public:
		int slot;

		ChangeToToneSlot(int _slot) {
			slot = _slot;
		}

		EffectResult Test(Request request);
		EffectResult Start(Request request);
		void Run();
		EffectResult Stop();
	};
}