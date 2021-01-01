#include "RainbowStringsEffect.hpp"

namespace CrowdControl::Effects {
	EffectResult RainbowStringsEffect::Test(Request request)
	{
		std::cout << "RainbowStringsEffect::Test()" << std::endl;

		if (ERMode::IsRainbowEnabled() || !MemHelpers::IsInSong())
			return EffectResult::Retry;

		return EffectResult::Success;
	}

	EffectResult RainbowStringsEffect::Start(Request request)
	{
		std::cout << "RainbowStringsEffect::Start()" << std::endl;

		if (ERMode::IsRainbowEnabled() || !MemHelpers::IsInSong())
			return EffectResult::Retry;

		running = true;

		ERMode::ToggleRainbowMode();

		SetDuration(request);
		endTime = std::chrono::steady_clock::now() + std::chrono::seconds(duration);

		return EffectResult::Success;
	}

	void RainbowStringsEffect::Run()
	{
		// Stop automatically after duration has elapsed
		if (running) {
			auto now = std::chrono::steady_clock::now();
			std::chrono::duration<double> duration = (endTime - now);

			if (duration.count() <= 0) Stop();
		}
	}

	EffectResult RainbowStringsEffect::Stop()
	{
		std::cout << "RainbowStringsEffect::Stop()" << std::endl;

		running = false;
		ERMode::ToggleRainbowMode();

		return EffectResult::Success;
	}
}