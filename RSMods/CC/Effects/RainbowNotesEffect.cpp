#include "RainbowNotesEffect.hpp"

namespace CrowdControl::Effects {
	
	/// <summary>
	/// Test the twitch mod's requirements.
	/// </summary>
	/// <param name="request"> - JSON Request</param>
	/// <returns>EffectResult::Success if test completed without any issues. EffectResult::Retry if we have to retry.</returns>
	EffectResult RainbowNotesEffect::Test(Request request)
	{
		std::cout << "RainbowNotesEffect::Test()" << std::endl;

		if (ERMode::IsRainbowNotesEnabled() || !MemHelpers::IsInSong() || EffectList::AreIncompatibleEffectsEnabled(incompatibleEffects))
			return EffectResult::Retry;

		return EffectResult::Success;
	}

	/// <summary>
	/// Makes note heads continously shift their colors 
	/// All note head changing is handled in ERMode, so it just toggles the switch in there
	/// Does not affect the strings!
	/// </summary>
	/// <returns> EffectResult::Retry if we aren't currently in a song or the same effect is running already, or EffectResult::Sucess if we are in a song</returns>
	EffectResult RainbowNotesEffect::Start(Request request)
	{
		std::cout << "RainbowNotesEffect::Start()" << std::endl;

		if (ERMode::IsRainbowNotesEnabled() || !MemHelpers::IsInSong() || EffectList::AreIncompatibleEffectsEnabled(incompatibleEffects))
			return EffectResult::Retry;

		running = true;

		ERMode::ToggleRainbowNotes();

		SetDuration(request);
		endTime = std::chrono::steady_clock::now() + std::chrono::seconds(duration);

		return EffectResult::Success;
	}

	/// <summary>
	/// Ensure that the mod only lasts for the time specified in the JSON request.
	/// </summary>
	void RainbowNotesEffect::Run()
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
	EffectResult RainbowNotesEffect::Stop()
	{
		std::cout << "RainbowNotesEffect::Stop()" << std::endl;

		running = false;
		ERMode::ToggleRainbowNotes();

		return EffectResult::Success;
	}
}