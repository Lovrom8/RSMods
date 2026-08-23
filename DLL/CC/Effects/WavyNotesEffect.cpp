#include "../../stdafx.h"
#include "WavyNotesEffect.hpp"

using namespace CrowdControl::Enums;

namespace CrowdControl::Effects {
	Enums::EffectStatus WavyNotesEffect::Test(const Structs::Request& request)
	{
		LOG_INFO("WavyNotesEffect::Test()" << std::endl);

		if (!CanStart())
			return Enums::EffectStatus::Retry;

		return Enums::EffectStatus::Success;
	}


	Enums::EffectStatus WavyNotesEffect::OnStart(const Structs::Request& request)
	{
		LOG_INFO("WavyNotesEffect::Start()" << std::endl);

		SetDuration(request);
		running = true;
		wavyNotesEnabled = true;

		return Enums::EffectStatus::Success;
	}

	Enums::EffectStatus WavyNotesEffect::OnStop()
	{
		LOG_INFO("WavyNotesEffect::Stop()" << std::endl);

		wavyNotesEnabled = false;
		running = false;

		return Enums::EffectStatus::Success;
	}
}