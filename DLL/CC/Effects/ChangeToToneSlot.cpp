#include "ChangeToToneSlot.hpp"

using namespace CrowdControl::Enums;

namespace CrowdControl::Effects {
	
	/// <summary>
	/// Test the twitch mod's requirements.
	/// </summary>
	/// <param name="request"> - JSON Request</param>
	/// <returns>EffectStatus::Success if test completed without any issues. EffectStatus::Retry if we have to retry.</returns>
	EffectStatus ChangeToToneSlot::Test(Request request)
	{
		_LOG_INIT;

		_LOG("ChangeToToneSlot::Test()" << std::endl);

		if (!CanStart(&EffectList::AllEffects))
			return EffectStatus::Retry;

		return EffectStatus::Success;
	}


	/// <summary>
	/// Sends a keystroke to the game for the current tone slot (presses number 1 for first tone slot, number 2 for second, etc.)
	/// </summary>
	/// <returns> EffectStatus::Retry if we aren't currently in a song, or EffectStatus::Success if we are</returns>
	EffectStatus ChangeToToneSlot::Start(Request request)
	{
		_LOG_INIT;

		_LOG("ChangeToToneSlot::Start()" << std::endl);

		if (!CanStart(&EffectList::AllEffects))
			return EffectStatus::Retry;

		Util::SendKey(Settings::GetVKCodeForString(std::to_string(slot)));
	
		_LOG("Changing tone to slot " << slot << std::endl);

		return EffectStatus::Success;
	}

	/// <summary>
	/// Mod cannot be stopped, since it has an instant effect.
	/// </summary>
	/// <returns>EffectStatus::Success</returns>
	EffectStatus ChangeToToneSlot::Stop() { return EffectStatus::Success; }
}