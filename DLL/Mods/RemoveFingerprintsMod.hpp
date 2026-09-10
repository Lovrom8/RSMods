#pragma once

#include "../Framework/Framework.hpp"

class RemoveFingerprintsMod : public Framework::IMod {
public:
	MOD_ID(RemoveFingerprintsMod)
	bool IsEnabled(const Framework::ModContext& c) const override;
	void OnInitialize(Framework::ModContext& c) override;
};
