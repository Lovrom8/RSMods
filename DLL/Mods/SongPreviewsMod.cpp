#include "../stdafx.h"
#include "SongPreviewsMod.hpp"

using Framework::ModContext;
using Framework::SettingDef;
using Framework::SettingDefs;
namespace Setting = Settings::Setting;

SettingDefs SongPreviewsMod::Settings() const {
    return {
        SettingDef::Toggle(Setting::SongPreviews, "SongPreviews", "Song Previews")
    };
}

bool SongPreviewsMod::IsEnabled(const ModContext& c) const {
    return c.IsOn(Setting::SongPreviews);
}

void SongPreviewsMod::OnEnabled(ModContext&) {
    VolumeControl::DisableSongPreviewAudio();
}

void SongPreviewsMod::OnDisabled(ModContext&) {
    VolumeControl::EnableSongPreviewAudio();
}

static Framework::ModRegistrar<SongPreviewsMod> _songPreviewsReg;
