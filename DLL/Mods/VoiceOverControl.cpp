#include "VoiceOverControl.hpp"

namespace VoiceOverControl {
	/// <summary>
	/// Play VoiceOver (struct) through Wwise::SoundEngine::PostEvent()
	/// </summary>
	/// <param name="voiceOver"> - VoiceOver struct</param>
	void PlayVoiceOver(VoiceOver voiceOver) {
		_LOG_INIT;

		// Send PostEvent with VoiceOver.EventName
		Wwise::SoundEngine::PostEvent(voiceOver.EventName.c_str(), AK_INVALID_GAME_OBJECT, 0, NULL, NULL, 0, NULL, 0);
		Wwise::SoundEngine::PostEvent(voiceOver.EventName.c_str(), 0x1234, 0, NULL, NULL, 0, NULL, 0);

		_LOG("Played VoiceOver: " << voiceOver.Text << std::endl);
	}

	/// <summary>
	/// Play VoiceOver (string) through Wwise::SoundEngine::PostEvent()
	/// </summary>
	/// <param name="voiceOver"> - VoiceOver name.</param>
	void PlayVoiceOver(std::string voiceOver) {
		_LOG_INIT;

		Wwise::SoundEngine::PostEvent(voiceOver.c_str(), AK_INVALID_GAME_OBJECT, 0, NULL, NULL, 0, NULL, 0);
		Wwise::SoundEngine::PostEvent(voiceOver.c_str(), 0x1234, 0, NULL, NULL, 0, NULL, 0);

		_LOG("Played VoiceOver: " << voiceOver << std::endl);
	}
}