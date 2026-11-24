// The stored input gain must never be cleared except in the one state calibration leaves behind,
// and the dB the log reports must match what the audio settings screen shows.
#include "../../InputGainPolicy.hpp"

#include <cmath>
#include <cstdio>

using namespace InputGainPolicy;

static int failures = 0;

static void Check(bool condition, const char* what) {
	if (!condition) {
		std::printf("FAIL: %s\n", what);
		++failures;
	}
}

static void CheckDb(float normalized, float expected, const char* what) {
	const float actual = NormalizedToDisplayDb(normalized);
	if (std::fabs(actual - expected) > 0.05f) {
		std::printf("FAIL: %s (expected %.2f db, got %.2f db)\n", what, expected, actual);
		++failures;
	}
}

static GainReading Reading(float normalized, bool overrideEnabled, bool available = true) {
	GainReading reading;
	reading.available = available;
	reading.normalized = normalized;
	reading.overrideEnabled = overrideEnabled;
	return reading;
}

int main() {
	// The conversion the screen uses. Both ends are fixed by the formula, so these fail if it drifts.
	CheckDb(0.0f, -8.0f, "0.0 reads as -8.0 db");
	CheckDb(1.0f, 10.0f, "1.0 reads as +10.0 db");
	CheckDb(0.17f, -1.2f, "the game default reads as about -1.2 db");
	CheckDb(0.7647f, 8.0f, "0.7647 reads as 8.0 db");
	Check(NormalizedToDisplayDb(0.5f) > NormalizedToDisplayDb(0.17f), "the conversion rises with the gain");

	// The one state we act on.
	Check(Decide(Reading(1.0f, false), true) == GainAction::Clear, "clamped with the override off is cleared");

	// Everything else is left alone.
	Check(Decide(Reading(1.0f, true), true) == GainAction::Leave, "clamped with the override on is kept");
	Check(Decide(Reading(0.17f, false), true) == GainAction::Leave, "the default gain is kept");
	Check(Decide(Reading(0.7647f, false), true) == GainAction::Leave, "a gain below the clamp is kept");
	Check(Decide(Reading(0.99f, false), true) == GainAction::Leave, "just below the clamp is kept");
	Check(Decide(Reading(1.0f, false), false) == GainAction::Leave, "nothing happens when the setting is off");
	Check(Decide(Reading(1.0f, false, false) , true) == GainAction::Leave, "nothing happens when the gain cannot be read");

	// The clamp test tolerates a conversion but not a real difference.
	Check(IsClamped(1.0f), "1.0 counts as clamped");
	Check(IsClamped(0.9999f), "a rounding away from the clamp still counts");
	Check(!IsClamped(0.99f), "a gain a percent below the clamp does not count");

	if (failures) {
		std::printf("%d input gain policy check(s) failed\n", failures);
		return 1;
	}
	std::printf("InputGainPolicy: all checks passed\n");
	return 0;
}
