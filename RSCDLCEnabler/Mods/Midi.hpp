#pragma once

#include "Windows.h"
#include <iostream>
#include <cstdlib>
#include <map>

#include "../Lib/Midi/RtMidi.h"
#include "../MemHelpers.hpp"

namespace Midi {
	void InitMidi();
	bool SendProgramChange(char programChange = '\000');
	bool SendControlChange(char toePosition = '\000');
	void GetMidiDeviceNames();
	void AutomateDownTuning();
	void AutomateTrueTuning();
	void RevertAutomatedTuning();
	void SendDataToThread_PC(char program, bool shouldWeSendPC = true);
	void SendDataToThread_CC(char toePosition, bool shouldWeSendCC = true);

	// Digitech Whammy DT
	void Digitech_Whammy_DT_Auto_Tuning(int highestTuning);
	void Digitech_Whammy_DT_Auto_TrueTune(int TrueTuning_Hertz); 

	// Digitech Whammy & Whammy Bass
	void Digitech_Whammy_Auto_TrueTune(int TrueTuning_Hertz); 

	extern int MidiCC, MidiPC;
	extern std::map<int, std::string> MidiDeviceNames; // All MIDI devices that currently connected
	extern int SelectedMidiDevice; 
	extern unsigned int NumberOfPorts;
	inline bool sendCC = false, sendPC = false;
	inline int dataToSendPC = 0, dataToSendCC = 0, lastCC = 0, lastPC = 666;
	inline int lastPC_TUNING = 0; // Only use if the song requires a tuning change AND a true tuning. (Hendrix Eb Standard)
	inline bool alreadyAutomatedTuningInThisSong = false, alreadyAutomatedTrueTuningInThisSong = false;
	inline int sleepFor = 33; // Sleep for 33ms or ~ 1/33rd of a second.
	inline unsigned int pedalToUse = 2; // 0 = No pedal. 1 = Whammy DT. 2 = Whammy / Whammy Bass
	inline bool DIGITECH_CHORDS_MODE = true;
};

// Midi Specifications
inline unsigned char programChangeStatus = 192, controlChangeStatus = 176, noteOnStatus = 144, noteOffStatus = 128;


// Digitech WHAMMY DT | Bass Whammy | Whammy
inline int DIGITECH_WHAMMY_CC_CHANNEL = 11;
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
inline std::map<char, char> DIGITECH_WHAMMY_activeBypassMap = {
	// Classic Mode

	// Whammy
	{0, 21}, // +2 OCT
	{1, 22}, // +1 OCT
	{2, 23}, // +5th
	{3, 24}, // +4th
	{4, 25}, // +2nd
	{5, 26}, // -2nd
	{6, 27}, // -4th
	{7, 28}, // -5th
	{8, 29}, // -1 OCT
	{9, 30}, // Dive Bomb

	// Detune
	{10, 31}, // Deep
	{11, 32}, // Shallow

	// Harmony (Up Pos || Down Pos)
	{12, 33}, // -4th || +3rd
	{13, 34}, // -4th || +5th
	{14, 35}, // -5th || +5th
	{15, 36}, // +5th || +6th
	{16, 37}, // +5th || +OCT
	{17, 38}, // -OCT || -4th
	{18, 39}, // -OCT || +OCT
	{19, 40}, // +OCT || +10th
	{20, 41}, // +1 OCT || +2 OCT

	// Chords Mode

	// Whammy
	{42, 63}, // +2 OCT
	{43, 64}, // +1 OCT
	{44, 65}, // +5th
	{45, 66}, // +4th
	{46, 67}, // +2nd
	{47, 68}, // -2nd
	{48, 69}, // -4th
	{49, 70}, // -5th
	{50, 71}, // -1 OCT
	{51, 72}, // Dive Bomb

	// Detune
	{52, 73}, // Deep
	{53, 74}, // Shallow

	// Harmony (Up Pos || Down Pos)
	{54, 75}, // -4th || +3rd
	{55, 76}, // -4th || +5th
	{56, 77}, // -5th || +5th
	{57, 78}, // +5th || +6th
	{58, 79}, // +5th || +OCT
	{59, 80}, // -OCT || -4th
	{60, 81}, // -OCT || +OCT
	{61, 82}, // +OCT || +10th
	{62, 83}, // +1 OCT || +2 OCT
};


// Pedal -> Functions

// Pedal -> Drop Tune Capable, True Tuning Capable
std::map<unsigned int, std::pair<bool, bool>> pedalCanUseMap = {
	{1, {true, true}}, // Whammy DT
	{2, {false, true}}, // Whammy / Bass Whammy
	{3, {false, false}} // Dummy pedal
};

// Pedal -> activeBypassMap
std::map<unsigned int, std::map<char, char>> pedalToActiveBypassMap = {
	{1, DIGITECH_WHAMMY_DT_activeBypassMap},
	{2, DIGITECH_WHAMMY_activeBypassMap}
};

// Pedal -> CC Channel
std::map<unsigned int, unsigned int> pedalToCC_Channel = {
	{1, DIGITECH_WHAMMY_CC_CHANNEL},
	{2, DIGITECH_WHAMMY_CC_CHANNEL}
};
