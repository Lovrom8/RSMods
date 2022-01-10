#include "VoiceOverControl.hpp"

namespace VoiceOverControl {
	void PlayVoiceOver(VoiceOver voiceOver) {
		Wwise::SoundEngine::PostEvent(voiceOver.EventName.c_str(), AK_INVALID_GAME_OBJECT, 0, NULL, NULL, 0, NULL, 0);
		Wwise::SoundEngine::PostEvent(voiceOver.EventName.c_str(), 0x1234, 0, NULL, NULL, 0, NULL, 0);

		std::cout << "Played VoiceOver: " << voiceOver.Text << std::endl;
	}

	void PlayVoiceOver(std::string voiceOver) {
		Wwise::SoundEngine::PostEvent(voiceOver.c_str(), AK_INVALID_GAME_OBJECT, 0, NULL, NULL, 0, NULL, 0);
		Wwise::SoundEngine::PostEvent(voiceOver.c_str(), 0x1234, 0, NULL, NULL, 0, NULL, 0);

		std::cout << "Played VoiceOver: " << voiceOver << std::endl;
	}
}