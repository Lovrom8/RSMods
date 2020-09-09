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
	float SongVolume();
	float PlayerOneGuitarVolume();
	float PlayerOneBassVolume();
	float PlayerTwoGuitarVolume();
	float PlayerTwoBassVolume();
	float MicrophoneVolume();
	float VoiceOverVolume();
	float SFXVolume();

	inline std::map<int, float> mixerVolumeSelections{
		{0, SongVolume()},
		{1, PlayerOneGuitarVolume()},
		{2, PlayerOneBassVolume()},
		{3, PlayerTwoGuitarVolume()},
		{4, PlayerTwoBassVolume()},
		{5, MicrophoneVolume()},
		{6, VoiceOverVolume()},
		{7, SFXVolume()}
	};

	inline std::map<int, char*> mixerInternalNames{ // Needs to be char* as that's what SetRTPCValue needs.
		{0, "Mixer_Music"} // SongVolume();
		{1, "Mixer_Player1"} // PlayerOneGuitarVolume()
		{2, "Mixer_Player1"} // PlayerOneBassVolume();
		{3, "Mixer_Player2"} // PlayerTwoGuitarVolume();
		{4, "Mixer_Player2"} // PlayerTwoBassVolume();
		{5, "Mixer_Mic"} // MicrophoneVolume();
		{6, "Mixer_VO"} // VoiceOverVolume();
		{7, "Mixer_SFX"} // SFXVolume();
	};

	inline std::map<int, std::string> drawTextName{
		{0, "Song Volume: "},
		{1, "Player 1 Guitar Volume: "},
		{2, "Player 1 Bass Volume: "},
		{3, "Player 2 Guitar Volume: "},
		{4, "Player 2 Bass Volume: "},
		{5, "Microphone Volume: "},
		{6, "Voice-Over Volume: "},
		{7, "SFX Volume: "}
	};
}