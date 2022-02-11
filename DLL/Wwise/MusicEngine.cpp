#include "MusicEngine.hpp"

// Most accurate documentation is available here: https://www.audiokinetic.com/library/2015.1.9_5624/?source=SDK&id=namespace_a_k_1_1_music_engine.html

namespace Wwise::MusicEngine {

	/// <summary>
	/// Get the music engine's default initialization settings values
	/// </summary>
	/// <param name="out_settings"> - Returned default platform-independent music engine settings</param>
	void GetDefaultInitSettings(AkMusicSettings* out_settings) {
		tMusic_GetDefaultInitSettings func = (tMusic_GetDefaultInitSettings)Wwise::Exports::func_Wwise_Music_GetDefaultInitSettings;
		return func(out_settings);
	}

	/// <summary>
	/// Query information on the active segment of a music object that is playing. Read more at: https://www.audiokinetic.com/library/2015.1.9_5624/?source=SDK&id=namespace_a_k_1_1_music_engine_aa2bcc4aa33f3d9aa8d2186bce00d1174.html
	/// </summary>
	/// <param name="in_playingID"> - Playing ID returned by AK::SoundEngine::PostEvent().</param>
	/// <param name="out_segmentInfo"> - Structure containing information about the active segment of the music structure that is playing.</param>
	/// <param name="in_bExtrapolate"> - Position is extrapolated based on time elapsed since last sound engine update.</param>
	/// <returns>AK_Success if there is a playing music structure associated with the specified playing ID.</returns>
	AKRESULT GetPlayingSegmentInfo(AkPlayingID in_playingID, AkSegmentInfo* out_segmentInfo, bool in_bExtrapolate) {
		tMusic_GetPlayingSegmentInfo func = (tMusic_GetPlayingSegmentInfo)Wwise::Exports::func_Wwise_Music_GetPlayingSegmentInfo;
		return func(in_playingID, out_segmentInfo, in_bExtrapolate);
	}

	/// <summary>
	/// Initialize the music engine. Needs to be called AFTER SoundEngine::Init() is run.
	/// </summary>
	/// <param name="in_pSettings"> - Initialization settings (can be NULL, to use the default values)</param>
	/// <returns>AK_Success if the Init was successful, AK_Fail otherwise.</returns>
	AKRESULT Init(AkMusicSettings* in_pSettings) {
		tMusic_Init func = (tMusic_Init)Wwise::Exports::func_Wwise_Music_Init;
		return func(in_pSettings);
	}

	/// <summary>
	/// Terminate the music engine. Needs to be called BEFORE SoundEngine::Term() is run.
	/// </summary>
	void Term() {
		tMusic_Term func = (tMusic_Term)Wwise::Exports::func_Wwise_Music_Term;
		return func();
	}
}