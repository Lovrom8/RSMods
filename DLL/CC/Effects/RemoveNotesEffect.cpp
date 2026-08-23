#include "../../stdafx.h"
#include "RemoveNotesEffect.hpp"

namespace CrowdControl::Effects {
	
	/// <summary>
	/// Test the twitch mod's requirements.
	/// </summary>
	/// <param name="request"> - JSON Request</param>
	/// <returns>Enums::EffectStatus::Success if test completed without any issues. Enums::EffectStatus::Retry if we have to retry.</returns>
	Enums::EffectStatus RemoveNotesEffect::Test(const Structs::Request& request)
	{
		LOG_INFO("RemoveNotesEffect::Test()" << std::endl);

		if (!CanStart())
			return Enums::EffectStatus::Retry;

		return Enums::EffectStatus::Success;
	}

	/// <summary>
	/// Sets the scale of each object related to note heads to 0, hence making it temporarily invisible 
	/// </summary>
	/// <returns> Enums::EffectStatus::Retry if we aren't currently in a song or the same effect is running already, or Enums::EffectStatus::Success if we are in a song</returns>
	Enums::EffectStatus RemoveNotesEffect::OnStart(const Structs::Request& request)
	{
		LOG_INFO("RemoveNotesEffect::Start()" << std::endl);

		ScaleNotes(0);

		SetDuration(request);
		running = true;

		return Enums::EffectStatus::Success;
	}

	/// <summary>
	/// Stops the mod.
	/// </summary>
	/// <returns>Enums::EffectStatus::Success</returns>
	Enums::EffectStatus RemoveNotesEffect::OnStop()
	{
		LOG_INFO("RemoveNotesEffect::Stop()" << std::endl);

		ScaleNotes(1);
		running = false;

		return Enums::EffectStatus::Success;
	}

	/// <summary>
	/// Change the scale of the notes
	/// </summary>
	/// <param name="scale"> - Scale to set the notes.</param>
	void RemoveNotesEffect::ScaleNotes(float scale) {		
		LOG_INFO("RemoveNotesEffect::ScaleNotes(" << scale << ")" << std::endl);

		std::map<std::string, float> scaleMap;

		for (const auto& className : ObjectUtil::AllNoteParts) {
			scaleMap[className] = scale;
		}

		ObjectUtil::SetObjectScales(scaleMap);
	}
}