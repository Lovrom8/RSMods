#pragma once

#include <atomic>
#include "../Framework/Framework.hpp"

class GreenScreenWallMod : public Framework::IMod {
public:
	MOD_ID(GreenScreenWallMod)
	bool IsEnabled(const Framework::ModContext& c) const override;
	Framework::SettingDefs Settings() const override;
	void OnInitialize(Framework::ModContext& c) override;
	void OnSettingsChanged(Framework::ModContext& c) override;

	static void SetLessonWall(bool active);

private:
	static inline std::atomic_bool lessonWallActive = false;
	std::atomic_bool settingEnabled = false;
};
