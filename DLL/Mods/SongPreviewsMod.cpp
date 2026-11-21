#include "../stdafx.h"
#include "SongPreviewsMod.hpp"

#include <stdexcept>

using Framework::ModContext;
using Framework::SettingDef;
using Framework::SettingDefs;
namespace Setting = Settings::Setting;

SettingDefs SongPreviewsMod::Settings() const {
    return {
        SettingDef::Toggle(Setting::SongPreviews, "SongPreviews", "Song Previews")
            .Hint("Plays a short audio preview of a song while it is highlighted in the song list.")
        	.Heading(Framework::SettingHeading::Audio)
    };
}

bool SongPreviewsMod::IsEnabled(const ModContext& c) const {
    return c.IsOn(Setting::SongPreviews);
}

void SongPreviewsMod::OnEnabled(ModContext&) {
	const auto result = VolumeControl::DisableSongPreviewAudio();
	if (result != MemUtil::PatchResult::Installed)
		throw std::runtime_error(result == MemUtil::PatchResult::RollbackIncomplete
			? "song-preview patch outcome is indeterminate"
			: "song-preview patch was not installed");
}

void SongPreviewsMod::OnDisabled(ModContext&) {
	if (VolumeControl::EnableSongPreviewAudio() != MemUtil::PatchResult::Original)
		throw std::runtime_error("song-preview patch rollback is indeterminate");
}

static Framework::ModRegistrar<SongPreviewsMod> _songPreviewsReg;
