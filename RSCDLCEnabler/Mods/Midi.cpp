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

	void Midi::AutomateDownTuning() {
		if (!alreadyAutomatedTuningInThisSong) {
			int highestTuning = 0, tuningBuffer = 0;
			Sleep(1500); // The menu is called when the animation starts. The tuning isn't set at that point, so we need to wait to get the value. This doesn't seem to lag the game.
			byte* TuningArray = MemHelpers::GetCurrentTuning();


			// Get Highest And Lowest Strings
			for (int i = 0; i < 6; i++) {

				// Create a buffer so we can work on a value near 256, and not worry about the tunings at, and above, E Standard that start at 0 because the tuning number breaks the 256 limit of a unsigned char.
				tuningBuffer = (int)TuningArray[i];
				if (tuningBuffer <= 24) // 24 would be 2 octaves above E standard which is where RSMods cuts the tuning numbers at. Anything above maybe +3 should never be used, but for consistency we allow it.
					tuningBuffer += 256;

				// Find the highest tuned string.
				if (highestTuning < tuningBuffer)
					highestTuning = tuningBuffer;
			}

			// Change tuning number (256 = E Standard, 255 = Eb Standard, 254 D Standard, etc) to drop number (-1 = Eb Standard, -2 D Standard, etc).
			highestTuning -= 256;
			
			switch (highestTuning) { // Send target tuning to the pedal

				// Above E Standard
				case 1:
					SendDataToThread_PC(42); // F Standard
					break;
				case 2:
					SendDataToThread_PC(43); // F# Standard
					break;
				case 3:
					SendDataToThread_PC(44); // G Standard
					break;
				case 4:
					SendDataToThread_PC(45); // Ab Standard
					break;
				case 5:
					SendDataToThread_PC(46); // A Standard
					break;
				case 6:
					SendDataToThread_PC(47); // Bb Standard
					break;
				case 7:
					SendDataToThread_PC(48); // B Standard
					break;
				case 12:
					SendDataToThread_PC(49); // E Standard +OCT
					break;

				// E Standard
				case 0:
					SendDataToThread_PC(78);
					break;

				// Below E Standard
				case -1:
					SendDataToThread_PC(59); // Eb Standard
					break;
				case -2:
					SendDataToThread_PC(58); // D Standard
					break;
				case -3:
					SendDataToThread_PC(57); // C# Standard
					break;
				case -4:
					SendDataToThread_PC(56); // C Standard
					break;
				case -5:
					SendDataToThread_PC(55); // B Standard
					break;
				case -6:
					SendDataToThread_PC(54); // Bb Standard
					break;
				case -7:
					SendDataToThread_PC(53); // A Standard
					break;
				case -12:
					SendDataToThread_PC(52); // E Standard -OCT
					break;
				
				// The pedal can't handle the tuning.
				default:
					SendDataToThread_PC(78);
					break;
			}
			alreadyAutomatedTuningInThisSong = true;
		}
	}

	void Midi::AutomateTrueTuning() {
		if (!alreadyAutomatedTrueTuningInThisSong) {
			int TrueTuning_Hertz = MemHelpers::GetTrueTuning();

			if (TrueTuning_Hertz == 440 || TrueTuning_Hertz == 220)
				return;

			if (MemHelpers::GetCurrentTuning()[0] == (byte)255) {
				Midi::SendDataToThread_PC(59);
				lastPC_TUNING = 59;
				Sleep(sleepFor); // Make sure we have this command done before we start messing with the PC and CC comands below.
			}

			switch (TrueTuning_Hertz) {

			// Below C1 bass
			case 220: 
				break;

			// Below A440
			case 425: // Waiting on ZZ to get back to me on this one. Light My Fire
				//Midi::SendDataToThread_PC(4);
				//Midi::SendDataToThread_CC(30);
				break;
			case 428: // Castles Made Of Sand
				Midi::SendDataToThread_PC(4);
				Midi::SendDataToThread_CC(30);
				break;
			case 431: // Heaven's On Fire | If 6 Was 9
				Midi::SendDataToThread_PC(4);
				Midi::SendDataToThread_CC(20);
				break;
			case 434: // Cemetery Gates
				Midi::SendDataToThread_PC(4);
				Midi::SendDataToThread_CC(15);
				break;
			case 437: // Voodoo Child (Slight Return)
				Midi::SendDataToThread_PC(4);
				Midi::SendDataToThread_CC(5);
				break;

			// A440
			case 440: 
				break;

			// Above A440
			case 443: // Champagne Supernova | 20th Century Boy | Love Spreads
				Midi::SendDataToThread_PC(3);
				Midi::SendDataToThread_CC(3);
				break;
			case 444: // Dream On | Ultra Soul
				Midi::SendDataToThread_PC(3);
				Midi::SendDataToThread_CC(4);
				break;
			case 445: // The End of the World | People Are Strange
				Midi::SendDataToThread_PC(3);
				Midi::SendDataToThread_CC(5);
				break;
			case 446: // Balls To The Wall | Spoonful | Rock and Roll, Hoochie Koo | Love Me Two Times
				Midi::SendDataToThread_PC(3);
				Midi::SendDataToThread_CC(6);
				break;
			case 447: // No More Mr. Nice Guy | Machinehead | Behind Blue Eyes | Manic Depression
				Midi::SendDataToThread_PC(3);
				Midi::SendDataToThread_CC(7);
				break;
			case 448: // UNKNOWN
				Midi::SendDataToThread_PC(3);
				Midi::SendDataToThread_CC(8);
				break;
			case 449: // Live Forever
				Midi::SendDataToThread_PC(3);
				Midi::SendDataToThread_CC(9);
				break;
			case 450: // Some Might Say
				Midi::SendDataToThread_PC(3);
				Midi::SendDataToThread_CC(10);
				break;
			case 451: // Don't Look Back In Anger
				Midi::SendDataToThread_PC(3);
				Midi::SendDataToThread_CC(11);
				break;
			case 454: // Baba O'Riley
				Midi::SendDataToThread_PC(3);
				Midi::SendDataToThread_CC(14);
				break;
			case 455: // Black Hole Sun
				Midi::SendDataToThread_PC(3);
				Midi::SendDataToThread_CC(15);
				break;
			case 456: // Friday I'm In Love
				Midi::SendDataToThread_PC(3);
				Midi::SendDataToThread_CC(16);
				break;
			case 461: // You Really Got Me
				Midi::SendDataToThread_PC(3);
				Midi::SendDataToThread_CC(22); // The song is so close to 462 that tuning is easier to do with the 462 pedal settings.
				break;

			// True Tuning Not Documented. If a user wants to give us the tuning, we should be able to get it to work.
			default:
				break;
			}
			alreadyAutomatedTrueTuningInThisSong = true;
		}
	}

	void Midi::RevertAutomatedTuning() { // Turn off the pedal after we are done with a song.
		if (lastPC != 78 || lastPC_TUNING != 0) { // If the song is in E Standard, and we leave, it tries to use "Bypass +2 OCT Whammy" | If the song required a drop tuning & a true tuning.
			std::cout << "Attmepting to turn off your drop pedal" << std::endl;
			SendDataToThread_PC(WHAMMY_DT_activeBypassMap.find(lastPC)->second); // Send the bypass code to revert back to normal guitar.
			SendDataToThread_CC(0); // Reset the expression pedal
		}
		alreadyAutomatedTuningInThisSong = false;
		alreadyAutomatedTrueTuningInThisSong = false;
	}

	void Midi::SendDataToThread_PC(char program, bool shouldWeSendPC) {
		sendPC = shouldWeSendPC;
		dataToSendPC = program;
	}
	void Midi::SendDataToThread_CC(char toePosition, bool shouldWeSendCC) {
		sendCC = shouldWeSendCC;
		dataToSendCC = toePosition;
	}
}
