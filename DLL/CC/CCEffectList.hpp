#pragma once
#include <map>

#include "CCEffect.hpp"
#include "CCStructs.hpp"

namespace CrowdControl::EffectList {
	extern std::map<std::string, Effects::CCEffect*> AllEffects;

	bool AreIncompatibleEffectsEnabled(std::vector<std::string> incompatibleEffects);
	bool IsEffectEnabled(std::string effectName);
}
