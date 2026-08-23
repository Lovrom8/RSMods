#pragma once

#include "CCEffect.hpp"
#include "CCStructs.hpp"

namespace CrowdControl::EffectList {
	std::map<std::string, CrowdControl::Effects::CCEffect*, std::less<>>& GetAllEffects();

	bool IsEffectEnabled(const std::string& effectName);
}
