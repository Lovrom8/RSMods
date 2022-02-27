#include "RemoveInstrumentEffect.hpp"

namespace CrowdControl::Effects { 

	/// <summary>
	/// Test the twitch mod's requirements.
	/// </summary>
	/// <param name="request"> - JSON Request</param>
	/// <returns>EffectResult::Success if test completed without any issues. EffectResult::Retry if we have to retry.</returns>
	EffectResult RemoveInstrumentEffect::Test(Request request)
	{
		_LOG_INIT;

		_LOG("RemoveInstrumentEffect::Test()" << std::endl);

		return EffectResult::Success;
	}

	/// <summary>
	/// Sets the scale of each instrument related object (headstock, inlays, fretbars, ...) to 0, hence making it temporarily invisble 
	/// </summary>
	/// <returns> EffectResult::Retry if we aren't currently in a song or the same effect is running already, or EffectResult::Sucess if we are in a song</returns>
	EffectResult RemoveInstrumentEffect::Start(Request request)
	{
		_LOG_INIT;

		_LOG("RemoveInstrumentEffect::Start()" << std::endl);

		if (!MemHelpers::IsInSong() || EffectList::AreIncompatibleEffectsEnabled(incompatibleEffects))
			return EffectResult::Retry;

		running = true;

		SetInstrumentScale(0);

		SetDuration(request);
		endTime = std::chrono::steady_clock::now() + std::chrono::seconds(duration);

		return EffectResult::Success;
	}

	/// <summary>
	/// Ensure that the mod only lasts for the time specified in the JSON request.
	/// </summary>
	void RemoveInstrumentEffect::Run()
	{
		// Stop automatically after duration has elapsed
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
	EffectResult RemoveInstrumentEffect::Stop()
	{
		_LOG_INIT;

		_LOG("RemoveInstrumentEffect::Stop()" << std::endl);

		running = false;
		SetInstrumentScale(1);

		return EffectResult::Success;
	}

	/// <summary>
	/// Set the scale of the instrument.
	/// </summary>
	/// <param name="scale"> - Scale to set the instrument to.</param>
	void RemoveInstrumentEffect::SetInstrumentScale(float scale) {
		_LOG_INIT;

		_LOG("RemoveInstrumentEffect::SetInstrumentScale(" << scale << ")" << std::endl);

		ObjectUtil::SetObjectScales({
			{"ActorGuitarStringStandard", scale},
			{"GRGuitarInlay", scale},
			{"MeshGuitarCapo", scale},
			{"MeshGuitarFrameBass2x2", scale},
			{"MeshGuitarFrameBass2x2Flip", scale},
			{"MeshGuitarFrameBass2x2LH", scale},
			{"MeshGuitarFrameBass2x2LHFlip", scale},
			{"MeshGuitarFrameBassInlinedLH", scale},
			{"MeshGuitarFrameBassInlinedLHFlip", scale},
			{"MeshGuitarFrameBassInlinedRH", scale},
			{"MeshGuitarFrameBassInlinedRHFlip", scale},
			{"MeshGuitarFrameStandardInlinedRH", scale},
			{"MeshGuitarFrameStandardInlinedRHFlip", scale},
			{"MeshGuitarFrameStandardInlinedLH", scale},
			{"MeshGuitarFrameStandardInlinedLHFlip", scale},
			{"MeshGuitarFrameStandard3x3", scale},
			{"MeshGuitarFrameStandard3x3Flip", scale},
			{"MeshGuitarFrameStandard3x3LH", scale},
			{"MeshGuitarFrameStandard3x3LHFlip", scale},
			{"MeshGuitarFretBarNormal", scale},
			{"MeshGuitarPegBass2x2", scale},
			{"MeshGuitarPegBassInlinedLH", scale},
			{"MeshGuitarPegBassInlinedRH", scale},
			{"MeshGuitarPegStandard3x3", scale},
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