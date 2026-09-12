#pragma once

#include "../QualityOfLife.hpp"
#include "../Framework/Framework.hpp"

class TwoRTCBypassMod : public Framework::IMod {
public:
	MOD_ID(TwoRTCBypassMod)

	bool IsEnabled(const Framework::ModContext& c) const override;
	Framework::SettingDefs Settings() const override;

	void OnEnabled(Framework::ModContext& c) override;
	void OnDisabled(Framework::ModContext& c) override;

private:
	void SetTwoRTCBypass(bool enable);

	unsigned char originalBytes[25]{};
	bool capturedOriginal = false;
};
