#include "../../stdafx.h"
#include "SmallNoteheadEffect.hpp"

namespace CrowdControl::Effects { // Scales notes in a song to unusually small size
	
	/// <summary>
	/// Test the twitch mod's requirements.
	/// </summary>
	/// <param name="request"> - JSON Request</param>
	/// <returns>Enums::EffectStatus::Success if test completed without any issues. Enums::EffectStatus::Retry if we have to retry.</returns>
	Enums::EffectStatus SmallNoteheadEffect::Test(const Structs::Request& request)
	{
		LOG_INFO("SmallNoteheadEffect::Test()" << std::endl);

		if (!CanStart())
			return Enums::EffectStatus::Retry;

		return Enums::EffectStatus::Success;
	}

	/// <summary>
	/// Set the scale of the noteheads to 0.5x
	/// </summary>
	/// <param name="request"> - JSON Request</param>
	/// <returns>Enums::EffectStatus::Success if test completed without any issues. Enums::EffectStatus::Retry if we have to retry.</returns>
	Enums::EffectStatus SmallNoteheadEffect::OnStart(const Structs::Request& request)
	{
		LOG_INFO("SmallNoteheadEffect::Start()" << std::endl);

		SetNoteHeadScale(0.5);

		SetDuration(request);
		running = true;

		return Enums::EffectStatus::Success;
	}

	/// <summary>
	/// Stops the mod.
	/// </summary>
	/// <returns>Enums::EffectStatus::Success</returns>
	Enums::EffectStatus SmallNoteheadEffect::OnStop()
	{
		LOG_INFO("SmallNoteheadEffect::Stop()" << std::endl);

		running = false;
		SetNoteHeadScale(1);

		return Enums::EffectStatus::Success;
	}

	/// <summary>
	/// Sets the scale only for objects which are of note mesh type
	/// </summary>
	/// <param name="scale"></param>
	void SmallNoteheadEffect::SetNoteHeadScale(float scale) {
		LOG_INFO("SmallNoteheadEffect::SetNoteHeadScale(" << scale << ")" << std::endl);

		ObjectUtil::SetObjectScales({
			{"MeshNoteSingleLH", scale},
			{"MeshNoteSingleRH", scale}
		});
	}
}
