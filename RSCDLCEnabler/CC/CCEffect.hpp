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

		void SetDuration(Request req) {
			for (auto& el : req.parameters.items()) {
				if (el.value().contains("duration")) {
					el.value().at("duration").get_to(duration);
					std::cout << "Set duration to : " << duration << " seconds" << std::endl;
					break;
				}
			}
		}
	};
}

