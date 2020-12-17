#include "Midi.hpp"

// Midi codes should follow this guide: http://fmslogo.sourceforge.net/manual/midi-table.html
namespace Midi {
	std::vector<MIDIOUTCAPSA> midiOutDevices;
	int SelectedMidiDevice = 0, MidiCC = 0, MidiPC = 666;
	unsigned int NumberOfPorts;

	void InitMidi() {
		try {
			RtMidiIn midiin;
			std::cout << "Starting MIDI" << std::endl;
		}
		catch (RtMidiError& error) {
			std::cout << "(MIDI) " << error.getMessage() << std::endl;
		}
	}

	void GetMidiDeviceNames() {
		NumberOfPorts = midiOutGetNumDevs();
		for (int device = 0; device < NumberOfPorts; device++) {
			MIDIOUTCAPSA temp;
			midiOutGetDevCapsA(device, &temp, sizeof(MIDIOUTCAPSA));
			midiOutDevices.push_back(temp);
		}
		
	}

	void ReadMidiSettingsFromINI(std::string ChordsMode, int PedalToUse, std::string AutoTuneForSongDevice) {
		if (ChordsMode == "on") { // Is Chords mode on (only some pedals)
			DIGITECH_CHORDS_MODE = true;
			std::cout << "(MIDI) Chords Mode: Enabled" << std::endl;
		}

		pedalToUse = PedalToUse; // What pedal they're using
		std::cout << "(MIDI) Pedal To Use: " << pedalToUse << std::endl;

		GetMidiDeviceNames();

		for (int device = 0; device < NumberOfPorts; device++) {
			if ((std::string)midiOutDevices.at(device).szPname == AutoTuneForSongDevice) {
				std::cout << "(MIDI) Found MIDI device: " << midiOutDevices.at(device).szPname << std::endl;
				SelectedMidiDevice = device;
				break;
			}
			std::cout << "(MIDI) Available MIDI device: " << midiOutDevices.at(device).szPname << std::endl;
		}
	}

	bool SendProgramChange(char programChange) {
		RtMidiOut* midiout = new RtMidiOut();
		std::vector<unsigned char> message;

		// Check available ports.
		NumberOfPorts = midiOutGetNumDevs();
		if (NumberOfPorts == 0) {
			std::cout << "No MIDI ports available!" << std::endl;
			delete midiout;
			sendPC = false;
			return false;
		}

		try {
			midiout->openPort(SelectedMidiDevice);

			// Send MIDI message
			message.push_back(programChangeStatus); // It's a Program Change
			message.push_back(programChange); // What program we changing?
			if (programChange != 78)
				std::cout << "Sending Midi Message: " << "PC: " << (int)message.back() << std::endl;
			midiout->sendMessage(&message);
		}
		catch (RtMidiError& error) {
			std::cout << "(MIDI) " << error.getMessage() << std::endl;
		}

		// Clean up
		delete midiout;
		sendPC = false;
		lastPC = programChange;
		return true;
	}

	bool SendControlChange(char toePosition) {

		if (pedalToUse == 0 || pedalToCC_Channel.find(pedalToUse) == pedalToCC_Channel.end())
			return false;
		

		RtMidiOut* midiout = new RtMidiOut();
		std::vector<unsigned char> message;

		// Check available ports.
		NumberOfPorts = midiOutGetNumDevs();
		if (NumberOfPorts == 0) {
			std::cout << "No MIDI ports available!" << std::endl;
			delete midiout;
			sendCC = false;
			return false;
		}

		try {
			// Open first available port.
			midiout->openPort(SelectedMidiDevice);

			// Send MIDI Message
			message.push_back(controlChangeStatus); // Say it's a Control Change
			message.push_back(pedalToCC_Channel.find(pedalToUse)->second); // Control to change
			message.push_back(toePosition); // New Control Value || 0 = off, 127 = on
			std::cout << "Sending Midi Message: " << "CC: " << (int)message.rbegin()[1] << " " << (int)message.back() << std::endl;
			midiout->sendMessage(&message);
		}
		catch (RtMidiError& error) {
			std::cout << "(MIDI) " << error.getMessage() << std::endl;
		}

		// Clean up
		delete midiout;
		sendCC = false;
		lastCC = toePosition;
		return true;
	}

	void AutomateDownTuning() {
		if (!alreadyAutomatedTuningInThisSong) {
			alreadyAutomatedTuningInThisSong = true;

			if (pedalToUse == 0 || !pedalCanUseMap.find(pedalToUse)->second.first) {
				std::cout << "Your pedal doesn't support drop tuning." << std::endl;
				return;
			}

			Sleep(1500); // The menu is called when the animation starts. The tuning isn't set at that point, so we need to wait to get the value. This doesn't seem to lag the game.

			int* highestLowestTuning = MemHelpers::GetHighestLowestString();

			int highestTuning = highestLowestTuning[0];
			int lowestTuning = highestLowestTuning[1];
			
			delete[] highestLowestTuning;

			int TrueTuning_Hertz = MemHelpers::GetTrueTuning();

			if (TrueTuning_Hertz < 260) // Give some leeway for A220 and it's true tuned offsets
				highestTuning -= 12;

			switch (pedalToUse) {
			 
			case 1:
				Digitech_Whammy_DT_Auto_Tuning(highestTuning);
				break;
			default:
				break;
			}
		}
	}

	void AutomateTrueTuning() {
		if (!alreadyAutomatedTrueTuningInThisSong && userWantsToUseAutoTuning) {
			alreadyAutomatedTrueTuningInThisSong = true;

			if (pedalToUse == 0 || !pedalCanUseMap.find(pedalToUse)->second.second) {
				std::cout << "Your pedal doesn't support true tuning." << std::endl;
				return;
			}

			int TrueTuning_Hertz = MemHelpers::GetTrueTuning();

			if (TrueTuning_Hertz == 440 || TrueTuning_Hertz == 220)
				return;

			
			switch (pedalToUse) {
			case 1:
				Digitech_Whammy_DT_Auto_TrueTune(TrueTuning_Hertz);
				break;
			case 2:
				Digitech_Whammy_Auto_TrueTune(TrueTuning_Hertz);
				break;

			default:
				break;
			}
		}
	}

	void RevertAutomatedTuning() { // Turn off the pedal after we are done with a song.

		if (pedalToUse == 0 || pedalToActiveBypassMap.find(pedalToUse) == pedalToActiveBypassMap.end() || !userWantsToUseAutoTuning)
			return;

		if (lastPC != 666) { // If the song is in E Standard, and we leave, it tries to use "Bypass +2 OCT Whammy"
			int cache = lastPC;
			
			std::map<char, char> activeBypassMap = pedalToActiveBypassMap.find(pedalToUse)->second;
			
			if (lastPC_TUNING != 0 && lastPC_TUNING != lastPC) {  // If the user was in a song that requires a down tune AND true tuning, we use this. Ex: If 6 was 9 (Eb Standard AND A431)
				std::cout << "Attmepting to turn off drop tuning" << std::endl;
				SendProgramChange(activeBypassMap.find(lastPC_TUNING)->second);
				std::cout << "Attmepting to turn off true tuning" << std::endl;
			}
			else
				std::cout << "Attmepting to turn off drop tuning" << std::endl;

			if (activeBypassMap.find(cache) != activeBypassMap.end())
				SendProgramChange(activeBypassMap.find(cache)->second); // Send the bypass code to revert back to normal guitar.
			SendControlChange(0); // Reset the expression pedal
		}

		alreadyAutomatedTuningInThisSong = false;
		alreadyAutomatedTrueTuningInThisSong = false;
		lastPC_TUNING = 0;
	}

	void SendDataToThread_PC(char program, bool shouldWeSendPC) {
		sendPC = shouldWeSendPC;
		dataToSendPC = program;
	}
	void SendDataToThread_CC(char toePosition, bool shouldWeSendCC) {
		sendCC = shouldWeSendCC;
		dataToSendCC = toePosition;
	}

	// Pedal Specific Functions

	void Digitech_Whammy_DT_Auto_Tuning(int highestTuning) {

		switch (highestTuning) {

				// Above E Standard
		case 12:
			SendProgramChange(49); // E Standard +OCT
			lastPC_TUNING = 49;
			break;
		case 7:
			SendProgramChange(48); // B Standard
			lastPC_TUNING = 48;
			break;
		case 6:
			SendProgramChange(47); // Bb Standard
			lastPC_TUNING = 47;
			break;
		case 5:
			SendProgramChange(46); // A Standard
			lastPC_TUNING = 46;
			break;
		case 4:
			SendProgramChange(45); // Ab Standard
			lastPC_TUNING = 45;
			break;
		case 3:
			SendProgramChange(44); // G Standard
			lastPC_TUNING = 44;
			break;
		case 2:
			SendProgramChange(43); // F# Standard
			lastPC_TUNING = 43;
			break;
		case 1:
			SendProgramChange(42); // F Standard
			lastPC_TUNING = 42;
			break;

			// E Standard
		case 0:
			lastPC = 666; // E Standard. Doesn't do anything, just sets the proper lastPC.
			lastPC_TUNING = 0;
			break;

			// Below E Standard
		case -1:
			SendProgramChange(59); // Eb Standard
			lastPC_TUNING = 59;
			break;
		case -2:
			SendProgramChange(58); // D Standard
			lastPC_TUNING = 58;
			break;
		case -3:
			SendProgramChange(57); // C# Standard
			lastPC_TUNING = 57;
			break;
		case -4:
			SendProgramChange(56); // C Standard
			lastPC_TUNING = 56;
			break;
		case -5:
			SendProgramChange(55); // B Standard
			lastPC_TUNING = 55;
			break;
		case -6:
			SendProgramChange(54); // Bb Standard
			lastPC_TUNING = 54;
			break;
		case -7:
			SendProgramChange(53); // A Standard
			lastPC_TUNING = 53;
			break;
		case -12:
			SendProgramChange(52); // E Standard -OCT
			lastPC_TUNING = 52;
			break;

			// The pedal can't handle the tuning.
		default:
			lastPC = 666; // Doesn't do anything, just sets the proper lastPC.
			break;
		}
	}

	void Digitech_Whammy_DT_Auto_TrueTune(int TrueTuning_Hertz) {

		// A440 / A220
		if (TrueTuning_Hertz == 440 || TrueTuning_Hertz == 220)
			return;

		// Above A440
		else if (TrueTuning_Hertz > 440) {
			SendProgramChange(5);
			SendControlChange((char)(TrueTuning_Hertz - 440));
		}

		// Below A440
		else {
			SendProgramChange(4);
			SendControlChange((char)round(1127.43 - (2.56667 * TrueTuning_Hertz)));
		}
	}

	void Digitech_Whammy_Auto_TrueTune(int TrueTuning_Hertz) {
		int offset = 0;
		if (DIGITECH_CHORDS_MODE)
			offset = 42;
		
		// A440 / A220
		if (TrueTuning_Hertz == 440 || TrueTuning_Hertz == 220)
			return;

		// Above A440
		else if (TrueTuning_Hertz > 440) {
			SendProgramChange(5 + offset);
			SendControlChange((char)(TrueTuning_Hertz - 440));
		}

		// Below A440
		else {
			SendProgramChange(4 + offset);
			SendControlChange((char)round(1127.43 - (2.56667 * TrueTuning_Hertz)));
		}
	}
}
