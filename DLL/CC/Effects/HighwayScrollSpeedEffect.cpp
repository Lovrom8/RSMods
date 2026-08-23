#include "../../stdafx.h"
#include "HighwayScrollSpeedEffect.hpp"

using namespace CrowdControl::Enums;

namespace CrowdControl::Effects {
	
	/// <summary>
	/// Test the twitch mod's requirements.
	/// </summary>
	/// <param name="request"> - JSON Request</param>
	/// <returns>Enums::EffectStatus::Success if test completed without any issues. Enums::EffectStatus::Retry if we have to retry.</returns>
	Enums::EffectStatus HighwayScrollSpeedEffect::Test(const Structs::Request& request)
	{
		LOG_INFO("HighwayScrollSpeedEffect::Test()" << std::endl);

		if (!CanStart())
			return Enums::EffectStatus::Retry;

		return Enums::EffectStatus::Success;
	}

	/// <summary>
	/// Change scroll speed multiplier to let the user have more, or less, time to react to the notes appearing on screen.
	/// </summary>
	/// <returns> Enums::EffectStatus::Retry if we aren't currently in a song or incompatible effects are running, or Enums::EffectStatus::Success if we are</returns>
	Enums::EffectStatus HighwayScrollSpeedEffect::OnStart(const Structs::Request& request)
	{
		LOG_INFO("HighwayScrollSpeedEffect::Start()" << std::endl);

		WriteScrollSpeedMultiplier(multiplier);

		SetDuration(request);
		running = true;

		return Enums::EffectStatus::Success;
	}

	/// <summary>
	/// Stops the mod.
	/// </summary>
	/// <returns>Enums::EffectStatus::Success</returns>
	Enums::EffectStatus HighwayScrollSpeedEffect::OnStop()
	{
		LOG_INFO("HighwayScrollSpeedEffect::Stop()" << std::endl);

		WriteScrollSpeedMultiplier(5.0);

		running = false;

		return Enums::EffectStatus::Success;
	}

	/// <summary>
	/// Changes the Scroll Speed Multiplier to the parameter.
	/// </summary>
	/// <param name="val"> - New Scroll Speed Modifier</param>
	void HighwayScrollSpeedEffect::WriteScrollSpeedMultiplier(double val) {
		LOG_INFO("HighwayScrollSpeedEffect::WriteScrollSpeedMultiplier(" << val << ")" << std::endl);

		MemUtil::SetStaticValue(Offsets::ptr_scrollSpeedMultiplier.Get(), val, sizeof(double));
	}
}