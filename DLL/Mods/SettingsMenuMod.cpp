#include "../stdafx.h"
#include "SettingsMenuMod.hpp"
#include "../Menu.hpp"

using Framework::ModContext;
using Framework::KeyEdge;
using Framework::Availability;
using Framework::KeyEvent;
using Framework::SettingDefs;
using Framework::SettingDef;
namespace Setting = Settings::Setting;

SettingDefs SettingsMenuMod::Settings() const {
	return {
		SettingDef::Toggle(Setting::SettingsMenuEnabled, Setting::SettingsMenuEnabled, "In-Game Settings Window",
			"Toggle Switches", "Toggle Switches", "on",
			"Press the key below in game to change mod settings without leaving Rocksmith.\n"
			"Changes made there apply right away and are saved like changes made here.")
			.Heading(Framework::SettingHeading::GameAndWindow),
		// F7/F8 are debug mesh-logging keys and F10 is the Windows menu key.
		Framework::KeyBind(Setting::Key::ToggleSettingsMenu, "Show In-Game Settings", "VK_F9"),
	};
}

bool SettingsMenuMod::IsEnabled(const ModContext& c) const {
	return c.IsOn(Setting::SettingsMenuEnabled);
}

void SettingsMenuMod::OnInitialize(ModContext& c) {
	c.Commands().BindSetting(
		Setting::Key::ToggleSettingsMenu,
		KeyEdge::Up,
		Availability::Initialized,
		[](ModContext&, const KeyEvent&) { Menu::menuEnabled = !Menu::menuEnabled; },
		// Switching it off from the open window must still let the key close it.
		[](const ModContext& context, const KeyEvent&) {
			return context.IsOn(Setting::SettingsMenuEnabled) || Menu::menuEnabled;
		},
		"Toggle in-game settings");
}

static Framework::ModRegistrar<SettingsMenuMod> _settingsMenuReg;
