#include "../stdafx.h"
#include "Midi.hpp"

namespace Setting = Settings::Setting;

namespace {
	// Split a string on a delimiter, keeping the trailing piece even if the user forgot a trailing comma.
	std::vector<std::string> SplitString(std::string str, const std::string& delim) {
		std::vector<std::string> parts;
		size_t position = 0;
		while ((position = str.find(delim)) != std::string::npos) {
			parts.push_back(str.substr(0, position));
			str.erase(0, position + delim.length());
		}
		parts.push_back(str);
		return parts;
	}
}

// Midi codes should follow this guide: http://fmslogo.sourceforge.net/manual/midi-table.html
namespace Midi {
	std::vector<MIDIOUTCAPSA> midiOutDevices;
	std::vector<MIDIINCAPSA> midiInDevices;
	int SelectedMidiOutDevice = 0;
	int SelectedMidiInDevice = 0;
	int MidiCC = 0;
	int MidiPC = 666;
	unsigned int NumberOfOutPorts;
	unsigned int NumberOfInPorts;

	/// <summary>
	/// Startup MIDI processing
	/// </summary>
	void InitMidi() {
		try {
			// Try to init a Midi in device.
			RtMidiIn midiin;
			LOG_INFO("Starting MIDI" << std::endl);
		}
		catch (RtMidiError& error) {
			LOG_ERROR("(MIDI) " << error.getMessage() << std::endl);
		}
	}

	/// <summary>
	/// Get all MIDI devices connected.
	/// </summary>
	void GetMidiDeviceNames() {
		// Get the number of Midi Out, and Midi In ports
		NumberOfOutPorts = midiOutGetNumDevs();
		NumberOfInPorts = midiInGetNumDevs();

		// For each Midi Out device, get it's capabilities.
		for (unsigned int device = 0; device < NumberOfOutPorts; device++) {
			MIDIOUTCAPSA temp;
			midiOutGetDevCapsA(device, &temp, sizeof(MIDIOUTCAPSA));
			midiOutDevices.push_back(temp);
		}

		// For each Midi In device, get it's capabilities.
		for (unsigned int device = 0; device < NumberOfInPorts; device++) {
			MIDIINCAPSA temp;
			midiInGetDevCapsA(device, &temp, sizeof(MIDIINCAPSA));
			midiInDevices.push_back(temp);
		}
	}

	/// <summary>
	/// Look through all MIDI devices to hook the one specified by the user.
	/// </summary>
	/// <param name="ChordsMode"> - Does the user have a pedal with Chorus Mode, and is it being used?</param>
	/// <param name="PedalToUse"> - What pedal is the user using?</param>
	/// <param name="MidiOutDevice"> - Name of MIDI device to send MIDI to.</param>
	/// <param name="MidiInDevice"> - Name of MIDI device to listen to.</param>
	void ReadMidiSettingsFromINI(const std::string& ChordsMode, int PedalToUse, const std::string& MidiOutDevice, const std::string& MidiInDevice) {
		// Is Chords mode on (only some Digitech pedals support it.)
		if (ChordsMode == "on") { 
			Digitech::DIGITECH_CHORDS_MODE = true;
			LOG_INFO("(MIDI) Chords Mode: Enabled" << std::endl);
		}
		
		// Verify that we have an actual pedal specified.
		if (PedalToUse != NULL)
			selectedPedal = supportedPedals.at((PedalToUse - 1) % supportedPedals.size());
		else
			return;

		LOG_INFO("(MIDI) Pedal To Use: " << selectedPedal.pedalName << std::endl);

		// Fill the list of Midi In, and Midi Out devices.
		GetMidiDeviceNames();

		// Find, and select, the devices that the user specified in their INI file.
		FindMidiOutDevices(MidiOutDevice);
		FindMidiInDevices(MidiInDevice);
	}

	void FindMidiOutDevices(const std::string& deviceToLookFor) {
		for (unsigned int device = 0; device < NumberOfOutPorts; device++) {

			std::string deviceName = "";

			// Parse Char Buffer, since the name is a null-terminated string, we need to terminate it ourselves in a string.
			for (int i = 0; i < 32; i++) {
				if (midiOutDevices.at(device).szPname[i] == (char)0)
					break;
				deviceName.push_back(midiOutDevices.at(device).szPname[i]);
			}

			// We found the device that is specified in the INI.
			if (deviceName.find(deviceToLookFor) != std::string::npos) {
				LOG_INFO("(MIDI) Connecting To Midi OUT Device: " << midiOutDevices.at(device).szPname << std::endl);
				SelectedMidiOutDevice = device;
				break;
			}
			// This is not the device specified in the INI.
			else
				LOG_INFO("(MIDI) Available MIDI OUT device: " << midiOutDevices.at(device).szPname << std::endl);
		}
	}

	void FindMidiInDevices(const std::string& deviceToLookFor) {
		for (unsigned int device = 0; device < NumberOfInPorts; device++) {
			std::string_view deviceName(midiInDevices[device].szPname);

			// We found the device that is specified in the INI.
			if (deviceName.find(deviceToLookFor) != std::string::npos) {
				LOG_INFO("(MIDI) Connecting To Midi IN Device: " << midiInDevices.at(device).szPname << std::endl);
				SelectedMidiInDevice = device;
				break;
			}
			else
				LOG_INFO("(MIDI) Available MIDI IN device: " << midiInDevices.at(device).szPname << std::endl);
		}
	}

	bool IsValidMidiMessage(std::vector<unsigned char>* message) {
		size_t messageSize = message->size();

		if (messageSize < 1) // Empty Message
			return false;

		switch (message->at(0)) {
			case MidiCommands::NoteOff:
				if (messageSize != 3) {
					LOG_ERROR("(MIDI IN) Invalid NoteOff" << std::endl);
					return false;
				}
				LOG_INFO("(MIDI IN) NoteOff. Key = " << (int)message->at(1) << ". Velocity = " << (int)message->at(2) << std::endl);
				break;
			case MidiCommands::NoteOn:
				if (messageSize != 3) {
					LOG_ERROR("(MIDI IN) Invalid NoteOn" << std::endl);
					return false;
				}
				LOG_INFO("(MIDI IN) NoteOn. Key = " << (int)message->at(1) << ". Velocity = " << (int)message->at(2) << std::endl);
				break;
			case MidiCommands::AfterTouch:
				if (messageSize != 3) {
					LOG_ERROR("(MIDI IN) Invalid AfterTouch" << std::endl);
					return false;
				}
				LOG_INFO("(MIDI IN) AfterTouch. Key = " << (int)message->at(1) << ". Touch = " << (int)message->at(2) << std::endl);
				break;
			case MidiCommands::CC:
				if (messageSize != 3) {
					LOG_ERROR("(MIDI IN) Invalid CC" << std::endl);
					return false;
				}

				LOG_INFO("(MIDI IN) CC. Controller# = " << (int)message->at(1) << ". Value = " << std::dec << (int)message->at(2) << std::endl);

				// Current Midi In mod to test | Midi RR Speed > 100%
				RiffRepeater::SetSpeed(static_cast<float>(message->at(2) * 2), true);
				RiffRepeater::EnableTimeStretch();

				// Midi In Mod already tested | Controlling WAH with expression pedal.
					//WwiseVariables::Wwise_Sound_SetRTPCValue_Char("Pedal_USWah_Auto", 1, 0x1234, 0, AkCurveInterpolation_Linear);
					//WwiseVariables::Wwise_Sound_SetRTPCValue_Char("Pedal_USWah_Auto", 1, AK_INVALID_GAME_OBJECT, 0, AkCurveInterpolation_Linear);
					//WwiseVariables::Wwise_Sound_SetRTPCValue_Char("Pedal_USWah_Sens", ceil((int)message->at(2) / 127.f * 100.f), 0x1234, 0, AkCurveInterpolation_Linear);
					//WwiseVariables::Wwise_Sound_SetRTPCValue_Char("Pedal_USWah_Sens", ceil((int)message->at(2) / 127.f * 100.f), AK_INVALID_GAME_OBJECT, 0, AkCurveInterpolation_Linear);

					//WwiseVariables::Wwise_Sound_SetRTPCValue_Char("Bass_Pedal_BassWah_Auto", 1, 0x1234, 0, AkCurveInterpolation_Linear);
					//WwiseVariables::Wwise_Sound_SetRTPCValue_Char("Bass_Pedal_BassWah_Auto", 1, AK_INVALID_GAME_OBJECT, 0, AkCurveInterpolation_Linear);
					//WwiseVariables::Wwise_Sound_SetRTPCValue_Char("Bass_Pedal_BassWah_Sens", ceil((int)message->at(2) / 127.f * 100.f), 0x1234, 0, AkCurveInterpolation_Linear);
					//WwiseVariables::Wwise_Sound_SetRTPCValue_Char("Bass_Pedal_BassWah_Sens", ceil((int)message->at(2) / 127.f * 100.f), AK_INVALID_GAME_OBJECT, 0, AkCurveInterpolation_Linear);

					//WwiseVariables::Wwise_Sound_SetRTPCValue_Char("Pedal_ModernWah_Auto", 1, 0x1234, 0, AkCurveInterpolation_Linear);
					//WwiseVariables::Wwise_Sound_SetRTPCValue_Char("Pedal_ModernWah_Auto", 1, AK_INVALID_GAME_OBJECT, 0, AkCurveInterpolation_Linear);
					//WwiseVariables::Wwise_Sound_SetRTPCValue_Char("Pedal_ModernWah_Sens", ceil((int)message->at(2) / 127.f * 100.f), 0x1234, 0, AkCurveInterpolation_Linear);
					//WwiseVariables::Wwise_Sound_SetRTPCValue_Char("Pedal_ModernWah_Sens", ceil((int)message->at(2) / 127.f * 100.f), AK_INVALID_GAME_OBJECT, 0, AkCurveInterpolation_Linear);

					//WwiseVariables::Wwise_Sound_SetRTPCValue_Char("Pedal_UKWah_Auto", 1, 0x1234, 0, AkCurveInterpolation_Linear);
					//WwiseVariables::Wwise_Sound_SetRTPCValue_Char("Pedal_UKWah_Auto", 1, AK_INVALID_GAME_OBJECT, 0, AkCurveInterpolation_Linear);
					//WwiseVariables::Wwise_Sound_SetRTPCValue_Char("Pedal_UKWah_Sens", ceil((int)message->at(2) / 127.f * 100.f), 0x1234, 0, AkCurveInterpolation_Linear);
					//WwiseVariables::Wwise_Sound_SetRTPCValue_Char("Pedal_UKWah_Sens", ceil((int)message->at(2) / 127.f * 100.f), AK_INVALID_GAME_OBJECT, 0, AkCurveInterpolation_Linear);
				break;
			case MidiCommands::PC:
				if (messageSize != 2) {
					LOG_ERROR("(MIDI IN) Invalid PC" << std::endl);
					return false;
				}
				LOG_INFO("(MIDI IN) PC. Instrument# = " << (int)message->at(1) << std::endl);
				break;
			case MidiCommands::Pressure:
				if (messageSize != 2) {
					LOG_ERROR("(MIDI IN) Invalid Pressure" << std::endl);
					return false;
				}
				break;
			case MidiCommands::PitchBend: // Pitch Bend
				if (messageSize != 3) {
					LOG_ERROR("(MIDI IN) Invalid PitchBend" << std::endl);
					return false;
				}
				LOG_INFO("(MIDI IN) PitchBend. LSB = " << (int)message->at(1) << ". MSB = " << (int)message->at(2) << std::endl);
				break;
			default:
				if (message->at(0) >= 0xF0) { // System Command
					LOG_INFO("(MIDI IN) System: ");
					for (size_t i = 0; i < messageSize; i++) {
						LOG_NOHEAD ((int)message->at(i) << " ");
					}
					LOG_NOHEAD("" << std::endl);
				}
				else { // Unknown Midi Command
					LOG_WARNING("(MIDI IN) Unknown: ");
					for (size_t i = 0; i < messageSize; i++) {
						LOG_NOHEAD((int)message->at(i) << " ");
					}
					LOG_NOHEAD("" << std::endl);
					return false; 
				}
				break;
		}
		return true;
	}

	void RespondToMidiIn(double, std::vector<unsigned char>* message, void*) {
		IsValidMidiMessage(message);
	}

	unsigned WINAPI ListenToMidiInThread() {
		auto midiin = std::make_unique<RtMidiIn>();

		NumberOfInPorts = midiInGetNumDevs();
		if (NumberOfInPorts == 0) {
			LOG_ERROR("No MIDI IN ports available!" << std::endl);
			return 0u - 1u;
		}

		midiin->openPort(SelectedMidiInDevice);
		midiin->setCallback(&RespondToMidiIn);
		midiin->ignoreTypes(false, false, true);

		while (!GameState::GameClosing) {
			Sleep(50);
		}
		return 0;
	}

	/// <summary>
	/// Send PC to pedal.
	/// </summary>
	/// <param name="programChange"> - Value of PC</param>
	/// <param name="alternativeChannel"> - Channel to use over the default. Mainly used for software pedals.</param>
	/// <returns>Message was sent or not.</returns>
	bool SendProgramChange(char programChange, char alternativeChannel) {
		auto midiout = std::make_unique<RtMidiOut>();
		std::vector<unsigned char> message;

		char channel = alternativeChannel == kUnusedMidiSelector ? selectedPedal.PC_Channel : alternativeChannel; // If we want to bypass the PC channel (mainly for software pedals) then alternative channel won't be default.

		// Are we using a dummy pedal?
		if (selectedPedal.pedalName == MidiPedal().pedalName) {
			LOG_ERROR("(MIDI) SendPC: DUMMY PEDAL" << std::endl);
			return false;
		}

		// Check available ports.
		NumberOfOutPorts = midiOutGetNumDevs();
		if (NumberOfOutPorts == 0) {
			LOG_ERROR("No MIDI OUT ports available!" << std::endl);
			sendPC = false;
			return false;
		}

		try {
			midiout->openPort(SelectedMidiOutDevice);

			// Send MIDI message
			message.push_back(programChangeStatus + channel); // Say it's a Program Change + channel
			message.push_back(programChange); // What program we changing to?
			midiout->sendMessage(&message);

			LOG_INFO("Sent Midi Message: PC" << (int)channel << ": " << (int)programChange << std::endl);
		}
		catch (RtMidiError& error) {
			LOG_ERROR("(MIDI) Error: " << error.getMessage() << std::endl);
		}

		sendPC = false;
		lastPC = programChange;
		return true;
	}

	/// <summary>
	/// Send CC to pedal
	/// </summary>
	/// <param name="toePosition"> - Value of CC</param>
	/// <param name="alternativeBank"> - Bank to use over the default one. Mainly used for software pedals.</param>
	/// <param name="alternativeChannel"> - Channel to use over the default. Mainly used for software pedals.</param>
	/// <returns>Message wwas sent or not.</returns>
	bool SendControlChange(char toePosition, char alternativeBank, char alternativeChannel) {
		char bank = alternativeBank == kUnusedMidiSelector ? selectedPedal.CC_Bank : alternativeBank; // If we want to bypass the CC bank (mainly for software pedals) then alternative bank won't be default.
		char channel = alternativeChannel == kUnusedMidiSelector ? selectedPedal.CC_Channel : alternativeChannel; // If we want to bypass the CC channel (mainly for software pedals) then alternative channel won't be default.

		if (selectedPedal.pedalName == MidiPedal().pedalName) {
			LOG_ERROR("(MIDI) SendCC: DUMMY PEDAL" << std::endl);
			return false;
		}

		auto midiout = std::make_unique<RtMidiOut>();
		std::vector<unsigned char> message;

		// Check available ports.
		NumberOfOutPorts = midiOutGetNumDevs();
		if (NumberOfOutPorts == 0) {
			LOG_ERROR("No MIDI ports available!" << std::endl);
			sendCC = false;
			return false;
		}

		try {
			// Open first available port.
			midiout->openPort(SelectedMidiOutDevice);

			// Send MIDI Message
			message.push_back(controlChangeStatus + channel); // Say it's a Control Change + channel#
			message.push_back(bank); // Bank to change
			message.push_back(toePosition); // New Control Value || 0 = off, 127 = on
			midiout->sendMessage(&message);

			LOG_INFO("Sending Midi Message: CC" << (int)channel << ": " << (int)bank << " " << (int)toePosition << std::endl);
		}
		catch (RtMidiError& error) {
			LOG_ERROR("(MIDI) Error: " << error.getMessage() << std::endl);
		}

		sendCC = false;
		lastCC = toePosition;
		return true;
	}

	/// <summary>
	/// Formats a tuning, true tuning, and instrument for the log, ex: "Tuning = { -2, 0, 0, 0, 0, 0 } . True Tuning = A440 . IsBass = false"
	/// </summary>
	static std::string DescribeTuning(const std::array<byte, 6>& tuning, int trueTuning_Hertz, bool bass) {
		std::ostringstream description;
		description << "Tuning = { ";
		for (size_t i = 0; i < tuning.size(); i++)
			description << (i ? ", " : "") << static_cast<int>(static_cast<signed char>(tuning[i])); // The game stores -5 as 251.
		description << " } . True Tuning = A" << trueTuning_Hertz << " . IsBass = " << std::boolalpha << bass;
		return description.str();
	}

	/// <summary>
	/// Send a command to the pedal to change to a specific setting based on the current song's tuning.
	/// </summary>
	void AutomateTuning() {
		if (!alreadyAutomatedTuningInThisSong) {
			alreadyAutomatedTuningInThisSong = true;

			if (!selectedPedal.supportsDropTuning) {
				LOG_ERROR("Your pedal doesn't support drop tuning." << std::endl);
				return;
			}

			Sleep(1500); // The menu is called when the animation starts. The tuning isn't set at that point, so we need to wait to get the value. This doesn't seem to lag the game.

			const bool bass = SongTuning::IsPlayerOnBass();
			std::array<int, 2> highestLowestTuning = SongTuning::GetHighestLowestString(bass); // Bass has 4 strings; charts often leave 5 and 6 at 0.

			int highestTuning = highestLowestTuning[0];
			int lowestTuning = highestLowestTuning[1];

			// Invalid pointer check
			if (highestTuning == 666 && lowestTuning == 666) {
				LOG_ERROR("(MIDI) Unable to read tuning in song." << std::endl);
				return;
			}

			int TrueTuning_Hertz = SongTuning::GetTrueTuning();

			// A220 is a -1200 cent offset: charts raise every string by 12 (B standard bass is 7) and drop the reference an
			// octave. GetHighestLowestString's A220 adjustment cancels out and the pedal functions double A220 back to A440, so
			// the octave has to come off here.
			if (TrueTuning_Hertz < 260)
				highestTuning -= 12;

			selectedPedal.autoTuneFunction(highestTuning + tuningOffset, static_cast<float>(TrueTuning_Hertz));

			LOG_INFO("(MIDI) Triggered Mod: Automated Tuning (Song) " << DescribeTuning(SongTuning::GetCurrentTuning(), TrueTuning_Hertz, bass) << std::endl);
		}
	}

	void AttemptTuningInTuner() {
		if (!alreadyAttemptedTuningInTuner) {
			alreadyAttemptedTuningInTuner = true;
			tunerAutoTuneFailed = false;

			if (!selectedPedal.supportsDropTuning) {
				LOG_ERROR("(MIDI) Your pedal doesn't support drop tuning." << std::endl);
				return;
			}

			// The menu is reported as soon as its animation starts, before the tuner has ticked. Give it up to 2 seconds.
			Tuning tunerTuning;
			for (int waitedMs = 0; waitedMs < 2000; waitedMs += 100) {
				tunerTuning = SongTuning::GetTuningAtTuner(false);
				if (tunerTuning.lowE != Tuning().lowE)
					break;
				Sleep(100);
			}
			if (tunerTuning.lowE == Tuning().lowE)
				tunerTuning = SongTuning::GetTuningAtTuner(); // Once more, logging why it failed.

			const bool bass = SongTuning::IsPlayerOnBass();
			std::array<int, 2> highestLowestTuning = SongTuning::GetHighestLowestString(tunerTuning, bass);

			int highestTuning = highestLowestTuning[0];
			int lowestTuning = highestLowestTuning[1];

			// Couldn't read the tuner. The player is now tuning by hand to what the tuner shows, so tuning the pedal once the
			// song starts would stack on top of that. Leave this song alone.
			if (highestTuning == 666 && lowestTuning == 666) {
				tunerAutoTuneFailed = true;
				LOG_ERROR("(MIDI) Cannot read tuning in tuner. Not automating tuning for this song, since it would stack on the manual tuning." << std::endl);
				return;
			}

			int TrueTuning_Hertz = SongTuning::GetTrueTuning();

			// Same A220 adjustment as the song path (see AutomateTuning).
			if (TrueTuning_Hertz < 260)
				highestTuning -= 12;

			selectedPedal.autoTuneFunction(highestTuning + tuningOffset, static_cast<float>(TrueTuning_Hertz));

			const std::array<byte, 6> tunerStrings = { tunerTuning.lowE, tunerTuning.strA, tunerTuning.strD, tunerTuning.strG, tunerTuning.strB, tunerTuning.highE };
			LOG_INFO("(MIDI) Triggered Mod: Automated Tuning (Tuner) " << DescribeTuning(tunerStrings, TrueTuning_Hertz, bass) << std::endl);
			alreadyAutomatedTuningInThisSong = true;
		}
	}

	/// <summary>
	/// Send a command to the pedal to turn off the modifications we did for the current song's tuning, and true-tuning.
	/// </summary>
	void RevertAutomatedTuning() { // Turn off the pedal after we are done with a song.
		if (selectedPedal.pedalName == MidiPedal().pedalName)
			return;

		if (lastPC != 666 || selectedPedal.softwarePedal) { // If the song is in E Standard, and we leave, it tries to use "Bypass +2 OCT Whammy"
			LOG_INFO("(MIDI) Attmepting to turn off automatic tuning" << std::endl);

			// User is using a software pedal (or a custom defined pedal).
			if (selectedPedal.softwarePedal) {
				// Did we drop tune in the last song?
				if (Midi::Software::sentSemitoneInThisSong) {
					if (Midi::Software::sendSemitoneCommand == programChangeStatus)
						SendProgramChange(Midi::Software::semiToneShutoffTrigger);
					else if (Midi::Software::sendSemitoneCommand == controlChangeStatus)
						SendControlChange(Midi::Software::semiToneShutoffTrigger);

					Midi::Software::sentSemitoneInThisSong = false;
				}
				// Did we true tune in the last song?
				if (Midi::Software::sentTrueTuningInThisSong) {
					if (Midi::Software::sendTrueTuningCommand == programChangeStatus)
						SendProgramChange(Midi::Software::trueTuningShutoffTrigger, Midi::Software::sendTrueTuningChannel);
					else if (Midi::Software::sendTrueTuningCommand == controlChangeStatus)
						SendControlChange(Midi::Software::trueTuningShutoffTrigger, Midi::Software::trueTuningBank, Midi::Software::sendTrueTuningChannel);
				}

				// Reset the bools.
				Midi::Software::sentTrueTuningInThisSong = false;
				alreadyAutomatedTuningInThisSong = false;
				alreadyAutomatedTrueTuningInThisSong = false;
				return;
			}
			
			std::map<char, char> activeBypassMap = selectedPedal.activeBypassMap;
			char originalPC = AsMidiByte(lastPC);

			// If the user was in a song that requires a down tune AND true tuning, we use this. Ex: If 6 was 9 (Eb Standard AND A431)
			if (lastPC_TUNING != 0 && lastPC_TUNING != lastPC)  
				SendProgramChange(activeBypassMap.find(AsMidiByte(lastPC_TUNING))->second);

			// Send the bypass code to revert back to normal guitar.
			if (auto it = activeBypassMap.find(originalPC); it != activeBypassMap.end())
				SendProgramChange(it->second);

			// Reset the expression pedal
			if (lastCC != 0)
				SendControlChange(0); 
		}

		alreadyAutomatedTuningInThisSong = false;
		alreadyAutomatedTrueTuningInThisSong = false;
		lastPC_TUNING = 0;
	}

	/// <summary>
	/// Send PC Value Async
	/// </summary>
	/// <param name="program"> - Value of PC</param>
	/// <param name="shouldWeSendPC"> - Should we stop sending PC values?</param>
	void SendDataToThread_PC(char program, bool shouldWeSendPC) {
		sendPC = shouldWeSendPC;
		dataToSendPC = program;
	}

	/// <summary>
	/// Send CC Value Async
	/// </summary>
	/// <param name="toePosition"> - Value of CC</param>
	/// <param name="shouldWeSendCC"> - Should we stop sending CC values?</param>
	void SendDataToThread_CC(char toePosition, bool shouldWeSendCC) {
		sendCC = shouldWeSendCC;
		dataToSendCC = toePosition;
	}

	/// <summary>
	/// Returns the tuning name based on what the offset is.
	/// </summary>
	/// <param name="offset"> - How far from E standard is the user.</param>
	/// <returns> - Tuning Name</returns>
	std::string GetTuningOffsetName(int offset) {
		switch (offset) {
			case -3: 
				return "G Standard / Drop F (Up)";
				break;
			case -2:
				return "F# Standard / Drop E (Up)";
				break;
			case -1:
				return "F Standard / Drop Eb (Up)";
				break;
			case 0:
				return "E Standard / Drop D";
				break;
			case 1:
				return "Eb Standard / Drop C#";
				break;
			case 2:
				return "D Standard / Drop C";
				break;
			case 3:
				return "C# Standard / Drop B";
				break;
			case 4:
				return "C Standard / Drop A#";
				break;
			case 5:
				return "B Standard / Drop A";
				break;
			case 6:
				return "A# Standard / Drop G#";
				break;
			case 7:
				return "A Standard / Drop G";
				break;
			case 8:
				return "G# Standard / Drop F#";
				break;
			case 9:
				return "G Standard / Drop F (Down)";
				break;
			case 10:
				return "F# Standard / Drop E (Down)";
				break;
			case 11:
				return "F Standard / Drop E (Down)";
				break;
			case 12:
				return "E Standard / Drop Eb (Down)";
				break;
			default:
				return "UNKNOWN TUNING";
				break;
		}
	}

	// Pedal Specific Functions

	// Digitech
	namespace Digitech {
		namespace WhammyDT {
			/// <summary>
			/// Auto Tune with the pedal "Digitech Whammy DT". Tuning names (in comments) are based off a guitar in E Standard.
			/// </summary>
			/// <param name="highestTuning"> - Highest tuned string in the current song's tuning</param>
			void AutoTuning(int highestTuning, float TrueTuning_Hertz) {
				bool alreadyAttemptedTrueTune = false;

				switch (highestTuning) {

					// Above E Standard
				case 12:
					SendProgramChange(49); // E Standard +OCT
					lastPC_TUNING = 49;
					break;
				case 11:
					[[fallthrough]];
				case 10:
					[[fallthrough]];
				case 9:
					[[fallthrough]];
				case 8:
					[[fallthrough]];
				case 7:

					SendProgramChange(48); // B Standard
					lastPC_TUNING = 48;

					if (highestTuning > 7) { // If the pedal doesn't have a dedicated setting, let's force it to work ;) | Needs to be below SendPC or it will not turn off the Whammy side
						AutoTrueTuningPastLimits(highestTuning - 7, TrueTuning_Hertz);
						alreadyAttemptedTrueTune = true;
					}
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
					[[fallthrough]];
				case -8:
					[[fallthrough]];
				case -9:
					[[fallthrough]];
				case -10:
					[[fallthrough]];
				case -11:
					SendProgramChange(53); // A Standard
					lastPC_TUNING = 53;

					if (highestTuning < -7) { // If the pedal doesn't have a dedicated setting, let's force it to work ;) | Needs to be below SendPC or it will not turn off the Whammy side
						AutoTrueTuningPastLimits(highestTuning + 7, TrueTuning_Hertz);
						alreadyAttemptedTrueTune = true;
					}
					break;
				case -12:
					SendProgramChange(52); // E Standard -OCT
					lastPC_TUNING = 52;
					break;

				default:

					if (highestTuning <= 24.0f || highestTuning >= -36.0f) { // Attempt to fake it, and maybe we'll make it.
						AutoTrueTuningPastLimits(highestTuning, TrueTuning_Hertz);
						alreadyAttemptedTrueTune = true;
					}
					else
						lastPC = 666; // Doesn't do anything, just sets the proper lastPC.
					break;
				}

				if (!alreadyAttemptedTrueTune && TrueTuning_Hertz != 440.f && TrueTuning_Hertz != 220.f)
					AutoTrueTuning(static_cast<int>(TrueTuning_Hertz));
			}

			/// <summary>
			/// Auto Tuning (past limits of pedal) and True-Tune with the pedal "Digitech Whammy DT". Based on the work done by PoizenJam.
			/// </summary>
			/// <param name="relativeTuning"> - Tuning from song minus what has already been tuned</param>
			/// <param name="TrueTuning_Hertz"> - True tuning (non-concert pitch)</param>
			void AutoTrueTuningPastLimits(int relativeTuning, float TrueTuning_Hertz) {
				int temp_PC, temp_CC;
				float Target_Hertz, Target_Semitones;

				// Find Target Hertz of combined True Tuning and Drop Tuning. If A < 260, double it before calculation.
				if (TrueTuning_Hertz < 260.0f)
					Target_Hertz = (float)(TrueTuning_Hertz * 2.0f * powf(2.0f, (relativeTuning / 12.0f)));
				else
					Target_Hertz = (float)(TrueTuning_Hertz * powf(2.0f, (relativeTuning / 12.0f)));

				// Convert Target_Hertz to Semitones(relative to A440)
				Target_Semitones = (float)(12.0f * log2(Target_Hertz / 440.0f));

				// Calculate PC needed to achieve target Semitones. If-Else block :(
				if ((roundf(Target_Semitones * 100) / 100) < -24.00f)
					temp_PC = 9;
				else if ((roundf(Target_Semitones * 100) / 100) < -12.00f)
					temp_PC = 8;
				else if ((roundf(Target_Semitones * 100) / 100) < -7.00f)
					temp_PC = 7;
				else if ((roundf(Target_Semitones * 100) / 100) < -5.00f)
					temp_PC = 6;
				else if ((roundf(Target_Semitones * 100) / 100) < 0.00f)
					temp_PC = 5;
				else if ((roundf(Target_Semitones * 100) / 100) < -2.00f)
					temp_PC = 4;
				else if ((roundf(Target_Semitones * 100) / 100) < 5.00f)
					temp_PC = 3;
				else if ((roundf(Target_Semitones * 100) / 100) < 7.00f)
					temp_PC = 2;
				else if ((roundf(Target_Semitones * 100) / 100) < 12.00f)
					temp_PC = 1;
				else
					temp_PC = 0;

				temp_CC = static_cast<int>(roundf(Target_Semitones * (127.0f / Digitech::WhammyDT::semiTones.at(temp_PC))));

				// Does the song actually NEED us to do any changes?
				if (temp_CC != 0) {
					SendProgramChange(AsMidiByte(temp_PC));
					SendControlChange(AsMidiByte(temp_CC));
				}
			}

			/// <summary>
			/// Auto True-Tune with the pedal "Digitech Whammy DT"
			/// </summary>
			/// <param name="TrueTuning_Hertz"> - True Tuning (non-concert pitch)</param>
			void AutoTrueTuning(int TrueTuning_Hertz) {

				if (TrueTuning_Hertz < 260)
					TrueTuning_Hertz *= 2;

				if (TrueTuning_Hertz == 440) // A440
					return;


				// Above A440
				else if (TrueTuning_Hertz > 440) {
					SendProgramChange(3); // Leave at +4th
					SendControlChange((char)(TrueTuning_Hertz - 440));
				}

				// Below A440
				else {
					SendProgramChange(4); // Leave at -2nd
					SendControlChange((char)round(1127.43 - (2.56667 * TrueTuning_Hertz)));
				}
			}
		}
		namespace BassWhammy {
			/// <summary>
			/// Auto Tune and True-Tune with the pedal "Digitech Whammy Bass". Made by PoizenJam.
			/// </summary>
			/// <param name="highestTuning"> - Highest tuned string in the current song.</param>
			/// <param name="TrueTuning_Hertz"> - True Tuning (non-concert pitch)</param>
			void AutoTuningAndTrueTuning(int highestTuning, float TrueTuning_Hertz) {
				int temp_PC, temp_CC, offset = 0;
				float Target_Hertz, Target_Semitones;

				// Chords Mode Offset
				if (Digitech::DIGITECH_CHORDS_MODE)
					offset = 42;

				// Find Target Hertz of combined True Tuning and Drop Tuning. If A < 260, double it before calculation.
				if (TrueTuning_Hertz < 260.0f)
					Target_Hertz = (float)(TrueTuning_Hertz * 2.0f * powf(2.0f, (highestTuning / 12.0f)));
				else
					Target_Hertz = (float)(TrueTuning_Hertz * powf(2.0f, (highestTuning / 12.0f)));

				// Convert Target_Hertz to Semitones(relative to A440)
				Target_Semitones = (float)(12.0f * log2(Target_Hertz / 440.0f));

				// Calculate PC (pre-digitech offset) needed to achieve target Semitones. If-Else block :(
				if ((roundf(Target_Semitones * 100) / 100) < -12.00f)
					temp_PC = 10;
				else if ((roundf(Target_Semitones * 100) / 100) < -7.00f)
					temp_PC = 9;
				else if ((roundf(Target_Semitones * 100) / 100) < -5.00f)
					temp_PC = 8;
				else if ((roundf(Target_Semitones * 100) / 100) < -2.00f)
					temp_PC = 7;
				else if ((roundf(Target_Semitones * 100) / 100) < 0.00f)
					temp_PC = 6;
				else if ((roundf(Target_Semitones * 100) / 100) < 2.00f)
					temp_PC = 5;
				else if ((roundf(Target_Semitones * 100) / 100) < 5.00f)
					temp_PC = 4;
				else if ((roundf(Target_Semitones * 100) / 100) < 7.00f)
					temp_PC = 3;
				else if ((roundf(Target_Semitones * 100) / 100) < 12.00f)
					temp_PC = 2;
				else
					temp_PC = 1;

				temp_CC = static_cast<int>(roundf(Target_Semitones * (127.0f / Digitech::BassWhammy::semiTones.at(temp_PC - 1))));

				// Does the song actually NEED us to do any changes?
				if (temp_CC != 0) {
					SendProgramChange(AsMidiByte(temp_PC + offset - 1));
					SendControlChange(AsMidiByte(temp_CC));
				}
			}
		}
		namespace WhammyFour {
			/// <summary>
			/// Auto Tune and True-Tune with the pedal "Digitech Whammy 4". Whammy 5 Made by PoizenJam, interpolated to Whammy 4 by Ffio.
			/// </summary>
			/// <param name="highestTuning"> - Highest tuned string in the current song.</param>
			/// <param name="TrueTuning_Hertz"> - True Tuning (non-concert pitch)</param>
			void AutoTuningAndTrueTuning(int highestTuning, float TrueTuning_Hertz) {
				int temp_PC, temp_CC, offset = 0;
				float Target_Hertz, Target_Semitones;

				// Find Target Hertz of combined True Tuning and Drop Tuning. If A < 260, double it before calculation.
				if (TrueTuning_Hertz < 260.0f)
					Target_Hertz = (float)(TrueTuning_Hertz * 2.0f * powf(2.0f, (highestTuning / 12.0f)));
				else
					Target_Hertz = (float)(TrueTuning_Hertz * powf(2.0f, (highestTuning / 12.0f)));

				// Convert Target_Hertz to Semitones(relative to A440)
				Target_Semitones = (float)(12.0f * log2(Target_Hertz / 440.0f));

				// Calculate PC needed to achieve target Semitones. If-Else block :(
				if ((roundf(Target_Semitones * 100) / 100) < -24.00f)
					temp_PC = 7;
				else if ((roundf(Target_Semitones * 100) / 100) < -12.00f)
					temp_PC = 6;
				else if ((roundf(Target_Semitones * 100) / 100) < 0.00f)
					temp_PC = 5;
				else if ((roundf(Target_Semitones * 100) / 100) < 12.00f)
					temp_PC = 4;
				else
					temp_PC = 3;

				temp_CC = static_cast<int>(roundf(Target_Semitones * (127.0f / Digitech::WhammyFour::semiTones.at(temp_PC - 3))));

				// Does the song actually NEED us to do any changes?
				if (temp_CC != 0) {
					SendProgramChange(AsMidiByte(temp_PC + offset - 1));
					SendControlChange(AsMidiByte(temp_CC));
				}
			}
		}

		namespace WhammyFive {
			/// <summary>
			/// Auto Tune and True-Tune with the pedal "Digitech Whammy 5". Made by PoizenJam.
			/// </summary>
			/// <param name="highestTuning"> - Highest tuned string in the current song.</param>
			/// <param name="TrueTuning_Hertz"> - True Tuning (non-concert pitch)</param>
			void AutoTuningAndTrueTuning(int highestTuning, float TrueTuning_Hertz) {
				int temp_PC;
				int temp_CC;
				int offset = 0;
				float Target_Hertz;
				float Target_Semitones;

				// Chords Mode Offset
				if (Digitech::DIGITECH_CHORDS_MODE)
					offset = 42;

				// Find Target Hertz of combined True Tuning and Drop Tuning. If A < 260, double it before calculation.
				if (TrueTuning_Hertz < 260.0f)
					Target_Hertz = (float)(TrueTuning_Hertz * 2.0f * powf(2.0f, (highestTuning / 12.0f)));
				else
					Target_Hertz = (float)(TrueTuning_Hertz * powf(2.0f, (highestTuning / 12.0f)));

				// Convert Target_Hertz to Semitones(relative to A440)
				Target_Semitones = (float)(12.0f * log2(Target_Hertz / 440.0f));

				// Calculate PC needed to achieve target Semitones. If-Else block :(
				if ((roundf(Target_Semitones * 100) / 100) < -24.00f)
					temp_PC = 10;
				else if ((roundf(Target_Semitones * 100) / 100) < -12.00f)
					temp_PC = 9;
				else if ((roundf(Target_Semitones * 100) / 100) < -7.00f)
					temp_PC = 8;
				else if ((roundf(Target_Semitones * 100) / 100) < -5.00f)
					temp_PC = 7;
				else if ((roundf(Target_Semitones * 100) / 100) < -2.00f)
					temp_PC = 6;
				else if ((roundf(Target_Semitones * 100) / 100) < 0.00f)
					temp_PC = 5;
				else if ((roundf(Target_Semitones * 100) / 100) < 5.00f)
					temp_PC = 4;
				else if ((roundf(Target_Semitones * 100) / 100) < 7.00f)
					temp_PC = 3;
				else if ((roundf(Target_Semitones * 100) / 100) < 12.00f)
					temp_PC = 2;
				else
					temp_PC = 1;

				temp_CC = static_cast<int>(roundf(Target_Semitones * (127.0f / Digitech::WhammyFive::semiTones.at(temp_PC - 1))));

				// Does the song actually NEED us to do any changes?
				if (temp_CC != 0) {
					SendProgramChange(AsMidiByte(temp_PC + offset - 1));
					SendControlChange(AsMidiByte(temp_CC));
				}
			}
		}
	}

	namespace Software {
		/// <summary>
		/// Auto tuning for a software pedal
		/// </summary>
		/// <param name="highestTuning"> - Highest tuned string in the current song.</param>
		/// <param name="TrueTuning_Hertz"> - True Tuning (non-concert pitch)</param>
		void AutoTuning(int highestTuning, float TrueTuning_Hertz) {
			// User wants us to reload our settings.
			if (Settings::async_UpdateMidiSettings) {
				ReloadSettings();
				Settings::async_UpdateMidiSettings = false;
			}

			// Send MIDI command to pedal if we find the tuning in the INI.
			if (auto it = semiToneMap.find(AsMidiByte(highestTuning)); it != semiToneMap.end()) {
				if (sendSemitoneCommand == programChangeStatus)
					SendProgramChange(it->second);
				else if (sendSemitoneCommand == controlChangeStatus)
					SendControlChange(it->second);

				sentSemitoneInThisSong = true;
			}
			else
				LOG_ERROR("(MIDI) Software Pedal Error: Attempted to tune to " << highestTuning << " but the user doesn't have a value set for it." << std::endl);

			// Bass fix was applied, so we need to adjust the true tuning.
			if (TrueTuning_Hertz < 260.f)
				TrueTuning_Hertz *= 2;

			if (TrueTuning_Hertz != 440)
				AutoTrueTuning(TrueTuning_Hertz);
		}

		void AutoTrueTuning(float TrueTuning_Hertz) {
			if (TrueTuning_Hertz < 260.f)
				TrueTuning_Hertz *= 2;

			// Send MIDI command to pedal if we find the true tuning in the INI.
			if (auto it = trueTuningMap.find(static_cast<int>(TrueTuning_Hertz)); it != trueTuningMap.end()) {
				if (sendTrueTuningCommand == programChangeStatus)
					SendProgramChange(it->second, sendTrueTuningChannel);
				else if (sendTrueTuningCommand == controlChangeStatus)
					SendControlChange(it->second, trueTuningBank, sendTrueTuningChannel);

				Midi::Software::sentTrueTuningInThisSong = true;
			}
			else
				LOG_ERROR("(MIDI) Software Pedal Error: Attempted to truetune to A" << TrueTuning_Hertz << " but the user doesn't have a value set for it." << std::endl);
		}

		void ReloadSettings() {
			semiToneMap.clear();
			trueTuningMap.clear();

			LoadSemitoneSettings();
			FillSemitoneMap();
			FillTrueTuningMap();
			LoadTrueTuningSettings();
		}

		void FillSemitoneMap() {
			std::vector<std::string> separated = SplitString(Settings::ReturnSettingValue(Setting::AutoTuneForSoftwareSemitoneTriggers), ", ");

			std::string delim = " ";

			// List of triggers (string) -> Map of triggers (char, char)
			for (auto& entry : separated) {

				std::string semiTone_string = entry.substr(0, entry.find(delim));
				std::string command_string = entry.erase(0, entry.find(delim) + delim.length());
				char semiTone = AsMidiByte(std::atoi(semiTone_string.c_str()));
				char command = AsMidiByte(std::atoi(command_string.c_str()));

				if (!semiToneMap.contains(semiTone))
					semiToneMap[semiTone] = command;
				else
					LOG_ERROR("(MIDI) Software Pedal Error: Semitone Triggers for "
							  << semiTone
							  << " is already set to "
							  << semiToneMap[semiTone]
							  << " found in the "
							  << std::distance(semiToneMap.begin(), semiToneMap.find(semiTone))
							  << " position"
							  << std::endl);
			}

			LOG_INFO("(MIDI) Software Pedal: Semitone Triggers" << std::endl);

			for (const auto& [semiTone, command] : semiToneMap)
			{
				LOG_NOHEAD("Semitone = " << (int)semiTone << ", Value To Send = " << (int)command << std::endl);
			}

			LOG_INFO("(MIDI) Software Pedal: Semitone Triggers --END" << std::endl);
		}

		void LoadSemitoneSettings() {
			std::vector<std::string> separated = SplitString(Settings::ReturnSettingValue(Setting::AutoTuneForSoftwareSemitoneSettings), ", ");


			// Example: 24, PC, 4
			// Send PC on channel 4 (0 indexed). When we leave a song, send 24 to PC channel 4.

			// Example: 45, CC, 12, 15
			// Send CC on channel 12 (0 indexed). Use bank 15. When we leave a song, send 45 to CC channel 12 bank 15.

			if (separated.size() > 0) {
				semiToneShutoffTrigger = static_cast<unsigned char>(std::atoi(separated.at(0).c_str()));

				if (separated.size() > 1) {
					if (separated.at(1) == "PC") {
						sendSemitoneCommand = programChangeStatus;
						if (separated.size() > 2)
							selectedPedal.PC_Channel = AsMidiByte(std::atoi(separated.at(2).c_str()) % 16); // Only 16 channels are supported.
						else
							LOG_ERROR("SOFTWARE LOADSETTINGS ERROR (semitones): PC set as the send method but no channel is specified" << std::endl);
					}
						
					if (separated.at(1) == "CC") {
						sendSemitoneCommand = controlChangeStatus;
						if (separated.size() > 2) {
							selectedPedal.CC_Channel = AsMidiByte(std::atoi(separated.at(2).c_str()) % 16); // Only 16 channels are supported.
							if (separated.size() > 3)
								selectedPedal.CC_Bank = AsMidiByte(std::atoi(separated.at(3).c_str()));
							else
								LOG_ERROR("SOFTWARE LOAD SETTINGS ERROR (semitones): CC set as the send method but no bank is specified. Defaulting to Bank 0." << std::endl);
						}
						else
							LOG_ERROR("SOFTWARE LOADSETTINGS ERROR (semitones): CC set as the send method but no channel is specified. Defaulting to Channel 0." << std::endl);
					}
				}
			}
			else {
				LOG_ERROR("SOFTWARE LOADSETTINGS ERROR (semitones): No settings to load!" << std::endl);
			}
		}

		void FillTrueTuningMap() {
			std::vector<std::string> separated = SplitString(Settings::ReturnSettingValue(Setting::AutoTuneForSoftwareTrueTuningTriggers), ", ");

			std::string delim = " ";

			// List of triggers (string) -> Map of triggers (int, char)
			for (auto& entry : separated) {

				std::string semiTone_string = entry.substr(0, entry.find(delim));
				std::string command_string = entry.erase(0, entry.find(delim) + delim.length());
				int trueTuning = std::atoi(semiTone_string.c_str());
				char command = AsMidiByte(std::atoi(command_string.c_str()));

				if (!trueTuningMap.contains(trueTuning))
					trueTuningMap[trueTuning] = command;
				else
					LOG_ERROR("(MIDI) Software Pedal Error: TrueTuning Triggers for "
							  << trueTuning
							  << " is already set to "
							  << trueTuningMap[trueTuning]
							  << " found in the "
							  << std::distance(trueTuningMap.begin(), trueTuningMap.find(trueTuning))
							  << " position"
						      << std::endl);
			}

			LOG_INFO("(MIDI) Software Pedal: TrueTuning Triggers" << std::endl);

			for (const auto& [trueTuning, command] : trueTuningMap)
			{
				LOG_NOHEAD("True Tuning = A" << trueTuning << ", Value To Send = " << (int)command << std::endl);
			}
				
			LOG_INFO("(MIDI) Software Pedal: TrueTuning Triggers --END" << std::endl);
		}

		void LoadTrueTuningSettings() {
			std::vector<std::string> separated = SplitString(Settings::ReturnSettingValue(Setting::AutoTuneForSoftwareTrueTuningSettings), ", ");

			// Example: 24, PC, 4
			// Send PC on channel 4 (0 indexed). When we leave a song, send 24 to PC channel 4.

			// Example: 45, CC, 12, 15
			// Send CC on channel 12 (0 indexed). Use bank 15. When we leave a song, send 45 to CC channel 12 bank 15.

			if (separated.size() > 0) {
				trueTuningShutoffTrigger = static_cast<unsigned char>(std::atoi(separated.at(0).c_str()));

				if (separated.size() > 1) {
					if (separated.at(1) == "PC") {
						sendTrueTuningCommand = programChangeStatus;
						if (separated.size() > 2)
							sendTrueTuningChannel = static_cast<unsigned char>(std::atoi(separated.at(2).c_str()) % 16); // Only 16 channels are supported.
						else
							LOG_ERROR("SOFTWARE LOADSETTINGS ERROR (truetuning): PC set as the send method but no channel is specified" << std::endl);
					}
					if (separated.at(1) == "CC") {
						sendTrueTuningCommand = controlChangeStatus;

						if (separated.size() > 2) {
							sendTrueTuningChannel = static_cast<unsigned char>(std::atoi(separated.at(2).c_str()) % 16); // Only 16 channels are supported.
							if (separated.size() > 3)
								trueTuningBank = static_cast<unsigned char>(std::atoi(separated.at(3).c_str()));
							else
								LOG_ERROR("SOFTWARE LOAD SETTINGS ERROR (truetuning): CC set as the send method but no bank is specified. Defaulting to Bank 0." << std::endl);
						}
						else
							LOG_ERROR("SOFTWARE LOADSETTINGS ERROR (truetuning): CC set as the send method but no channel is specified. Defaulting to Channel 0." << std::endl);

					}
				}
			}
			else {
				LOG_ERROR("SOFTWARE LOADSETTINGS ERROR (truetuning): No settings to load!" << std::endl);
			}
		}
	}
}
