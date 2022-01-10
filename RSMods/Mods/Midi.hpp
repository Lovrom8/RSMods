#pragma once

#include "Windows.h"
#include <iostream>
#include <cstdlib>
#include <map>
#include <string>
#include <thread>

#include "../Lib/Midi/RtMidi.h"
#include "../MemHelpers.hpp"
#include "../Structs.hpp"
#include "RiffRepeater.hpp"
#include "../Wwise/SoundEngine.hpp"

namespace Midi {
	void InitMidi();
	void AutomateTuning();
	void AttemptTuningInTuner();
	void RevertAutomatedTuning();
	void SendDataToThread_PC(char program, bool shouldWeSendPC = true);
	void SendDataToThread_CC(char toePosition, bool shouldWeSendCC = true);
	void ReadMidiSettingsFromINI(std::string ChordsMode, int PedalToUse, std::string MidiOutDevice, std::string MidiInDevice);
	bool SendProgramChange(char programChange = '\000', char alternativeChannel = 255);
	bool SendControlChange(char toePosition = '\000', char alternativeBank = 255, char alternativeChannel = 255);
	std::string GetTuningOffsetName(int offset);
	bool IsValidMidiMessage(std::vector<unsigned char>* message);
	unsigned WINAPI ListenToMidiInThread();
	void RespondToMidiIn(double deltaTime, std::vector<unsigned char>* message, void* userData);
	void FindMidiInDevices(std::string deviceToLookFor);
	void FindMidiOutDevices(std::string deviceToLookFor);

	inline bool disableMidiIn = false;
	extern int MidiCC, MidiPC;
	inline bool scannedForMidiDevices = false, attemptedToDetachMidiInThread = false;
	extern std::vector<MIDIINCAPSA> midiInDevices; // All MIDI in devices currently connected
	extern std::vector<MIDIOUTCAPSA> midiOutDevices; // All MIDI out devices currently connected
	extern int SelectedMidiOutDevice, SelectedMidiInDevice;
	extern unsigned int NumberOfOutPorts, NumberOfInPorts;
	inline bool sendCC = false, sendPC = false;
	inline int dataToSendPC = 0, dataToSendCC = 0, lastCC = 0, lastPC = 666;
	inline int lastPC_TUNING = 0; // Only use if the song requires a tuning change AND a true tuning. (Hendrix Eb Standard)
	inline bool alreadyAttemptedTuningInTuner = false, alreadyAutomatedTuningInThisSong = false, alreadyAutomatedTrueTuningInThisSong = false, userWantsToUseAutoTuning = false;
	inline int sleepFor = 33; // Sleep for 33ms or ~ 1/33rd of a second.
	inline MidiPedal selectedPedal = MidiPedal();
	inline int tuningOffset;

	enum MidiCommands {
		NoteOff		= 0x80,
		NoteOn		= 0x90,
		AfterTouch	= 0xA0,
		CC			= 0xB0,
		PC			= 0xC0,
		Pressure	= 0xD0,
		PitchBend	= 0xE0,
		// System commands start at 0xF0
	};

	namespace Digitech {
		namespace WhammyDT {
			inline std::map<char, char> activeBypassMap = {

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

			inline std::vector<float> semiTones = {
				24.0f, 12.0f, 7.0f, 5.0f, 2.0f, -5.0f, -7.0f, -12.0f, -24.0f, -36.0f,
			};

			void AutoTuning(int highestTuning, float TrueTuning_Hertz);
			void AutoTrueTuningPastLimits(int relativeTuning, float TrueTuning_Hertz);
			void AutoTrueTuning(int TrueTuning_Hertz);
		}

		inline bool DIGITECH_CHORDS_MODE = false;

		namespace BassWhammy {
			inline std::map<char, char> activeBypassMap = {
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

			inline std::vector<float> semiTones = {
				24.0f, 12.0f, 7.0f, 5.0f, 2.0f, -2.0f, -5.0f, -7.0f, -12.0f, -36.0f
			};

			// Bass Whammy
			void AutoTuningAndTrueTuning(int highestTuning, float TrueTuning_Hertz);
		}

		namespace Whammy {
			inline std::map<char, char> activeBypassMap = {
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
			inline std::vector<float> semiTones = {
				24.0f, 12.0f, 7.0f, 5.0f, -2.0f, -5.0f, -7.0f, -12.0f, -24.0f, -36.0f
			};

			void AutoTuningAndTrueTuning(int highestTuning, float TrueTuning_Hertz);
		}
	}
	 
	namespace Software {

		inline std::map<char, char> semiToneMap; // DON'T TREAT THIS AS AN ACTIVE BYPASS MAP! This is a map of { SemiTone, MidiValue }.
		inline std::map<int, char> trueTuningMap; // This is a map of { TrueTuning, MidiValue }.

		void AutoTuning(int highestTuning, float TrueTuning_Hertz);

		void AutoTrueTuning(float TrueTuning_Hertz);

		void ReloadSettings();

		void FillSemitoneMap();
		void LoadSemitoneSettings();

		void FillTrueTuningMap();
		void LoadTrueTuningSettings();


		inline unsigned char sendSemitoneCommand = '\0';
		inline unsigned char sendTrueTuningCommand = '\0';
		inline unsigned char sendTrueTuningChannel = '\0';
		inline unsigned char semiToneShutoffTrigger = '\0';
		inline unsigned char trueTuningShutoffTrigger = '\0';
		inline unsigned char trueTuningBank = '\1';
		inline bool sentTrueTuningInThisSong = false;
		inline bool sentSemitoneInThisSong = false;
	}

	inline std::vector<MidiPedal> supportedPedals = {
		MidiPedal("DIGITECH Whammy DT", 11, 0, 0, true, true, Digitech::WhammyDT::semiTones, Digitech::WhammyDT::activeBypassMap, Digitech::WhammyDT::AutoTuning),
		MidiPedal("DIGITECH Bass Whammy", 11, 0, 0, true, true, Digitech::BassWhammy::semiTones, Digitech::BassWhammy::activeBypassMap, Digitech::BassWhammy::AutoTuningAndTrueTuning),
		MidiPedal("DIGITECH Whammy", 11, 0, 0, true, true, Digitech::Whammy::semiTones, Digitech::Whammy::activeBypassMap, Digitech::Whammy::AutoTuningAndTrueTuning),
		MidiPedal("Software Pedal", 0, 0, 0, true, true, std::vector<float>{}, Software::semiToneMap, Software::AutoTuning, true),
	};
};

// Midi Specifications
inline unsigned char programChangeStatus = 192, controlChangeStatus = 176, noteOnStatus = 144, noteOffStatus = 128;