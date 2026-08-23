#pragma once
#include "../../Mods/ExtendedRangeMode.hpp"

namespace CrowdControl::Effects {
	class RainbowNotesEffect : public CCEffect
	{
	public:
		explicit RainbowNotesEffect(int64_t durationMilliseconds) {
			duration_ms = durationMilliseconds;
		}

		bool CanStart() override
		{
			return !ERMode::IsRainbowNotesEnabled() && CCEffect::CanStart();
		}

		std::vector<std::string> ClaimsExclusive() const override
		{
			return { "note-visuals" };
		}

		Enums::EffectStatus Test(const Structs::Request& request) override;

	protected:
		Enums::EffectStatus OnStart(const Structs::Request& request) override;
		Enums::EffectStatus OnStop() override;
	};
}
