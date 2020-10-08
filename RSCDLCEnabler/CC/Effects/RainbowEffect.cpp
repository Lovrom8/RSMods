#include "RainbowEffect.hpp"

namespace CrowdControl::Effects {
	EffectResult RainbowEffect::Test(Request request)
	{
		std::cout << "RainbowEffect::Test()" << std::endl;

		if (ERMode::IsRainbowEnabled() || !MemHelpers::IsInSong() || EffectList::AreIncompatibleEffectsEnabled(incompatibleEffects))
			return EffectResult::Retry;

		return EffectResult::Success;
	}

	EffectResult RainbowEffect::Start(Request request)
	{
		std::cout << "RainbowEffect::Start()" << std::endl;

		if (ERMode::IsRainbowEnabled() || !MemHelpers::IsInSong() || EffectList::AreIncompatibleEffectsEnabled(incompatibleEffects))
			return EffectResult::Retry;

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