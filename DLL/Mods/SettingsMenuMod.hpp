#pragma once

#include "../Framework/Framework.hpp"

// Owns the key that opens the in-game settings window (SettingsMenu), so it can be rebound like any mod key.
class SettingsMenuMod : public Framework::IMod {
public:
	MOD_ID(SettingsMenuMod)
	bool IsEnabled(const Framework::ModContext& c) const override;
	Framework::SettingDefs Settings() const override;

	void OnInitialize(Framework::ModContext& c) override;
};
