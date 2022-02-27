#include "FYourFCEffect.hpp"

namespace CrowdControl::Effects { // Kills user's current note streak
	
	/// <summary>
	/// Test the twitch mod's requirements.
	/// </summary>
	/// <param name="request"> - JSON Request</param>
	/// <returns>EffectResult::Success if test completed without any issues. EffectResult::Retry if we have to retry.</returns>
	EffectResult FYourFCEffect::Test(Request request)
	{
		_LOG_INIT;

		_LOG_HEAD << "FYourFC::Test()" << LOG.endl();

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
		_LOG_INIT;

		_LOG_HEAD << "FYourFC::Start()" << LOG.endl();

		if (!MemHelpers::IsInSong() || running)
			return EffectResult::Retry;

		running = true;

		Settings::UpdateTwitchSetting("FYourFC", "on");

		SetDuration(request);
		endTime = std::chrono::steady_clock::now() + std::chrono::seconds(duration);

		return EffectResult::Success;
	}

	/// <summary>
	/// Ensure that the mod only lasts for the time specified in the JSON request.
	/// </summary>
	void FYourFCEffect::Run()
	{
		// Stop automatically after duration has elapsed
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
	EffectResult FYourFCEffect::Stop()
	{
		_LOG_INIT;

		_LOG_HEAD << "FYourFC::Stop()" << LOG.endl();

		running = false;
		Settings::UpdateTwitchSetting("FYourFC", "off");

		return EffectResult::Success;
	}
}
