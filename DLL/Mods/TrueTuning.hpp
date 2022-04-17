#pragma once

#include "Windows.h"
#include "../Offsets.hpp"
#include "../MemUtil.hpp"
#include "../Log.hpp"
#include "../D3D/D3DHooks.hpp"

namespace TrueTuning
{
	void DisableTrueTuning();
	void EnableTrueTuning();
}