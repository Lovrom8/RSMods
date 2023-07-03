#pragma once
#include "../CCEffect.hpp"
#include "../../Mods/ExtendedRangeMode.hpp"
#include "../CCEffectList.hpp"

namespace CrowdControl::Effects {
	// Changes current note heads to a custom generated texture
	class SolidNotesRandomEffect : public CCEffect
	{
	public:
		SolidNotesRandomEffect(int64_t durationMilliseconds) {
			duration_ms = durationMilliseconds;

			incompatibleEffects = { "removenotes", "transparentnotes", "solidcustom", "solidcustomrgb", "rainbownotes" };
		}

		EffectStatus Test(Request request) override;
		EffectStatus Start(Request request) override;
		EffectStatus Stop() override;

		/**
		 * \brief Can this effect start? By default checks that a song is being played, no incompatible effects are running, and this effect is not running
		 * \return True when this effect can start, false otherwise
		 */
		bool CanStart(std::map<std::string, CCEffect*>* AllEffects) override
		{
			return ERMode::ColorsSaved && MemHelpers::IsInSong() && !AreIncompatibleEffectsRunning(AllEffects) && !running;
		}

	};

	class SolidNotesCustomEffect : public CCEffect
	{
	public:
		SolidNotesCustomEffect(int64_t durationMilliseconds) {
			duration_ms = durationMilliseconds;

			incompatibleEffects = { "removenotes", "transparentnotes", "solidrandom", "solidcustomrgb", "rainbownotes" };
		}

		EffectStatus Test(Request request) override;
		EffectStatus Start(Request request) override;
		EffectStatus Stop() override;
	};

	class SolidNotesCustomRGBEffect : public CCEffect
	{
	public:
		SolidNotesCustomRGBEffect(int64_t durationMilliseconds) {
			duration_ms = durationMilliseconds;

			incompatibleEffects = { "removenotes", "transparentnotes", "solidcustom", "solidrandom", "rainbownotes" };
		}

		EffectStatus Test(Request request) override;
		EffectStatus Start(Request request) override;
		EffectStatus Stop() override;
	};
}
