#include "TransparentNotesEffect.hpp"

using namespace CrowdControl::Enums;

namespace CrowdControl::Effects { // Changes textures for noteheads to a nonexistent (effectively transparent) texture
	
	/// <summary>
	/// Test the twitch mod's requirements.
	/// </summary>
	/// <param name="request"> - JSON Request</param>
	/// <returns>EffectResult::Success if test completed without any issues. EffectResult::Retry if we have to retry.</returns>
	EffectResult TransparentNotesEffect::Test(Request request)
	{
		_LOG_INIT;

		_LOG_HEAD << "TransparentNotesEffect::Test()" << LOG.endl();

		if (!MemHelpers::IsInSong() || EffectList::AreIncompatibleEffectsEnabled(incompatibleEffects) || running)
			return EffectResult::Retry;

		return EffectResult::Success;
	}

	/// <summary>
	/// Make the notes appear transparent.
	/// </summary>
	/// <param name="request"> - JSON Request</param>
	/// <returns>EffectResult::Success if test completed without any issues. EffectResult::Retry if we have to retry.</returns>
	EffectResult TransparentNotesEffect::Start(Request request)
	{
		_LOG_INIT;

		_LOG_HEAD << "TransparentNotesEffect::Start()" << LOG.endl();

		if (!MemHelpers::IsInSong() || EffectList::AreIncompatibleEffectsEnabled(incompatibleEffects) || running)
			return EffectResult::Retry;

		running = true;

		Settings::UpdateTwitchSetting("TransparentNotes", "on");
		
		SetDuration(request);
		endTime = std::chrono::steady_clock::now() + std::chrono::seconds(duration);

		return EffectResult::Success;
	}


	/// <summary>
	/// Ensure that the mod only lasts for the time specified in the JSON request.
	/// </summary>
	void TransparentNotesEffect::Run()
	{
		if (running) {
			auto now = std::chrono::steady_clock::now();
			std::chrono::duration<double> duration = (endTime - now);

			if (duration.count() <= 0) Stop();
		}
	}

	/// <summary>
	/// Stops the mod.
	/// </summary>
	/// <returns>EffectResult::Success</returns>
	EffectResult TransparentNotesEffect::Stop()
	{
		_LOG_INIT;

		_LOG_HEAD << "TransparentNotesEffect::Stop()" << LOG.endl();

		running = false;
		Settings::UpdateTwitchSetting("TransparentNotes", "off");

		return EffectResult::Success;
	}
}