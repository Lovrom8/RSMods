#include "ShuffleTonesEffect.hpp"

namespace CrowdControl::Effects { // Scales notes in a song to unusually small size TODO actually implement
	
	/// <summary>
	/// Test the twitch mod's requirements.
	/// </summary>
	/// <param name="request"> - JSON Request</param>
	/// <returns>EffectResult::Success if test completed without any issues. EffectResult::Retry if we have to retry.</returns>
	EffectResult ShuffleTonesEffect::Test(Request request)
	{
		_LOG_INIT;

		_LOG_HEAD << "ShuffleTonesEffect::Test()" << LOG.endl();

		if (!MemHelpers::IsInSong() || running)
			return EffectResult::Retry;

		return EffectResult::Success;
	}

	/// <summary>
	/// Shuffle the tone used by the player
	/// </summary>
	/// <param name="request"> - JSON Request</param>
	/// <returns>EffectResult::Success if test completed without any issues. EffectResult::Retry if we have to retry.</returns>
	EffectResult ShuffleTonesEffect::Start(Request request)
	{
		_LOG_INIT;

		_LOG_HEAD << "ShuffleTonesEffect::Start()" << LOG.endl();

		if (!MemHelpers::IsInSong() || running)
			return EffectResult::Retry;

		running = true;
		SetDuration(request);
		endTime = std::chrono::steady_clock::now() + std::chrono::seconds(duration);

		return EffectResult::Success;
	}

	/// <summary>
	/// Change the tone to a random (int) value 1-4
	/// </summary>
	void SwitchToneRand() {
		static std::random_device rd;
		static std::mt19937 rng(rd());

		std::uniform_int_distribution<> distrib(1, 4);

		Util::SendKey(Settings::GetVKCodeForString(std::to_string(distrib(rng))));
	}

	/// <summary>
	/// Ensure that the mod only lasts for the time specified in the JSON request.
	/// Also changes the tone every tick.
	/// </summary>
	void ShuffleTonesEffect::Run()
	{
		if (running) {

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

	/// <summary>
	/// Stops the mod by setting the tone back to the tone of the song.
	/// </summary>
	/// <returns>EffectResult::Success</returns>
	EffectResult ShuffleTonesEffect::Stop()
	{
		_LOG_INIT;

		_LOG_HEAD << "ShuffleTonesEffect::Stop()" << LOG.endl();

		running = false;

		// Return to tone slot 1 at the end
		Util::SendKey(Settings::GetVKCodeForString("1"));

		return EffectResult::Success;
	}
}
