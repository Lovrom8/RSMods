#include "RainbowNotesEffect.hpp"

namespace CrowdControl::Effects {
	EffectResult RainbowNotesEffect::Test(Request request)
	{
		std::cout << "RainbowNotesEffect::Test()" << std::endl;

		if (ERMode::IsRainbowNotesEnabled() || !MemHelpers::IsInSong() || EffectList::AreIncompatibleEffectsEnabled(incompatibleEffects))
			return EffectResult::Retry;

		return EffectResult::Success;
	}

	EffectResult RainbowNotesEffect::Start(Request request)
	{
		std::cout << "RainbowNotesEffect::Start()" << std::endl;

		if (ERMode::IsRainbowNotesEnabled() || !MemHelpers::IsInSong() || EffectList::AreIncompatibleEffectsEnabled(incompatibleEffects))
			return EffectResult::Retry;

		running = true;

		ERMode::ToggleRainbowNotes();

		SetDuration(request);
		endTime = std::chrono::steady_clock::now() + std::chrono::seconds(duration);

		return EffectResult::Success;
	}

	void RainbowNotesEffect::Run()
	{
		// Stop automatically after duration has elapsed
		if (running) {
			auto now = std::chrono::steady_clock::now();
			std::chrono::duration<double> duration = (endTime - now);

			if (duration.count() <= 0) Stop();
		}
	}

	EffectResult RainbowNotesEffect::Stop()
	{
		std::cout << "RainbowNotesEffect::Stop()" << std::endl;

		running = false;
		ERMode::ToggleRainbowNotes();

		return EffectResult::Success;
	}
}