#include "../stdafx.h"
#include "ExtendedRangeMod.hpp"
#include "Midi.hpp"
#include "ExtendedRangeMode.hpp"
#include "../D3D/D3D.hpp"
#include "../D3D/D3DHelper.hpp"
#include "../D3D/D3DHooks.hpp"

using Framework::ModContext;
using Framework::DrawContext;
using Framework::DrawResult;
using Framework::DrawOutcome;
using Framework::DrawPath;
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

	// Priority 20: Extended Range / Custom Colors note head, stems, and tail coloring.
	c.Draw().Register("ExtendedRangeNotes", 20, DrawPath::Both, [](DrawContext& ctx) -> DrawResult {
		if (!ctx.inSong) return { DrawOutcome::Pass };
		if (!ERMode::AttemptedERInThisSong || !ERMode::UseEROrColorsInThisSong) return { DrawOutcome::Pass };

		LPDIRECT3DTEXTURE9 tex = s_noteTexture.load(std::memory_order_relaxed);
		if (!tex) return { DrawOutcome::Pass };

		if (ctx.path == DrawPath::Primitive) {
			if (ctx.mesh.Stride == 12) {
				return { DrawOutcome::ReplaceTexture, 1, tex };
			}
			return { DrawOutcome::Pass };
		}

		// DrawPath::Indexed
		if (IsToBeRemoved(sevenstring, ctx.mesh) || IsExtraRemoved(noteModifiers, ctx.thicc)) {
			return { DrawOutcome::ReplaceTexture, 1, tex };
		}

		// Note stems, bends, slides, and accents
		if ((ctx.mesh.Stride == 32 && ctx.mesh.PrimCount == 2 && ctx.mesh.NumVertices == 4) ||
		    (ctx.mesh.Stride == 32 && ctx.mesh.PrimCount == 4 && ctx.mesh.NumVertices == 6)) {
			// If RainbowNotes is active, let RainbowNotes (Priority 10) own stems
			if (ERMode::RainbowNotesEnabled.load(std::memory_order_relaxed) && ERMode::customNoteColorH.load(std::memory_order_relaxed) > 0) {
				return { DrawOutcome::Pass };
			}

			auto crc1 = ctx.StageCRC(1);
			if (crc1 && (*crc1 == crcStemsAccents || *crc1 == crcBendSlideIndicators)) {
				return { DrawOutcome::ReplaceTexture, 1, tex };
			}
		}

		return { DrawOutcome::Pass };
	});

	c.Draw().RegisterTextureLifecycle(&ExtendedRangeMod::RegenerateTextures, &ExtendedRangeMod::ReleaseTextures);
}

void ExtendedRangeMod::OnEnabled(ModContext& c) {
	s_active = true;
	c.Draw().CancelTextureRelease();
	D3DHooks::RecreateTextures = true;
}

void ExtendedRangeMod::OnDisabled(ModContext& c) {
	s_active = false;
	s_noteTexture.store(nullptr, std::memory_order_release);
	D3DHooks::RecreateTextures = true;
	c.Draw().RequestTextureRelease();
}

void ExtendedRangeMod::OnShutdown(ModContext&) {
	s_active = false;
	s_noteTexture.store(nullptr, std::memory_order_relaxed);
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

	if (ERMode::AttemptedERInThisSong) {
		UpdateNoteTexture(c);
		return;
	}

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

	GameState::ToggleCB(ERMode::UseERExclusivelyInThisSong);
	UpdateNoteTexture(c);
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
	s_noteTexture.store(nullptr, std::memory_order_relaxed);

	if (ERMode::AttemptedERInThisSong) {
		ERMode::UseERExclusivelyInThisSong = false;
		ERMode::UseEROrColorsInThisSong = false;
		ERMode::AttemptedERInThisSong = false;
	}
}

void ExtendedRangeMod::OnSettingsChanged(ModContext& c) {
	UpdateNoteTexture(c);
}

void ExtendedRangeMod::UpdateNoteTexture(const ModContext& c) {
	if (!GameState::IsInSong() || !ERMode::AttemptedERInThisSong || !ERMode::UseEROrColorsInThisSong) {
		s_noteTexture.store(nullptr, std::memory_order_relaxed);
		return;
	}

	switch (c.NoteColorMode()) {
		case NoteColorMode::SameAsStrings:
			s_noteTexture.store(ERMode::customStringColorTexture, std::memory_order_relaxed);
			break;
		case NoteColorMode::Custom:
			if (c.IsOn(Setting::SeparateNoteColors))
				s_noteTexture.store(ERMode::customNoteColorTexture, std::memory_order_relaxed);
			else
				s_noteTexture.store(nullptr, std::memory_order_relaxed);
			break;
		default:
			s_noteTexture.store(nullptr, std::memory_order_relaxed);
			break;
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
