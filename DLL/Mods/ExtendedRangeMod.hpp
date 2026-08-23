#pragma once

#include <chrono>
#include <optional>

#include "../Framework/Framework.hpp"

// It is always enabled: the string-coloring pass runs every menu and song tick
// regardless of the ExtendedRangeEnabled toggle (that toggle only gates the
// in-song ER detection, which happens inside ERMode).
class ExtendedRangeMod : public Framework::IMod {
public:
	MOD_ID(ExtendedRangeMod)

	std::vector<std::string_view> ClaimsExclusive() const override { return { "string-colors" }; }

	void OnInitialize(Framework::ModContext& c) override;
	void OnShutdown(Framework::ModContext& c) override;
	void OnSongEnter(Framework::ModContext& c) override;
	void OnSongTick(Framework::ModContext& c) override;
	void OnMenuTick(Framework::ModContext& c) override;
	void OnSongExit(Framework::ModContext& c) override;

private:
	using Clock = std::chrono::steady_clock;

	// Tuning takes ~1.5s to settle before the game's numbers are trustworthy. This used to be a
	// blocking Sleep, but every hook runs on MainThread, which also drains keybind commands - so
	// blocking here froze every keybind and every other mod's tick for the duration. Instead we arm
	// a deadline and let the detection land on a later maintenance tick, holding off the colour pass
	// until then so nothing is recoloured off half-settled tuning (which is what the Sleep bought).
	static constexpr std::chrono::milliseconds TuningSettleTime{ 1500 };

	// Set while a detection is pending; cleared once it runs or the phase it belongs to ends.
	std::optional<Clock::time_point> songSettleUntil;
	std::optional<Clock::time_point> tunerSettleUntil;

	void ApplyColors();
	static Clock::time_point SettleDeadline(const Framework::ModContext& c);
	static bool SkipTuningSettle(const Framework::ModContext& c);
};
