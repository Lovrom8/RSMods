#include "RemoveNotesEffect.hpp"

namespace CrowdControl::Effects { // Prevents the game from drawing note head meshes
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

	void RemoveNotesEffect::Run()
	{
		// Stop automatically after duration has elapsed
		if (running) {
			auto now = std::chrono::steady_clock::now();
			std::chrono::duration<double> duration = (endTime - now);

			if (duration.count() <= 0) Stop();
		}
	}

	EffectResult RemoveNotesEffect::Stop()
	{
		std::cout << "RemoveNotesEffect::Stop()" << std::endl;

		ScaleNotes(1);

		running = false;
		//Settings::UpdateTwitchSetting("RemoveNotes", "off");

		return EffectResult::Success;
	}

	void RemoveNotesEffect::ScaleNotes(float scale) {
		std::cout << "RemoveNotesEffect::ScaleNotes(" << scale << ")" << std::endl;

		std::map<std::string, float> scaleMap;

		for (auto className : ObjectUtil::AllNoteParts) {
			scaleMap[className] = scale;
		}

		ObjectUtil::SetObjectScales(scaleMap);
	}
}