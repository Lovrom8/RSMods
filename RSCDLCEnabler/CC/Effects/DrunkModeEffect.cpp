#include "DrunkModeEffect.hpp"

using namespace CrowdControl::Enums;

namespace CrowdControl::Effects { // Makes some of game's object very woobly (lyrics, backgrounds, etc.)
	EffectResult DrunkModeEffect::Test(Request request)
	{
		std::cout << "DrunkModeEffect::Test()" << std::endl;

		if (!MemHelpers::IsInSong() || running)
			return EffectResult::Retry;

		return EffectResult::Success;
	}

	/// <summary>
	/// Drunk mode is achieved by writing different continously values to a variable controlling the horiztonal position of the camera
	/// Loft needs to be enabled for it to have the full effect, so we also call ToggleDrunkMode
	/// </summary>
	/// <returns> EffectResult::Retry if we aren't currently in a song or incompatible effects are running, or EffectResult::Sucess if we are</returns>
	EffectResult DrunkModeEffect::Start(Request request)
	{
		std::cout << "DrunkModeEffect::Start()" << std::endl;

		if (!MemHelpers::IsInSong() || running)
			return EffectResult::Retry;

		running = true;

		Settings::UpdateTwitchSetting("DrunkMode", "on");
		MemHelpers::ToggleDrunkMode(true);

		SetDuration(request);
		endTime = std::chrono::steady_clock::now() + std::chrono::seconds(duration);

		return EffectResult::Success;
	}

	void DrunkModeEffect::Run()
	{
		// Stop automatically after duration has elapsed
		if (running) {
			auto now = std::chrono::steady_clock::now();
			std::chrono::duration<double> duration = (endTime - now);

			if (duration.count() <= 0) Stop();
		}
	}

	EffectResult DrunkModeEffect::Stop()
	{
		std::cout << "DrunkModeEffect::Stop()" << std::endl;

		running = false;
		MemHelpers::ToggleDrunkMode(false);
		Settings::UpdateTwitchSetting("DrunkMode", "off");

		return EffectResult::Success;
	}
}
