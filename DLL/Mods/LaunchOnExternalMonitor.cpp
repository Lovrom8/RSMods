#include "../stdafx.h"
#include "LaunchOnExternalMonitor.hpp"

using Framework::ModContext;
namespace Setting = Settings::Setting;

bool LaunchOnExternalMonitorMod::IsEnabled(const ModContext& c) const {
	return c.IsOn(Setting::SecondaryMonitor);
}

Framework::SettingDefs LaunchOnExternalMonitorMod::Settings() const {
	return {
		Framework::SettingDef::Toggle(Setting::SecondaryMonitor, "SecondaryMonitor", "Launch on Secondary Monitor"),
		Framework::SettingDef::Numeric(Setting::SecondaryMonitorXPosition, "Secondary Monitor X Position")
			.Ini("Mod Settings", "SecondaryMonitorXPosition")
			.Default("0")
			.WithVisibleWhen(Setting::SecondaryMonitor),
		Framework::SettingDef::Numeric(Setting::SecondaryMonitorYPosition, "Secondary Monitor Y Position")
			.Ini("Mod Settings", "SecondaryMonitorYPosition")
			.Default("0")
			.WithVisibleWhen(Setting::SecondaryMonitor)
	};
}

void LaunchOnExternalMonitorMod::OnMenuTick(ModContext& c) {
	MoveOnce(c);
}

void LaunchOnExternalMonitorMod::OnSongTick(ModContext& c) {
	MoveOnce(c);
}

// One-shot: move the window to the configured position on the first loaded tick after enable.
// SendRocksmithToScreen blocks until the window handle exists, so this only runs once the game
// is loaded (menu/song ticks), never during the Loading phase.
void LaunchOnExternalMonitorMod::MoveOnce(ModContext& c) {
	if (moved)
		return;

	SendRocksmithToScreen(
		c.Int(Setting::SecondaryMonitorXPosition),
		c.Int(Setting::SecondaryMonitorYPosition));
	moved = true;
}

// Move Rocksmith to a separate monitor. startX/startY are the top-left corner of the target screen.
void LaunchOnExternalMonitorMod::SendRocksmithToScreen(int startX, int startY) {
	HWND hWnd = D3DHooks::GetGameWindow();

	while (!hWnd) {
		Sleep(500);
		hWnd = FindWindowA(nullptr, "Rocksmith 2014");
	}

	// Set the window's top left corner to startX and startY, preserving its current size.
	RECT windowSize;
	if (GetWindowRect(hWnd, &windowSize)) {
		SetWindowPos(hWnd, HWND_TOP, startX, startY, windowSize.right - windowSize.left, windowSize.bottom - windowSize.top, SWP_SHOWWINDOW);
	}
}

static Framework::ModRegistrar<LaunchOnExternalMonitorMod> _launchOnExternalMonitorReg;
