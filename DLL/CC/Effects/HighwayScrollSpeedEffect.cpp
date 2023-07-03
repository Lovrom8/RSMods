#include "HighwayScrollSpeedEffect.hpp"

using namespace CrowdControl::Enums;

namespace CrowdControl::Effects {
	
	/// <summary>
	/// Test the twitch mod's requirements.
	/// </summary>
	/// <param name="request"> - JSON Request</param>
	/// <returns>EffectStatus::Success if test completed without any issues. EffectStatus::Retry if we have to retry.</returns>
	EffectStatus HighwayScrollSpeedEffect::Test(Request request)
	{
		_LOG_INIT;

		_LOG("HighwayScrollSpeedEffect::Test()" << std::endl);

		if (!CanStart(&EffectList::AllEffects))
			return EffectStatus::Retry;

		return EffectStatus::Success;
	}

	/// <summary>
	/// Change scroll speed multiplier to let the user have more, or less, time to react to the notes appearing on screen.
	/// </summary>
	/// <returns> EffectStatus::Retry if we aren't currently in a song or incompatible effects are running, or EffectStatus::Sucess if we are</returns>
	EffectStatus HighwayScrollSpeedEffect::Start(Request request)
	{
		_LOG_INIT;

		_LOG("HighwayScrollSpeedEffect::Start()" << std::endl);

		if (!CanStart(&EffectList::AllEffects))
			return EffectStatus::Retry;
		
		WriteScrollSpeedMultiplier(multiplier);

		SetDuration(request);
		running = true;

		return EffectStatus::Success;
	}

	/// <summary>
	/// Stops the mod.
	/// </summary>
	/// <returns>EffectStatus::Success</returns>
	EffectStatus HighwayScrollSpeedEffect::Stop()
	{
		_LOG_INIT;

		_LOG("HighwayScrollSpeedEffect::Stop()" << std::endl);

		WriteScrollSpeedMultiplier(5.0);

		running = false;

		return EffectStatus::Success;
	}

	/// <summary>
	/// Changes the Scroll Speed Multiplier to the parameter.
	/// </summary>
	/// <param name="val"> - New Scroll Speed Modifier</param>
	void HighwayScrollSpeedEffect::WriteScrollSpeedMultiplier(double val) {
		_LOG_INIT;

		_LOG("HighwayScrollSpeedEffect::WriteScrollSpeedMultiplier(" << val << ")" << std::endl);

		MemUtil::SetStaticValue(Offsets::ptr_scrollSpeedMultiplier, val, sizeof(double));
	}
}