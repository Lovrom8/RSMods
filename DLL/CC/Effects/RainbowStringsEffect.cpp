#include "RainbowStringsEffect.hpp"

namespace CrowdControl::Effects {
	
	/// <summary>
	/// Test the twitch mod's requirements.
	/// </summary>
	/// <param name="request"> - JSON Request</param>
	/// <returns>EffectStatus::Success if test completed without any issues. EffectStatus::Retry if we have to retry.</returns>
	EffectStatus RainbowStringsEffect::Test(Request request)
	{
		_LOG_INIT;

		_LOG("RainbowStringsEffect::Test()" << std::endl);

		if (!CanStart(&EffectList::AllEffects))
			return EffectStatus::Retry;

		return EffectStatus::Success;
	}

	/// <summary>
	/// Makes strings continuously shift their colors 
	/// As it is with rainbow notes, all color changing is handled in ERMode, so it just toggles the switch in there
	/// Does not affect the note heads!
	/// </summary>
	/// <returns> EffectStatus::Retry if we aren't currently in a song or the same effect is running already, or EffectStatus::Success if we are in a song</returns>
	EffectStatus RainbowStringsEffect::Start(Request request)
	{
		_LOG_INIT;

		_LOG("RainbowStringsEffect::Start()" << std::endl);

		if (!CanStart(&EffectList::AllEffects))
			return EffectStatus::Retry;

		ERMode::RainbowEnabled = true;

		SetDuration(request);
		running = true;

		return EffectStatus::Success;
	}

	/// <summary>
	/// Stops the mod.
	/// </summary>
	/// <returns>EffectStatus::Success</returns>
	EffectStatus RainbowStringsEffect::Stop()
	{
		_LOG_INIT;

		_LOG("RainbowStringsEffect::Stop()" << std::endl);

		running = false;
		ERMode::RainbowEnabled = false;
		ERMode::ResetAllStrings();

		return EffectStatus::Success;
	}
}