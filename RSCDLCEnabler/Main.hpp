// DLL Hijack. Do NOT remove this line!
#include "d3dx9_42.hpp"

// DLL Settings
#include "MemHelpers.hpp"
#include "Lib/Detours/detours.h"
#include "Offsets.hpp"
#include "MemUtil.hpp"
#include "Settings.hpp"
#include "Structs.hpp"
#include "Functions.hpp"
#include "D3D/D3D.hpp"
#include "D3D/D3DHooks.hpp"
#include "WwiseHijack.hpp"

// Lua
#include "Lua/lua.hpp"

// Windows Libraries
#include <process.h>
#include <iostream>
#include <fstream>

// Mods
#include "Mods/Enumeration.hpp"
#include "Mods/CustomSongTitles.hpp"
#include "Mods/CollectColors.hpp"
#include "Mods/GuitarSkeletons.hpp"
#include "Mods/GuitarSpeak.hpp"

bool regenerateUserDefinedTexture = false;
bool drawMixerText = true;

// Mixer Values
inline unsigned int currentVolumeIndex = 0; // Mixer volume to change. 0 - Song, 1 - P1Guitar, 2 - P1Bass, 3 - P2Guitar, 4 - P2Bass, 5 - Mic, 6 - VO, 7 - SFX

inline std::vector<std::string> mixerInternalNames = { // Needs to be char* as that's what SetRTPCValue needs.
		{"Master_Volume"}, // Master Volume
		{"Mixer_Music"}, // Song Volume
		{"Mixer_Player1"}, // Player 1 Guitar & Bass (both are handled with this singular name)
		{"Mixer_Player2"}, // Player 2 Guitar & Bass (both are handled with this singular name)
		{"Mixer_Mic"}, // My Microphone Volume
		{"Mixer_VO"}, // Rocksmith Dad Voice Over
		{"Mixer_SFX"}, // Menu SFX Volume
};

inline std::vector<std::string> drawMixerTextName = {
	{"Master Volume: "},
	{"Song Volume: "},
	{"Player 1 Volume: "},
	{"Player 2 Volume: "},
	{"Microphone Volume: "},
	{"Voice-Over Volume: "},
	{"SFX Volume: "},
};

bool Contains(std::string str, const char* key){ return str.find(key) != std::string::npos; }