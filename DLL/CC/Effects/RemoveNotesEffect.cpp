#include "RemoveNotesEffect.hpp"

namespace CrowdControl::Effects {
	
	/// <summary>
	/// Test the twitch mod's requirements.
	/// </summary>
	/// <param name="request"> - JSON Request</param>
	/// <returns>EffectStatus::Success if test completed without any issues. EffectStatus::Retry if we have to retry.</returns>
	EffectStatus RemoveNotesEffect::Test(Request request)
	{
		_LOG_INIT;

		_LOG("RemoveNotesEffect::Test()" << std::endl);

		if (!CanStart(&EffectList::AllEffects))
			return EffectStatus::Retry;

		return EffectStatus::Success;
	}

	/// <summary>
	/// Sets the scale of each object related to note heads to 0, hence making it temporarily invisible 
	/// </summary>
	/// <returns> EffectStatus::Retry if we aren't currently in a song or the same effect is running already, or EffectStatus::Success if we are in a song</returns>
	EffectStatus RemoveNotesEffect::Start(Request request)
	{
		_LOG_INIT;

		_LOG("RemoveNotesEffect::Start()" << std::endl);

		if (!CanStart(&EffectList::AllEffects))
			return EffectStatus::Retry;

		ScaleNotes(0);

		SetDuration(request);
		running = true;

		return EffectStatus::Success;
	}

	/// <summary>
	/// Stops the mod.
	/// </summary>
	/// <returns>EffectStatus::Success</returns>
	EffectStatus RemoveNotesEffect::Stop()
	{
		_LOG_INIT;

		_LOG("RemoveNotesEffect::Stop()" << std::endl);

		ScaleNotes(1);
		running = false;

		return EffectStatus::Success;
	}

	/// <summary>
	/// Change the scale of the notes
	/// </summary>
	/// <param name="scale"> - Scale to set the notes.</param>
	void RemoveNotesEffect::ScaleNotes(float scale) {
		_LOG_INIT;
		
		_LOG("RemoveNotesEffect::ScaleNotes(" << scale << ")" << std::endl);

		std::map<std::string, float> scaleMap;

		for (const auto& className : ObjectUtil::AllNoteParts) {
			scaleMap[className] = scale;
		}

		ObjectUtil::SetObjectScales(scaleMap);
	}
}