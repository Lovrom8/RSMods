#include "../stdafx.h"
#include "AllowAudioInBackgroundMod.hpp"

#include <stdexcept>

using Framework::ModContext;
using Framework::SettingDef;
using Framework::SettingDefs;
namespace Setting = Settings::Setting;

SettingDefs AllowAudioInBackgroundMod::Settings() const {
    return {
        SettingDef::Toggle(Setting::AllowAudioInBackground, "AllowAudioInBackground", "Allow Audio in Background")
            .Hint("Keeps game audio playing when Rocksmith loses window focus.")
        	.Heading(Framework::SettingHeading::Audio)
    };
}

bool AllowAudioInBackgroundMod::IsEnabled(const ModContext& c) const {
    return c.IsOn(Setting::AllowAudioInBackground);
}

void AllowAudioInBackgroundMod::OnEnabled(ModContext&) {
    if (VolumeControl::AllowAltTabbingWithAudio() != MemUtil::PatchResult::Installed)
        throw std::runtime_error("background-audio patch could not be installed");
}

void AllowAudioInBackgroundMod::OnDisabled(ModContext&) {
    if (VolumeControl::DisableAltTabbingWithAudio() != MemUtil::PatchResult::Original)
        throw std::runtime_error("background-audio patch could not be restored");
}

static Framework::ModRegistrar<AllowAudioInBackgroundMod> _allowAudioInBackgroundReg;
