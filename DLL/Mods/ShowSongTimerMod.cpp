#include "../stdafx.h"
#include "ShowSongTimerMod.hpp"
#include "../SongTimer.hpp"

using Framework::ModContext;
using Framework::GamePhase;
using Framework::KeyEdge;
using Framework::Availability;
using Framework::KeyEvent;
using Settings::When;
namespace Setting = Settings::Setting;

// It has to stay Active (and tick) in manual mode too, because the timer is published through 
// ctx.Hud() every tick and HUD elements cleared whenever a mod deactivates.
bool ShowSongTimerMod::IsEnabled(const ModContext& c) const {
	return c.IsOn(Setting::ShowSongTimerEnabled);
}

bool ShowSongTimerMod::IsAutomatic(const ModContext& c) {
	return c.When(Setting::ShowSongTimerWhen) == When::Automatic;
}

void ShowSongTimerMod::OnInitialize(ModContext& c) {
	automatic = IsAutomatic(c);

	c.Commands().BindSetting(
		Setting::Key::ShowSongTimer,
		KeyEdge::Up,
		Availability::Active,
		[this](ModContext&, const KeyEvent&) { shown = !shown; },
		{},
		"Show Song Timer");
}

// A mode flip mid-session behaves like the old activate/deactivate did: switching to automatic while in a
// song shows the timer, switching to manual hides it until the key raises it again.
void ShowSongTimerMod::OnSettingsChanged(ModContext& c) {
	const bool nowAutomatic = IsAutomatic(c);
	if (nowAutomatic == automatic) return;

	automatic = nowAutomatic;
	shown = automatic && c.phase == GamePhase::Song;
}

void ShowSongTimerMod::OnSongEnter(ModContext& c) {
	if (automatic) shown = true;
}

void ShowSongTimerMod::OnSongExit(ModContext& c) {
	if (automatic) shown = false;
}

void ShowSongTimerMod::OnMenuTick(ModContext& c) {
	Publish(c);
}

void ShowSongTimerMod::OnSongTick(ModContext& c) {
	Publish(c);
}

// Publish the current snapshot to the shared HUD.
void ShowSongTimerMod::Publish(ModContext& c) const {
	const float seconds = shown ? SongTimer::SongTimer() : 0.f;

	Framework::HudText snapshot;
	snapshot.visible = shown && seconds != 0.f;
	if (snapshot.visible) {
		snapshot.text = SongTimer::FormatTime(seconds);
	}

	c.Hud().Set("song-timer", { Framework::HudAnchor::TopRight }, std::move(snapshot));
}

static Framework::ModRegistrar<ShowSongTimerMod> _showSongTimerReg;
