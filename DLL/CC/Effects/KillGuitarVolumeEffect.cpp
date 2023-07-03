#include "KillGuitarVolumeEffect.hpp"

namespace CrowdControl::Effects {
	
	/// <summary>
	/// Test the twitch mod's requirements.
	/// </summary>
	/// <param name="request"> - JSON Request</param>
	/// <returns>EffectStatus::Success if test completed without any issues. EffectStatus::Retry if we have to retry.</returns>
	EffectStatus KillGuitarVolumeEffect::Test(Request request)
	{
		_LOG_INIT;

		_LOG("KillGuitarVolumeEffect::Test()" << std::endl);

		if (!CanStart(&EffectList::AllEffects))
			return EffectStatus::Retry;

		return EffectStatus::Success;
	}

	/// <summary>
	/// We can individually set volumes of the WWise objects the game uses
	/// The current volume is read by using Wwise_Sound_Query_GetRTPCValue_Char
	/// New volume is set using Wwise_Sound_Query_SetRTPCValue_Char, the game calls it with both AK_INVALID_GAME_OBJECT and 0x1234 as object IDs 
	/// </summary>
	/// <returns> EffectStatus::Retry if we aren't currently in a song or the same effect is running already, or EffectStatus::Sucess if we are in a song</returns>
	EffectStatus KillGuitarVolumeEffect::Start(Request request)
	{
		_LOG_INIT;

		_LOG("KillGuitarVolumeEffect::Start()" << std::endl);

		if (!CanStart(&EffectList::AllEffects))
			return EffectStatus::Retry;

		RTPCValue_type type = RTPCValue_GameObject; // Save old volume
		
		Wwise::SoundEngine::Query::GetRTPCValue("Mixer_Player1", AK_INVALID_GAME_OBJECT, &oldVolume, &type);

		Wwise::SoundEngine::SetRTPCValue("Mixer_Player1", 0.0f, AK_INVALID_GAME_OBJECT, 2000, AkCurveInterpolation_Linear);
		Wwise::SoundEngine::SetRTPCValue("Mixer_Player1", 0.0f, 0x1234, 2000, AkCurveInterpolation_Linear);

		SetDuration(request);
		running = true;

		return EffectStatus::Success;
	}


	/// <summary>
	/// Ensure that the mod only lasts for the time specified in the JSON request.
	/// </summary>
	void KillGuitarVolumeEffect::Run()
	{
		if (running) {
			auto now = std::chrono::steady_clock::now();
			std::chrono::duration<double> duration = (endTime - now);

			// Start fadeout before effect actually stops
			if (duration < std::chrono::milliseconds(2000) && !ending) {
				// Restore volume
				Wwise::SoundEngine::SetRTPCValue("Mixer_Player1", oldVolume, AK_INVALID_GAME_OBJECT, 2000, AkCurveInterpolation_Linear);
				Wwise::SoundEngine::SetRTPCValue("Mixer_Player1", oldVolume, 0x1234, 2000, AkCurveInterpolation_Linear);
				ending = true;
			}

			if (duration.count() <= 0) Stop();
		}
	}

	/// <summary>
	/// Restore the volume back to it's original values
	/// </summary>
	/// <returns> EffectStatus::Success</returns>
	EffectStatus KillGuitarVolumeEffect::Stop()
	{
		_LOG_INIT;

		_LOG("KillGuitarVolumeEffect::Stop()" << std::endl);

		// Make sure volume was set to original value by setting it immediately effective
		Wwise::SoundEngine::SetRTPCValue("Mixer_Player1", oldVolume, AK_INVALID_GAME_OBJECT, 0, AkCurveInterpolation_Linear);
		Wwise::SoundEngine::SetRTPCValue("Mixer_Player1", oldVolume, 0x1234, 0, AkCurveInterpolation_Linear);

		running = false;
		ending = false;

		return EffectStatus::Success;
	}
}