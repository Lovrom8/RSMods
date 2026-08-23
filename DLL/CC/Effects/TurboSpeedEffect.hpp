#pragma once
#include "../../Mods/RiffRepeater.hpp"

namespace CrowdControl::Effects {
	class TurboSpeedEffect : public CCEffect
	{
	public:
		explicit TurboSpeedEffect(int64_t durationMilliseconds) {
			duration_ms = durationMilliseconds;
		}

		std::vector<std::string> ClaimsExclusive() const override
		{
			return { "song-speed" };
		}

		Enums::EffectStatus Test(const Structs::Request& request) override;

	protected:
		Enums::EffectStatus OnStart(const Structs::Request& request) override;
		Enums::EffectStatus OnStop() override;
	};
}
