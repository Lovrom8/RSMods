#pragma once

#include "../Framework/Framework.hpp"

class NonStopPlayTimerMod : public Framework::IMod {
public:
    MOD_ID(NonStopPlayTimerMod)
    Framework::SettingDefs Settings() const override;

    void OnEnabled(Framework::ModContext& c) override;
    void OnDisabled(Framework::ModContext& c) override;
    void OnSettingsChanged(Framework::ModContext& c) override;

private:
    void ApplyTimer(Framework::ModContext& c);

    const double DefaultTimeLimit = 10.9899997711182; // The default pre-song timer for Non-Stop Play.

    bool active = false;
};