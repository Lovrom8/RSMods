#pragma once

#include "../Framework/Framework.hpp"

class GuitarSpeakMod : public Framework::IMod {
public:
	MOD_ID(GuitarSpeakMod)

	void OnMenuTick(Framework::ModContext& c) override;
	void OnSongTick(Framework::ModContext& c) override;

	Framework::SettingDefs Settings() const override;

private:
	bool guitarSpeakPresent = false;
};
