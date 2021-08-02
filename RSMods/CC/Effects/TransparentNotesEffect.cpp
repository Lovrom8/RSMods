#include "TransparentNotesEffect.hpp"

using namespace CrowdControl::Enums;

namespace CrowdControl::Effects { // Changes textures for noteheads to a nonexistent (effectively transparent) texture
	EffectResult TransparentNotesEffect::Test(Request request)
	{
		std::cout << "TransparentNotesEffect::Test()" << std::endl;

		if (!MemHelpers::IsInSong() || EffectList::AreIncompatibleEffectsEnabled(incompatibleEffects) || running)
			return EffectResult::Retry;

		return EffectResult::Success;
	}

	EffectResult TransparentNotesEffect::Start(Request request)
	{
		std::cout << "TransparentNotesEffect::Start()" << std::endl;

		if (!MemHelpers::IsInSong() || EffectList::AreIncompatibleEffectsEnabled(incompatibleEffects) || running)
			return EffectResult::Retry;

		running = true;

		Settings::UpdateTwitchSetting("TransparentNotes", "on");
		
		SetDuration(request);
		endTime = std::chrono::steady_clock::now() + std::chrono::seconds(duration);

		return EffectResult::Success;
	}

	void TransparentNotesEffect::Run()
	{
		// Stop automatically after duration has elapsed
		if (running) {
			auto now = std::chrono::steady_clock::now();
			std::chrono::duration<double> duration = (endTime - now);

			if (duration.count() <= 0) Stop();
		}
	}

	EffectResult TransparentNotesEffect::Stop()
	{
		std::cout << "TransparentNotesEffect::Stop()" << std::endl;

		running = false;
		Settings::UpdateTwitchSetting("TransparentNotes", "off");

		return EffectResult::Success;
	}
}