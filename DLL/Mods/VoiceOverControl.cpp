#include "VoiceOverControl.hpp"

namespace VoiceOverControl {
	/// <summary>
	/// Play VoiceOver (struct) through Wwise::SoundEngine::PostEvent()
	/// </summary>
	/// <param name="voiceOver"> - VoiceOver struct</param>
	void PlayVoiceOver(VoiceOver voiceOver) {
		// Send PostEvent with VoiceOver.EventName
		Wwise::SoundEngine::PostEvent(voiceOver.EventName.c_str(), AK_INVALID_GAME_OBJECT, 0, NULL, NULL, 0, NULL, 0);
		Wwise::SoundEngine::PostEvent(voiceOver.EventName.c_str(), 0x1234, 0, NULL, NULL, 0, NULL, 0);

		std::cout << "Played VoiceOver: " << voiceOver.Text << std::endl;
	}

	/// <summary>
	/// Play VoiceOver (string) through Wwise::SoundEngine::PostEvent()
	/// </summary>
	/// <param name="voiceOver"> - VoiceOver name.</param>
	void PlayVoiceOver(std::string voiceOver) {
		Wwise::SoundEngine::PostEvent(voiceOver.c_str(), AK_INVALID_GAME_OBJECT, 0, NULL, NULL, 0, NULL, 0);
		Wwise::SoundEngine::PostEvent(voiceOver.c_str(), 0x1234, 0, NULL, NULL, 0, NULL, 0);

		std::cout << "Played VoiceOver: " << voiceOver << std::endl;
	}
}