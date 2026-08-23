#pragma once

namespace CrowdControl::Effects {
	class RemoveInstrumentEffect : public CCEffect
	{
	public:
		explicit RemoveInstrumentEffect(int64_t durationMilliseconds) {
			duration_ms = durationMilliseconds;
		}

		// Claims both instrument and string-colors: hiding the instrument conflicts with
		// effects that recolour strings (invertedstrings, rainbowstrings).
		std::vector<std::string> ClaimsExclusive() const override
		{
			return { "instrument", "string-colors" };
		}

		Enums::EffectStatus Test(const Structs::Request& request) override;

	protected:
		Enums::EffectStatus OnStart(const Structs::Request& request) override;
		Enums::EffectStatus OnStop() override;

	private:
		static void SetInstrumentScale(float scale);
	};
}
