#pragma once

#include "Windows.h"
#include <iostream>
#include <cstdlib>
#include <map>

#include "../Midi/RtMidi.h"

namespace Midi {
	void InitMidi();
	bool SendMidiMessage(char programChange = NULL, char toeDown = NULL);
};

// Midi Specifications
inline int programChangeStatus = 192, controlChangeStatus = 176, noteOnStatus = 144, noteOffStatus = 128;

// Pedal Specific

	// WHAMMY_DT
	inline int WHAMMY_DT_CC_CHANNEL = 11;
	inline std::map<char, char> WHAMMY_DT_activeBypassMap = {

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
		{13, 34}, // +2nd || +3rd
		{14, 35}, // +b 3rd || +3rd
		{15, 36}, // +3rd || +4th
		{16, 37}, // +4th || +5th
		{17, 38}, // +5th || +6th
		{18, 39}, // +5th || +7th
		{19, 40}, // -4th || -3rd
		{20, 41}, // -5th || -4th
		{21, 42}, // -1 OCT || +1 OCT

		// Drop Tune Effect
		{43, 61}, // +1
		{44, 62}, // +2
		{45, 63}, // +3
		{46, 64}, // +4
		{47, 65}, // +5
		{48, 66}, // +6
		{49, 67}, // +7
		{50, 68}, // +OCT
		{51, 69}, // +OCT + Dry || Nice :eyes:

		{52, 70}, // -OCT + Dry
		{53, 71}, // -OCT
		{54, 72}, // -7
		{55, 73}, // -6
		{56, 74}, // -5
		{57, 75}, // -4
		{58, 76}, // -3
		{59, 77}, // -2
		{60, 78}, // -1
	};