#include "BigNoteheadEffect.hpp"

using namespace CrowdControl::Enums;

namespace CrowdControl::Effects { // Scales notes in a song to unusually big size
	EffectResult BigNoteheadEffect::Test(Request request)
	{
		std::cout << "BigNoteheadEffect::Test()" << std::endl;

		return EffectResult::Success;
	}

	EffectResult BigNoteheadEffect::Start(Request request)
	{
		std::cout << "BigNoteheadEffect::Start()" << std::endl;

		if (!MemHelpers::IsInSong())
			return EffectResult::Retry;

		running = true;
		endTime = std::chrono::steady_clock::now() + std::chrono::seconds(duration);

		std::cout << "BigNoteheadEffect::Running" << std::endl;

		return EffectResult::Success;
	}

	void MakeNotesBig(float scale) {
		//std::cout << "BigNoteheadEffect::MakeNotesBig()" << std::endl;

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

	void BigNoteheadEffect::Run()
	{
		// Stop automatically after duration has elapsed
		if (running) {
			auto now = std::chrono::steady_clock::now();
			std::chrono::duration<double> duration = (nextTickTime - now);

			if (duration.count() <= 0) {
				nextTickTime = now + std::chrono::milliseconds(tickIntervalMilliseconds);

				//TODO: if other note scaling effects are running, dont do anything
				if (true) {
					MakeNotesBig(running ? 2.5 : 1);
				}
			}

			duration = (endTime - now);

			if (duration.count() <= 0) Stop();
		}
	}

	EffectResult BigNoteheadEffect::Stop()
	{
		std::cout << "BigNoteheadEffect::Stop()" << std::endl;

		running = false;

		MakeNotesBig(1); //TODO: when it's made compatible with other note scaling effects, put the scaling part outside if (running)

		return EffectResult::Success;
	}
}
