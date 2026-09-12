#include "../stdafx.h"
#include "AllowAudioInBackgroundMod.hpp"

using Framework::ModContext;
using Framework::SettingDef;
using Framework::SettingDefs;
namespace Setting = Settings::Setting;

SettingDefs AllowAudioInBackgroundMod::Settings() const {
    return {
        SettingDef::Toggle(Setting::AllowAudioInBackground, "AllowAudioInBackground", "Allow Audio in Background")
    };
}

bool AllowAudioInBackgroundMod::IsEnabled(const ModContext& c) const {
    return c.IsOn(Setting::AllowAudioInBackground);
}

void AllowAudioInBackgroundMod::OnEnabled(ModContext&) {
    VolumeControl::AllowAltTabbingWithAudio();
}

void AllowAudioInBackgroundMod::OnDisabled(ModContext&) {
    VolumeControl::DisableAltTabbingWithAudio();
}

static Framework::ModRegistrar<AllowAudioInBackgroundMod> _allowAudioInBackgroundReg;
