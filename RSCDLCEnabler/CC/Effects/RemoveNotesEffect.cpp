#include "RemoveNotesEffect.hpp"

using namespace CrowdControl::Enums;

namespace CrowdControl::Effects { // Prevents the game from drawing note head meshes
	EffectResult RemoveNotesEffect::Test(Request request)
	{
		std::cout << "RemoveNotesEffect::Test()" << std::endl;

		return EffectResult::Success;
	}

	EffectResult RemoveNotesEffect::Start(Request request)
	{
		std::cout << "RemoveNotesEffect::Start()" << std::endl;

		running = true;
		Settings::UpdateTwitchSetting("RemoveNotes", "on");

		endTime = std::chrono::steady_clock::now() + std::chrono::seconds(duration);

		return EffectResult::Success;
	}

	void RemoveNotesEffect::Run()
	{
		// Stop automatically after duration has elapsed
		if (running) {
			auto now = std::chrono::steady_clock::now();
			std::chrono::duration<double> duration = (endTime - now);

			if (duration.count() <= 0) Stop();
		}
	}

	EffectResult RemoveNotesEffect::Stop()
	{
		std::cout << "RemoveNotesEffect::Stop()" << std::endl;

		running = false;
		Settings::UpdateTwitchSetting("RemoveNotes", "off");

		return EffectResult::Success;
	}
}