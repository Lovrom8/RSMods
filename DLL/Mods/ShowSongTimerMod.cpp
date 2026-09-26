#include "../stdafx.h"
#include "ShowSongTimerMod.hpp"

using Framework::ModContext;
using Framework::KeyEdge;
using Framework::Availability;
using Framework::KeyEvent;
using Settings::When;
namespace Setting = Settings::Setting;

// This mod owns only the automatic mode; manual show/hide is driven by the ShowSongTimerKey keybinding,
// which stays independent of the mod. The timer only renders in a song (see GameOverlay::DisplaySongTimer).
bool ShowSongTimerMod::IsEnabled(const ModContext& c) const {
	return c.IsOn(Setting::ShowSongTimerEnabled) && c.When(Setting::ShowSongTimerWhen) == When::Automatic;
}

void ShowSongTimerMod::OnInitialize(ModContext& c) {
	c.Commands().BindSetting(
		Setting::Key::ShowSongTimer,
		KeyEdge::Up,
		Availability::Initialized,
		[](ModContext&, const KeyEvent&) {
			D3DHooks::showSongTimerOnScreen = !D3DHooks::showSongTimerOnScreen;
		},
		[](const ModContext& context, const KeyEvent&) {
			return context.IsOn(Setting::ShowSongTimerEnabled);
		},
		"Show Song Timer");
}

void ShowSongTimerMod::OnSongEnter(ModContext&) {
	D3DHooks::showSongTimerOnScreen = true;
}

void ShowSongTimerMod::OnSongExit(ModContext&) {
	D3DHooks::showSongTimerOnScreen = false;
}

static Framework::ModRegistrar<ShowSongTimerMod> _showSongTimerReg;
