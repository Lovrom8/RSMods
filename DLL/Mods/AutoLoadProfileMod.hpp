#pragma once

#include "../Framework/Framework.hpp"

// "Fork in the toaster": during boot, auto-advances through the profile-select / login dialogs so the
// game lands in a profile without user input. Loading-phase only: it drives the menus while the game
// is still coming up, and there is nothing to revert once the main menu is reached.
class AutoLoadProfileMod : public Framework::IMod {
public:
	MOD_ID(AutoLoadProfileMod)
	bool IsEnabled(const Framework::ModContext& c) const override;
	Framework::SettingDefs Settings() const override;

	void OnTick(Framework::ModContext& c) override;

private:
	void LoadSpecificProfile(Framework::ModContext& c);

	// Latches once the pinned profile turns out not to exist, so we stop cycling the list forever.
	bool requestedProfileMissing = false;
};
