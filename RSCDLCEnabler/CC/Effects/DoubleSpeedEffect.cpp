#include "DoubleSpeedEffect.hpp"

using namespace CrowdControl::Enums;

namespace CrowdControl::Effects {
	EffectResult DoubleSpeedEffect::Test(Request request)
	{
		std::cout << "DoubleSpeedEffect::Test()" << std::endl;

		if (!MemHelpers::IsInSong() || EffectList::AreIncompatibleEffectsEnabled(incompatibleEffects) || running)
			return EffectResult::Retry;

		return EffectResult::Success;
	}

	/// <summary>
	/// The scroll speed has special memory PAGE flags sets, which prevent it from being written over, so first they need to be set to PAGE_READWRITE
	/// Afterwards, the speed is set to double the original speed
	/// </summary>
	/// <returns> EffectResult::Retry if we aren't currently in a song or incompatible effects are running, or EffectResult::Sucess if we are</returns>
	EffectResult DoubleSpeedEffect::Start(Request request)
	{
		std::cout << "DoubleSpeedEffect::Start()" << std::endl;

		if (!MemHelpers::IsInSong() || EffectList::AreIncompatibleEffectsEnabled(incompatibleEffects) || running)
			return EffectResult::Retry;

		running = true;

		DWORD oldProtect;
		VirtualProtect((LPVOID)Offsets::ptr_scrollSpeedMultiplier, 8, PAGE_READWRITE, &oldProtect);
		Offsets::ref_scrollSpeedMultiplier = 10.0;
		VirtualProtect((LPVOID)Offsets::ptr_scrollSpeedMultiplier, 8, oldProtect, &oldProtect);

		SetDuration(request);
		endTime = std::chrono::steady_clock::now() + std::chrono::seconds(duration);

		return EffectResult::Success;
	}

	void DoubleSpeedEffect::Run() 
	{
		// Stop automatically after duration has elapsed
		if (running) {
			auto now = std::chrono::steady_clock::now();
			std::chrono::duration<double> duration = (endTime - now);

			if (duration.count() <= 0) Stop();
		}
	}

	/// <summary>
	/// Reapply the original PAGE flags and set the original speed
	/// </summary>
	/// <returns>EffectResult::Sucess</returns>
	EffectResult DoubleSpeedEffect::Stop()
	{ 
		std::cout << "DoubleSpeedEffect::Stop()" << std::endl;

		DWORD oldProtect;
		VirtualProtect((LPVOID)Offsets::ptr_scrollSpeedMultiplier, 8, PAGE_READWRITE, &oldProtect);
		Offsets::ref_scrollSpeedMultiplier = 5.0;
		VirtualProtect((LPVOID)Offsets::ptr_scrollSpeedMultiplier, 8, oldProtect, &oldProtect);

		running = false;

		return EffectResult::Success; 
	}
}