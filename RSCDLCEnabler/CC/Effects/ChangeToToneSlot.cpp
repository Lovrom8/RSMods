#include "ChangeToToneSlot.hpp"

using namespace CrowdControl::Enums;

namespace CrowdControl::Effects {
	EffectResult ChangeToToneSlot::Test(Request request)
	{
		std::cout << "ChangeToToneSlot::Test()" << std::endl;

		if (!MemHelpers::IsInSong())
			return EffectResult::Retry;

		return EffectResult::Success;
	}

	EffectResult ChangeToToneSlot::Start(Request request)
	{
		std::cout << "ChangeToToneSlot::Start()" << std::endl;

		if (!MemHelpers::IsInSong())
			return EffectResult::Retry;

		Util::SendKey(Settings::GetVKCodeForString(std::to_string(ChangeToToneSlot::slot)));
	
		std::cout << "Changing tone to slot " << slot << std::endl;

		return EffectResult::Success;
	}

	void ChangeToToneSlot::Run() { }
	EffectResult ChangeToToneSlot::Stop() { return EffectResult::Success; }
}