#include "../stdafx.h"
#include "CCEffect.hpp"

#include "../Framework/ResourceLedger.hpp"

namespace CrowdControl::Effects {
	void CCEffect::Run() {
		if (running) {
			auto now = std::chrono::steady_clock::now();
			std::chrono::duration<double> duration = (endTime - now);

			if (duration.count() <= 0) Stop();
		}
	}

	void CCEffect::SetDuration(const Structs::Request& req) {
		if (req.duration)
			duration_ms = req.duration;

		// Is this ever used?
		for (const auto& el : req.parameters.items()) {
			if (el.value().contains("duration")) {
				el.value().at("duration").get_to(duration_ms);
				// Assuming this is in seconds, convert to ms
				duration_ms *= 1000;
				break;
			}
		}

		LOG_INFO("Set duration to " << duration_ms << "ms" << std::endl);

		endTime = std::chrono::steady_clock::now() + std::chrono::milliseconds(duration_ms);
	}

	bool CCEffect::CanStart() {
		return GameState::IsInSong() && !running;
	}

	Enums::EffectStatus CCEffect::Start(const Structs::Request& request) {
		if (!CanStart())
			return Enums::EffectStatus::Retry;

		const auto resources = ClaimsExclusive();
		if (!resources.empty() && !Framework::Ledger().TryClaim(this, resources))
			return Enums::EffectStatus::Retry;

		const auto result = OnStart(request);

		// Release if OnStart failed, or if it succeeded without entering the running state: an
		// instantaneous effect (e.g. ChangeToToneSlot) holds nothing ongoing, so it must not keep
		// the resource. Ongoing effects set running=true in OnStart and hold their claim until Stop.
		if (result != Enums::EffectStatus::Success || !running)
			Framework::Ledger().Release(this);
		return result;
	}

	Enums::EffectStatus CCEffect::Stop() {
		const auto result = OnStop();
		Framework::Ledger().Release(this);
		return result;
	}
}
