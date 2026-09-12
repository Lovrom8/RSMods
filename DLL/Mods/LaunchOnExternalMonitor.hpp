#pragma once

#include "../Framework/Framework.hpp"

class LaunchOnExternalMonitorMod : public Framework::IMod {
public:
	MOD_ID(LaunchOnExternalMonitorMod)
	bool IsEnabled(const Framework::ModContext& c) const override;
	Framework::SettingDefs Settings() const override;

	void OnMenuTick(Framework::ModContext& c) override;
	void OnSongTick(Framework::ModContext& c) override;

private:
	void MoveOnce(Framework::ModContext& c);
	void SendRocksmithToScreen(int startX, int startY);

	bool moved = false;
};
