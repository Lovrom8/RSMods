#pragma once

#include <cstdio>
#include <chrono>
#include <iostream>

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

		int64_t duration_ms = 0;
		std::chrono::steady_clock::time_point endTime;

		virtual EffectStatus Test(Request request) = 0;
		virtual EffectStatus Start(Request request) = 0;
		virtual EffectStatus Stop() = 0;

		/**
		 * \brief Run/Update this effect, checks duration, and stops timed effect when duration has expired. Called approximately every 10ms
		 */
		virtual void Run() {
			if (running) {
				auto now = std::chrono::steady_clock::now();
				std::chrono::duration<double> duration = (endTime - now);

				if (duration.count() <= 0) Stop();
			}
		}

		/// <summary>
		/// Sets duration for the current effect, and calculates endTime
		/// Manually loops through the JSON members of the struct and sets the value of the member called "duration"
		/// </summary>
		void SetDuration(Request req) {
			_LOG_INIT;

			if(req.duration)
				duration_ms = req.duration;

			// Is this ever used?
			 for (auto& el : req.parameters.items()) {
				if (el.value().contains("duration")) {
					el.value().at("duration").get_to(duration_ms);
					// Assuming this is in seconds, convert to ms
					duration_ms *= 1000;
					break;
				}
			}

			 _LOG("Set duration to " << duration_ms << "ms" << std::endl);

			endTime = std::chrono::steady_clock::now() + std::chrono::milliseconds(duration_ms);
		}

		/**
		 * \brief Can this effect start? By default checks that a song is being played, no incompatible effects are running, and this effect is not running
		 * \return True when this effect can start, false otherwise
		 */
		virtual bool CanStart(std::map<std::string, CCEffect*>* AllEffects)
		{
			return MemHelpers::IsInSong() && !AreIncompatibleEffectsRunning(AllEffects) && !running;
		}

		bool AreIncompatibleEffectsRunning(const std::map<std::string, CCEffect*>* AllEffects) const
		{
			for (auto& effectName : incompatibleEffects)
				if (AllEffects->at(effectName)->running)
					return true;

			return false;
		}

	protected:
		/**
		 * \brief List of incompatible effect codes
		 */
		std::vector<std::string> incompatibleEffects = { };
	};
}

