#include "BigNoteheadEffect.hpp"

namespace CrowdControl::Effects { // Scales notes in a song to unusually big size
	
	
	/// <summary>
	/// Test the twitch mod's requirements.
	/// </summary>
	/// <param name="request"> - JSON Request</param>
	/// <returns>EffectResult::Success if test completed without any issues. EffectResult::Retry if we have to retry.</returns>
	EffectResult BigNoteheadEffect::Test(Request request)
	{
		std::cout << "BigNoteheadEffect::Test()" << std::endl;

		if (!MemHelpers::IsInSong() || EffectList::AreIncompatibleEffectsEnabled(incompatibleEffects) || running)
			return EffectResult::Retry;

		return EffectResult::Success;
	}

	/// <summary>
	/// Start the mod by making noteheads 2.5x the size.
	/// </summary>
	/// <returns>EffectResult::Success if test completed without any issues. EffectResult::Retry if we have to retry.</returns>
	EffectResult BigNoteheadEffect::Start(Request request)
	{
		std::cout << "BigNoteheadEffect::Start()" << std::endl;

		if (!MemHelpers::IsInSong() || EffectList::AreIncompatibleEffectsEnabled(incompatibleEffects) || running)
			return EffectResult::Retry;

		running = true;

		SetNoteHeadScale(2.5);
	
		SetDuration(request);
		endTime = std::chrono::steady_clock::now() + std::chrono::seconds(duration);

		return EffectResult::Success;
	}

	/// <summary>
	/// Ensure that the mod only lasts for the time specified in the JSON request.
	/// </summary>
	void BigNoteheadEffect::Run()
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
	EffectResult BigNoteheadEffect::Stop()
	{
		std::cout << "BigNoteheadEffect::Stop()" << std::endl;

		running = false;

		SetNoteHeadScale(1);

		return EffectResult::Success;
	}

	/// <summary>
	/// Sets the scale only for objects which are of note meshe type
	/// </summary>
	void BigNoteheadEffect::SetNoteHeadScale(float scale) {
		std::cout << "BigNoteheadEffect::SetNoteHeadScale(" << scale << ")" << std::endl;

		ObjectUtil::SetObjectScales({
			{"MeshNoteSingleLH", scale},
			{"MeshNoteSingleRH", scale}
			});
	}
}
