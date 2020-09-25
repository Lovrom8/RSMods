#pragma once

namespace CrowdControl::Enums {
	enum RequestType {
		Test = 0,
		Start = 1,
		Stop = 2
	};

	enum EffectResult {
		/// <summary>The effect executed successfully.</summary>
		Success = 0,
		/// <summary>The effect failed to trigger, but is still available for use. Viewer(s) will be refunded.</summary>
		Failure = 1,
		/// <summary>Same as <see cref="Failure"/> but the effect is no longer available for use.</summary>
		Unavailable = 2,
		/// <summary>The effect cannot be triggered right now, try again in a few seconds.</summary>
		Retry = 3
	};
}