#include "HalfSpeedEffect.hpp"

using namespace CrowdControl::Enums;

namespace CrowdControl::Effects {
	EffectResult HalfSpeedEffect::Test(Request request)
	{
		std::cout << "HalfSpeedEffect::Test()" << std::endl;

		return EffectResult::Success;
	}

	EffectResult HalfSpeedEffect::Start(Request request)
	{
		std::cout << "HalfSpeedEffect::Start()" << std::endl;

		if (!MemHelpers::IsInSong())
			return EffectResult::Retry;

		DWORD oldProtect;
		VirtualProtect((LPVOID)Offsets::ptr_scrollSpeedMultiplier, 8, PAGE_READWRITE, &oldProtect);
		Offsets::ref_scrollSpeedMultiplier = 2.5;
		VirtualProtect((LPVOID)Offsets::ptr_scrollSpeedMultiplier, 8, oldProtect, &oldProtect);

		running = true;
		endTime = std::chrono::steady_clock::now() + std::chrono::seconds(duration);

		return EffectResult::Success;
	}

	void HalfSpeedEffect::Run()
	{
		// Stop automatically after duration has elapsed
		if (running) {
			auto now = std::chrono::steady_clock::now();
			std::chrono::duration<double> duration = (endTime - now);

			if (duration.count() <= 0) Stop();
		}
	}

	EffectResult HalfSpeedEffect::Stop()
	{
		std::cout << "HalfSpeedEffect::Stop()" << std::endl;

		DWORD oldProtect;
		VirtualProtect((LPVOID)Offsets::ptr_scrollSpeedMultiplier, 8, PAGE_READWRITE, &oldProtect);
		Offsets::ref_scrollSpeedMultiplier = 5.0;
		VirtualProtect((LPVOID)Offsets::ptr_scrollSpeedMultiplier, 8, oldProtect, &oldProtect);

		running = false;

		return EffectResult::Success;
	}
}