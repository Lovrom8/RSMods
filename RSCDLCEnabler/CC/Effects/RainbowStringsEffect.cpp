#include "RainbowStringsEffect.hpp"

namespace CrowdControl::Effects {
	EffectResult RainbowStringsEffect::Test(Request request)
	{
		std::cout << "RainbowStringsEffect::Test()" << std::endl;

		if (ERMode::IsRainbowEnabled() || !MemHelpers::IsInSong())
			return EffectResult::Retry;

		return EffectResult::Success;
	}

	/// <summary>
	/// Makes strings continously shift their colors 
	/// As it is with rainbow notes, all color changing is handled in ERMode, so it just toggles the switch in there
	/// Does not affect the note heads!
	/// </summary>
	/// <returns> EffectResult::Retry if we aren't currently in a song or the same effect is running already, or EffectResult::Sucess if we are in a song</returns>
	EffectResult RainbowStringsEffect::Start(Request request)
	{
		std::cout << "RainbowStringsEffect::Start()" << std::endl;

		if (ERMode::IsRainbowEnabled() || !MemHelpers::IsInSong())
			return EffectResult::Retry;

		running = true;

		ERMode::RainbowEnabled = true;

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
		ERMode::RainbowEnabled = false;
		ERMode::ResetAllStrings();

		return EffectResult::Success;
	}
}