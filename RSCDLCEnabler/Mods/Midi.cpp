#include "Midi.hpp"

// Midi codes should follow this guide: http://fmslogo.sourceforge.net/manual/midi-table.html

bool Midi::SendMidiMessage(char programChange, char toeDown) {
    RtMidiOut* midiout = new RtMidiOut();
    std::vector<unsigned char> message;

    // Check available ports.
    unsigned int nPorts = midiout->getPortCount();
    if (nPorts == 0) {
        std::cout << "No MIDI ports available!" << std::endl;
        delete midiout;
        return false;
    }

    // Open first available port.
    midiout->openPort(0);

    if (programChange != NULL) {
        message.push_back(programChangeStatus);
        message.push_back(programChange);
        std::cout << "Sending Midi Message: " << "PC: " << message.back() << std::endl;
        midiout->sendMessage(&message);
    }

    if (toeDown != NULL) {
        message[0] = controlChangeStatus;
        message[1] = WHAMMY_DT_CC_CHANNEL;
        message[2] = toeDown;
        std::cout << "Sending Midi Message: " << "CC: " << message[1] << " " << message[2] << std::endl;
        midiout->sendMessage(&message);
    }

    // Clean up
    delete midiout;
    return true;
}

void Midi::InitMidi() {
    try {
        RtMidiIn midiin;
        std::cout << "Starting Midi" << std::endl;
    }
    catch (RtMidiError& error) {
        std::cout << error.getMessage() << std::endl;
    }
}