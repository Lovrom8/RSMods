#pragma once

#include "../Framework/Framework.hpp"

// Boot selection priority: configured name, remembered id, then newest local profile.
// Unreadable or absent profiles fall back to the native selector.
class ProfileAutoLoadMod : public Framework::IMod {
public:
	MOD_ID(ProfileAutoLoadMod)

	bool IsEnabled(const Framework::ModContext&) const override { return false; }
	Framework::SettingDefs Settings() const override;

	// Install selector hooks at DLL attach.
	static void Install();

	// Dispatch queued selection from EndScene on the game thread.
	static void OnFrame();
};
