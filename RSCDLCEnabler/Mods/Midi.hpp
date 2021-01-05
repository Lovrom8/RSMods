#pragma once

#include "Windows.h"
#include <iostream>
#include <cstdlib>
#include <map>
#include <string>

#include "../Lib/Midi/RtMidi.h"
#include "../MemHelpers.hpp"

namespace Midi {
	void InitMidi();
	void GetMidiDeviceNames();
	void ReadMidiSettingsFromINI(std::string ChordsMode, int PedalToUse, std::string AutoTuneForSongDevice);
	bool SendProgramChange(char programChange = '\000');
	bool SendControlChange(char toePosition = '\000');
	void AutomateDownTuning();
	void AutomateTrueTuning();
	void RevertAutomatedTuning();
	void SendDataToThread_PC(char program, bool shouldWeSendPC = true);
	void SendDataToThread_CC(char toePosition, bool shouldWeSendCC = true);

	// Digitech Whammy DT
	void Digitech_Whammy_DT_Auto_Tuning(int highestTuning);
	void Digitech_Whammy_DT_Auto_TrueTune(int TrueTuning_Hertz); 

	// Digitech Whammy Bass
	void Digitech_Whammy_Bass_Auto_Tuning_And_TrueTuning(int highestTuning, int TrueTuning_Hertz);

	// Digitech Whammy
	void Digitech_Whammy_Auto_Tuning_And_TrueTuning(int highestTuning, int TrueTuning_Hertz);

	extern int MidiCC, MidiPC;
	inline bool scannedForMidiDevices = false;
	extern std::vector<MIDIOUTCAPSA> midiOutDevices; // All MIDI out devices currently connected
	extern int SelectedMidiDevice; 
	extern unsigned int NumberOfPorts;
	inline bool sendCC = false, sendPC = false;
	inline int dataToSendPC = 0, dataToSendCC = 0, lastCC = 0, lastPC = 666;
	inline int lastPC_TUNING = 0; // Only use if the song requires a tuning change AND a true tuning. (Hendrix Eb Standard)
	inline bool alreadyAutomatedTuningInThisSong = false, alreadyAutomatedTrueTuningInThisSong = false, userWantsToUseAutoTuning = false;
	inline int sleepFor = 33; // Sleep for 33ms or ~ 1/33rd of a second.
	inline unsigned int pedalToUse = 0; // 0 = No pedal. 1 = Whammy DT. 2 = Whammy Bass. 3 = Whammy. 4 = Dummy
};

// Midi Specifications
inline unsigned char programChangeStatus = 192, controlChangeStatus = 176, noteOnStatus = 144, noteOffStatus = 128;


// Digitech WHAMMY DT | Bass Whammy | Whammy
inline int DIGITECH_WHAMMY_CC_CHANNEL = 11;
inline bool DIGITECH_CHORDS_MODE = false;
inline std::map<char, char> DIGITECH_WHAMMY_DT_activeBypassMap = {

	// Whammy
	{0, 21}, // +2 OCT
	{1, 22}, // +1 OCT
	{2, 23}, // +5th
	{3, 24}, // +4th
	{4, 25}, // -2nd
	{5, 26}, // -4th
	{6, 27}, // -5th
	{7, 28}, // -1 OCT
	{8, 29}, // -2 OCT
	{9, 30}, // Dive Bomb

	// Detune
	{10, 31}, // Deep
	{11, 32}, // Shallow

	// Harmony (Up Pos || Down Pos)
	{12, 33}, // +2nd || +3rd
	{13, 34}, // +b 3rd || +3rd
	{14, 35}, // +3rd || +4th
	{15, 36}, // +4th || +5th
	{16, 37}, // +5th || +6th
	{17, 38}, // +5th || +7th
	{18, 39}, // -4th || -3rd
	{19, 40}, // -5th || -4th
	{20, 41}, // -1 OCT || +1 OCT

	// Drop Tune Effect
	{42, 60}, // +1
	{43, 61}, // +2
	{44, 62}, // +3
	{45, 63}, // +4
	{46, 64}, // +5
	{47, 65}, // +6
	{48, 66}, // +7
	{49, 67}, // +OCT
	{50, 68}, // +OCT + Dry

	{51, 69}, // -OCT + Dry || Nice :eyes:
	{52, 70}, // -OCT
	{53, 71}, // -7
	{54, 72}, // -6
	{55, 73}, // -5
	{56, 74}, // -4
	{57, 75}, // -3
	{58, 76}, // -2
	{59, 77}, // -1

	{78, 78} // NULL (Use for E Standard / other tunings that can't be used on the pedal)
};
inline std::map<char, char> DIGITECH_WHAMMY_BASS_activeBypassMap = {
	// Classic Mode

	// Whammy
	{1, 22}, // +2 OCT
	{2, 23}, // +1 OCT
	{3, 24}, // +5th
	{4, 25}, // +4th
	{5, 26}, // +2nd
	{6, 27}, // -2nd
	{7, 28}, // -4th
	{8, 29}, // -5th
	{9, 30}, // -1 OCT
	{10, 31}, // Dive Bomb

	// Detune
	{11, 32}, // Deep
	{12, 33}, // Shallow

	// Harmony (Up Pos || Down Pos)
	{13, 34}, // -4th || +3rd
	{14, 35}, // -4th || +5th
	{15, 36}, // -5th || +5th
	{16, 37}, // +5th || +6th
	{17, 38}, // +5th || +OCT
	{18, 39}, // -OCT || -4th
	{19, 40}, // -OCT || +OCT
	{20, 41}, // +OCT || +10th
	{21, 42}, // +1 OCT || +2 OCT

	// Chords Mode

	// Whammy
	{43, 64}, // +2 OCT
	{44, 65}, // +1 OCT
	{45, 66}, // +5th
	{46, 67}, // +4th
	{47, 68}, // +2nd
	{48, 69}, // -2nd
	{49, 70}, // -4th
	{50, 71}, // -5th
	{51, 72}, // -1 OCT
	{52, 73}, // Dive Bomb

	// Detune
	{53, 74}, // Deep
	{54, 75}, // Shallow

	// Harmony (Up Pos || Down Pos)
	{55, 76}, // -4th || +3rd
	{56, 77}, // -4th || +5th
	{57, 78}, // -5th || +5th
	{58, 79}, // +5th || +6th
	{59, 80}, // +5th || +OCT
	{60, 81}, // -OCT || -4th
	{61, 82}, // -OCT || +OCT
	{62, 83}, // +OCT || +10th
	{63, 84}, // +1 OCT || +2 OCT
};

inline std::map<char, char> DIGITECH_WHAMMY_activeBypassMap = {
	// Classic Mode

	// Whammy
	{1, 22}, // +2 OCT
	{2, 23}, // +1 OCT
	{3, 24}, // +5th
	{4, 25}, // +4th
	{5, 26}, // -2nd
	{6, 27}, // -4th
	{7, 28}, // -5th
	{8, 29}, // -1 OCT
	{9, 30}, // -2 OCT
	{10, 31}, // Dive Bomb

	// Detune
	{11, 32}, // Deep
	{12, 33}, // Shallow

	// Harmony (Up Pos || Down Pos)
	{13, 34}, // -4th || +3rd
	{14, 35}, // -4th || +5th
	{15, 36}, // -5th || +5th
	{16, 37}, // +5th || +6th
	{17, 38}, // +5th || +OCT
	{18, 39}, // -OCT || -4th
	{19, 40}, // -OCT || +OCT
	{20, 41}, // +OCT || +10th
	{21, 42}, // +1 OCT || +2 OCT

	// Chords Mode

	// Whammy
	{43, 64}, // +2 OCT
	{44, 65}, // +1 OCT
	{45, 66}, // +5th
	{46, 67}, // +4th
	{47, 68}, // +2nd
	{48, 69}, // -2nd
	{49, 70}, // -4th
	{50, 71}, // -5th
	{51, 72}, // -1 OCT
	{52, 73}, // Dive Bomb

	// Detune
	{53, 74}, // Deep
	{54, 75}, // Shallow

	// Harmony (Up Pos || Down Pos)
	{55, 76}, // -4th || +3rd
	{56, 77}, // -4th || +5th
	{57, 78}, // -5th || +5th
	{58, 79}, // +5th || +6th
	{59, 80}, // +5th || +OCT
	{60, 81}, // -OCT || -4th
	{61, 82}, // -OCT || +OCT
	{62, 83}, // +OCT || +10th
	{63, 84}, // +1 OCT || +2 OCT
};

inline std::vector<float> DIGITECH_WHAMMY_BASS_semiTones = {
	24.0f, 12.0f, 7.0f, 5.0f, 2.0f, -2.0f, -5.0f, -7.0f, -12.0f, -36.0f
};

inline std::vector<float> DIGITECH_WHAMMY_semiTones = {
	24.0f, 12.0f, 7.0f, 5.0f, -2.0f, -5.0f, -7.0f, -12.0f, -24.0f, -36.0f
};

// Pedal -> Functions

// Pedal -> Drop Tune Capable, True Tuning Capable
inline std::map<unsigned int, std::pair<bool, bool>> pedalCanUseMap = {
	// Digitech
	{1, {true, true}}, // Whammy DT
	{2, {true, true}}, // Whammy Bass
	{3, {true, true}}, // Whammy

	// Misc
	{4, {false, false}} // Dummy Pedal
};

// Pedal -> activeBypassMap
inline std::map<unsigned int, std::map<char, char>> pedalToActiveBypassMap = {
	{1, DIGITECH_WHAMMY_DT_activeBypassMap},
	{2, DIGITECH_WHAMMY_BASS_activeBypassMap},
	{3, DIGITECH_WHAMMY_activeBypassMap}
};

// Pedal -> CC Channel
inline std::map<unsigned int, unsigned int> pedalToCC_Channel = {
	{1, DIGITECH_WHAMMY_CC_CHANNEL},
	{2, DIGITECH_WHAMMY_CC_CHANNEL},
	{3, DIGITECH_WHAMMY_CC_CHANNEL}
};
