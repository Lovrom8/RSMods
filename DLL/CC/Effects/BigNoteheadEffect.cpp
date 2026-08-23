#include "../../stdafx.h"
#include "BigNoteheadEffect.hpp"

namespace CrowdControl::Effects {
	/// <summary>
	/// Test the twitch mod's requirements.
	/// </summary>
	/// <param name="request"> - JSON Request</param>
	/// <returns>Enums::EffectStatus::Success if test completed without any issues. Enums::EffectStatus::Retry if we have to retry.</returns>
	Enums::EffectStatus BigNoteheadEffect::Test(const Structs::Request& request)
	{
		LOG_INFO("BigNoteheadEffect::Test()" << std::endl);

		if (!CanStart())
			return Enums::EffectStatus::Retry;

		return Enums::EffectStatus::Success;
	}

	/// <summary>
	/// Start the mod by making noteheads 2.5x the size.
	/// </summary>
	/// <returns>Enums::EffectStatus::Success if test completed without any issues. Enums::EffectStatus::Retry if we have to retry.</returns>
	Enums::EffectStatus BigNoteheadEffect::OnStart(const Structs::Request& request)
	{
		LOG_INFO("BigNoteheadEffect::Start()" << std::endl);

		SetNoteHeadScale(2.5);

		SetDuration(request);
		running = true;

		return Enums::EffectStatus::Success;
	}

	/// <summary>
	/// Stops the mod.
	/// </summary>
	/// <returns>Enums::EffectStatus::Success</returns>
	Enums::EffectStatus BigNoteheadEffect::OnStop()
	{
		LOG_INFO("BigNoteheadEffect::Stop()" << std::endl);

		SetNoteHeadScale(1);
		running = false;

		return Enums::EffectStatus::Success;
	}

	/// <summary>
	/// Sets the scale only for objects which are of note mesh type
	/// </summary>
	void BigNoteheadEffect::SetNoteHeadScale(float scale) {
		LOG_INFO("BigNoteheadEffect::SetNoteHeadScale(" << scale << ")" << std::endl);

		ObjectUtil::SetObjectScales({
			{"MeshNoteSingleLH", scale},
			{"MeshNoteSingleRH", scale}
		});
	}
}
