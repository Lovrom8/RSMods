#pragma once
#include "../CCEffect.hpp"
#include "../../MemHelpers.hpp"

namespace CrowdControl::Effects {
	class ChangeToToneSlot : public CCEffect
	{
	public:
		int slot;

		ChangeToToneSlot(int _slot) {
			slot = _slot;
		}

		EffectResult Test();
		EffectResult Start();
		void Run();
		EffectResult Stop();
	};
}