#pragma once

namespace CrowdControl::Effects {
	class HighwayScrollSpeedEffect : public CCEffect
	{
	public:
		double multiplier = 5.0;

		explicit HighwayScrollSpeedEffect(int64_t durationMilliseconds, double speedMultiplier) : multiplier(speedMultiplier)
		{
			duration_ms = durationMilliseconds;
		}

		std::vector<std::string> ClaimsExclusive() const override
		{
			return { "scroll-speed" };
		}

		Enums::EffectStatus Test(const Structs::Request& request);

	protected:
		Enums::EffectStatus OnStart(const Structs::Request& request);
		Enums::EffectStatus OnStop();

	private:
		static void WriteScrollSpeedMultiplier(double val);
	};
}
