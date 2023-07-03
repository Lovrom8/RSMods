#include "SmallNoteheadEffect.hpp"

namespace CrowdControl::Effects { // Scales notes in a song to unusually small size
	
	/// <summary>
	/// Test the twitch mod's requirements.
	/// </summary>
	/// <param name="request"> - JSON Request</param>
	/// <returns>EffectStatus::Success if test completed without any issues. EffectStatus::Retry if we have to retry.</returns>
	EffectStatus SmallNoteheadEffect::Test(Request request)
	{
		_LOG_INIT;

		_LOG("SmallNoteheadEffect::Test()" << std::endl);

		if (!CanStart(&EffectList::AllEffects))
			return EffectStatus::Retry;

		return EffectStatus::Success;
	}

	/// <summary>
	/// Set the scale of the noteheads to 0.5x
	/// </summary>
	/// <param name="request"> - JSON Request</param>
	/// <returns>EffectStatus::Success if test completed without any issues. EffectStatus::Retry if we have to retry.</returns>
	EffectStatus SmallNoteheadEffect::Start(Request request)
	{
		_LOG_INIT;

		_LOG("SmallNoteheadEffect::Start()" << std::endl);

		if (!CanStart(&EffectList::AllEffects))
			return EffectStatus::Retry;

		SetNoteHeadScale(0.5);

		SetDuration(request);
		running = true;

		return EffectStatus::Success;
	}

	/// <summary>
	/// Stops the mod.
	/// </summary>
	/// <returns>EffectStatus::Success</returns>
	EffectStatus SmallNoteheadEffect::Stop()
	{
		_LOG_INIT;

		_LOG("SmallNoteheadEffect::Stop()" << std::endl);

		running = false;
		SetNoteHeadScale(1);

		return EffectStatus::Success;
	}

	/// <summary>
	/// Sets the scale only for objects which are of note mesh type
	/// </summary>
	/// <param name="scale"></param>
	void SmallNoteheadEffect::SetNoteHeadScale(float scale) {
		_LOG_INIT;

		_LOG("SmallNoteheadEffect::SetNoteHeadScale(" << scale << ")" << std::endl);

		ObjectUtil::SetObjectScales({
			{"MeshNoteSingleLH", scale},
			{"MeshNoteSingleRH", scale}
			});
	}
}
