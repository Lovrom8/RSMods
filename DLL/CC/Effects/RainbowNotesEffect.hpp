#pragma once
#include "../CCEffect.hpp"
#include "../CCEffectList.hpp"
#include "../../Mods/ExtendedRangeMode.hpp"

namespace CrowdControl::Effects {
	class RainbowNotesEffect : public CCEffect
	{
	public:
		RainbowNotesEffect(int64_t durationMilliseconds) {
			duration_ms = durationMilliseconds;

			incompatibleEffects = { "removenotes", "transparentnotes", "solidrandom",  "solidcustom", "solidcustomrgb" };
		}
		
		/**
		 * \brief Can this effect start? By default checks that a song is being played, no incompatible effects are running, and this effect is not running
		 * \return True when this effect can start, false otherwise
		 */
		bool CanStart(std::map<std::string, CCEffect*>* AllEffects) override
		{
			return MemHelpers::IsInSong() && !ERMode::IsRainbowNotesEnabled() && !AreIncompatibleEffectsRunning(AllEffects) && !running;
		}

		EffectStatus Test(Request request) override;
		EffectStatus Start(Request request) override;
		EffectStatus Stop() override;
	};
}
