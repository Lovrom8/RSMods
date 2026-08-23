#pragma once

namespace CrowdControl::Effects {
	class InvertedStringsEffect : public CCEffect
	{
	public:
		explicit InvertedStringsEffect(int64_t durationMilliseconds) {
			duration_ms = durationMilliseconds;
		}

		std::vector<std::string> ClaimsExclusive() const override
		{
			return { "string-colors" };
		}

		Enums::EffectStatus Test(const Structs::Request& request);

	protected:
		Enums::EffectStatus OnStart(const Structs::Request& request);
		Enums::EffectStatus OnStop();
	};
}
