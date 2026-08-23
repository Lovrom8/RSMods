#include "../../stdafx.h"
#include "KillGuitarVolumeEffect.hpp"
namespace Setting = Settings::Setting;

namespace CrowdControl::Effects {
	
	/// <summary>
	/// Test the twitch mod's requirements.
	/// </summary>
	/// <param name="request"> - JSON Request</param>
	/// <returns>Enums::EffectStatus::Success if test completed without any issues. Enums::EffectStatus::Retry if we have to retry.</returns>
	Enums::EffectStatus KillGuitarVolumeEffect::Test(const Structs::Request& request)
	{
		LOG_INFO("KillGuitarVolumeEffect::Test()" << std::endl);

		if (!CanStart())
			return Enums::EffectStatus::Retry;

		return Enums::EffectStatus::Success;
	}

	/// <summary>
	/// We can individually set volumes of the WWise objects the game uses
	/// The current volume is read by using Wwise_Sound_Query_GetRTPCValue_Char
	/// New volume is set using Wwise_Sound_Query_SetRTPCValue_Char, the game calls it with both AK_INVALID_GAME_OBJECT and 0x1234 as object IDs 
	/// </summary>
	/// <returns> Enums::EffectStatus::Retry if we aren't currently in a song or the same effect is running already, or Enums::EffectStatus::Success if we are in a song</returns>
	Enums::EffectStatus KillGuitarVolumeEffect::OnStart(const Structs::Request& request)
	{
		LOG_INFO("KillGuitarVolumeEffect::Start()" << std::endl);

		RTPCValue_type type = RTPCValue_GameObject; // Save old volume

		Wwise::SoundEngine::Query::GetRTPCValue(Setting::Channel::Player1, AK_INVALID_GAME_OBJECT, &oldVolume, &type);

		Wwise::SoundEngine::SetRTPCValue(Setting::Channel::Player1, 0.0f, AK_INVALID_GAME_OBJECT, 2000, AkCurveInterpolation_Linear);
		Wwise::SoundEngine::SetRTPCValue(Setting::Channel::Player1, 0.0f, 0x1234, 2000, AkCurveInterpolation_Linear);

		SetDuration(request);
		running = true;

		return Enums::EffectStatus::Success;
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
				Wwise::SoundEngine::SetRTPCValue(Setting::Channel::Player1, oldVolume, AK_INVALID_GAME_OBJECT, 2000, AkCurveInterpolation_Linear);
				Wwise::SoundEngine::SetRTPCValue(Setting::Channel::Player1, oldVolume, 0x1234, 2000, AkCurveInterpolation_Linear);
				ending = true;
			}

			if (duration.count() <= 0) Stop();
		}
	}

	/// <summary>
	/// Restore the volume back to its original values
	/// </summary>
	/// <returns> Enums::EffectStatus::Success</returns>
	Enums::EffectStatus KillGuitarVolumeEffect::OnStop()
	{
		LOG_INFO("KillGuitarVolumeEffect::Stop()" << std::endl);

		// Make sure volume was set to original value by setting it immediately effective
		Wwise::SoundEngine::SetRTPCValue(Setting::Channel::Player1, oldVolume, AK_INVALID_GAME_OBJECT, 0, AkCurveInterpolation_Linear);
		Wwise::SoundEngine::SetRTPCValue(Setting::Channel::Player1, oldVolume, 0x1234, 0, AkCurveInterpolation_Linear);

		running = false;
		ending = false;

		return Enums::EffectStatus::Success;
	}
}