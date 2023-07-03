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
#include "Effects/HighwayScrollSpeedEffect.hpp"
#include "Effects/ZoomEffect.hpp"
#include "Effects/RemoveInstrumentEffect.hpp"
#include "Effects/TurboSpeedEffect.hpp"
#include "Effects/WavyNotesEffect.hpp"

using namespace CrowdControl::Effects;
using namespace CrowdControl::Structs;
using namespace CrowdControl::EffectList;

namespace CrowdControl::EffectList {
	std::map<std::string, CCEffect*> AllEffects{
		{ "rainbowstrings", new RainbowStringsEffect(20000) },
		{ "rainbownotes", new RainbowNotesEffect(20000)},
		{ "drunkmode", new DrunkModeEffect(10000) },
		{ "fyourfc", new FYourFCEffect() },
		{ "solidrandom", new SolidNotesRandomEffect(10000) },
		{ "solidcustom", new SolidNotesCustomEffect(10000) },
		{ "solidcustomrgb", new SolidNotesCustomRGBEffect(10000) },
		{ "removenotes", new RemoveNotesEffect(20000) },
		{ "transparentnotes", new TransparentNotesEffect(20000) },
		{ "changetoneslot1", new ChangeToToneSlot(1) },
		{ "changetoneslot2", new ChangeToToneSlot(2) },
		{ "changetoneslot3", new ChangeToToneSlot(3) },
		{ "changetoneslot4", new ChangeToToneSlot(4) },
		{ "shuffletones", new ShuffleTonesEffect(20000) },
		{ "killguitarvolume", new KillGuitarVolumeEffect(10000) },
		{ "killmusicvolume", new KillMusicVolumeEffect(10000) },
		{ "bignoteheads", new BigNoteheadEffect(20000) },
		{ "smallnoteheads", new SmallNoteheadEffect(20000) },
		{ "invertedstrings", new InvertedStringsEffect(20000) },
		{ "halfscrollspeed", new HighwayScrollSpeedEffect(30000, 1.5) },
		{ "doublescrollspeed", new HighwayScrollSpeedEffect(30000, 10.0) },
		{ "triplescrollspeed", new HighwayScrollSpeedEffect(30000, 15.0) },
		{ "zoomin", new ZoomEffect(20000, 2.0) },
		{ "zoomout", new ZoomEffect(20000, 0.5) },
		{ "removeinstrument", new RemoveInstrumentEffect(20000) },
		{ "turbospeed", new TurboSpeedEffect(10000) },
		{ "wavynotes", new WavyNotesEffect(20000) }
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
		for (auto &effectName : incompatibleEffects)
			if (AllEffects[effectName]->running)
				return true;

		return false;
	}
}