#include "../../stdafx.h"
#include "TurboSpeedEffect.hpp"

using namespace CrowdControl::Enums;

namespace CrowdControl::Effects {

	/// <summary>
	/// Test the twitch mod's requirements.
	/// </summary>
	/// <param name="request"> - JSON Request</param>
	/// <returns>Enums::EffectStatus::Success if test completed without any issues. Enums::EffectStatus::Retry if we have to retry.</returns>
	Enums::EffectStatus TurboSpeedEffect::Test(const Structs::Request& request)
	{
		LOG_INFO("TurboSpeedEffect::Test()" << std::endl);

		if (!CanStart())
			return Enums::EffectStatus::Retry;

		return Enums::EffectStatus::Success;
	}

	/// <summary>
	/// Set Riff Repeater speed to 200%.
	/// </summary>
	/// <param name="request"> - JSON Request</param>
	/// <returns>Enums::EffectStatus::Success if test completed without any issues. Enums::EffectStatus::Retry if we have to retry.</returns>
	Enums::EffectStatus TurboSpeedEffect::OnStart(const Structs::Request& request)
	{
		LOG_INFO("TurboSpeedEffect::Start()" << std::endl);

		RiffRepeater::SetSpeed(200.f, true);
		RiffRepeater::EnableTimeStretch();

		SetDuration(request);
		running = true;

		return Enums::EffectStatus::Success;
	}

	/// <summary>
	/// Stops the mod.
	/// </summary>
	/// <returns>Enums::EffectStatus::Success</returns>
	Enums::EffectStatus TurboSpeedEffect::OnStop()
	{
		LOG_INFO("TurboSpeedEffect::Stop()" << std::endl);

		RiffRepeater::SetSpeed(100.f);
		RiffRepeater::DisableTimeStretch();

		running = false;

		return Enums::EffectStatus::Success;
	}
}