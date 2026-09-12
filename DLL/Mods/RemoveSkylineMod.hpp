#pragma once

#include <atomic>
#include "../Framework/Framework.hpp"

class RemoveSkylineMod : public Framework::IMod {
public:
	MOD_ID(RemoveSkylineMod)
	bool IsEnabled(const Framework::ModContext& c) const override;
	Framework::SettingDefs Settings() const override;

	void OnInitialize(Framework::ModContext& c) override;
	void OnEnabled(Framework::ModContext& c) override;
	void OnDisabled(Framework::ModContext& c) override;
	void OnSettingsChanged(Framework::ModContext& c) override;
	void OnMenuTick(Framework::ModContext& c) override;
	void OnSongTick(Framework::ModContext& c) override;

private:
	void ApplyStartup(Framework::ModContext& c);

	std::atomic<bool> toggleSkyline = false;
	std::atomic<bool> skylineOff = false;
	std::atomic<bool> drawSkylineInMenu = false;
};
