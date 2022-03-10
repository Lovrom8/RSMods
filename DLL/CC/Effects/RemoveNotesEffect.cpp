#include "RemoveNotesEffect.hpp"

namespace CrowdControl::Effects { // Prevents the game from drawing note head meshes
	
	/// <summary>
	/// Test the twitch mod's requirements.
	/// </summary>
	/// <param name="request"> - JSON Request</param>
	/// <returns>EffectResult::Success if test completed without any issues. EffectResult::Retry if we have to retry.</returns>
	EffectResult RemoveNotesEffect::Test(Request request)
	{
		std::cout << "RemoveNotesEffect::Test()" << std::endl;

		if (!MemHelpers::IsInSong() || EffectList::AreIncompatibleEffectsEnabled(incompatibleEffects) || running)
			return EffectResult::Retry;

		return EffectResult::Success;
	}

	/// <summary>
	/// Sets the scale of each object related to note heads to 0, hence making it temporarily invisble 
	/// </summary>
	/// <returns> EffectResult::Retry if we aren't currently in a song or the same effect is running already, or EffectResult::Sucess if we are in a song</returns>
	EffectResult RemoveNotesEffect::Start(Request request)
	{
		std::cout << "RemoveNotesEffect::Start()" << std::endl;

		if (!MemHelpers::IsInSong() || EffectList::AreIncompatibleEffectsEnabled(incompatibleEffects) || running)
			return EffectResult::Retry;

		running = true;

		ScaleNotes(0);
	
		SetDuration(request);
		endTime = std::chrono::steady_clock::now() + std::chrono::seconds(duration);

		return EffectResult::Success;
	}


	/// <summary>
	/// Ensure that the mod only lasts for the time specified in the JSON request.
	/// </summary>
	void RemoveNotesEffect::Run()
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
	EffectResult RemoveNotesEffect::Stop()
	{
		std::cout << "RemoveNotesEffect::Stop()" << std::endl;

		ScaleNotes(1);

		running = false;

		return EffectResult::Success;
	}

	/// <summary>
	/// Change the scale of the notes
	/// </summary>
	/// <param name="scale"> - Scale to set the notes.</param>
	void RemoveNotesEffect::ScaleNotes(float scale) {
		std::cout << "RemoveNotesEffect::ScaleNotes(" << scale << ")" << std::endl;

		std::map<std::string, float> scaleMap;

		for (auto className : ObjectUtil::AllNoteParts) {
			scaleMap[className] = scale;
		}

		ObjectUtil::SetObjectScales(scaleMap);
	}
}