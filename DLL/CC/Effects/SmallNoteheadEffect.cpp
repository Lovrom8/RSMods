#include "SmallNoteheadEffect.hpp"

namespace CrowdControl::Effects { // Scales notes in a song to unusually small size
	
	/// <summary>
	/// Test the twitch mod's requirements.
	/// </summary>
	/// <param name="request"> - JSON Request</param>
	/// <returns>EffectResult::Success if test completed without any issues. EffectResult::Retry if we have to retry.</returns>
	EffectResult SmallNoteheadEffect::Test(Request request)
	{
		_LOG_INIT;

		_LOG_HEAD << "SmallNoteheadEffect::Test()" << LOG.endl();

		if (!MemHelpers::IsInSong() || EffectList::AreIncompatibleEffectsEnabled(incompatibleEffects) || running)
			return EffectResult::Retry;

		return EffectResult::Success;
	}

	/// <summary>
	/// Set the scale of the noteheads to 0.5x
	/// </summary>
	/// <param name="request"> - JSON Request</param>
	/// <returns>EffectResult::Success if test completed without any issues. EffectResult::Retry if we have to retry.</returns>
	EffectResult SmallNoteheadEffect::Start(Request request)
	{
		_LOG_INIT;

		_LOG_HEAD << "SmallNoteheadEffect::Start()" << LOG.endl();

		if (!MemHelpers::IsInSong() || EffectList::AreIncompatibleEffectsEnabled(incompatibleEffects) || running)
			return EffectResult::Retry;

		running = true;

		SetNoteHeadScale(0.5);

		SetDuration(request);
		endTime = std::chrono::steady_clock::now() + std::chrono::seconds(duration);

		return EffectResult::Success;
	}

	/// <summary>
	/// Ensure that the mod only lasts for the time specified in the JSON request.
	/// </summary>
	void SmallNoteheadEffect::Run()
	{
		if (running) {
			
			auto now = std::chrono::steady_clock::now();
			std::chrono::duration<double> duration = (endTime - now);

			if (duration.count() <= 0) Stop();
		}
	}

	/// <summary>
	/// Stops the mod.
	/// </summary>
	/// <returns>EffectResult::Success</returns>
	EffectResult SmallNoteheadEffect::Stop()
	{
		_LOG_INIT;

		_LOG_HEAD << "SmallNoteheadEffect::Stop()" << LOG.endl();

		running = false;
		SetNoteHeadScale(1);

		return EffectResult::Success;
	}

	/// <summary>
	/// Sets the scale only for objects which are of note meshe type
	/// </summary>
	/// <param name="scale"></param>
	void SmallNoteheadEffect::SetNoteHeadScale(float scale) {
		_LOG_INIT;

		_LOG_HEAD << "SmallNoteheadEffect::SetNoteHeadScale(" << scale << ")" << LOG.endl();

		ObjectUtil::SetObjectScales({
			{"MeshNoteSingleLH", scale},
			{"MeshNoteSingleRH", scale}
			});
	}
}
