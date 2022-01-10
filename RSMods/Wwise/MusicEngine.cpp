#include "MusicEngine.hpp"

namespace Wwise::MusicEngine {
	void GetDefaultInitSettings(AkMusicSettings* out_settings) {
		tMusic_GetDefaultInitSettings func = (tMusic_GetDefaultInitSettings)Wwise::Exports::func_Wwise_Music_GetDefaultInitSettings;
		return func(out_settings);
	}

	AKRESULT GetPlayingSegmentInfo(AkPlayingID in_playingID, AkSegmentInfo* out_segmentInfo, bool in_bExtrapolate) {
		tMusic_GetPlayingSegmentInfo func = (tMusic_GetPlayingSegmentInfo)Wwise::Exports::func_Wwise_Music_GetPlayingSegmentInfo;
		return func(in_playingID, out_segmentInfo, in_bExtrapolate);
	}

	AKRESULT Init(AkMusicSettings* in_pSettings) {
		tMusic_Init func = (tMusic_Init)Wwise::Exports::func_Wwise_Music_Init;
		return func(in_pSettings);
	}

	void Term() {
		tMusic_Term func = (tMusic_Term)Wwise::Exports::func_Wwise_Music_Term;
		return func();
	}
}