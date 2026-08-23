#pragma once

namespace CrowdControl::Effects {
	class ShuffleTonesEffect : public CCEffect
	{
	public:
		explicit ShuffleTonesEffect(int64_t durationMilliseconds) {
			duration_ms = durationMilliseconds;
		}

		uint32_t tickIntervalMilliseconds = 2000;
		std::chrono::steady_clock::time_point nextTickTime;

		std::vector<std::string> ClaimsExclusive() const override
		{
			return { "tone-slot" };
		}

		Enums::EffectStatus Test(const Structs::Request& request) override;
		void Run() override;

	protected:
		Enums::EffectStatus OnStart(const Structs::Request& request) override;
		Enums::EffectStatus OnStop() override;
	};
}
