#pragma once
#include "../CCEffect.hpp"
#include "../../D3D/D3D.hpp"
#include "../CCEffectList.hpp"

namespace CrowdControl::Effects {
	class InvertedStringsEffect : public CCEffect
	{
	public:
		InvertedStringsEffect(unsigned int durationSeconds) {
			duration = durationSeconds;
		}

		EffectResult Test(Request request);
		EffectResult Start(Request request);
		void Run();
		EffectResult Stop();

	private:
		std::vector<std::string> incompatibleEffects =
			{ "removeinstrument" };
	};
}