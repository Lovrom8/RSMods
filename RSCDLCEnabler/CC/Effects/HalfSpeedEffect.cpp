#include "HalfSpeedEffect.hpp"

using namespace CrowdControl::Enums;

namespace CrowdControl::Effects {
	EffectResult HalfSpeedEffect::Test(Request request)
	{
		std::cout << "HalfSpeedEffect::Test()" << std::endl;

		return EffectResult::Success;
	}

	EffectResult HalfSpeedEffect::Start(Request request)
	{
		std::cout << "HalfSpeedEffect::Start()" << std::endl;

		if (!MemHelpers::IsInSong())
			return EffectResult::Retry;

		*(float*)Offsets::ptr_scrollSpeedMultiplier = 1.0f;

		running = true;
		endTime = std::chrono::steady_clock::now() + std::chrono::seconds(duration);

		return EffectResult::Success;
	}

	void HalfSpeedEffect::Run() 
	{
		// Stop automatically after duration has elapsed
		if (running) {
			auto now = std::chrono::steady_clock::now();
			std::chrono::duration<double> duration = (endTime - now);

			if (duration.count() <= 0) Stop();
		}
	}

	EffectResult HalfSpeedEffect::Stop()
	{
		std::cout << "HalfSpeedEffect::Stop()" << std::endl;

		*(float*)Offsets::ptr_scrollSpeedMultiplier = 2.315f;

		return EffectResult::Success;
	}
}