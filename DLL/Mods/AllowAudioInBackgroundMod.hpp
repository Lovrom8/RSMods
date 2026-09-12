#pragma once

#include "VolumeControl.hpp"

#include "../Framework/Framework.hpp"

class AllowAudioInBackgroundMod : public Framework::IMod {
public:
	MOD_ID(AllowAudioInBackgroundMod)

	bool IsEnabled(const Framework::ModContext& c) const override;
	Framework::SettingDefs Settings() const override;

	void OnEnabled(Framework::ModContext& c) override;
	void OnDisabled(Framework::ModContext& c) override;
};