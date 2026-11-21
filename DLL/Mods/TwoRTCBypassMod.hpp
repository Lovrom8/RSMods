#pragma once

#include "../Framework/Framework.hpp"
#include "../SitePatchGroup.hpp"

class TwoRTCBypassMod : public Framework::IMod {
public:
	MOD_ID(TwoRTCBypassMod)

	bool IsEnabled(const Framework::ModContext& c) const override;
	Framework::SettingDefs Settings() const override;

	void OnEnabled(Framework::ModContext& c) override;
	void OnDisabled(Framework::ModContext& c) override;

private:
	void SetTwoRTCBypass(bool enable);

	SitePatch::PatchedSiteGroup bypassPatch_{ "TWO RTC" };
};
