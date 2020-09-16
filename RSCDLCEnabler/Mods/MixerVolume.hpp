#pragma once

#include "windows.h"
#include <vector>
#include <string>
#include <iostream>
#include <fstream>

#include "../MemUtil.hpp"
#include "../Offsets.hpp"
#include "../Structs.hpp"

namespace MixerVolume {
	float SongVolume(); // 0
	float PlayerOneGuitarVolume(); // 1
	float PlayerOneBassVolume(); // 2
	float PlayerTwoGuitarVolume(); // 3
	float PlayerTwoBassVolume(); // 4
	float MicrophoneVolume(); // 5
	float VoiceOverVolume(); // 6
	float SFXVolume(); // 7
	float SelectVolume(std::string volumeToSelect);
	int GetIndex(std::vector<std::string> vector, std::string string);

	inline std::vector<std::string> mixerInternalNames = { // Needs to be char* as that's what SetRTPCValue needs.
		{"Mixer_Music"}, // SongVolume();
		{"Mixer_Player1"}, // PlayerOneGuitarVolume()
		{"Mixer_Player1"}, // PlayerOneBassVolume();
		{"Mixer_Player2"}, // PlayerTwoGuitarVolume();
		{"Mixer_Player2"}, // PlayerTwoBassVolume();
		{"Mixer_Mic"}, // MicrophoneVolume();
		{"Mixer_VO"}, // VoiceOverVolume();
		{"Mixer_SFX"} // SFXVolume();
		// There is a mention of "Master_Volume". Maybe this can be changed to have Rocksmith fit into the same standard as all modern games (some people say RS is quieter than other games even at the same volume, assumed to be a console issue / reason).
	};

	inline std::vector<std::string> drawTextName = {
		{"Song Volume: "},
		{"Player 1 Guitar Volume: "},
		{"Player 1 Bass Volume: "},
		{"Player 2 Guitar Volume: "},
		{"Player 2 Bass Volume: "},
		{"Microphone Volume: "},
		{"Voice-Over Volume: "},
		{"SFX Volume: "}
	};
}