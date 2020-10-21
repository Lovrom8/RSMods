#include "TurboSpeedEffect.hpp"

using namespace CrowdControl::Enums;

namespace CrowdControl::Effects {
	EffectResult TurboSpeedEffect::Test(Request request)
	{
		std::cout << "TurboSpeedEffect::Test()" << std::endl;

		if (!MemHelpers::IsInSong() || running)
			return EffectResult::Retry;

		return EffectResult::Success;
	}

	EffectResult TurboSpeedEffect::Start(Request request)
	{
		std::cout << "TurboSpeedEffect::Start()" << std::endl;

		if (!MemHelpers::IsInSong() || running)
			return EffectResult::Retry;

		running = true;

		// TODO: finish

		SetDuration(request);
		endTime = std::chrono::steady_clock::now() + std::chrono::seconds(duration);

		return EffectResult::Success;
	}

	void TurboSpeedEffect::Run()
	{
		// Stop automatically after duration has elapsed
		if (running) {
			auto now = std::chrono::steady_clock::now();
			std::chrono::duration<double> duration = (endTime - now);

			if (duration.count() <= 0) Stop();
		}
	}

	EffectResult TurboSpeedEffect::Stop()
	{
		std::cout << "TurboSpeedEffect::Stop()" << std::endl;

		// TODO: finish

		running = false;

		return EffectResult::Success;
	}
}