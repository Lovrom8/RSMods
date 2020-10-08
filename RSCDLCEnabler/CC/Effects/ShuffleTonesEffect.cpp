#include "ShuffleTonesEffect.hpp"

namespace CrowdControl::Effects { // Scales notes in a song to unusually small size TODO actually implement
	EffectResult ShuffleTonesEffect::Test(Request request)
	{
		std::cout << "ShuffleTonesEffect::Test()" << std::endl;

		if (!MemHelpers::IsInSong() || running)
			return EffectResult::Retry;

		return EffectResult::Success;
	}

	EffectResult ShuffleTonesEffect::Start(Request request)
	{
		std::cout << "ShuffleTonesEffect::Start()" << std::endl;

		if (!MemHelpers::IsInSong() || running)
			return EffectResult::Retry;

		running = true;
		endTime = std::chrono::steady_clock::now() + std::chrono::seconds(duration);

		return EffectResult::Success;
	}

	void SwitchToneRand() {
		static std::random_device rd;
		static std::mt19937 rng(rd());

		std::uniform_int_distribution<> distrib(1, 4);

		Util::SendKey(Settings::GetVKCodeForString(std::to_string(distrib(rng))));
	}

	void ShuffleTonesEffect::Run()
	{
		// Stop automatically after duration has elapsed
		if (running) {
			std::cout << "ShuffleTonesEffect::Run()" << std::endl;

			auto now = std::chrono::steady_clock::now();
			std::chrono::duration<double> duration = (nextTickTime - now);

			if (duration.count() <= 0) {
				nextTickTime = now + std::chrono::milliseconds(tickIntervalMilliseconds);

				SwitchToneRand();
			}

			duration = (endTime - now);

			if (duration.count() <= 0) Stop();
		}
	}

	EffectResult ShuffleTonesEffect::Stop()
	{
		std::cout << "ShuffleTonesEffect::Stop()" << std::endl;

		running = false;

		return EffectResult::Success;
	}
}
