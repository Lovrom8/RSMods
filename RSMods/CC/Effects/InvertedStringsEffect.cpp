#include "InvertedStringsEffect.hpp"

using namespace CrowdControl::Enums;

namespace CrowdControl::Effects {
	struct StrPos {
		float x, y, z;

		StrPos(float _x, float _y, float _z) {
			x = _x;
			y = _y;
			z = _z;
		}
	};
	std::vector<StrPos> stringPositions;
	std::vector<ObjectUtil::Object*> stringObjs;

	EffectResult InvertedStringsEffect::Test(Request request)
	{
		std::cout << "InvertedStringsEffect::Test()" << std::endl;

		if (!MemHelpers::IsInSong() || EffectList::AreIncompatibleEffectsEnabled(incompatibleEffects) || running)
			return EffectResult::Retry;

		return EffectResult::Success;
	}

	void SaveInitialStringPos() {
		ObjectUtil::Object* rootObject = ObjectUtil::GetRootObject();
		std::vector<ObjectUtil::Object*> children = ObjectUtil::GetChildrenOfObject(rootObject);

		std::cout << children.size() << "/" << rootObject->childCount << std::endl;

		for (auto child : children)
		{
			std::string className = child->className;
			//if (className == "ActorGuitarStringStandard") {
			if (className.find("String") != std::string::npos || className.find("string") != std::string::npos){
				stringObjs.push_back(child);
				stringPositions.push_back(StrPos(child->x, child->y, child->z));
				std::cout << className << std::endl;
			}
		}
	}

	void InvertStringPositions() {
		for (int idx = 0; idx < 6; idx++) {
			stringObjs[idx]->x = stringPositions[5-idx].x;
			stringObjs[idx]->y = stringPositions[5-idx].y;
			stringObjs[idx]->z = stringPositions[5-idx].z;
		}
	}

	void RevertStringPositions() {
		for (int idx = 0; idx < 6; idx++) {
			stringObjs[idx]->x = stringPositions[idx].x;
			stringObjs[idx]->y = stringPositions[idx].y;
			stringObjs[idx]->z = stringPositions[idx].z;
		}
	}

	EffectResult InvertedStringsEffect::Start(Request request)
	{
		std::cout << "InvertedStringsEffect::Start()" << std::endl;

		if (!MemHelpers::IsInSong() || EffectList::AreIncompatibleEffectsEnabled(incompatibleEffects) || running)
			return EffectResult::Retry;

		running = true;

		SaveInitialStringPos();
		InvertStringPositions();

		SetDuration(request);
		endTime = std::chrono::steady_clock::now() + std::chrono::seconds(duration);

		return EffectResult::Success;
	}

	void InvertedStringsEffect::Run()
	{
		// Stop automatically after duration has elapsed
		if (running) {
			auto now = std::chrono::steady_clock::now();
			std::chrono::duration<double> duration = (endTime - now);

			if (duration.count() <= 0) Stop();
		}
	}

	EffectResult InvertedStringsEffect::Stop()
	{
		std::cout << "InvertedStringsEffect::Stop()" << std::endl;

		running = false;
		RevertStringPositions();

		return EffectResult::Success;
	}
}