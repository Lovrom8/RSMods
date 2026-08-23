#pragma once

namespace CrowdControl::Effects {
	class ChangeToToneSlot : public CCEffect
	{
	public:
		int slot;

		explicit ChangeToToneSlot(int _slot) : slot(_slot) {}

		std::vector<std::string> ClaimsExclusive() const override
		{
			return { "tone-slot" };
		}

		Enums::EffectStatus Test(const Structs::Request& request) override;

	protected:
		Enums::EffectStatus OnStart(const Structs::Request& request) override;
		Enums::EffectStatus OnStop() override;
	};
}
