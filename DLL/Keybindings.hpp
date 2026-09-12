#pragma once

#include <Windows.h>

#include "Framework/Framework.hpp"
#include "Menu.hpp"
#include "Mods/VoiceOverControl.hpp"
#include "Twitch.hpp"
#include "CC/ControlServer.hpp"

namespace Keybindings {
	void HandleKeyUp(WPARAM keyPressed, LPARAM lParam);
	void HandleKeyDown(WPARAM keyPressed, LPARAM lParam);

	void InitializeCommands();
	void UpdateSettingsOnGUIChange(LPARAM lParam);
}
