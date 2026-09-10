#include "../stdafx.h"
#include "ExtendedRangeMod.hpp"
#include "Midi.hpp"
#include "ExtendedRangeMode.hpp"
#include "../D3D/D3DHooks.hpp"

using Framework::ModContext;
using Framework::KeyEdge;
using Framework::Availability;
using Framework::KeyEvent;
using Settings::StringColorMode;
using Settings::NoteColorMode;
namespace Setting = Settings::Setting;

void ExtendedRangeMod::OnInitialize(ModContext& c) {
	c.Commands().BindSetting(
		Setting::Key::RainbowStrings,
		KeyEdge::Up,
		Availability::Active,
		[](ModContext&, const KeyEvent&) {
			ERMode::ToggleRainbowMode();
			if (!ERMode::RainbowEnabled) ERMode::ResetAllStrings();
		},
		[](const ModContext& context, const KeyEvent&) {
			return context.IsOn(Setting::RainbowStringsEnabled);
		},
		"Rainbow Strings");

	c.Commands().BindSetting(
		Setting::Key::ToggleExtendedRange,
		KeyEdge::Up,
		Availability::Active,
		[](ModContext&, const KeyEvent&) {
			ERMode::UseERExclusivelyInThisSong = !ERMode::UseERExclusivelyInThisSong;
			GameState::ToggleCB(ERMode::UseERExclusivelyInThisSong);
		},
		{},
		"Toggle Extended Range");
}

void ExtendedRangeMod::OnEnabled(ModContext&) {
	s_active = true;
	D3DHooks::RecreateTextures = true;
}

void ExtendedRangeMod::OnDisabled(ModContext&) {
	s_active = false;
	ReleaseTextures();
}

void ExtendedRangeMod::OnShutdown(ModContext&) {
	s_active = false;
	ReleaseTextures();
	ERMode::StopRainbowThread();
}

void ExtendedRangeMod::RegenerateTextures(IDirect3DDevice9* pDevice) {
	if (!pDevice) return;

	if (!s_active) {
		ReleaseTextures();
		return;
	}

	ERMode::RegenerateTextures(pDevice);
}

void ExtendedRangeMod::ReleaseTextures() {
	ERMode::ReleaseTextures();
}

// Edge: this mod became active in a song.
void ExtendedRangeMod::OnSongEnter(ModContext& c) {
	// Tuner detection must not bleed into the song.
	ERMode::AttemptedERInTuner = false;
	ERMode::UseERInTuner = false;
	tunerSettleUntil.reset();

	if (ERMode::AttemptedERInThisSong) return;

	// Arm the tuning-settle wait; OnSongTick runs the detection once it expires.
	songSettleUntil = SettleDeadline(c);
}

void ExtendedRangeMod::OnSongTick(ModContext& c) {
	if (songSettleUntil) {
		if (Clock::now() < *songSettleUntil) return; // Still settling - leave the strings alone.

		songSettleUntil.reset();

		ERMode::UseERExclusivelyInThisSong = SongTuning::IsExtendedRangeSong();
		ERMode::UseEROrColorsInThisSong =
			(c.IsOn(Setting::ExtendedRangeEnabled) && ERMode::UseERExclusivelyInThisSong) ||
			c.ColorMode() == StringColorMode::Custom ||
			(c.IsOn(Setting::SeparateNoteColors) && c.NoteColorMode() != NoteColorMode::Default);
		ERMode::AttemptedERInThisSong = true;
	}

	ApplyColors();
}

void ExtendedRangeMod::OnMenuTick(ModContext& c) {
	if (GameState::Menus::IsInPreSongTuner()) {
		if (!ERMode::AttemptedERInTuner) {
			if (!tunerSettleUntil) tunerSettleUntil = SettleDeadline(c);

			if (Clock::now() < *tunerSettleUntil) return; // Still settling - leave the strings alone.

			tunerSettleUntil.reset();
			ERMode::AttemptedERInTuner = true;
			ERMode::UseERInTuner = SongTuning::IsExtendedRangeTuner();
		}
	}
	else {
		ERMode::AttemptedERInTuner = false;
		ERMode::UseERInTuner = false;
		tunerSettleUntil.reset();
	}

	ApplyColors();
}

// Edge: this mod stopped being active in a song. Formerly the ER block of
// CleanupSongSpecificStates.
void ExtendedRangeMod::OnSongExit(ModContext&) {
	songSettleUntil.reset(); // The player may have backed out mid-settle.

	if (ERMode::AttemptedERInThisSong) {
		ERMode::UseERExclusivelyInThisSong = false;
		ERMode::UseEROrColorsInThisSong = false;
		ERMode::AttemptedERInThisSong = false;
	}
}

// Always-on string coloring. Runs at the END of each phase tick so the flag updates above are visible to
// Toggle7StringMode in the same frame.
// DoRainbow() launches the rainbow animation on its own thread (it must not block MainThread, which
// also drains keybind commands, otherwise the effect could never be toggled back off).
void ExtendedRangeMod::ApplyColors() {
	if (ERMode::IsRainbowEnabled() || ERMode::IsRainbowNotesEnabled())
		ERMode::DoRainbow();
	else
		ERMode::Toggle7StringMode();
}

// When the detection may run. Zero-length when the wait is skipped, so it lands on this same tick.
ExtendedRangeMod::Clock::time_point ExtendedRangeMod::SettleDeadline(const ModContext& c) {
	const auto wait = SkipTuningSettle(c) ? std::chrono::milliseconds::zero() : TuningSettleTime;

	return Clock::now() + wait;
}

// Skip the tuning-settle wait once MIDI has auto-tuned in the pre-song tuner: the pedal tuning is
// already applied and the game's tuning numbers are stable. MidiMod owns and latches this flag.
bool ExtendedRangeMod::SkipTuningSettle(const ModContext&) {
	return Midi::appliedTunerAutoTune;
}

static Framework::ModRegistrar<ExtendedRangeMod> _extendedRangeReg;
