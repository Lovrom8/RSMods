#include "RemoveInstrumentEffect.hpp"

namespace CrowdControl::Effects { 

	/// <summary>
	/// Test the twitch mod's requirements.
	/// </summary>
	/// <param name="request"> - JSON Request</param>
	/// <returns>EffectStatus::Success if test completed without any issues. EffectStatus::Retry if we have to retry.</returns>
	EffectStatus RemoveInstrumentEffect::Test(Request request)
	{
		_LOG_INIT;

		_LOG("RemoveInstrumentEffect::Test()" << std::endl);

		if (!CanStart(&EffectList::AllEffects))
			return EffectStatus::Retry;

		return EffectStatus::Success;
	}

	/// <summary>
	/// Sets the scale of each instrument related object (headstock, inlays, fretbars, ...) to 0, hence making it temporarily invisible 
	/// </summary>
	/// <returns> EffectStatus::Retry if we aren't currently in a song or the same effect is running already, or EffectStatus::Success if we are in a song</returns>
	EffectStatus RemoveInstrumentEffect::Start(Request request)
	{
		_LOG_INIT;

		_LOG("RemoveInstrumentEffect::Start()" << std::endl);

		if (!CanStart(&EffectList::AllEffects))
			return EffectStatus::Retry;

		SetInstrumentScale(0);

		SetDuration(request);
		running = true;

		return EffectStatus::Success;
	}

	/// <summary>
	/// Stops the mod.
	/// </summary>
	/// <returns>EffectStatus::Success</returns>
	EffectStatus RemoveInstrumentEffect::Stop()
	{
		_LOG_INIT;

		_LOG("RemoveInstrumentEffect::Stop()" << std::endl);

		SetInstrumentScale(1);
		running = false;

		return EffectStatus::Success;
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