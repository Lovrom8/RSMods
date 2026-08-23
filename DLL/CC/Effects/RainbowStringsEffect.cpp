#include "../../stdafx.h"
#include "RainbowStringsEffect.hpp"

namespace CrowdControl::Effects {
	
	/// <summary>
	/// Test the twitch mod's requirements.
	/// </summary>
	/// <param name="request"> - JSON Request</param>
	/// <returns>Enums::EffectStatus::Success if test completed without any issues. Enums::EffectStatus::Retry if we have to retry.</returns>
	Enums::EffectStatus RainbowStringsEffect::Test(const Structs::Request& request)
	{
		LOG_INFO("RainbowStringsEffect::Test()" << std::endl);

		if (!CanStart())
			return Enums::EffectStatus::Retry;

		return Enums::EffectStatus::Success;
	}

	/// <summary>
	/// Makes strings continuously shift their colors 
	/// As it is with rainbow notes, all color changing is handled in ERMode, so it just toggles the switch in there
	/// Does not affect the note heads!
	/// </summary>
	/// <returns> Enums::EffectStatus::Retry if we aren't currently in a song or the same effect is running already, or Enums::EffectStatus::Success if we are in a song</returns>
	Enums::EffectStatus RainbowStringsEffect::OnStart(const Structs::Request& request)
	{
		LOG_INFO("RainbowStringsEffect::Start()" << std::endl);

		ERMode::RainbowEnabled = true;

		SetDuration(request);
		running = true;

		return Enums::EffectStatus::Success;
	}

	/// <summary>
	/// Stops the mod.
	/// </summary>
	/// <returns>Enums::EffectStatus::Success</returns>
	Enums::EffectStatus RainbowStringsEffect::OnStop()
	{
		LOG_INFO("RainbowStringsEffect::Stop()" << std::endl);

		running = false;
		ERMode::RainbowEnabled = false;
		ERMode::ResetAllStrings();

		return Enums::EffectStatus::Success;
	}
}