#include "../../stdafx.h"
#include "FYourFCEffect.hpp"

namespace CrowdControl::Effects { // Kills user's current note streak
	
	/// <summary>
	/// Test the twitch mod's requirements.
	/// </summary>
	/// <param name="request"> - JSON Request</param>
	/// <returns>Enums::EffectStatus::Success if test completed without any issues. Enums::EffectStatus::Retry if we have to retry.</returns>
	Enums::EffectStatus FYourFCEffect::Test(const Structs::Request& request)
	{
		LOG_INFO("FYourFC::Test()" << std::endl);

		if (!CanStart())
			return Enums::EffectStatus::Retry;

		return Enums::EffectStatus::Success;
	}

	/// <summary>
	/// Kills the player's current note streak for a certain duration
	/// </summary>
	/// <returns> Enums::EffectStatus::Retry if we aren't currently in a song or incompatible effects are running, or Enums::EffectStatus::Success if we are</returns>
	Enums::EffectStatus FYourFCEffect::OnStart(const Structs::Request& request)
	{
		LOG_INFO("FYourFC::Start()" << std::endl);

		uintptr_t currentNoteStreak = 0;

		if (GameState::Menus::IsInLearnASongModes()) {
			currentNoteStreak = MemUtil::FindDMAAddy(Offsets::baseHandle + Offsets::ptr_currentNoteStreak, Offsets::ptr_currentNoteStreakLASOffsets);
		}
		else if (GameState::Menus::IsInScoreAttackModes()) {
			currentNoteStreak = MemUtil::FindDMAAddy(Offsets::baseHandle + Offsets::ptr_currentNoteStreak, Offsets::ptr_currentNoteStreakSAOffsets);
		}

		if (currentNoteStreak != 0)
			*(int32_t*)currentNoteStreak = 0;

		return Enums::EffectStatus::Success;
	}

	/// <summary>
	/// Stops the mod.
	/// </summary>
	/// <returns>Enums::EffectStatus::Success</returns>
	Enums::EffectStatus FYourFCEffect::OnStop()
	{
		LOG_INFO("FYourFC::Stop()" << std::endl);

		return Enums::EffectStatus::Success;
	}
}
