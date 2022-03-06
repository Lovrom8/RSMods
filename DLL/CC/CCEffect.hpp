#pragma once

#include "stdio.h"
#include <chrono>
#include <iostream>
#include <chrono>

#include "CCEnums.hpp"
#include "CCStructs.hpp"

#include "../Log.hpp"
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

		/// <summary>
		/// Sets duration for the current effect
		/// Manually loops through the JSON members of the struct and sets the value of the member called "duration"
		/// </summary>
		void SetDuration(Request req) {
			_LOG_INIT;

			for (auto& el : req.parameters.items()) {
				if (el.value().contains("duration")) {
					el.value().at("duration").get_to(duration);
					_LOG("Set duration to : " << duration << " seconds" << std::endl);
					break;
				}
			}
		}
	};
}

