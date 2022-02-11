#pragma once
#include "../CCEffect.hpp"
#include "../CCEffectList.hpp"

namespace CrowdControl::Effects {
	class ZoomEffect : public CCEffect
	{
	public:
		float factor = 1.0;

		ZoomEffect(unsigned int durationSeconds, float zoomFactor) {
			duration = durationSeconds;
			factor = zoomFactor;
		}

		EffectResult Test(Request request);
		EffectResult Start(Request request);
		void Run();
		EffectResult Stop();

	private:
		std::vector<std::string> incompatibleEffects = { "zoomin", "zoomout" };
	};
}
