#include "ChangeToToneSlot.hpp"

using namespace CrowdControl::Enums;

namespace CrowdControl::Effects {
	
	/// <summary>
	/// Test the twitch mod's requirements.
	/// </summary>
	/// <param name="request"> - JSON Request</param>
	/// <returns>EffectResult::Success if test completed without any issues. EffectResult::Retry if we have to retry.</returns>
	EffectResult ChangeToToneSlot::Test(Request request)
	{
		_LOG_INIT;

		_LOG_HEAD << "ChangeToToneSlot::Test()" << LOG.endl();

		if (!MemHelpers::IsInSong())
			return EffectResult::Retry;

		return EffectResult::Success;
	}


	/// <summary>
	/// Sends a keystroke to the game for the current tone slot (presses number 1 for first tone slot, number 2 for second, etc.)
	/// </summary>
	/// <returns> EffectResult::Retry if we aren't currently in a song, or EffectResult::Sucess if we are</returns>
	EffectResult ChangeToToneSlot::Start(Request request)
	{
		_LOG_INIT;

		_LOG_HEAD << "ChangeToToneSlot::Start()" << LOG.endl();

		if (!MemHelpers::IsInSong())
			return EffectResult::Retry;

		Util::SendKey(Settings::GetVKCodeForString(std::to_string(ChangeToToneSlot::slot)));
	
		_LOG_HEAD << "Changing tone to slot " << slot << LOG.endl();

		return EffectResult::Success;
	}


	/// <summary>
	/// It's an instant effect and doesn't have a duration
	/// </summary>
	void ChangeToToneSlot::Run() { }

	/// <summary>
	/// Mod cannot be stopped, since it has an instant effect.
	/// </summary>
	/// <returns>EffectResult::Success</returns>
	EffectResult ChangeToToneSlot::Stop() { return EffectResult::Success; }
}