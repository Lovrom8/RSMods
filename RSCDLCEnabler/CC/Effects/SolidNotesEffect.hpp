#pragma once
#include "../CCEffect.hpp"
#include "../../Mods/ExtendedRangeMode.hpp"
#include "../CCEffectList.hpp"

namespace CrowdControl::Effects {
	class SolidNotesRandomEffect : public CCEffect
	{
	public:
		SolidNotesRandomEffect(unsigned int durationSeconds) {
			duration = durationSeconds;
		}

		EffectResult Test(Request request);
		EffectResult Start(Request request);
		void Run();
		EffectResult Stop();

	private:
		std::vector<std::string> incompatibleEffects =
		{ "removenotes", "transparentnotes", "solidcustom", "solidcustomrgb", "rainbownotes" };
	};

	class SolidNotesCustomEffect : public CCEffect
	{
	public:
		SolidNotesCustomEffect(unsigned int durationSeconds) {
			duration = durationSeconds;
		}

		EffectResult Test(Request request);
		EffectResult Start(Request request);
		void Run();
		EffectResult Stop();

	private:
		std::vector<std::string> incompatibleEffects =
		{ "removenotes", "transparentnotes", "solidrandom", "solidcustomrgb", "rainbownotes" };
	};

	class SolidNotesCustomRGBEffect : public CCEffect
	{
	public:
		SolidNotesCustomRGBEffect(unsigned int durationSeconds) {
			duration = durationSeconds;
		}

		EffectResult Test(Request request);
		EffectResult Start(Request request);
		void Run();
		EffectResult Stop();

	private:
		std::vector<std::string> incompatibleEffects =
		{ "removenotes", "transparentnotes", "solidcustom", "solidrandom", "rainbownotes" };
	};
}
