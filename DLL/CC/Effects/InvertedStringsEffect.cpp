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
	/// <returns>EffectResult::Success if test completed without any issues. EffectResult::Retry if we have to retry.</returns>
	EffectResult InvertedStringsEffect::Test(Request request)
	{
		_LOG_INIT;

		_LOG_HEAD << "InvertedStringsEffect::Test()" << LOG.endl();

		if (!MemHelpers::IsInSong() || EffectList::AreIncompatibleEffectsEnabled(incompatibleEffects) || running)
			return EffectResult::Retry;

		return EffectResult::Success;
	}

	/// <summary>
	/// Save the original positioning from the guitar strings
	/// </summary>
	void SaveInitialStringPos() {
		_LOG_INIT;

		// Get the root object, and it's children.
		ObjectUtil::Object* rootObject = ObjectUtil::GetRootObject();
		std::vector<ObjectUtil::Object*> children = ObjectUtil::GetChildrenOfObject(rootObject);

		_LOG_HEAD << children.size() << "/" << rootObject->childCount << LOG.endl();

		for (auto child : children)
		{
			std::string className = child->className;
			
			// Only save the guitar / bass strings positions.
			if (className.find("String") != std::string::npos || className.find("string") != std::string::npos){
				stringObjs.push_back(child);
				stringPositions.push_back(StrPos(child->x, child->y, child->z));
				_LOG_HEAD << className << LOG.endl();
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
	/// <returns>EffectResult::Success if completed successfully, or EffectResult::Retry if we can't run it.</returns>
	EffectResult InvertedStringsEffect::Start(Request request)
	{
		_LOG_INIT;

		_LOG_HEAD << "InvertedStringsEffect::Start()" << LOG.endl();

		if (!MemHelpers::IsInSong() || EffectList::AreIncompatibleEffectsEnabled(incompatibleEffects) || running)
			return EffectResult::Retry;

		running = true;

		SaveInitialStringPos();
		InvertStringPositions();

		SetDuration(request);
		endTime = std::chrono::steady_clock::now() + std::chrono::seconds(duration);

		return EffectResult::Success;
	}

	/// <summary>
	/// Ensure that the mod only lasts for the time specified in the JSON request.
	/// </summary>
	void InvertedStringsEffect::Run()
	{
		if (running) {
			auto now = std::chrono::steady_clock::now();
			std::chrono::duration<double> duration = (endTime - now);

			if (duration.count() <= 0) Stop();
		}
	}

	/// <summary>
	/// Stops the mod.
	/// </summary>
	/// <returns>EffectResult::Success</returns>
	EffectResult InvertedStringsEffect::Stop()
	{
		_LOG_INIT;

		_LOG_HEAD << "InvertedStringsEffect::Stop()" << LOG.endl();

		running = false;
		RevertStringPositions();

		return EffectResult::Success;
	}
}