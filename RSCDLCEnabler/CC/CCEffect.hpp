#pragma once

#include "stdio.h"
#include <chrono>
#include <iostream>
#include <chrono>
#include "CCEnums.hpp"
#include "CCStructs.hpp"
#include "../Settings.hpp"
#include "../ObjectUtil.hpp"
#include "../Offsets.hpp"
#include "../MemHelpers.hpp"

using namespace CrowdControl::Enums;
using namespace CrowdControl::Structs;

namespace CrowdControl::Effects {
	class CCEffect
	{
	public:
		bool running = false;
		unsigned int duration = 0;
		std::chrono::steady_clock::time_point endTime;

		virtual EffectResult Test(Request request) = 0;
		virtual EffectResult Start(Request request) = 0;
		virtual void Run() = 0;
		virtual EffectResult Stop() = 0;
	};
}

