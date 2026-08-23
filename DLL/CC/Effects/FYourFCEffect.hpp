#pragma once

namespace CrowdControl::Effects {
	class FYourFCEffect : public CCEffect
	{
	public:
		FYourFCEffect() = default;

		Enums::EffectStatus Test(const Structs::Request& request) override;

	protected:
		Enums::EffectStatus OnStart(const Structs::Request& request) override;
		Enums::EffectStatus OnStop() override;
	};
}
