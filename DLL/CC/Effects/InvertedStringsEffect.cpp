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

	
	/// <summary>
	/// Test the twitch mod's requirements.
	/// </summary>
	/// <param name="request"> - JSON Request</param>
	/// <returns>EffectStatus::Success if test completed without any issues. EffectStatus::Retry if we have to retry.</returns>
	EffectStatus InvertedStringsEffect::Test(Request request)
	{
		_LOG_INIT;

		_LOG("InvertedStringsEffect::Test()" << std::endl);

		if (!CanStart(&EffectList::AllEffects))
			return EffectStatus::Retry;

		return EffectStatus::Success;
	}

	/// <summary>
	/// Save the original positioning from the guitar strings
	/// </summary>
	void SaveInitialStringPos() {
		_LOG_INIT;

		// Get the root object, and it's children.
		ObjectUtil::Object* rootObject = ObjectUtil::GetRootObject();
		std::vector<ObjectUtil::Object*> children = ObjectUtil::GetChildrenOfObject(rootObject);

		_LOG(children.size() << "/" << rootObject->childCount << std::endl);

		for (auto child : children)
		{
			std::string className = child->className;
			
			// Only save the guitar / bass strings positions.
			if (className.find("String") != std::string::npos || className.find("string") != std::string::npos){
				stringObjs.push_back(child);
				stringPositions.push_back(StrPos(child->x, child->y, child->z));
				_LOG(className << std::endl);
			}
		}
	}

	/// <summary>
	/// Set the string positions to the opposite string. 
	/// 6th string gets placed where the 1st string is.
	/// 5th string gets placed where the 2nd string is. etc.
	/// </summary>
	void InvertStringPositions() {
		for (int idx = 0; idx < 6; idx++) {
			stringObjs[idx]->x = stringPositions[5-idx].x;
			stringObjs[idx]->y = stringPositions[5-idx].y;
			stringObjs[idx]->z = stringPositions[5-idx].z;
		}
	}


	/// <summary>
	/// Change the string positions back to their original positions.
	/// </summary>
	void RevertStringPositions() {
		for (int idx = 0; idx < 6; idx++) {
			stringObjs[idx]->x = stringPositions[idx].x;
			stringObjs[idx]->y = stringPositions[idx].y;
			stringObjs[idx]->z = stringPositions[idx].z;
		}
	}

	/// <summary>
	/// Save the original positions of the guitar / bass strings, then inverting them.
	/// </summary>
	/// <param name="request"></param>
	/// <returns>EffectStatus::Success if completed successfully, or EffectStatus::Retry if we can't run it.</returns>
	EffectStatus InvertedStringsEffect::Start(Request request)
	{
		_LOG_INIT;

		_LOG("InvertedStringsEffect::Start()" << std::endl);

		if (!CanStart(&EffectList::AllEffects))
			return EffectStatus::Retry;

		SaveInitialStringPos();
		InvertStringPositions();

		SetDuration(request);
		running = true;

		return EffectStatus::Success;
	}

	/// <summary>
	/// Stops the mod.
	/// </summary>
	/// <returns>EffectStatus::Success</returns>
	EffectStatus InvertedStringsEffect::Stop()
	{
		_LOG_INIT;

		_LOG("InvertedStringsEffect::Stop()" << std::endl);

		running = false;
		RevertStringPositions();

		return EffectStatus::Success;
	}
}