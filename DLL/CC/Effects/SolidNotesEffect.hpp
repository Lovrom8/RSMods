#pragma once
#include "../../Mods/ExtendedRangeMode.hpp"

namespace CrowdControl::Effects {
	// Changes current note heads to a custom generated texture
	class SolidNotesRandomEffect : public CCEffect
	{
	public:
		explicit SolidNotesRandomEffect(int64_t durationMilliseconds) {
			duration_ms = durationMilliseconds;
		}

		bool CanStart() override
		{
			return ERMode::ColorsSaved && CCEffect::CanStart();
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

	class SolidNotesCustomEffect : public CCEffect
	{
	public:
		explicit SolidNotesCustomEffect(int64_t durationMilliseconds) {
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
	};

	class SolidNotesCustomRGBEffect : public CCEffect
	{
	public:
		explicit SolidNotesCustomRGBEffect(int64_t durationMilliseconds) {
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
	};
}
