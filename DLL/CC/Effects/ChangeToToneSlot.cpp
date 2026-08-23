#include "../../stdafx.h"
#include "ChangeToToneSlot.hpp"
#include "../../Keyboard.hpp"

using namespace CrowdControl::Enums;

namespace CrowdControl::Effects {
	/// <summary>
	/// Test the twitch mod's requirements.
	/// </summary>
	/// <param name="request"> - JSON Request</param>
	/// <returns>Enums::EffectStatus::Success if test completed without any issues. Enums::EffectStatus::Retry if we have to retry.</returns>
	Enums::EffectStatus ChangeToToneSlot::Test(const Structs::Request& request)
	{
		LOG_INFO("ChangeToToneSlot::Test()" << std::endl);

		if (!CanStart())
			return Enums::EffectStatus::Retry;

		return Enums::EffectStatus::Success;
	}


	/// <summary>
	/// Sends a keystroke to the game for the current tone slot (presses number 1 for first tone slot, number 2 for second, etc.)
	/// </summary>
	/// <returns> Enums::EffectStatus::Retry if we aren't currently in a song, or Enums::EffectStatus::Success if we are</returns>
	Enums::EffectStatus ChangeToToneSlot::OnStart(const Structs::Request& request)
	{
		LOG_INFO("ChangeToToneSlot::Start()" << std::endl);

		Keyboard::SendKey(Settings::GetVKCodeForString(std::to_string(slot)));

		LOG_INFO("Changing tone to slot " << slot << std::endl);

		return Enums::EffectStatus::Success;
	}

	/// <summary>
	/// Mod cannot be stopped, since it has an instant effect.
	/// </summary>
	/// <returns>Enums::EffectStatus::Success</returns>
	Enums::EffectStatus ChangeToToneSlot::OnStop() { return Enums::EffectStatus::Success; }
}