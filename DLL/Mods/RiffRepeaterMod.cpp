#include "../stdafx.h"
#include "RiffRepeaterMod.hpp"
#include "../SongTimer.hpp"

using Framework::ModContext;
using Framework::KeyEdge;
using Framework::Availability;
using Framework::KeyEvent;
using Framework::GamePhase;
using Framework::HudText;
using Framework::HudAnchor;
using Framework::SettingDefs;
using Framework::Toggle;
using Framework::Numeric;
namespace Setting = Settings::Setting;

SettingDefs RiffRepeaterMod::Settings() const {
	return {
		Toggle(Setting::LinearRiffRepeater, "LinearRiffRepeater", "Linear Riff Repeater"),
		Toggle(Setting::AllowRewind, "AllowRewind", "Allow Rewind"),
		Numeric(Setting::RewindBy, "Rewind By (ms)")
			.Ini("Mod Settings", "RewindBy")
			.Default("5000")
			.WithVisibleWhen(Setting::AllowRewind),
		Numeric(Setting::RewindLeadup, "Rewind Leadup (ms)")
			.Ini("Mod Settings", "RewindLeadup")
			.Default("2000")
			.WithVisibleWhen(Setting::AllowRewind),
		Toggle(Setting::AllowLooping, "AllowLooping", "Allow Looping"),
		Numeric(Setting::LoopingLeadUp, "Looping Leadup (ms)")
			.Ini("Mod Settings", "LoopingLeadUp")
			.Default("0")
			.WithVisibleWhen(Setting::AllowLooping),
		Toggle(Setting::RRSpeedAboveOneHundred, "RRSpeedAboveOneHundred", "Riff Repeater Speed Above 100%"),
		Numeric(Setting::RRSpeedInterval, "RR Speed Interval")
			.Ini("Mod Settings", "RRSpeedInterval")
			.Default("2")
			.WithVisibleWhen(Setting::RRSpeedAboveOneHundred),
	};
}

void RiffRepeaterMod::OnInitialize(ModContext& c) {
	c.Commands().BindSetting(
		Setting::Key::Rewind,
		KeyEdge::Up,
		Availability::Active,
		[](ModContext& context, const KeyEvent&) { Rewind(context); },
		[](const ModContext& context, const KeyEvent&) {
			return context.IsOn(Setting::AllowRewind) && GameState::Menus::IsInLASPlayingModes();
		});

	c.Commands().BindSetting(
		Setting::Key::LoopStart,
		KeyEdge::Down,
		Availability::Active,
		[this](ModContext&, const KeyEvent& event) { SetLoopStart(event); },
		[](const ModContext& context, const KeyEvent&) {
			return context.IsOn(Setting::AllowLooping) && GameState::Menus::IsInModesWithAllowedFastRiffRepeater();
		},
		"Loop Start Point Set");

	c.Commands().BindSetting(
		Setting::Key::LoopEnd,
		KeyEdge::Down,
		Availability::Active,
		[this](ModContext&, const KeyEvent& event) { SetLoopEnd(event); },
		[](const ModContext& context, const KeyEvent&) {
			return context.IsOn(Setting::AllowLooping) && GameState::Menus::IsInModesWithAllowedFastRiffRepeater();
		},
		"Loop End Point Set");

	c.Commands().BindSetting(
		Setting::Key::RRSpeed,
		KeyEdge::Down,
		Availability::Active,
		[](ModContext& context, const KeyEvent& event) { ChangeSpeed(context, event); },
		[](const ModContext& context, const KeyEvent&) {
			return context.IsOn(Setting::RRSpeedAboveOneHundred) &&
				GameState::Menus::IsInModesWithAllowedFastRiffRepeater() &&
				RiffRepeater::loggedCurrentSongID;
		});
}

void RiffRepeaterMod::Rewind(const ModContext& c) {
	auto seekTo = static_cast<AkTimeMs>((SongTimer::SongTimer() * 1000) - c.Int(Setting::RewindBy) - c.Int(Setting::RewindLeadup));
	if (seekTo < 0) seekTo = 0;
	Wwise::SoundEngine::SeekOnEvent(std::string("Play_" + GameState::GetSongKey()).c_str(), 0x1234, seekTo, false);

	const AkTimeMs greyNoteTimerMs = seekTo - c.Int(Setting::RewindLeadup);
	SongTimer::SetGreyNoteTimer(greyNoteTimerMs / 1000.f);

	LOG_INFO("(REWIND) Seeked to " << seekTo << "ms." << std::endl);
}

void RiffRepeaterMod::SetLoopStart(const KeyEvent& event) {
	if (event.control) {
		loopStart = loopEnd = 0.f;
		return;
	}

	loopStart = SongTimer::SongTimer();
	if (loopEnd <= loopStart) loopEnd = 0.f;
}

void RiffRepeaterMod::SetLoopEnd(const KeyEvent& event) {
	if (event.control) {
		loopEnd = 0.f;
		return;
	}

	loopEnd = SongTimer::SongTimer();
	if (loopEnd <= loopStart) loopEnd = 0.f;
}

void RiffRepeaterMod::OnSongExit(ModContext&) {
	loopStart = 0.f;
	roughLoopStart = 0.f;
	loopEnd = 0.f;
}

void RiffRepeaterMod::OnDisabled(ModContext&) {
	loopStart = 0.f;
	roughLoopStart = 0.f;
	loopEnd = 0.f;
}

void RiffRepeaterMod::ChangeSpeed(const ModContext& c, const KeyEvent& event) {
	const float interval = static_cast<float>(c.Int(Setting::RRSpeedInterval));

	float realSongSpeed = RiffRepeater::GetSpeed(true);
	realSongSpeed += event.control ? -interval : interval;
	realSongSpeed = std::clamp(realSongSpeed, 25.f, 1600.f);

	RiffRepeater::SetSpeed(realSongSpeed, true);
	RiffRepeater::EnableTimeStretch();
	RiffRepeater::saveNewRRSpeedToFile = true;

	LOG_INFO("Triggered Mod: Song Speed set to " << realSongSpeed << "%" << std::endl);
}

void RiffRepeaterMod::OnTick(ModContext& c) {
	SyncLinearSpeeds(c);
}

// Patch (or revert) the linear Riff Repeater speed logic to track the setting.
void RiffRepeaterMod::SyncLinearSpeeds(ModContext& c) {
	if (c.IsOn(Setting::LinearRiffRepeater) && !RiffRepeater::currentlyEnabled_LinearRR) {
		RiffRepeater::EnableLinearSpeeds();
	}
	else if (c.IsOff(Setting::LinearRiffRepeater) && RiffRepeater::currentlyEnabled_LinearRR) {
		RiffRepeater::DisableLinearSpeeds();
	}
}

// Leaving a song for any menu other than the score screens drops the >100% time stretch.
void RiffRepeaterMod::OnMenuTick(ModContext& c) {
	if (!GameState::Menus::IsInScoreMenus() && RiffRepeater::currentlyEnabled_Above100) {
		RiffRepeater::DisableTimeStretch();
	}

	UpdateLoopState(c);
	PublishHud(c);
}

void RiffRepeaterMod::OnSongTick(ModContext& c) {
	// First time we see this song, log its id so the >100% speed table can prep.
	if (RiffRepeater::readyToLogSongID && RiffRepeater::LogSongID(GameState::GetSongKey())) {
		RiffRepeater::readyToLogSongID = false;
	}

	if (c.IsOn(Setting::RRSpeedAboveOneHundred)) {
		RiffRepeater::EnableTimeStretch();
	}

	UpdateLoopState(c);
	PublishHud(c);
}

void RiffRepeaterMod::UpdateLoopState(ModContext& c) {
	if (!c.IsOn(Setting::AllowLooping) || (loopStart == 0.f && loopEnd == 0.f)) {
		return;
	}

	if (GameState::Menus::IsInLearnASongModes()) {
		if (c.phase == GamePhase::Song) {
			// Prevent the user from creating a loop that starts at a negative timestamp.
			const float leadUp = static_cast<float>(c.Int(Setting::LoopingLeadUp)) / 1000.f;
			if (leadUp >= loopStart) {
				roughLoopStart = 0.f;
			}
			else {
				roughLoopStart = loopStart - leadUp;
			}

			// If we are paused, reset the grey note timer.
			if (GameState::Menus::IsInLearnASongPauseModes()) {
				// Resets grey note timer to loopStart. This makes it so notes in the loop are not deactivated.
				// Deactivated notes are greyed out, and do not register with note detection.
				// As an added bonus the game also automatically adds a bit of lead time so the player has some time to prepare.
				if (SongTimer::GetGreyNoteTimer() != loopStart) {
					SongTimer::SetGreyNoteTimer(loopStart);
				}
			}
			// If not paused AND a full loop is armed, we are actively watching for the loop end. Ask for a
			// tighter tick so the seek-back lands within a frame or two instead of up to one 250 ms tick;
			// then seek once we reach the end. The request self-clears when the loop is cleared.
			else if (loopStart != 0.f && loopEnd != 0.f) {
				c.RequestFastTick();
				if (SongTimer::SongTimer() >= loopEnd) {
					Wwise::SoundEngine::SeekOnEvent(std::string("Play_" + GameState::GetSongKey()).c_str(), 0x1234, static_cast<AkTimeMs>(roughLoopStart * 1000), false);
				}
			}
		}
	}
	else if (GameState::Menus::IsInModesWithAllowedFastRiffRepeater()) {
		// Reset loopStart and loopEnd to 0.f as the user wants to do a loop with RR, or is changing some settings.
		loopStart = 0.f;
		loopEnd = 0.f;
	}
}

void RiffRepeaterMod::PublishHud(ModContext& c) {
	const bool speedVisible = (c.IsOn(Setting::RRSpeedAboveOneHundred) && RiffRepeater::loggedCurrentSongID &&
		(GameState::Menus::IsInModesWithAllowedFastRiffRepeater() || GameState::Menus::IsOnScoreScreens())) ||
		RiffRepeater::currentlyEnabled_Above100;

	HudText speedSnapshot;
	speedSnapshot.visible = speedVisible;
	if (speedVisible) {
		const float speed = RiffRepeater::GetSpeed(true);
		speedSnapshot.text = "Song Speed: " + std::to_string(static_cast<int>(roundf(speed))) + "%";
	}
	
	c.Hud().Set("rr-speed", { HudAnchor::TopCenter, 0 }, std::move(speedSnapshot));

	const bool loopVisible = c.IsOn(Setting::AllowLooping) &&
		(loopStart != 0.f || loopEnd != 0.f) &&
		GameState::Menus::IsInLearnASongModes();

	HudText loopSnapshot;
	loopSnapshot.visible = loopVisible;
	if (loopVisible) {
		loopSnapshot.text = "Loop: " + SongTimer::FormatTime(loopStart) + " - " + SongTimer::FormatTime(loopEnd);
	}
	c.Hud().Set("loop-timer", { HudAnchor::TopCenter, 10 }, std::move(loopSnapshot));
}


static Framework::ModRegistrar<RiffRepeaterMod> _riffRepeaterReg;
