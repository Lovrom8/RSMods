#include "../../stdafx.h"
#include "TransparentNotesEffect.hpp"
#include "../../Framework/Framework.hpp"

namespace Setting = Settings::Setting;

using namespace CrowdControl::Enums;

namespace CrowdControl::Effects { // Changes textures for noteheads to a nonexistent (effectively transparent) texture
	
	/// <summary>
	/// Test the twitch mod's requirements.
	/// </summary>
	/// <param name="request"> - JSON Request</param>
	/// <returns>Enums::EffectStatus::Success if test completed without any issues. Enums::EffectStatus::Retry if we have to retry.</returns>
	Enums::EffectStatus TransparentNotesEffect::Test(const Structs::Request& request)
	{
		LOG_INFO("TransparentNotesEffect::Test()" << std::endl);

		if (!CanStart())
			return Enums::EffectStatus::Retry;

		return Enums::EffectStatus::Success;
	}

	/// <summary>
	/// Make the notes appear transparent.
	/// </summary>
	/// <param name="request"> - JSON Request</param>
	/// <returns>Enums::EffectStatus::Success if test completed without any issues. Enums::EffectStatus::Retry if we have to retry.</returns>
	Enums::EffectStatus TransparentNotesEffect::OnStart(const Structs::Request& request)
	{
		LOG_INFO("TransparentNotesEffect::Start()" << std::endl);

		Framework::Registry().EnqueueSettingsUpdate([] {
			Settings::UpdateTwitchSetting(Setting::Twitch::TransparentNotes, "on");
		});

		SetDuration(request);
		running = true;

		return Enums::EffectStatus::Success;
	}

	/// <summary>
	/// Stops the mod.
	/// </summary>
	/// <returns>Enums::EffectStatus::Success</returns>
	Enums::EffectStatus TransparentNotesEffect::OnStop()
	{
		LOG_INFO("TransparentNotesEffect::Stop()" << std::endl);

		Framework::Registry().EnqueueSettingsUpdate([] {
			Settings::UpdateTwitchSetting(Setting::Twitch::TransparentNotes, "off");
		});
		running = false;

		return Enums::EffectStatus::Success;
	}
}
