#include "SmallNoteheadEffect.hpp"

using namespace CrowdControl::Enums;

namespace CrowdControl::Effects { // Scales notes in a song to unusually small size
	EffectResult SmallNoteheadEffect::Test(Request request)
	{
		std::cout << "SmallNoteheadEffect::Test()" << std::endl;

		return EffectResult::Success;
	}

	EffectResult SmallNoteheadEffect::Start(Request request)
	{
		std::cout << "SmallNoteheadEffect::Start()" << std::endl;

		if (!MemHelpers::IsInSong())
			return EffectResult::Retry;

		running = true;
		endTime = std::chrono::steady_clock::now() + std::chrono::seconds(duration);

		SetNoteHeadScale(0.5);

		return EffectResult::Success;
	}

	void SmallNoteheadEffect::Run()
	{
		// Stop automatically after duration has elapsed
		if (running) {
			
			auto now = std::chrono::steady_clock::now();
			std::chrono::duration<double> duration = (endTime - now);

			if (duration.count() <= 0) Stop();
		}
	}

	EffectResult SmallNoteheadEffect::Stop()
	{
		std::cout << "SmallNoteheadEffect::Stop()" << std::endl;

		running = false;
		SetNoteHeadScale(1);

		return EffectResult::Success;
	}

	void SmallNoteheadEffect::SetNoteHeadScale(float scale) {
		std::cout << "SmallNoteheadEffect::SetNoteHeadScale(" << scale << ")" << std::endl;

		ObjectUtil::SetObjectScales({
			{"MeshNoteSingleLH", scale},
			{"MeshNoteSingleRH", scale}
			});
	}
}
