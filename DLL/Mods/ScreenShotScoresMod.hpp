#pragma once

#include <chrono>
#include <string>

#include "../Framework/Framework.hpp"

class ScreenShotScoresMod : public Framework::IMod {
public:
	MOD_ID(ScreenShotScoresMod)
	bool IsEnabled(const Framework::ModContext& c) const override;
	Framework::SettingDefs Settings() const override;

	void OnDisabled(Framework::ModContext& c) override;
	void OnMenuTick(Framework::ModContext& c) override;
	void OnSongEnter(Framework::ModContext& c) override;

private:
	void ResetCapture();

	std::string scoreMenu;
	std::chrono::steady_clock::time_point captureAt{};
	bool captured = false;
};
