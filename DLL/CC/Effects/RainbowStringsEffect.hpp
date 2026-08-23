#pragma once
#include "../../Mods/ExtendedRangeMode.hpp"

namespace CrowdControl::Effects {
	class RainbowStringsEffect : public CCEffect
	{
	public:
		explicit RainbowStringsEffect(int64_t durationMilliseconds) {
			duration_ms = durationMilliseconds;
		}

		bool CanStart() override
		{
			return !ERMode::IsRainbowEnabled() && CCEffect::CanStart();
		}

		std::vector<std::string> ClaimsExclusive() const override
		{
			return { "string-colors" };
		}

		Enums::EffectStatus Test(const Structs::Request& request) override;

	protected:
		Enums::EffectStatus OnStart(const Structs::Request& request) override;
		Enums::EffectStatus OnStop() override;
	};
}
