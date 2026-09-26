#pragma once

#include "RiffRepeater.hpp"
#include "../MidiPedal.h"
#include <mmeapi.h>

namespace Midi {
	constexpr char kUnusedMidiSelector = static_cast<char>(-1);

	void InitMidi();
	void AutomateTuning();
	void AttemptTuningInTuner();
	void RevertAutomatedTuning();
	void SendDataToThread_PC(char program, bool shouldWeSendPC = true);
	void SendDataToThread_CC(char toePosition, bool shouldWeSendCC = true);
	void ReadMidiSettingsFromINI(const std::string& ChordsMode, int PedalToUse, const std::string& MidiOutDevice, const std::string& MidiInDevice);
	bool SendProgramChange(char programChange = '\000', char alternativeChannel = kUnusedMidiSelector);
	bool SendControlChange(char toePosition = '\000', char alternativeBank = kUnusedMidiSelector, char alternativeChannel = kUnusedMidiSelector);
	std::string GetTuningOffsetName(int offset);
	bool IsValidMidiMessage(std::vector<unsigned char>* message);
	unsigned WINAPI ListenToMidiInThread();
	void RespondToMidiIn(double deltaTime, std::vector<unsigned char>* message, void* userData);
	void FindMidiInDevices(const std::string& deviceToLookFor);
	void FindMidiOutDevices(const std::string& deviceToLookFor);

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
	inline bool tunerAutoTuneFailed = false; // The tuner was shown but couldn't be read, so the player tuned by hand. Cleared when the song ends or the next tuner opens.
	// Latches once we auto-tune in the pre-song tuner and is never reset for the process lifetime.
	inline bool appliedTunerAutoTune = false;
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

	constexpr char AsMidiByte(int value) noexcept {
		return static_cast<char>(value);
	};

	namespace Digitech {
		namespace WhammyDT {
			inline std::map<char, char> activeBypassMap = {

				// Whammy
				{AsMidiByte(0), AsMidiByte(21)}, // +2 OCT
				{AsMidiByte(1), AsMidiByte(22)}, // +1 OCT
				{AsMidiByte(2), AsMidiByte(23)}, // +5th
				{AsMidiByte(3), AsMidiByte(24)}, // +4th
				{AsMidiByte(4), AsMidiByte(25)}, // -2nd
				{AsMidiByte(5), AsMidiByte(26)}, // -4th
				{AsMidiByte(6), AsMidiByte(27)}, // -5th
				{AsMidiByte(7), AsMidiByte(28)}, // -1 OCT
				{AsMidiByte(8), AsMidiByte(29)}, // -2 OCT
				{AsMidiByte(9), AsMidiByte(30)}, // Dive Bomb

				// Detune
				{AsMidiByte(10), AsMidiByte(31)}, // Deep
				{AsMidiByte(11), AsMidiByte(32)}, // Shallow

				// Harmony (Up Pos || Down Pos)
				{AsMidiByte(12), AsMidiByte(33)}, // +2nd || +3rd
				{AsMidiByte(13), AsMidiByte(34)}, // +b 3rd || +3rd
				{AsMidiByte(14), AsMidiByte(35)}, // +3rd || +4th
				{AsMidiByte(15), AsMidiByte(36)}, // +4th || +5th
				{AsMidiByte(16), AsMidiByte(37)}, // +5th || +6th
				{AsMidiByte(17), AsMidiByte(38)}, // +5th || +7th
				{AsMidiByte(18), AsMidiByte(39)}, // -4th || -3rd
				{AsMidiByte(19), AsMidiByte(40)}, // -5th || -4th
				{AsMidiByte(20), AsMidiByte(41)}, // -1 OCT || +1 OCT

				// Drop Tune Effect
				{AsMidiByte(42), AsMidiByte(60)}, // +1
				{AsMidiByte(43), AsMidiByte(61)}, // +2
				{AsMidiByte(44), AsMidiByte(62)}, // +3
				{AsMidiByte(45), AsMidiByte(63)}, // +4
				{AsMidiByte(46), AsMidiByte(64)}, // +5
				{AsMidiByte(47), AsMidiByte(65)}, // +6
				{AsMidiByte(48), AsMidiByte(66)}, // +7
				{AsMidiByte(49), AsMidiByte(67)}, // +OCT
				{AsMidiByte(50), AsMidiByte(68)}, // +OCT + Dry

				{AsMidiByte(51), AsMidiByte(69)}, // -OCT + Dry || Nice :eyes:
				{AsMidiByte(52), AsMidiByte(70)}, // -OCT
				{AsMidiByte(53), AsMidiByte(71)}, // -7
				{AsMidiByte(54), AsMidiByte(72)}, // -6
				{AsMidiByte(55), AsMidiByte(73)}, // -5
				{AsMidiByte(56), AsMidiByte(74)}, // -4
				{AsMidiByte(57), AsMidiByte(75)}, // -3
				{AsMidiByte(58), AsMidiByte(76)}, // -2
				{AsMidiByte(59), AsMidiByte(77)}, // -1

				{AsMidiByte(78), AsMidiByte(78)} // NULL (Use for E Standard / other tunings that can't be used on the pedal)
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
				{AsMidiByte(1), AsMidiByte(22)}, // +2 OCT
				{AsMidiByte(2), AsMidiByte(23)}, // +1 OCT
				{AsMidiByte(3), AsMidiByte(24)}, // +5th
				{AsMidiByte(4), AsMidiByte(25)}, // +4th
				{AsMidiByte(5), AsMidiByte(26)}, // +2nd
				{AsMidiByte(6), AsMidiByte(27)}, // -2nd
				{AsMidiByte(7), AsMidiByte(28)}, // -4th
				{AsMidiByte(8), AsMidiByte(29)}, // -5th
				{AsMidiByte(9), AsMidiByte(30)}, // -1 OCT
				{AsMidiByte(10), AsMidiByte(31)}, // Dive Bomb

				// Detune
				{AsMidiByte(11), AsMidiByte(32)}, // Deep
				{AsMidiByte(12), AsMidiByte(33)}, // Shallow

				// Harmony (Up Pos || Down Pos)
				{AsMidiByte(13), AsMidiByte(34)}, // -4th || +3rd
				{AsMidiByte(14), AsMidiByte(35)}, // -4th || +5th
				{AsMidiByte(15), AsMidiByte(36)}, // -5th || +5th
				{AsMidiByte(16), AsMidiByte(37)}, // +5th || +6th
				{AsMidiByte(17), AsMidiByte(38)}, // +5th || +OCT
				{AsMidiByte(18), AsMidiByte(39)}, // -OCT || -4th
				{AsMidiByte(19), AsMidiByte(40)}, // -OCT || +OCT
				{AsMidiByte(20), AsMidiByte(41)}, // +OCT || +10th
				{AsMidiByte(21), AsMidiByte(42)}, // +1 OCT || +2 OCT

				// Chords Mode

				// Whammy
				{AsMidiByte(43), AsMidiByte(64)}, // +2 OCT
				{AsMidiByte(44), AsMidiByte(65)}, // +1 OCT
				{AsMidiByte(45), AsMidiByte(66)}, // +5th
				{AsMidiByte(46), AsMidiByte(67)}, // +4th
				{AsMidiByte(47), AsMidiByte(68)}, // +2nd
				{AsMidiByte(48), AsMidiByte(69)}, // -2nd
				{AsMidiByte(49), AsMidiByte(70)}, // -4th
				{AsMidiByte(50), AsMidiByte(71)}, // -5th
				{AsMidiByte(51), AsMidiByte(72)}, // -1 OCT
				{AsMidiByte(52), AsMidiByte(73)}, // Dive Bomb

				// Detune
				{AsMidiByte(53), AsMidiByte(74)}, // Deep
				{AsMidiByte(54), AsMidiByte(75)}, // Shallow

				// Harmony (Up Pos || Down Pos)
				{AsMidiByte(55), AsMidiByte(76)}, // -4th || +3rd
				{AsMidiByte(56), AsMidiByte(77)}, // -4th || +5th
				{AsMidiByte(57), AsMidiByte(78)}, // -5th || +5th
				{AsMidiByte(58), AsMidiByte(79)}, // +5th || +6th
				{AsMidiByte(59), AsMidiByte(80)}, // +5th || +OCT
				{AsMidiByte(60), AsMidiByte(81)}, // -OCT || -4th
				{AsMidiByte(61), AsMidiByte(82)}, // -OCT || +OCT
				{AsMidiByte(62), AsMidiByte(83)}, // +OCT || +10th
				{AsMidiByte(63), AsMidiByte(84)}, // +1 OCT || +2 OCT
			};

			inline std::vector<float> semiTones = {
				24.0f, 12.0f, 7.0f, 5.0f, 2.0f, -2.0f, -5.0f, -7.0f, -12.0f, -36.0f
			};

			// Bass Whammy
			void AutoTuningAndTrueTuning(int highestTuning, float TrueTuning_Hertz);
		}

		// Documentation for this is hard to find because not many places call it the "Whammy 4" / "Whammy V4" etc.
		// Here is a link to a Whammy 4 manual that I was able to find for future reference.
		// https://www.manualslib.com/manual/380879/Digitech-Whammy.html
		namespace WhammyFour {
			inline std::map<char, char> activeBypassMap = {
				// Classic Mode

				// Detune
				{AsMidiByte(1), AsMidiByte(18)}, // Shallow
				{AsMidiByte(2), AsMidiByte(19)}, // Deep

				{AsMidiByte(3), AsMidiByte(20)}, // +2 OCT
				{AsMidiByte(4), AsMidiByte(21)}, // +1 OCT
				{AsMidiByte(5), AsMidiByte(22)}, // -1 OCT
				{AsMidiByte(6), AsMidiByte(23)}, // -2 OCT
				{AsMidiByte(7), AsMidiByte(24)}, // Dive Bomb  (-3 OCT)
				{AsMidiByte(8), AsMidiByte(25)}, // Drop Tune (- 2 semitones || OPPOSITE)

				// Harmony (Up Pos || Down Pos)
				{AsMidiByte(9), AsMidiByte(26)}, // -OCT || +OCT
				{AsMidiByte(10), AsMidiByte(27)}, // -5th || -4th
				{AsMidiByte(11), AsMidiByte(28)}, // -4th || -3rd
				{AsMidiByte(12), AsMidiByte(29)}, // +5th || +7th
				{AsMidiByte(13), AsMidiByte(30)}, // +5th || +6th
				{AsMidiByte(14), AsMidiByte(31)}, // +4th || +5th
				{AsMidiByte(15), AsMidiByte(32)}, // +3rd || +4th
				{AsMidiByte(16), AsMidiByte(33)}, // +b3rd || +3rd
				{AsMidiByte(17), AsMidiByte(34)}, // +2nd || +3rd
			};
			inline std::vector<float> semiTones = {
				24.0f, 12.0f, -12.0f, -24.0f, -36.0f
			};

			void AutoTuningAndTrueTuning(int highestTuning, float TrueTuning_Hertz);
		}

		namespace WhammyFive {
			inline std::map<char, char> activeBypassMap = {
				// Classic Mode

				// Whammy
				{AsMidiByte(1), AsMidiByte(22)}, // +2 OCT
				{AsMidiByte(2), AsMidiByte(23)}, // +1 OCT
				{AsMidiByte(3), AsMidiByte(24)}, // +5th
				{AsMidiByte(4), AsMidiByte(25)}, // +4th
				{AsMidiByte(5), AsMidiByte(26)}, // -2nd
				{AsMidiByte(6), AsMidiByte(27)}, // -4th
				{AsMidiByte(7), AsMidiByte(28)}, // -5th
				{AsMidiByte(8), AsMidiByte(29)}, // -1 OCT
				{AsMidiByte(9), AsMidiByte(30)}, // -2 OCT
				{AsMidiByte(10), AsMidiByte(31)}, // Dive Bomb

				// Detune
				{AsMidiByte(11), AsMidiByte(32)}, // Deep
				{AsMidiByte(12), AsMidiByte(33)}, // Shallow

				// Harmony (Up Pos || Down Pos)
				{AsMidiByte(13), AsMidiByte(34)}, // -4th || +3rd
				{AsMidiByte(14), AsMidiByte(35)}, // -4th || +5th
				{AsMidiByte(15), AsMidiByte(36)}, // -5th || +5th
				{AsMidiByte(16), AsMidiByte(37)}, // +5th || +6th
				{AsMidiByte(17), AsMidiByte(38)}, // +5th || +OCT
				{AsMidiByte(18), AsMidiByte(39)}, // -OCT || -4th
				{AsMidiByte(19), AsMidiByte(40)}, // -OCT || +OCT
				{AsMidiByte(20), AsMidiByte(41)}, // +OCT || +10th
				{AsMidiByte(21), AsMidiByte(42)}, // +1 OCT || +2 OCT

				// Chords Mode

				// Whammy
				{AsMidiByte(43), AsMidiByte(64)}, // +2 OCT
				{AsMidiByte(44), AsMidiByte(65)}, // +1 OCT
				{AsMidiByte(45), AsMidiByte(66)}, // +5th
				{AsMidiByte(46), AsMidiByte(67)}, // +4th
				{AsMidiByte(47), AsMidiByte(68)}, // +2nd
				{AsMidiByte(48), AsMidiByte(69)}, // -2nd
				{AsMidiByte(49), AsMidiByte(70)}, // -4th
				{AsMidiByte(50), AsMidiByte(71)}, // -5th
				{AsMidiByte(51), AsMidiByte(72)}, // -1 OCT
				{AsMidiByte(52), AsMidiByte(73)}, // Dive Bomb

				// Detune
				{AsMidiByte(53), AsMidiByte(74)}, // Deep
				{AsMidiByte(54), AsMidiByte(75)}, // Shallow

				// Harmony (Up Pos || Down Pos)
				{AsMidiByte(55), AsMidiByte(76)}, // -4th || +3rd
				{AsMidiByte(56), AsMidiByte(77)}, // -4th || +5th
				{AsMidiByte(57), AsMidiByte(78)}, // -5th || +5th
				{AsMidiByte(58), AsMidiByte(79)}, // +5th || +6th
				{AsMidiByte(59), AsMidiByte(80)}, // +5th || +OCT
				{AsMidiByte(60), AsMidiByte(81)}, // -OCT || -4th
				{AsMidiByte(61), AsMidiByte(82)}, // -OCT || +OCT
				{AsMidiByte(62), AsMidiByte(83)}, // +OCT || +10th
				{AsMidiByte(63), AsMidiByte(84)}, // +1 OCT || +2 OCT
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

	// Always add to the end of this list, even if it makes sense logically to put it in the middle.
	// The order of this list is driven by setting data, so if you put something in the middle it ruins everyone's settings using a lower option.
	inline std::vector<MidiPedal> supportedPedals = {
		MidiPedal("DIGITECH Whammy DT", 11, 0, 0, true, true, Digitech::WhammyDT::semiTones, Digitech::WhammyDT::activeBypassMap, Digitech::WhammyDT::AutoTuning),
		MidiPedal("DIGITECH Bass Whammy", 11, 0, 0, true, true, Digitech::BassWhammy::semiTones, Digitech::BassWhammy::activeBypassMap, Digitech::BassWhammy::AutoTuningAndTrueTuning),
		MidiPedal("DIGITECH Whammy 5", 11, 0, 0, true, true, Digitech::WhammyFive::semiTones, Digitech::WhammyFive::activeBypassMap, Digitech::WhammyFive::AutoTuningAndTrueTuning),
		MidiPedal("Software Pedal", 0, 0, 0, true, true, std::vector<float>{}, Software::semiToneMap, Software::AutoTuning, true),
		MidiPedal("DIGITECH Whammy 4", 11, 0, 0, true, true, Digitech::WhammyFour::semiTones, Digitech::WhammyFour::activeBypassMap, Digitech::WhammyFour::AutoTuningAndTrueTuning),
	};
};

// Midi Specifications
inline unsigned char programChangeStatus = 192, controlChangeStatus = 176, noteOnStatus = 144, noteOffStatus = 128;