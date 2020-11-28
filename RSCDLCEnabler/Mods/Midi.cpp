#include "Midi.hpp"

// Midi codes should follow this guide: http://fmslogo.sourceforge.net/manual/midi-table.html
namespace Midi {
	std::map<int, std::string> MidiDeviceNames;
	int SelectedMidiDevice = 0, MidiCC = 0, MidiPC = 0;
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

	bool SendProgramChange(char programChange) {
		RtMidiOut* midiout = new RtMidiOut();
		std::vector<unsigned char> message;

		// Check available ports.
		NumberOfPorts = midiout->getPortCount();
		if (NumberOfPorts == 0) {
			std::cout << "No MIDI ports available!" << std::endl;
			delete midiout;
			sendPC = false;
			return false;
		}

		try {
			// Open first available port.
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
		RtMidiOut* midiout = new RtMidiOut();
		std::vector<unsigned char> message;

		// Check available ports.
		NumberOfPorts = midiout->getPortCount();
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
			message.push_back(WHAMMY_DT_CC_CHANNEL); // Control to change
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

	void GetMidiDeviceNames() {
		RtMidiOut* midiout = new RtMidiOut();
		NumberOfPorts = midiout->getPortCount();
		if (NumberOfPorts == 0) {
			std::cout << "No MIDI ports available!" << std::endl;
			delete midiout;

			MidiDeviceNames.clear();
			return;
		}

		MidiDeviceNames.clear();
		for (int devId = 0; devId < NumberOfPorts;devId++) {
			try {
				MidiDeviceNames[devId] = midiout->getPortName(devId);
			}
			catch (RtMidiError& error) {
				std::cout << "(MIDI) " << error.getMessage() << std::endl;
			}
		}

		delete midiout;
	}

	void AutomateDownTuning() {
		if (!alreadyAutomatedTuningInThisSong) {
			Sleep(1500); // The menu is called when the animation starts. The tuning isn't set at that point, so we need to wait to get the value. This doesn't seem to lag the game.

			int* highestLowestTuning = MemHelpers::GetHighestLowestString();

			int highestTuning = highestLowestTuning[0];
			int lowestTuning = highestLowestTuning[1];
			
			int TrueTuning_Hertz = MemHelpers::GetTrueTuning();

			if (TrueTuning_Hertz < 260) { // Give some leeway for A220 and it's true tuned offsets
				highestTuning = (highestTuning - 12);
				std::cout << highestTuning << std::endl;
			}
			switch (highestTuning) { // Send target tuning to the pedal

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
					break;
			}
			alreadyAutomatedTuningInThisSong = true;
		}
	}

	void AutomateTrueTuning() {
		if (!alreadyAutomatedTrueTuningInThisSong) {
			alreadyAutomatedTrueTuningInThisSong = true;
			int TrueTuning_Hertz = MemHelpers::GetTrueTuning();

			if (TrueTuning_Hertz == 440 || TrueTuning_Hertz == 220)
				return;

			int* highestLowestTuning = MemHelpers::GetHighestLowestString();

			int highestTuning = highestLowestTuning[0];
			int lowestTuning = highestLowestTuning[1];

			switch (TrueTuning_Hertz) {

			// Below C1 bass
			case 220: 
				break;

			// Below A440
			case 425: // Light My Fire
				SendProgramChange(4);
				SendControlChange(36);
				break;
			case 428: // Castles Made Of Sand
				SendProgramChange(4);
				SendControlChange(30);
				break;
			case 431: // Heaven's On Fire | If 6 Was 9
				SendProgramChange(4);
				SendControlChange(20);
				break;
			case 434: // Cemetery Gates
				SendProgramChange(4);
				SendControlChange(15);
				break;
			case 437: // Voodoo Child (Slight Return)
				SendProgramChange(4);
				SendControlChange(5);
				break;

			// A440
			case 440: 
				break;

			// Above A440
			case 443: // Champagne Supernova | 20th Century Boy | Love Spreads
				SendProgramChange(3);
				SendControlChange(3);
				break;
			case 444: // Dream On | Ultra Soul
				SendProgramChange(3);
				SendControlChange(4);
				break;
			case 445: // The End of the World | People Are Strange
				SendProgramChange(3);
				SendControlChange(5);
				break;
			case 446: // Balls To The Wall | Spoonful | Rock and Roll, Hoochie Koo | Love Me Two Times
				SendProgramChange(3);
				SendControlChange(6);
				break;
			case 447: // No More Mr. Nice Guy | Machinehead | Behind Blue Eyes | Manic Depression
				SendProgramChange(3);
				SendControlChange(7);
				break;
			case 448: // UNKNOWN
				SendProgramChange(3);
				SendControlChange(8);
				break;
			case 449: // Live Forever
				SendProgramChange(3);
				SendControlChange(9);
				break;
			case 450: // Some Might Say
				SendProgramChange(3);
				SendControlChange(10);
				break;
			case 451: // Don't Look Back In Anger
				SendProgramChange(3);
				SendControlChange(11);
				break;
			case 454: // Baba O'Riley
				SendProgramChange(3);
				SendControlChange(14);
				break;
			case 455: // Black Hole Sun
				SendProgramChange(3);
				SendControlChange(15);
				break;
			case 456: // Friday I'm In Love
				SendProgramChange(3);
				SendControlChange(16);
				break;
			case 461: // You Really Got Me
				SendProgramChange(3);
				SendControlChange(22); // The song is so close to 462 that tuning is easier to do with the 462 pedal settings.
				break;

			// True Tuning Not Documented. If a user wants to give us the tuning, we should be able to get it to work.
			default:
				break;
			}
		}
	}

	void RevertAutomatedTuning() { // Turn off the pedal after we are done with a song.
		if (lastPC != 78) { // If the song is in E Standard, and we leave, it tries to use "Bypass +2 OCT Whammy" | If the song required a drop tuning & a true tuning.
			int cache = lastPC;
			if (lastPC_TUNING != 0 && lastPC_TUNING != lastPC) {  // If the user was in a song that requires a down tune AND true tuning, we use this. Ex: If 6 was 9 (Eb Standard AND A431)
				std::cout << "Attmepting to turn off true tuning" << std::endl;
				SendProgramChange(WHAMMY_DT_activeBypassMap.find(lastPC_TUNING)->second);
			}
			std::cout << "Attmepting to turn off drop tuning" << std::endl;
			SendProgramChange(WHAMMY_DT_activeBypassMap.find(cache)->second); // Send the bypass code to revert back to normal guitar.
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
}
