#include "Midi.hpp"

// Midi codes should follow this guide: http://fmslogo.sourceforge.net/manual/midi-table.html

void Midi::InitMidi() {
    try {
        RtMidiIn midiin;
        std::cout << "Starting MIDI" << std::endl;
    }
    catch (RtMidiError& error) {
        std::cout << error.getMessage() << std::endl;
    }
}

bool Midi::SendProgramChange(char programChange) {
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

    // Send MIDI message
    message.push_back(programChangeStatus); // It's a Program Change
    message.push_back(programChange); // What program we changing?
    std::cout << "Sending Midi Message: " << "PC: " << (int)message.back() << std::endl;
    midiout->sendMessage(&message);

    // Clean up
    delete midiout;
    return true;
}

bool Midi::SendControlChange(char toePosition) {
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
   
    //// Send MIDI Message
    //message[0] = controlChangeStatus; // Say it's a Control Change
    //message[1] = WHAMMY_DT_CC_CHANNEL; // Control to change
    //message[2] = toePosition; // New Control Value || 0 = off, 127 = on
    //std::cout << "Sending Midi Message: " << "PC: " << (int)message[2] << std::endl;
    //midiout->sendMessage(&message);

    // Clean up
    delete midiout;
    return true;
}
