#include "DrunkModeEffect.hpp"

using namespace CrowdControl::Enums;

namespace CrowdControl::Effects { // Makes some of game's object very woobly (lyrics, backgrounds, etc.)
	
	/// <summary>
	/// Test the twitch mod's requirements.
	/// </summary>
	/// <param name="request"> - JSON Request</param>
	/// <returns>EffectStatus::Success if test completed without any issues. EffectStatus::Retry if we have to retry.</returns>
	EffectStatus DrunkModeEffect::Test(Request request)
	{
		_LOG_INIT;

		_LOG("DrunkModeEffect::Test()" << std::endl);

		if (!CanStart(&EffectList::AllEffects))
			return EffectStatus::Retry;

			return EffectStatus::Success;
	}

	/// <summary>
	/// Drunk mode is achieved by writing different continuously values to a variable controlling the horizontal position of the camera
	/// Loft needs to be enabled for it to have the full effect, so we also call ToggleDrunkMode
	/// </summary>
	/// <returns> EffectStatus::Retry if we aren't currently in a song or incompatible effects are running, or EffectStatus::Success if we are</returns>
	EffectStatus DrunkModeEffect::Start(Request request)
	{
		_LOG_INIT;

		_LOG("DrunkModeEffect::Start()" << std::endl);

		if (!CanStart(&EffectList::AllEffects))
			return EffectStatus::Retry;

		Settings::UpdateTwitchSetting("DrunkMode", "on");
		MemHelpers::ToggleDrunkMode(true);

		SetDuration(request);
		running = true;

		return EffectStatus::Success;
	}

	/// <summary>
	/// Stops the mod.
	/// </summary>
	/// <returns>EffectStatus::Success</returns>
	EffectStatus DrunkModeEffect::Stop()
	{
		_LOG_INIT;

		_LOG("DrunkModeEffect::Stop()" << std::endl);

		running = false;
		MemHelpers::ToggleDrunkMode(false);
		Settings::UpdateTwitchSetting("DrunkMode", "off");

		return EffectStatus::Success;
	}
}
