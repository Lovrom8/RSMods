// DLL Hijack. Do NOT remove this line!
#include "Proxy.hpp"

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
#include "Wwise/SoundEngine.hpp"
#include "Wwise/Logging.hpp"
#include "CC/ControlServer.hpp"
#include "Tests.hpp"
#include "Log.hpp"

// Windows Libraries
#include <chrono>
#include <process.h>
#include <iostream>
#include <fstream>
#include <thread>

// Mods
#include "Mods/Enumeration.hpp"
#include "Mods/CustomSongTitles.hpp"
#include "Mods/CollectColors.hpp"
#include "Mods/GuitarSkeletons.hpp"
#include "Mods/GuitarSpeak.hpp"
#include "Mods/DisableControllers.hpp"
#include "Mods/Midi.hpp"
#include "Mods/VolumeControl.hpp"
#include "Mods/LaunchOnExternalMonitor.hpp"
#include "Mods/VoiceOverControl.hpp"
#include "Mods/RiffRepeater.hpp"
#include "Mods/AudioDevices.hpp"
#include "Mods/BugPrevention.hpp"
#include "Mods/TrueTuning.hpp"

std::vector<std::string> effectQueue;
std::vector<std::string> enabledEffects;

bool takenScreenshotOfThisScreen = false; // Has the user taken a screenshot of their scores in this menu
bool saveNewRRSpeedToFile = false;
inline bool ImGuiInit = false; // Has ImGui already been init? If we close the game with this being false, then we get an assert.
inline const double DefaultNSPTimeLimit = 10.9899997711182; // The default time for NSP.

// Volume adjustment mod
bool displayMixer = false;
bool displayCurrentVolume = false;
auto displayVolumeStartTime = std::chrono::steady_clock::time_point(); // Defaults to epoch time
unsigned int currentVolumeIndex = 0; // Mixer volume to change. 0 - Master, 1 - Song, 2 - P1, 3 - P2, 4 - Mic, 5 - VO, 6 - SFX

// Looping functionality.
inline float loopStart = NULL; // The start of the loop, as specified by the user.
inline float roughLoopStart = NULL; // Just like loopStart, except we account for the lead-in time.
inline float loopEnd = NULL; // The end of the loop, as specified by the user.

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

inline std::string selectedUser = "";

bool Contains(std::string str, const char* key){ return str.find(key) != std::string::npos; }