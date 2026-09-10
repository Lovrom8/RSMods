#pragma once

#include "../Framework/Framework.hpp"

class RemoveInlaysMod : public Framework::IMod {
public:
	MOD_ID(RemoveInlaysMod)
	bool IsEnabled(const Framework::ModContext& c) const override;
	void OnInitialize(Framework::ModContext& c) override;
};
