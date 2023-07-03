#include "FYourFCEffect.hpp"

namespace CrowdControl::Effects { // Kills user's current note streak
	
	/// <summary>
	/// Test the twitch mod's requirements.
	/// </summary>
	/// <param name="request"> - JSON Request</param>
	/// <returns>EffectStatus::Success if test completed without any issues. EffectStatus::Retry if we have to retry.</returns>
	EffectStatus FYourFCEffect::Test(Request request)
	{
		_LOG_INIT;

		_LOG("FYourFC::Test()" << std::endl);

		if (!CanStart(&EffectList::AllEffects))
			return EffectStatus::Retry;

		return EffectStatus::Success;
	}

	/// <summary>
	/// Kills the player's current note streak for a certain duration
	/// </summary>
	/// <returns> EffectStatus::Retry if we aren't currently in a song or incompatible effects are running, or EffectStatus::Sucess if we are</returns>
	EffectStatus FYourFCEffect::Start(Request request)
	{
		_LOG_INIT;

		_LOG("FYourFC::Start()" << std::endl);

		if (!CanStart(&EffectList::AllEffects))
			return EffectStatus::Retry;

		uintptr_t currentNoteStreak = 0;

		if (MemHelpers::Contains(D3DHooks::currentMenu, learnASongModes))
			currentNoteStreak = MemUtil::FindDMAAddy(Offsets::baseHandle + Offsets::ptr_currentNoteStreak, Offsets::ptr_currentNoteStreakLASOffsets);
		else if (MemHelpers::Contains(D3DHooks::currentMenu, scoreAttackModes))
			currentNoteStreak = MemUtil::FindDMAAddy(Offsets::baseHandle + Offsets::ptr_currentNoteStreak, Offsets::ptr_currentNoteStreakSAOffsets);

		if (currentNoteStreak != 0)
			*(int32_t*)currentNoteStreak = 0;

		return EffectStatus::Success;
	}

	/// <summary>
	/// Stops the mod.
	/// </summary>
	/// <returns>EffectStatus::Success</returns>
	EffectStatus FYourFCEffect::Stop()
	{
		_LOG_INIT;

		_LOG("FYourFC::Stop()" << std::endl);

		return EffectStatus::Success;
	}
}
