#include "RemoveInstrumentEffect.hpp"

namespace CrowdControl::Effects { // Scales notes in a song to unusually small size
	EffectResult RemoveInstrumentEffect::Test(Request request)
	{
		std::cout << "RemoveInstrumentEffect::Test()" << std::endl;

		return EffectResult::Success;
	}

	EffectResult RemoveInstrumentEffect::Start(Request request)
	{
		std::cout << "RemoveInstrumentEffect::Start()" << std::endl;

		if (!MemHelpers::IsInSong() || EffectList::AreIncompatibleEffectsEnabled(incompatibleEffects))
			return EffectResult::Retry;

		running = true;
		endTime = std::chrono::steady_clock::now() + std::chrono::seconds(duration);

		SetInstrumentScale(0);

		return EffectResult::Success;
	}

	void RemoveInstrumentEffect::Run()
	{
		// Stop automatically after duration has elapsed
		if (running) {

			auto now = std::chrono::steady_clock::now();
			std::chrono::duration<double> duration = (endTime - now);

			if (duration.count() <= 0) Stop();
		}
	}

	EffectResult RemoveInstrumentEffect::Stop()
	{
		std::cout << "RemoveInstrumentEffect::Stop()" << std::endl;

		running = false;
		SetInstrumentScale(1);

		return EffectResult::Success;
	}

	void RemoveInstrumentEffect::SetInstrumentScale(float scale) {
		std::cout << "RemoveInstrumentEffect::SetInstrumentScale(" << scale << ")" << std::endl;

		ObjectUtil::SetObjectScales({
			{"ActorGuitarStringStandard", scale},
			{"GRGuitarInlay", scale},
			{"MeshGuitarCapo", scale},
			{"MeshGuitarFrameStandardInlinedRH", scale},
			{"MeshGuitarFrameStandardInlinedRHFlip", scale},
			{"MeshGuitarFrameStandardInlinedLH", scale},
			{"MeshGuitarFrameStandardInlinedLHFlip", scale},
			{"MeshGuitarFretBarNormal", scale},
			{"MeshGuitarPegStandardInlinedRH", scale},
			{"MeshGuitarPegStandardInlinedLH", scale},
			{"MeshGuitarNumber0", scale},
			{"MeshGuitarNumber1", scale},
			{"MeshGuitarNumber2", scale},
			{"MeshGuitarNumber3", scale},
			{"MeshGuitarNumber4", scale},
			{"MeshGuitarNumber5", scale},
			{"MeshGuitarNumber6", scale},
			{"MeshGuitarNumber7", scale},
			{"MeshGuitarNumber8", scale},
			{"MeshGuitarNumber9", scale}
			});
	}
}