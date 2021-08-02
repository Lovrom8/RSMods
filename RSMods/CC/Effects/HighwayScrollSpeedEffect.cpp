#include "HighwayScrollSpeedEffect.hpp"

using namespace CrowdControl::Enums;

namespace CrowdControl::Effects {
	EffectResult HighwayScrollSpeedEffect::Test(Request request)
	{
		std::cout << "HighwayScrollSpeedEffect::Test()" << std::endl;

		if (!MemHelpers::IsInSong() || EffectList::AreIncompatibleEffectsEnabled(incompatibleEffects) || running)
			return EffectResult::Retry;

		return EffectResult::Success;
	}

	/// <summary>
	///
	/// </summary>
	/// <returns> EffectResult::Retry if we aren't currently in a song or incompatible effects are running, or EffectResult::Sucess if we are</returns>
	EffectResult HighwayScrollSpeedEffect::Start(Request request)
	{
		std::cout << "HighwayScrollSpeedEffect::Start()" << std::endl;

		if (!MemHelpers::IsInSong() || EffectList::AreIncompatibleEffectsEnabled(incompatibleEffects) || running)
			return EffectResult::Retry;

		running = true;

		WriteScrollSpeedMultiplier(multiplier);

		SetDuration(request);
		endTime = std::chrono::steady_clock::now() + std::chrono::seconds(duration);

		return EffectResult::Success;
	}

	void HighwayScrollSpeedEffect::Run()
	{
		// Stop automatically after duration has elapsed
		if (running) {
			auto now = std::chrono::steady_clock::now();
			std::chrono::duration<double> duration = (endTime - now);

			if (duration.count() <= 0) Stop();
		}
	}

	EffectResult HighwayScrollSpeedEffect::Stop()
	{
		std::cout << "HighwayScrollSpeedEffect::Stop()" << std::endl;

		WriteScrollSpeedMultiplier(5.0);

		running = false;

		return EffectResult::Success;
	}

	void HighwayScrollSpeedEffect::WriteScrollSpeedMultiplier(double val) {
		std::cout << "HighwayScrollSpeedEffect::WriteScrollSpeedMultiplier(" << val << ")" << std::endl;

		// The scroll speed has special memory PAGE flags sets, which prevent it from being written over, so first they need to be set to PAGE_READWRITE
		DWORD oldProtect;
		VirtualProtect((LPVOID)Offsets::ptr_scrollSpeedMultiplier, 8, PAGE_READWRITE, &oldProtect);
		Offsets::ref_scrollSpeedMultiplier = val;
		VirtualProtect((LPVOID)Offsets::ptr_scrollSpeedMultiplier, 8, oldProtect, &oldProtect);
	}
}