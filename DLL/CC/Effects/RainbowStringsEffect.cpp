#include "RainbowStringsEffect.hpp"

namespace CrowdControl::Effects {
	
	/// <summary>
	/// Test the twitch mod's requirements.
	/// </summary>
	/// <param name="request"> - JSON Request</param>
	/// <returns>EffectResult::Success if test completed without any issues. EffectResult::Retry if we have to retry.</returns>
	EffectResult RainbowStringsEffect::Test(Request request)
	{
		_LOG_INIT;

		_LOG_HEAD << "RainbowStringsEffect::Test()" << LOG.endl();

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
		_LOG_INIT;

		_LOG_HEAD << "RainbowStringsEffect::Start()" << LOG.endl();

		if (ERMode::IsRainbowEnabled() || !MemHelpers::IsInSong())
			return EffectResult::Retry;

		running = true;

		ERMode::RainbowEnabled = true;

		SetDuration(request);
		endTime = std::chrono::steady_clock::now() + std::chrono::seconds(duration);

		return EffectResult::Success;
	}


	/// <summary>
	/// Ensure that the mod only lasts for the time specified in the JSON request.
	/// </summary>
	void RainbowStringsEffect::Run()
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
	EffectResult RainbowStringsEffect::Stop()
	{
		_LOG_INIT;

		_LOG_HEAD << "RainbowStringsEffect::Stop()" << LOG.endl();

		running = false;
		ERMode::RainbowEnabled = false;
		ERMode::ResetAllStrings();

		return EffectResult::Success;
	}
}