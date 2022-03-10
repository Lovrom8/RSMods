#include "TurboSpeedEffect.hpp"

using namespace CrowdControl::Enums;

namespace CrowdControl::Effects {

	/// <summary>
	/// Test the twitch mod's requirements.
	/// </summary>
	/// <param name="request"> - JSON Request</param>
	/// <returns>EffectResult::Success if test completed without any issues. EffectResult::Retry if we have to retry.</returns>
	EffectResult TurboSpeedEffect::Test(Request request)
	{
		_LOG_INIT;

		_LOG_HEAD << "TurboSpeedEffect::Test()" << LOG.endl();

		if (!MemHelpers::IsInSong() || running)
			return EffectResult::Retry;

		return EffectResult::Success;
	}

	/// <summary>
	/// Set Riff Repeater speed to 200%.
	/// </summary>
	/// <param name="request"> - JSON Request</param>
	/// <returns>EffectResult::Success if test completed without any issues. EffectResult::Retry if we have to retry.</returns>
	EffectResult TurboSpeedEffect::Start(Request request)
	{
		_LOG_INIT;

		_LOG_HEAD << "TurboSpeedEffect::Start()" << LOG.endl();

		if (!MemHelpers::IsInSong() || running)
			return EffectResult::Retry;

		running = true;

		RiffRepeater::SetSpeed(200.f, true);
		RiffRepeater::EnableTimeStretch();

		SetDuration(request);
		endTime = std::chrono::steady_clock::now() + std::chrono::seconds(duration);

		return EffectResult::Success;
	}

	/// <summary>
	/// Ensure that the mod only lasts for the time specified in the JSON request.
	/// </summary>
	void TurboSpeedEffect::Run()
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
	EffectResult TurboSpeedEffect::Stop()
	{
		_LOG_INIT;

		_LOG_HEAD << "TurboSpeedEffect::Stop()" << LOG.endl();

		RiffRepeater::SetSpeed(100.f);
		RiffRepeater::DisableTimeStretch();

		running = false;

		return EffectResult::Success;
	}
}