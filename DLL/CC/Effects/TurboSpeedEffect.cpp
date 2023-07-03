#include "TurboSpeedEffect.hpp"

using namespace CrowdControl::Enums;

namespace CrowdControl::Effects {

	/// <summary>
	/// Test the twitch mod's requirements.
	/// </summary>
	/// <param name="request"> - JSON Request</param>
	/// <returns>EffectStatus::Success if test completed without any issues. EffectStatus::Retry if we have to retry.</returns>
	EffectStatus TurboSpeedEffect::Test(Request request)
	{
		_LOG_INIT;

		_LOG("TurboSpeedEffect::Test()" << std::endl);

		if (!CanStart(&EffectList::AllEffects))
			return EffectStatus::Retry;

		return EffectStatus::Success;
	}

	/// <summary>
	/// Set Riff Repeater speed to 200%.
	/// </summary>
	/// <param name="request"> - JSON Request</param>
	/// <returns>EffectStatus::Success if test completed without any issues. EffectStatus::Retry if we have to retry.</returns>
	EffectStatus TurboSpeedEffect::Start(Request request)
	{
		_LOG_INIT;

		_LOG("TurboSpeedEffect::Start()" << std::endl);

		if (!CanStart(&EffectList::AllEffects))
			return EffectStatus::Retry;
		
		RiffRepeater::SetSpeed(200.f, true);
		RiffRepeater::EnableTimeStretch();

		SetDuration(request);
		running = true;

		return EffectStatus::Success;
	}

	/// <summary>
	/// Stops the mod.
	/// </summary>
	/// <returns>EffectStatus::Success</returns>
	EffectStatus TurboSpeedEffect::Stop()
	{
		_LOG_INIT;

		_LOG("TurboSpeedEffect::Stop()" << std::endl);

		RiffRepeater::SetSpeed(100.f);
		RiffRepeater::DisableTimeStretch();

		running = false;

		return EffectStatus::Success;
	}
}