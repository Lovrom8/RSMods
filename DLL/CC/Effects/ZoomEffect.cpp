#include "ZoomEffect.hpp"

using namespace CrowdControl::Enums;

namespace CrowdControl::Effects {
	EffectResult ZoomEffect::Test(Request request)
	{
		std::cout << "ZoomEffect::Test()" << std::endl;

		if (!MemHelpers::IsInSong() || running)
			return EffectResult::Retry;

		return EffectResult::Success;
	}


	EffectResult ZoomEffect::Start(Request request)
	{
		std::cout << "ZoomEffect::Start()" << std::endl;

		if (!MemHelpers::IsInSong() || EffectList::AreIncompatibleEffectsEnabled(incompatibleEffects) || running)
			return EffectResult::Retry;

		running = true;

		auto rootObject = ObjectUtil::GetRootObject();
		rootObject->scale = factor;

		SetDuration(request);
		endTime = std::chrono::steady_clock::now() + std::chrono::seconds(duration);

		return EffectResult::Success;
	}

	void ZoomEffect::Run()
	{
		// Stop automatically after duration has elapsed
		if (running) {
			auto now = std::chrono::steady_clock::now();
			std::chrono::duration<double> duration = (endTime - now);

			if (duration.count() <= 0) Stop();
		}
	}

	EffectResult ZoomEffect::Stop()
	{
		std::cout << "ZoomEffect::Stop()" << std::endl;

		auto rootObject = ObjectUtil::GetRootObject();
		rootObject->scale = 1.0f;

		running = false;

		return EffectResult::Success;
	}
}