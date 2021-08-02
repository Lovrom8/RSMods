#include "FYourFCEffect.hpp"

namespace CrowdControl::Effects { // Kills user's current note streak
	EffectResult FYourFCEffect::Test(Request request)
	{
		std::cout << "FYourFC::Test()" << std::endl;

		if (!MemHelpers::IsInSong() || running)
			return EffectResult::Retry;

		return EffectResult::Success;
	}

	/// <summary>
	/// Kills the player's current note streak for a certain duration
	/// </summary>
	/// <returns> EffectResult::Retry if we aren't currently in a song or incompatible effects are running, or EffectResult::Sucess if we are</returns>
	EffectResult FYourFCEffect::Start(Request request)
	{
		std::cout << "FYourFC::Start()" << std::endl;

		if (!MemHelpers::IsInSong() || running)
			return EffectResult::Retry;

		running = true;

		Settings::UpdateTwitchSetting("FYourFC", "on");

		SetDuration(request);
		endTime = std::chrono::steady_clock::now() + std::chrono::seconds(duration);

		return EffectResult::Success;
	}

	void FYourFCEffect::Run()
	{
		// Stop automatically after duration has elapsed
		if (running) {
			auto now = std::chrono::steady_clock::now();
			std::chrono::duration<double> duration = (endTime - now);

			if (duration.count() <= 0) Stop();
		}
	}

	EffectResult FYourFCEffect::Stop()
	{
		std::cout << "FYourFC::Stop()" << std::endl;

		running = false;
		Settings::UpdateTwitchSetting("FYourFC", "off");

		return EffectResult::Success;
	}
}
