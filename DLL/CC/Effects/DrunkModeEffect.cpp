#include "DrunkModeEffect.hpp"

using namespace CrowdControl::Enums;

namespace CrowdControl::Effects { // Makes some of game's object very woobly (lyrics, backgrounds, etc.)
	
	/// <summary>
	/// Test the twitch mod's requirements.
	/// </summary>
	/// <param name="request"> - JSON Request</param>
	/// <returns>EffectResult::Success if test completed without any issues. EffectResult::Retry if we have to retry.</returns>
	EffectResult DrunkModeEffect::Test(Request request)
	{
		_LOG_INIT;

		_LOG_HEAD << "DrunkModeEffect::Test()" << LOG.endl();

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
		_LOG_INIT;

		_LOG_HEAD << "DrunkModeEffect::Start()" << LOG.endl();

		if (!MemHelpers::IsInSong() || running)
			return EffectResult::Retry;

		running = true;

		Settings::UpdateTwitchSetting("DrunkMode", "on");
		MemHelpers::ToggleDrunkMode(true);

		SetDuration(request);
		endTime = std::chrono::steady_clock::now() + std::chrono::seconds(duration);

		return EffectResult::Success;
	}

	/// <summary>
	/// Ensure that the mod only lasts for the time specified in the JSON request.
	/// </summary>
	void DrunkModeEffect::Run()
	{
		if (running) {
			auto now = std::chrono::steady_clock::now();
			std::chrono::duration<double> duration = (endTime - now);

			if (duration.count() <= 0) Stop();
		}
	}

	/// <summary>
	/// Stops the mod.
	/// </summary>
	/// <returns>EffectResult::Success</returns>
	EffectResult DrunkModeEffect::Stop()
	{
		_LOG_INIT;

		_LOG_HEAD << "DrunkModeEffect::Stop()" << LOG.endl();

		running = false;
		MemHelpers::ToggleDrunkMode(false);
		Settings::UpdateTwitchSetting("DrunkMode", "off");

		return EffectResult::Success;
	}
}
