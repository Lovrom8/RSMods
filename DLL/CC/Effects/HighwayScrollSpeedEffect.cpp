#include "HighwayScrollSpeedEffect.hpp"

using namespace CrowdControl::Enums;

namespace CrowdControl::Effects {
	
	/// <summary>
	/// Test the twitch mod's requirements.
	/// </summary>
	/// <param name="request"> - JSON Request</param>
	/// <returns>EffectResult::Success if test completed without any issues. EffectResult::Retry if we have to retry.</returns>
	EffectResult HighwayScrollSpeedEffect::Test(Request request)
	{
		std::cout << "HighwayScrollSpeedEffect::Test()" << std::endl;

		if (!MemHelpers::IsInSong() || EffectList::AreIncompatibleEffectsEnabled(incompatibleEffects) || running)
			return EffectResult::Retry;

		return EffectResult::Success;
	}

	/// <summary>
	/// Change scroll speed multiplier to let the user have more, or less, time to react to the notes appearing on screen.
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

	/// <summary>
	/// Ensure that the mod only lasts for the time specified in the JSON request.
	/// </summary>
	void HighwayScrollSpeedEffect::Run()
	{
		if (running) {
			auto now = std::chrono::steady_clock::now();
			std::chrono::duration<double> duration = (endTime - now);

			if (duration.count() <= 0) Stop();
		}
	}

	/// <summary>
	/// Stops the mod.
	/// </summary>
	/// <returns>EffectResult::Success</returns>
	EffectResult HighwayScrollSpeedEffect::Stop()
	{
		std::cout << "HighwayScrollSpeedEffect::Stop()" << std::endl;

		WriteScrollSpeedMultiplier(5.0);

		running = false;

		return EffectResult::Success;
	}

	/// <summary>
	/// Changes the Scroll Speed Multiplier to the parameter.
	/// </summary>
	/// <param name="val"> - New Scroll Speed Modifier</param>
	void HighwayScrollSpeedEffect::WriteScrollSpeedMultiplier(double val) {
		std::cout << "HighwayScrollSpeedEffect::WriteScrollSpeedMultiplier(" << val << ")" << std::endl;

		// The scroll speed has special memory PAGE flags sets, which prevent it from being written over, so first they need to be set to PAGE_READWRITE
		DWORD oldProtect;
		VirtualProtect((LPVOID)Offsets::ptr_scrollSpeedMultiplier, 8, PAGE_READWRITE, &oldProtect);
		Offsets::ref_scrollSpeedMultiplier = val;
		VirtualProtect((LPVOID)Offsets::ptr_scrollSpeedMultiplier, 8, oldProtect, &oldProtect);
	}
}