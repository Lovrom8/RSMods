#include "../../stdafx.h"
#include "RainbowNotesEffect.hpp"

namespace CrowdControl::Effects {
	/// <summary>
	/// Test the twitch mod's requirements.
	/// </summary>
	/// <param name="request"> - JSON Request</param>
	/// <returns>Enums::EffectStatus::Success if test completed without any issues. Enums::EffectStatus::Retry if we have to retry.</returns>
	Enums::EffectStatus RainbowNotesEffect::Test(const Structs::Request& request)
	{
		LOG_INFO("RainbowNotesEffect::Test()" << std::endl);

		if (!CanStart())
			return Enums::EffectStatus::Retry;

		return Enums::EffectStatus::Success;
	}

	/// <summary>
	/// Makes note heads continuously shift their colors 
	/// All note head changing is handled in ERMode, so it just toggles the switch in there
	/// Does not affect the strings!
	/// </summary>
	/// <returns> Enums::EffectStatus::Retry if we aren't currently in a song or the same effect is running already, or Enums::EffectStatus::Success if we are in a song</returns>
	Enums::EffectStatus RainbowNotesEffect::OnStart(const Structs::Request& request)
	{
		LOG_INFO("RainbowNotesEffect::Start()" << std::endl);

		ERMode::ToggleRainbowNotes();

		SetDuration(request);
		running = true;

		return Enums::EffectStatus::Success;
	}

	/// <summary>
	/// Stops the mod.
	/// </summary>
	/// <returns>Enums::EffectStatus::Success</returns>
	Enums::EffectStatus RainbowNotesEffect::OnStop()
	{
		LOG_INFO("RainbowNotesEffect::Stop()" << std::endl);

		running = false;
		ERMode::ToggleRainbowNotes();

		return Enums::EffectStatus::Success;
	}
}