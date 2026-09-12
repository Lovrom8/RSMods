#pragma once

#include "../Framework/Framework.hpp"

class RainbowNotesMod : public Framework::IMod {
public:
	MOD_ID(RainbowNotesMod)

	bool IsEnabled(const Framework::ModContext& c) const override;
	Framework::SettingDefs Settings() const override;

	void OnInitialize(Framework::ModContext& c) override;
};
