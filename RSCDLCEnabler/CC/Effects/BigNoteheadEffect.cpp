#include "BigNoteheadEffect.hpp"

namespace CrowdControl::Effects { // Scales notes in a song to unusually big size
	EffectResult BigNoteheadEffect::Test(Request request)
	{
		std::cout << "BigNoteheadEffect::Test()" << std::endl;

		if (!MemHelpers::IsInSong() || EffectList::AreIncompatibleEffectsEnabled(incompatibleEffects) || running)
			return EffectResult::Retry;

		return EffectResult::Success;
	}

	EffectResult BigNoteheadEffect::Start(Request request)
	{
		std::cout << "BigNoteheadEffect::Start()" << std::endl;

		if (!MemHelpers::IsInSong() || EffectList::AreIncompatibleEffectsEnabled(incompatibleEffects) || running)
			return EffectResult::Retry;

		running = true;

		SetNoteHeadScale(2.5);

		request.parameters.at(0).get_to(duration);
		endTime = std::chrono::steady_clock::now() + std::chrono::seconds(duration);

		return EffectResult::Success;
	}

	void BigNoteheadEffect::Run()
	{
		// Stop automatically after duration has elapsed
		if (running) {
			auto now = std::chrono::steady_clock::now();
			std::chrono::duration<double> duration = (endTime - now);

			if (duration.count() <= 0) Stop();
		}
	}

	EffectResult BigNoteheadEffect::Stop()
	{
		std::cout << "BigNoteheadEffect::Stop()" << std::endl;

		running = false;

		SetNoteHeadScale(1);

		return EffectResult::Success;
	}

	void BigNoteheadEffect::SetNoteHeadScale(float scale) {
		std::cout << "BigNoteheadEffect::SetNoteHeadScale(" << scale << ")" << std::endl;

		ObjectUtil::SetObjectScales({
			{"MeshNoteSingleLH", scale},
			{"MeshNoteSingleRH", scale}
			});
	}
}
