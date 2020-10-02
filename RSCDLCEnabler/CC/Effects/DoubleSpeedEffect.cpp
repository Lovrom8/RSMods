#include "DoubleSpeedEffect.hpp"

using namespace CrowdControl::Enums;

namespace CrowdControl::Effects {
	EffectResult DoubleSpeedEffect::Test(Request request)
	{
		std::cout << "DoubleSpeedEffect::Test()" << std::endl;

		return EffectResult::Success;
	}

	EffectResult DoubleSpeedEffect::Start(Request request)
	{
		std::cout << "DoubleSpeedEffect::Start()" << std::endl;

		if (!MemHelpers::IsInSong())
			return EffectResult::Retry;

		//*(double*)Offsets::ptr_scrollSpeedMultiplier = 5.0f;
		*(float*)Offsets::ptr_scrollSpeedMultiplier = 5.0f;

		running = true;
		endTime = std::chrono::steady_clock::now() + std::chrono::seconds(duration);

		return EffectResult::Success;
	}

	void DoubleSpeedEffect::Run() 
	{
		// Stop automatically after duration has elapsed
		if (running) {
			auto now = std::chrono::steady_clock::now();
			std::chrono::duration<double> duration = (endTime - now);

			if (duration.count() <= 0) Stop();
		}
	}

	EffectResult DoubleSpeedEffect::Stop()
	{ 
		std::cout << "DoubleSpeedEffect::Stop()" << std::endl;

		//*(double*)Offsets::ptr_scrollSpeedMultiplier = 5.0;
		*(float*)Offsets::ptr_scrollSpeedMultiplier = 2.315;

		return EffectResult::Success; 
	}
}