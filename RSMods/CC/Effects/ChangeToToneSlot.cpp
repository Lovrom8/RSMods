#include "ChangeToToneSlot.hpp"

using namespace CrowdControl::Enums;

namespace CrowdControl::Effects {
	EffectResult ChangeToToneSlot::Test(Request request)
	{
		std::cout << "ChangeToToneSlot::Test()" << std::endl;

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
		std::cout << "ChangeToToneSlot::Start()" << std::endl;

		if (!MemHelpers::IsInSong())
			return EffectResult::Retry;

		Util::SendKey(Settings::GetVKCodeForString(std::to_string(ChangeToToneSlot::slot)));
	
		std::cout << "Changing tone to slot " << slot << std::endl;

		return EffectResult::Success;
	}


	/// <summary>
	/// It's an instant effect and doesn't have a duration
	/// </summary>
	void ChangeToToneSlot::Run() { }
	EffectResult ChangeToToneSlot::Stop() { return EffectResult::Success; }
}