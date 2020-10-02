#include "SmallNoteheadEffect.hpp"

using namespace CrowdControl::Enums;

namespace CrowdControl::Effects { // Scales notes in a song to unusually small size
	EffectResult SmallNoteheadEffect::Test(Request request)
	{
		std::cout << "SmallNoteheadEffect::Test()" << std::endl;

		return EffectResult::Success;
	}

	EffectResult SmallNoteheadEffect::Start(Request request)
	{
		std::cout << "SmallNoteheadEffect::Start()" << std::endl;

		if (!MemHelpers::IsInSong())
			return EffectResult::Retry;

		running = true;
		endTime = std::chrono::steady_clock::now() + std::chrono::seconds(duration);

		return EffectResult::Success;
	}

	void MakeNotesSmall(float scale) { //TODO: move ScaleObjects to ObjectUtil
		std::cout << "SmallNoteheadEffect::MakeNotesSmall()" << std::endl;

		ObjectUtil::Object* rootObject = ObjectUtil::GetRootObject();
		std::vector<ObjectUtil::Object*> children = ObjectUtil::GetChildrenOfObject(rootObject);

		std::cout << children.size() << "/" << rootObject->childCount << std::endl;

		for (auto child : children)
		{
			std::string className = child->className;
			if (!MemHelpers::IsInStringArray(className, 0, ObjectUtil::NoteHeadParts))
				continue;

			child->scale = scale;
		}
	}

	void SmallNoteheadEffect::Run()
	{
		// Stop automatically after duration has elapsed
		if (running) {
			
			auto now = std::chrono::steady_clock::now();
			std::chrono::duration<double> duration = (nextTickTime - now);

			if (duration.count() <= 0) {
				nextTickTime = now + std::chrono::milliseconds(tickIntervalMilliseconds);

				//TODO: if other note scaling effects are running, dont do anything
				if (true) {
					MakeNotesSmall(running ? 0.5 : 1); //TODO: when it's made compatible with other note scaling effects, put the scaling part outside if (running)
				}
			}

			duration = (endTime - now);

			if (duration.count() <= 0) Stop();
		}
	}

	EffectResult SmallNoteheadEffect::Stop()
	{
		std::cout << "SmallNoteheadEffect::Stop()" << std::endl;

		running = false;
		MakeNotesSmall(1);

		return EffectResult::Success;
	}
}
