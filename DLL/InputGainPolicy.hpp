#pragma once

#include <cmath>

// Rocksmith's input calibration raises the normalized mic gain toward a target and clamps it to
// [0.01, 1.0]. When it cannot reach the target it stops at the clamp, and that 1.0 is written to
// the profile. The game then pushes the stored value to the capture endpoint shortly after every
// later boot, so the input stays at full scale until something clears it. The in-game "Restore
// gain to default" does clear it; recalibrating does not, because the stored value is reapplied.
//
// This header holds the decision and the game's own display conversion, with no game calls, so
// both are testable.
namespace InputGainPolicy {
	// The clamp the calibration search stops at.
	inline constexpr float kClampedGain = 1.0f;
	// Gains are stored as single-precision and compared against a clamp the game wrote, so an
	// exact compare is too strict for a value that may have been through a conversion.
	inline constexpr float kClampEpsilon = 0.0005f;

	// The audio settings screen shows 20*log10(lerp(10^-0.4, 10^0.5, x)) for a stored gain x,
	// which is -8.0 db at 0 and +10.0 db at 1.
	inline float NormalizedToDisplayDb(float normalized) {
		const double low = std::pow(10.0, -0.4);
		const double high = std::pow(10.0, 0.5);
		const double value = low + (high - low) * static_cast<double>(normalized);
		return static_cast<float>(20.0 * std::log10(value));
	}

	// What we managed to read from the game before deciding anything.
	struct GainReading {
		// False when the gain could not be read, for example on an executable we do not know.
		bool available = false;
		float normalized = 0.0f;
		// The player's own "set the input gain manually" choice.
		bool overrideEnabled = false;
	};

	enum class GainAction {
		Leave,
		Clear,
	};

	inline bool IsClamped(float normalized) {
		return normalized >= kClampedGain - kClampEpsilon;
	}

	// Clear only a gain sitting at the clamp with the override off. A player who wants maximum
	// input gain sets it deliberately with the override, and that choice is left alone.
	inline GainAction Decide(const GainReading& reading, bool settingOn) {
		if (!settingOn || !reading.available || reading.overrideEnabled)
			return GainAction::Leave;
		return IsClamped(reading.normalized) ? GainAction::Clear : GainAction::Leave;
	}
}
