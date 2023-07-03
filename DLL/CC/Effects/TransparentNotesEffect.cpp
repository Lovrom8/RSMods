#include "TransparentNotesEffect.hpp"

using namespace CrowdControl::Enums;

namespace CrowdControl::Effects { // Changes textures for noteheads to a nonexistent (effectively transparent) texture
	
	/// <summary>
	/// Test the twitch mod's requirements.
	/// </summary>
	/// <param name="request"> - JSON Request</param>
	/// <returns>EffectStatus::Success if test completed without any issues. EffectStatus::Retry if we have to retry.</returns>
	EffectStatus TransparentNotesEffect::Test(Request request)
	{
		_LOG_INIT;

		_LOG("TransparentNotesEffect::Test()" << std::endl);

		if (!CanStart(&EffectList::AllEffects))
			return EffectStatus::Retry;

		return EffectStatus::Success;
	}

	/// <summary>
	/// Make the notes appear transparent.
	/// </summary>
	/// <param name="request"> - JSON Request</param>
	/// <returns>EffectStatus::Success if test completed without any issues. EffectStatus::Retry if we have to retry.</returns>
	EffectStatus TransparentNotesEffect::Start(Request request)
	{
		_LOG_INIT;

		_LOG("TransparentNotesEffect::Start()" << std::endl);

		if (!CanStart(&EffectList::AllEffects))
			return EffectStatus::Retry;

		Settings::UpdateTwitchSetting("TransparentNotes", "on");
		
		SetDuration(request);
		running = true;

		return EffectStatus::Success;
	}

	/// <summary>
	/// Stops the mod.
	/// </summary>
	/// <returns>EffectStatus::Success</returns>
	EffectStatus TransparentNotesEffect::Stop()
	{
		_LOG_INIT;

		_LOG("TransparentNotesEffect::Stop()" << std::endl);

		Settings::UpdateTwitchSetting("TransparentNotes", "off");
		running = false;

		return EffectStatus::Success;
	}
}