#include "../stdafx.h"
#include "CCEffectList.hpp"

#include "Effects/BigNoteheadEffect.hpp"
#include "Effects/ChangeToToneSlot.hpp"
#include "Effects/DrunkModeEffect.hpp"
#include "Effects/FYourFCEffect.hpp"
#include "Effects/HighwayScrollSpeedEffect.hpp"
#include "Effects/InvertedStringsEffect.hpp"
#include "Effects/KillGuitarVolumeEffect.hpp"
#include "Effects/KillMusicVolumeEffect.hpp"
#include "Effects/RainbowNotesEffect.hpp"
#include "Effects/RainbowStringsEffect.hpp"
#include "Effects/RemoveInstrumentEffect.hpp"
#include "Effects/RemoveNotesEffect.hpp"
#include "Effects/ShuffleTonesEffect.hpp"
#include "Effects/SmallNoteheadEffect.hpp"
#include "Effects/SolidNotesEffect.hpp"
#include "Effects/TransparentNotesEffect.hpp"
#include "Effects/TurboSpeedEffect.hpp"
#include "Effects/WavyNotesEffect.hpp"
#include "Effects/ZoomEffect.hpp"

using namespace CrowdControl::Effects;
using namespace CrowdControl::Structs;
using namespace CrowdControl::EffectList;

namespace CrowdControl::EffectList {
	 std::map<std::string, CCEffect*, std::less<>>& GetAllEffects() {
		static std::map<std::string, CCEffect*, std::less<>> AllEffects{
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
		return AllEffects;
	}

	/// <summary>
	/// Check if the selected effect is running
	/// </summary>
	/// <param name="effectName"> - Internal name of the effect </param>
	/// <returns>True if the effect is running (only one effect of the same type can be running in the same time)</returns>
	bool IsEffectEnabled(const std::string& effectName) {
		const auto& allEffects = CrowdControl::EffectList::GetAllEffects();
		if (auto it = allEffects.find(effectName); it != allEffects.end()) {
			return it->second->running;
		}
		return false;
	}

}