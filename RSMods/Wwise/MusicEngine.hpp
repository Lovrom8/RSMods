#pragma once

#include "Types.hpp"
#include "Exports.hpp"

namespace Wwise::MusicEngine {
	void GetDefaultInitSettings(AkMusicSettings* out_settings);
	AKRESULT GetPlayingSegmentInfo(AkPlayingID in_playingID, AkSegmentInfo* out_segmentInfo, bool in_bExtrapolate);
	AKRESULT Init(AkMusicSettings* in_pSettings);
	void Term(void);
}
typedef void(__cdecl* tMusic_GetDefaultInitSettings)(AkMusicSettings* out_settings);
typedef AKRESULT(__cdecl* tMusic_GetPlayingSegmentInfo)(AkPlayingID in_playingID, AkSegmentInfo* out_segmentInfo, bool in_bExtrapolate);
typedef AKRESULT(__cdecl* tMusic_Init)(AkMusicSettings* in_pSettings);
typedef void(__cdecl* tMusic_Term)(void);