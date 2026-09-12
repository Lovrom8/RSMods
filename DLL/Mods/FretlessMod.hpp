#pragma once

#include "../Framework/Framework.hpp"

class FretlessMod : public Framework::IMod {
public:
	MOD_ID(FretlessMod)
	bool IsEnabled(const Framework::ModContext& c) const override;
	Framework::SettingDefs Settings() const override;
	void OnInitialize(Framework::ModContext& c) override;
};
