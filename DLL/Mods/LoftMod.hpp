#pragma once

#include "../Framework/Framework.hpp"

class LoftMod : public Framework::IMod {
public:
	MOD_ID(LoftMod)
	bool IsEnabled(const Framework::ModContext& c) const override;
	Framework::SettingDefs Settings() const override;

	void OnInitialize(Framework::ModContext& c) override;
	void OnDisabled(Framework::ModContext& c) override;
	void OnSongTick(Framework::ModContext& c) override;
	void OnMenuTick(Framework::ModContext& c) override;

private:
	void ApplyAlwaysOn(Framework::ModContext& c);

	bool loftOff = false;
};
