#include "../stdafx.h"
#include "ScreenShotScoresMod.hpp"
#include "../Keyboard.hpp"

using Framework::ModContext;
using Framework::SettingDefs;
using Framework::SettingDef;
namespace Setting = Settings::Setting;

SettingDefs ScreenShotScoresMod::Settings() const {
	return {
		SettingDef::Toggle(Setting::ScreenShotScores, "ScreenShotScores", "Screenshot Scores")
	};
}

bool ScreenShotScoresMod::IsEnabled(const ModContext& c) const {
	return c.IsOn(Setting::ScreenShotScores);
}

void ScreenShotScoresMod::OnDisabled(ModContext&) {
	ResetCapture();
}

void ScreenShotScoresMod::OnMenuTick(ModContext& c) {
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
