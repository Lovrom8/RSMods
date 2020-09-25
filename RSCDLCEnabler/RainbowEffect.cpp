#include "RainbowEffect.hpp"
#include <iostream>
#include "Mods/ExtendedRangeMode.hpp"
#include <chrono>

using namespace CrowdControl::Enums;

namespace CrowdControl::Effects {
	EffectResult RainbowEffect::Test()
	{
		std::cout << "RainbowEffect::Test()" << std::endl;

		return EffectResult::Success;
	}

	EffectResult RainbowEffect::Start()
	{
		std::cout << "RainbowEffect::Start()" << std::endl;

		if (ERMode::IsRainbowEnabled()) {
			return EffectResult::Retry;
		}

		running = true;
		ERMode::ToggleRainbowMode();

		endTime = std::chrono::steady_clock::now() + std::chrono::seconds(duration);

		return EffectResult::Success;
	}

	void RainbowEffect::Run()
	{
		// Stop automatically after duration has elapsed
		if (running) {
			auto now = std::chrono::steady_clock::now();
			std::chrono::duration<double> duration = (endTime - now);

			if (duration.count() <= 0) Stop();
		}
	}

	EffectResult RainbowEffect::Stop()
	{
		std::cout << "RainbowEffect::Stop()" << std::endl;

		running = false;
		ERMode::ToggleRainbowMode();

		return EffectResult::Success;
	}
}