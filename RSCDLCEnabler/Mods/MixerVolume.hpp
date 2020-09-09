#pragma once

#include "windows.h"
#include <vector>
#include <string>
#include <iostream>
#include <fstream>
#include "../DirectX/d3dx9.h"
#include "../DirectX/d3d9types.h"

#include "../MemUtil.hpp"
#include "../Offsets.hpp"
#include "../Structs.hpp"
#include "../Settings.hpp"

namespace MixerVolume {
	float SongVolume();
	float PlayerOneGuitarVolume();
	float PlayerOneBassVolume();
	float PlayerTwoGuitarVolume();
	float PlayerTwoBassVolume();
	float MicrophoneVolume();
	float VoiceOverVolume();
	float SFXVolume();
}