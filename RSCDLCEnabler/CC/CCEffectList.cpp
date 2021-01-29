#include "CCEffectList.hpp"
#include "Effects/RainbowStringsEffect.hpp"
#include "Effects/RainbowNotesEffect.hpp"
#include "Effects/DrunkModeEffect.hpp"
#include "Effects/FYourFCEffect.hpp"
#include "Effects/SolidNotesEffect.hpp"
#include "Effects/TransparentNotesEffect.hpp"
#include "Effects/RemoveNotesEffect.hpp"
#include "Effects/ChangeToToneSlot.hpp"
#include "Effects/KillGuitarVolumeEffect.hpp"
#include "Effects/KillMusicVolumeEffect.hpp"
#include "Effects/ShuffleTonesEffect.hpp"
#include "Effects/BigNoteheadEffect.hpp"
#include "Effects/SmallNoteheadEffect.hpp"
#include "Effects/InvertedStringsEffect.hpp"
#include "Effects/HalfSpeedEffect.hpp"
#include "Effects/DoubleSpeedEffect.hpp"
#include "Effects/ZoomInEffect.hpp"
#include "Effects/RemoveInstrumentEffect.hpp"
#include "Effects/TurboSpeedEffect.hpp"

using namespace CrowdControl::Effects;
using namespace CrowdControl::Structs;
using namespace CrowdControl::EffectList;

namespace CrowdControl::EffectList {
	std::map<std::string, CCEffect*> AllEffects{
		{ "rainbowstrings", new RainbowStringsEffect(20) },
		{ "rainbownotes", new RainbowNotesEffect(20)},
		{ "drunkmode", new DrunkModeEffect(10) },
		{ "fyourfc", new FYourFCEffect(5) },
		{ "solidrandom", new SolidNotesRandomEffect(10) },
		{ "solidcustom", new SolidNotesCustomEffect(10) },
		{ "solidcustomrgb", new SolidNotesCustomRGBEffect(10) },
		{ "removenotes", new RemoveNotesEffect(20) },
		{ "transparentnotes", new TransparentNotesEffect(20) },
		{ "changetoneslot1", new ChangeToToneSlot(1) },
		{ "changetoneslot2", new ChangeToToneSlot(2) },
		{ "changetoneslot3", new ChangeToToneSlot(3) },
		{ "changetoneslot4", new ChangeToToneSlot(4) },
		{ "shuffletones", new ShuffleTonesEffect(20) },
		{ "killguitarvolume", new KillGuitarVolumeEffect(10) },
		{ "killmusicvolume", new KillMusicVolumeEffect(10) },
		{ "bignoteheads", new BigNoteheadEffect(20) },
		{ "smallnoteheads", new SmallNoteheadEffect(20) },
		{ "invertedstrings", new InvertedStringsEffect(20) },
		{ "halfsongspeed", new HalfSpeedEffect(30) },
		{ "doublesongspeed", new DoubleSpeedEffect(30) },
		{ "zoomin", new ZoomInEffect(20) },
		{ "removeinstrument", new RemoveInstrumentEffect(20) },
		{ "turbospeed", new TurboSpeedEffect(20) }
	};

	/// <summary>
	/// Check if the selected effect is running
	/// </summary>
	/// <param name="effectName"> - Internal name of the effect </param>
	/// <returns>True if the effect is running (only one effect of the same type can be running in the same time)</returns>
	bool IsEffectEnabled(std::string effectName) {
		if (AllEffects.count(effectName) == 0)
			return false;

		return AllEffects[effectName]->running;
	}

	/// <summary>
	/// Check if there's any running effects that are in the list of effects that aren't compatible with the effect you are trying to enable
	/// </summary>
	/// <param name="incompatibleEffects"> - List of incompatible effects for the current effect</param>
	/// <returns>True if any of the effects that are mutually incompatible with this effects are currently running</returns>
	bool AreIncompatibleEffectsEnabled(std::vector<std::string> incompatibleEffects) {
		for (auto effectName : incompatibleEffects)
			if (AllEffects[effectName]->running)
				return true;

		return false;
	}
}