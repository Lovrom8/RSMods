#pragma once

namespace CrowdControl::Effects {
	class ZoomEffect : public CCEffect
	{
	public:
		float factor = 1.0;

		explicit ZoomEffect(int64_t durationMilliseconds, float zoomFactor) : factor(zoomFactor)
		{
			duration_ms = durationMilliseconds;
		}

		std::vector<std::string> ClaimsExclusive() const override
		{
			return { "zoom" };
		}

		Enums::EffectStatus Test(const Structs::Request& request) override;

	protected:
		Enums::EffectStatus OnStart(const Structs::Request& request) override;
		Enums::EffectStatus OnStop() override;
	};
}
