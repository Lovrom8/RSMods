#include "../stdafx.h"
#include "ScreenShotScoresMod.hpp"
#include "../Keyboard.hpp"

using Framework::ModContext;
namespace Setting = Settings::Setting;

bool ScreenShotScoresMod::IsEnabled(const ModContext& c) const {
	return c.IsOn(Setting::ScreenShotScores);
}

void ScreenShotScoresMod::OnDisabled(ModContext&) {
	ResetCapture();
}

void ScreenShotScoresMod::OnMenuTick(ModContext&) {
	if (!GameState::Menus::IsInScoreMenus()) {
		if (!scoreMenu.empty()) {
			ResetCapture();
		}
		
		return;
	}

	const auto now = std::chrono::steady_clock::now();
	if (scoreMenu != GameState::currentMenu) {
		scoreMenu = GameState::currentMenu;
		captureAt = now + std::chrono::seconds(8);
		captured = false;
		return;
	}

	if (!captured && now >= captureAt) {
		Keyboard::TakeScreenshot();
		captured = true;
	}
}

void ScreenShotScoresMod::OnSongEnter(ModContext&) {
	ResetCapture();
}

void ScreenShotScoresMod::ResetCapture() {
	scoreMenu.clear();
	captureAt = {};
	captured = false;
}

static Framework::ModRegistrar<ScreenShotScoresMod> _screenShotScoresReg;
