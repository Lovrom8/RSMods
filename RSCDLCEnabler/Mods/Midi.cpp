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
			std::cout << error.getMessage() << std::endl;
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
			return false;
		}

		try {
			// Open first available port.
			midiout->openPort(SelectedMidiDevice);

			// Send MIDI message
			message.push_back(programChangeStatus); // It's a Program Change
			message.push_back(programChange); // What program we changing?
			std::cout << "Sending Midi Message: " << "PC: " << (int)message.back() << std::endl;
			midiout->sendMessage(&message);
		}
		catch (RtMidiError& error) {
			error.printMessage();
		}

		// Clean up
		delete midiout;
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
			return false;
		}

		try {
			// Open first available port.
			midiout->openPort(SelectedMidiDevice);

			// Send MIDI Message
			message.push_back(controlChangeStatus); // Say it's a Control Change
			message.push_back(WHAMMY_DT_CC_CHANNEL); // Control to change
			message.push_back(toePosition); // New Control Value || 0 = off, 127 = on
			std::cout << "Sending Midi Message: " << "PC: " << (int)message.rbegin()[1] << " " << (int)message.back() << std::endl;
			midiout->sendMessage(&message);
		}
		catch (RtMidiError& error) {
			error.printMessage();
		}

		// Clean up
		delete midiout;
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
				error.printMessage();
			}
		}

		delete midiout;
	}
}
