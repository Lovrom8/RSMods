#include "../../stdafx.h"
#include "DrunkModeEffect.hpp"
#include "../../Framework/Framework.hpp"

namespace Setting = Settings::Setting;

using namespace CrowdControl::Enums;

namespace CrowdControl::Effects { // Makes some of game's object very woobly (lyrics, backgrounds, etc.)
	
	/// <summary>
	/// Test the twitch mod's requirements.
	/// </summary>
	/// <param name="request"> - JSON Request</param>
	/// <returns>Enums::EffectStatus::Success if test completed without any issues. Enums::EffectStatus::Retry if we have to retry.</returns>
	Enums::EffectStatus DrunkModeEffect::Test(const Structs::Request& request)
	{
		LOG_INFO("DrunkModeEffect::Test()" << std::endl);

		if (!CanStart())
			return Enums::EffectStatus::Retry;

		return Enums::EffectStatus::Success;
	}

	/// <summary>
	/// Drunk mode is achieved by writing different continuously values to a variable controlling the horizontal position of the camera
	/// Loft needs to be enabled for it to have the full effect, so we also call ToggleDrunkMode
	/// </summary>
	/// <returns> Enums::EffectStatus::Retry if we aren't currently in a song or incompatible effects are running, or Enums::EffectStatus::Success if we are</returns>
	Enums::EffectStatus DrunkModeEffect::OnStart(const Structs::Request& request)
	{
		LOG_INFO("DrunkModeEffect::Start()" << std::endl);

		Framework::Registry().EnqueueSettingsUpdate([] {
			Settings::UpdateTwitchSetting(Setting::Twitch::DrunkMode, "on");
		});
		Loft::ToggleDrunkMode(true);

		SetDuration(request);
		running = true;

		return Enums::EffectStatus::Success;
	}

	/// <summary>
	/// Stops the mod.
	/// </summary>
	/// <returns>Enums::EffectStatus::Success</returns>
	Enums::EffectStatus DrunkModeEffect::OnStop()
	{
		LOG_INFO("DrunkModeEffect::Stop()" << std::endl);

		running = false;
		Loft::ToggleDrunkMode(false);
		Framework::Registry().EnqueueSettingsUpdate([] {
			Settings::UpdateTwitchSetting(Setting::Twitch::DrunkMode, "off");
		});

		return Enums::EffectStatus::Success;
	}
}
