#pragma once

#include "../Framework/Framework.hpp"

// Skip routes boot sign-in directly to local profiles and suppresses RDV downloads.
// Ask retains native login and remembers a confirmed skip for later launches.
class UplayLoginMod : public Framework::IMod {
public:
	MOD_ID(UplayLoginMod)

	bool IsEnabled(const Framework::ModContext&) const override { return false; }
	Framework::SettingDefs Settings() const override;

	// Install sign-in hooks at DLL attach; unsupported builds remain unchanged.
	static void Install();
	// Active until the boot profile finishes loading.
	static bool IsSourceProfileLoading();
};
