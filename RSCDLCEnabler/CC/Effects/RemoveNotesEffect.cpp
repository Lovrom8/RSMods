#include "RemoveNotesEffect.hpp"

using namespace CrowdControl::Enums;

namespace CrowdControl::Effects { // Prevents the game from drawing note head meshes
	EffectResult RemoveNotesEffect::Test(Request request)
	{
		std::cout << "RemoveNotesEffect::Test()" << std::endl;

		return EffectResult::Success;
	}

	EffectResult RemoveNotesEffect::Start(Request request)
	{
		std::cout << "RemoveNotesEffect::Start()" << std::endl;

		if (!MemHelpers::IsInSong())
			return EffectResult::Retry;

		running = true;
		//Settings::UpdateTwitchSetting("RemoveNotes", "on");

		endTime = std::chrono::steady_clock::now() + std::chrono::seconds(duration);

		return EffectResult::Success;
	}

	void ScaleNotes(float scale) {
		std::cout << "RemoveNotesEffect::ScaleNotes()" << std::endl;

		ObjectUtil::Object* rootObject = ObjectUtil::GetRootObject();
		std::vector<ObjectUtil::Object*> children = ObjectUtil::GetChildrenOfObject(rootObject);

		std::cout << children.size() << "/" << rootObject->childCount << std::endl;

		for (auto child : children)
		{
			if (
				strcmp(child->className, "ActorChordLetter") != 0 &&
				strcmp(child->className, "MeshBracketLarge") != 0 &&
				strcmp(child->className, "MeshNoteBendArrow") != 0 &&
				strcmp(child->className, "MeshNoteBendPanel") != 0 &&
				strcmp(child->className, "MeshNoteIconFretHandMute") != 0 &&
				strcmp(child->className, "MeshNoteIconHammerOn") != 0 &&
				strcmp(child->className, "MeshNoteIconHarmonic") != 0 &&
				strcmp(child->className, "MeshNoteIconPalmMute") != 0 &&
				strcmp(child->className, "MeshNoteIconPinchHarmonic") != 0 &&
				strcmp(child->className, "MeshNoteIconPop") != 0 &&
				strcmp(child->className, "MeshNoteIconPullOff") != 0 &&
				strcmp(child->className, "MeshNoteIconSlap") != 0 &&
				strcmp(child->className, "MeshNoteIconTap") != 0 &&
				strcmp(child->className, "MeshNoteOpenAccent") != 0 &&
				strcmp(child->className, "MeshNoteOpenArpeggio") != 0 &&
				strcmp(child->className, "MeshNoteOpenLH") != 0 &&
				strcmp(child->className, "MeshNoteOpenRH") != 0 &&
				strcmp(child->className, "MeshNoteSingleAccent") != 0 &&
				strcmp(child->className, "MeshNoteSingleArpeggio") != 0 &&
				strcmp(child->className, "MeshNoteSingleLH") != 0 &&
				strcmp(child->className, "MeshNoteSingleRH") != 0 &&
				strcmp(child->className, "MeshStemBracket") != 0 &&
				strcmp(child->className, "MeshStemDoubleStop") != 0 &&
				strcmp(child->className, "MeshStemNote") != 0 &&
				strcmp(child->className, "MeshStemNoteOpen") != 0 &&
				strcmp(child->className, "MeshStrumBracketLarge") != 0 &&
				strcmp(child->className, "MeshStrumBracketSmall") != 0 &&
				strcmp(child->className, "MeshStrumLarge") != 0 &&
				strcmp(child->className, "MeshStrumLargeAccent") != 0 &&
				strcmp(child->className, "MeshStrumLargeArpeggio") != 0 &&
				strcmp(child->className, "MeshStrumLargeDoubleStop") != 0 &&
				strcmp(child->className, "MeshStrumLargeFretMute") != 0 &&
				strcmp(child->className, "MeshStrumLargePalmMute") != 0 &&
				strcmp(child->className, "MeshStrumSmall") != 0 &&
				strcmp(child->className, "MeshStrumSmallAccent") != 0 &&
				strcmp(child->className, "MeshStrumSmallDoubleStop") != 0 &&
				strcmp(child->className, "MeshStrumSmallDoubleStopFretMute") != 0 &&
				strcmp(child->className, "MeshStrumSmallDoubleStopPalmMute") != 0 &&
				strcmp(child->className, "MeshStrumSmallFretMute") != 0 &&
				strcmp(child->className, "MeshStrumSmallPalmMute") != 0 &&
				strcmp(child->className, "NoteTail") != 0 &&
				strcmp(child->className, "NoteTailMesh") != 0
				) continue;

			child->scale = scale;
		}
	}

	void RemoveNotesEffect::Run()
	{
		// Stop automatically after duration has elapsed
		if (running) {
			std::cout << "RemoveNotesEffect::Run()" << std::endl;

			// Due to rocksmith reusing models, this effect needs to tick constantly to fully remove the effect
			auto now = std::chrono::steady_clock::now();
			std::chrono::duration<double> duration = (nextTickTime - now);

			if (duration.count() <= 0) {
				nextTickTime = now + std::chrono::milliseconds(tickIntervalMilliseconds);

				//TODO: if other note scaling effects are running, dont do anything
				if (true) {
					ScaleNotes(running ? 0 : 1);
				}
			}

			duration = (endTime - now);

			if (duration.count() <= 0) Stop();
		}
	}

	EffectResult RemoveNotesEffect::Stop()
	{
		std::cout << "RemoveNotesEffect::Stop()" << std::endl;

		ScaleNotes(1);

		running = false;
		//Settings::UpdateTwitchSetting("RemoveNotes", "off");

		return EffectResult::Success;
	}
}