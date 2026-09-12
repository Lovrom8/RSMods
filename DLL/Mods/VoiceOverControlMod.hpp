#pragma once

#include "../Framework/Framework.hpp"

class VoiceOverControlMod : public Framework::IMod {
public:
	MOD_ID(VoiceOverControlMod)

	void OnInitialize(Framework::ModContext& c) override;

private:
	void DrawMenu();
};
