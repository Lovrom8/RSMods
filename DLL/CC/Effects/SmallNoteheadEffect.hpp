#pragma once

namespace CrowdControl::Effects {
	class SmallNoteheadEffect : public CCEffect
	{
	public:
		explicit SmallNoteheadEffect(int64_t durationMilliseconds) {
			duration_ms = durationMilliseconds;
		}

		std::vector<std::string> ClaimsExclusive() const override
		{
			return { "note-visuals" };
		}

		Enums::EffectStatus Test(const Structs::Request& request) override;

	protected:
		Enums::EffectStatus OnStart(const Structs::Request& request) override;
		Enums::EffectStatus OnStop() override;

	private:
		static void SetNoteHeadScale(float scale);
	};
}
