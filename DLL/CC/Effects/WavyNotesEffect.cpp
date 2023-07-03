#include "WavyNotesEffect.hpp"

using namespace CrowdControl::Enums;

namespace CrowdControl::Effects {
	EffectStatus WavyNotesEffect::Test(Request request)
	{
		_LOG_INIT;

		_LOG("WavyNotesEffect::Test()" << std::endl);

		if (!CanStart(&EffectList::AllEffects))
			return EffectStatus::Retry;

		return EffectStatus::Success;
	}


	EffectStatus WavyNotesEffect::Start(Request request)
	{
		_LOG_INIT;

		_LOG("WavyNotesEffect::Start()" << std::endl);

		if (!CanStart(&EffectList::AllEffects))
			return EffectStatus::Retry;
		
		SetDuration(request);
		running = true;
		wavy_notes_enabled = true;

		return EffectStatus::Success;
	}

	EffectStatus WavyNotesEffect::Stop()
	{
		_LOG_INIT;

		_LOG("WavyNotesEffect::Stop()" << std::endl);

		wavy_notes_enabled = false;
		running = false;

		return EffectStatus::Success;
	}
}