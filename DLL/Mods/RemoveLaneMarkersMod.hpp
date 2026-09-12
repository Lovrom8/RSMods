#pragma once

#include "../Framework/Framework.hpp"

class RemoveLaneMarkersMod : public Framework::IMod {
public:
	MOD_ID(RemoveLaneMarkersMod)
	bool IsEnabled(const Framework::ModContext& c) const override;
	Framework::SettingDefs Settings() const override;
	void OnInitialize(Framework::ModContext& c) override;
};
