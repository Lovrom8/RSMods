#include "KillMusicVolumeEffect.hpp"

namespace CrowdControl::Effects {
	EffectResult KillMusicVolumeEffect::Test(Request request)
	{
		std::cout << "KillMusicVolumeEffect::Test()" << std::endl;

		if (!MemHelpers::IsInSong() || running)
			return EffectResult::Retry;

		return EffectResult::Success;
	}

	EffectResult KillMusicVolumeEffect::Start(Request request)
	{
		std::cout << "KillMusicVolumeEffect::Start()" << std::endl;

		if (!MemHelpers::IsInSong() || running)
			return EffectResult::Retry;

		running = true;

		RTPCValue_type type = RTPCValue_GameObject; // Save old volume
		WwiseVariables::Wwise_Sound_Query_GetRTPCValue_Char("Mixer_Music", 0xffffffff, &oldVolume, &type);

		WwiseVariables::Wwise_Sound_SetRTPCValue_Char("Mixer_Music", 0.0f, 0xffffffff, 2000, AkCurveInterpolation_Linear);
		WwiseVariables::Wwise_Sound_SetRTPCValue_Char("Mixer_Music", 0.0f, 0x00001234, 2000, AkCurveInterpolation_Linear);

		endTime = std::chrono::steady_clock::now() + std::chrono::seconds(duration);

		return EffectResult::Success;
	}

	void KillMusicVolumeEffect::Run()
	{
		// Stop automatically after duration has elapsed
		if (running) {
			auto now = std::chrono::steady_clock::now();
			std::chrono::duration<double> duration = (endTime - now);

			if (duration.count() <= 0) Stop();
		}
	}

	EffectResult KillMusicVolumeEffect::Stop()
	{
		std::cout << "KillGuitarVolumeEffect::Stop()" << std::endl;

		running = false;

		// Restore volume
		WwiseVariables::Wwise_Sound_SetRTPCValue_Char("Mixer_Music", oldVolume, 0xffffffff, 2000, AkCurveInterpolation_Linear);
		WwiseVariables::Wwise_Sound_SetRTPCValue_Char("Mixer_Music", oldVolume, 0x00001234, 2000, AkCurveInterpolation_Linear);

		return EffectResult::Success;
	}
}