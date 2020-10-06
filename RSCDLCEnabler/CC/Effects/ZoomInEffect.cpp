#include "ZoomInEffect.hpp"

using namespace CrowdControl::Enums;

namespace CrowdControl::Effects { 
	EffectResult ZoomInEffect::Test(Request request)
	{
		std::cout << "ZoomInEffect::Test()" << std::endl;

		return EffectResult::Success;
	}

	EffectResult ZoomInEffect::Start(Request request)
	{
		std::cout << "ZoomInEffect::Start()" << std::endl;

		if (!MemHelpers::IsInSong())
			return EffectResult::Retry;

		auto rootObject = ObjectUtil::GetRootObject(); 
		rootObject->scale = 2.0f; // TODO: determine correct values

		running = true;
		endTime = std::chrono::steady_clock::now() + std::chrono::seconds(duration);

		return EffectResult::Success;
	}

	void ZoomInEffect::Run()
	{
		// Stop automatically after duration has elapsed
		if (running) {
			auto now = std::chrono::steady_clock::now();
			std::chrono::duration<double> duration = (endTime - now);

			if (duration.count() <= 0) Stop();
		}
	}

	EffectResult ZoomInEffect::Stop()
	{
		std::cout << "ZoomInEffect::Stop()" << std::endl;

		auto rootObject = ObjectUtil::GetRootObject();
		rootObject->scale = 1.0f;

		running = false;

		return EffectResult::Success;
	}
}