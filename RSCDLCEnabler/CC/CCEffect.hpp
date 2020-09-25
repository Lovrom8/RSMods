#pragma once

#include "ControlServer.hpp"
#include "stdio.h"
#include <chrono>
#include <iostream>
#include <chrono>
#include "../Settings.hpp"

using CrowdControl::Enums::EffectResult;

namespace CrowdControl::Effects {
	class CCEffect
	{
	public:
		bool running = false;
		unsigned int duration = 0;
		std::chrono::steady_clock::time_point endTime;

		virtual EffectResult Test() = 0;
		virtual EffectResult Start() = 0;
		virtual void Run() = 0;
		virtual EffectResult Stop() = 0;
	};
}

