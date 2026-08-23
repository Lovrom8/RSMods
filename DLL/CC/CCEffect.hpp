#pragma once

#include "CCEnums.hpp"
#include "CCStructs.hpp"

#include "../ObjectUtil.hpp"

namespace CrowdControl::Effects {
	class CCEffect
	{
	public:
		bool running = false;

		int64_t duration_ms = 0;
		std::chrono::steady_clock::time_point endTime;

		virtual ~CCEffect() = default;

		virtual Enums::EffectStatus Test(const Structs::Request& request) = 0;

		// Non-virtual wrappers: CanStart + TryClaim(ClaimsExclusive()) before dispatching to
		// OnStart; Release(this) always runs after OnStop so no effect can forget to release.
		Enums::EffectStatus Start(const Structs::Request& request);
		Enums::EffectStatus Stop();

		virtual void Run();

		void SetDuration(const Structs::Request& req);

		virtual bool CanStart();

		// Returns the resource names this effect holds exclusively while running. Override in
		// each effect; the base returns an empty list (no exclusive claim).
		virtual std::vector<std::string> ClaimsExclusive() const { return {}; }

	protected:
		virtual Enums::EffectStatus OnStart(const Structs::Request& request) = 0;
		virtual Enums::EffectStatus OnStop() = 0;
	};
}
