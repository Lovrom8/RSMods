#pragma once

#include "../Framework/Framework.hpp"

class CalibrationMod : public Framework::IMod {
public:
	MOD_ID(CalibrationMod)

	void OnInitialize(Framework::ModContext& c) override;

private:
	void DrawMenu();
};
