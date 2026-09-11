#include "../stdafx.h"
#include "AutoLoadProfileMod.hpp"
#include "../Keyboard.hpp"

using Framework::ModContext;
using Framework::GamePhase;
using Framework::SettingDefs;
using Framework::SettingDef;
namespace Setting = Settings::Setting;

SettingDefs AutoLoadProfileMod::Settings() const {
	return {
		SettingDef::Toggle(Setting::ForceProfileEnabled, "ForceProfileLoad", "Auto Load Profile"),
		SettingDef::String(Setting::ProfileToLoad, "Profile to Load")
			.WithVisibleWhen(Setting::ForceProfileEnabled)
	};
}

bool AutoLoadProfileMod::IsEnabled(const ModContext& c) const {
	return c.IsOn(Setting::ForceProfileEnabled);
}

// Only acts while the game is still booting toward the main menu. Reads GameState::currentMenu, which the
// loading-state pass refreshes (via the memory-safe GetCurrentMenu) before the registry ticks.
void AutoLoadProfileMod::OnTick(ModContext& c) {
	if (c.phase != GamePhase::Loading || requestedProfileMissing ||
		GameState::Menus::IsInMenusWithDisallowedAutoEnter()) {
		return;
	}

	// Skip the UPlay login dialog - depending on the menu it might need either ESC or Enter, so spam both.
	if (GameState::currentMenu == "SelectionListDialog" ||
		GameState::currentMenu == "UplayLoginDialog") {
		Keyboard::SendEscapeKey();
		Keyboard::AutoEnterGame();
	}
	else if (c.Value(Setting::ProfileToLoad) != "" && GameState::currentMenu == "ProfileSelect") {
		LoadSpecificProfile(c); // The user pinned a profile to always load.
	}
	else { // No preference - just select the first / top profile.
		Keyboard::AutoEnterGame();
	}
}

// Walk the profile list until the pinned profile is highlighted (then enter), or give up if it is missing.
void AutoLoadProfileMod::LoadSpecificProfile(ModContext& c) {
	const std::string selectedUser = GameState::CurrentSelectedUser();

	if (selectedUser == c.Value(Setting::ProfileToLoad)) {
		Keyboard::AutoEnterGame();
	}
	else if (selectedUser == "New profile") {
		LOG_ERROR("(Auto Load) Invalid Profile Name" << std::endl); // The profile they're looking for doesn't exist :(
		requestedProfileMissing = true;
	}
	else { // Not our profile yet. Move down one.
		Keyboard::PressDownArrowKey();
	}
}

static Framework::ModRegistrar<AutoLoadProfileMod> _autoLoadProfileReg;
