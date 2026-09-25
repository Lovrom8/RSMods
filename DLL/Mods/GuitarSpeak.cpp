#include "../stdafx.h"
#include "GuitarSpeak.hpp"

namespace Setting = Settings::Setting;

/*
Modified / Translated from the Visual Basic code provided by UKLooney
Open source here: https://github.com/uklooney/G2RS
*/

/// <summary>
/// Gets the current note being played in MIDI.
/// </summary>
/// <returns>Current Note (MIDI Number)</returns>
byte GuitarSpeak::GetCurrentNote() {
	uintptr_t noteAdr = MemUtil::FindDMAAddy(Offsets::baseHandle + Offsets::ptr_guitarSpeak, Offsets::ptr_guitarSpeakOffets);

	if (!noteAdr) { //TODO: check the address 
		//LOG_ERROR("(GS) Note Address can't be found!" << std::endl);
		return (BYTE)noNote;
	}

	return *(byte*)noteAdr;
}

/// <summary>
/// Convert MIDI Note to NoteOctave format (ex: 40 -> E2)
/// </summary>
/// <returns>NoteOctave (ex: E2)</returns>
std::string GuitarSpeak::GetCurrentNoteName() {
	byte currentNote = GetCurrentNote();

	if (currentNote == noNote || currentNote == endOfNote || currentNote >= 96)
		return "";

	int octave = (currentNote / 12) - 1; // The game starts reading at -1 instead of 0 so we need to offset by -1.
	int note = currentNote % 12;

	return noteLetters.at(note) + std::to_string(octave);
}

/// <summary>
/// Listen to all notes being played and press keys when specific notes are being played.
/// </summary>
/// <returns>Looped, but returns false if loop is broken.</returns>
bool GuitarSpeak::RunGuitarSpeak() {
	FillKeyList();

	while (true) {
		Sleep(timer);

		GameState::currentMenu = GameState::GetCurrentMenu();

		// If someone wants to tune in the setting menu they skip the check
		if (GameState::Menus::IsInTuningMenus() && Settings::IsOff(Setting::GuitarSpeakWhileTuning)) {
			if (verbose)
				LOG_INFO("(GS) Entered Tuning Menu! Stopping Guitar Speak." << std::endl);
			break; // We aren't needed here anymore.
		}

		// Disabled due to entering Lessons, a song, or a calibration menu.
		if (GameState::Menus::IsInLessonModes() || GameState::IsInSong() || GameState::Menus::IsInCalibrationMenus()) {
			if (verbose)
				LOG_INFO("(GS) Entered Song Menu! Stopping Guitar Speak." << std::endl);
			break; // We aren't needed here anymore.
		}

		int currentNote = GetCurrentNote();
		std::string buttonToPress = "";

		// The limit of Rocksmith.
		// If you go over 96, the values will fall out of the array and the game will crash as it's looking for a value that doesn't exist | C7
		if (currentNote >= 96) 
			currentNote = noNote;

		if (verbose && currentNote != endOfNote && currentNote != noNote)
			LOG_INFO("(GS) Note: " << currentNote << std::endl);

		lastNoteBuffer = currentNoteBuffer;
		currentNoteBuffer = currentNote;

		if (currentNote == lastNoteBuffer && currentNote != currentNoteBuffer)
			currentNoteBuffer = currentNote;

		if (currentNote != lastNoteBuffer)
			lastNoteBuffer = currentNote;

		// New note detected
		if (currentNoteBuffer != endOfNote && currentNoteBuffer != lastNote) { 
			newNote = true;
			lastNote = currentNote;

			if (verbose)
				LOG_INFO("(GS) New Note Detected" << std::endl);
		}
		// If the note ends
		if (currentNoteBuffer == endOfNote && lastNote != noNote) { 
			newNote = false;
			currentNote = 0;
			lastNote = 0;

			if (verbose)
				LOG_INFO("(GS) Note Ended" << std::endl);
		}

		if (newNote && currentNote != noNote)
			buttonToPress = strKeyList[currentNote];

		newNote = false;

		if (!buttonToPress.empty()) {

			// We should send a keystroke, and the key being pressed isn't null.
			if (sendKeystrokesToRS2014) { 

				if (buttonToPress == (std::string)"CLOSE") {
					sendKeystrokesToRS2014 = false;

					if (verbose)
						LOG_INFO("(GS) Closing Guitar Speak." << std::endl);

					break; // We aren't needed here anymore.
				}

				// Press the key the user set for this note.
				else if (auto it = keyToVKey.find(buttonToPress); it != keyToVKey.end()) {
					PostMessage(D3DHooks::GetGameWindow(), WM_KEYDOWN, it->second, 0);
					Sleep(30);
					PostMessage(D3DHooks::GetGameWindow(), WM_KEYUP, it->second, 0);

					if (verbose)
						LOG_INFO("(GS) " << it->first << " was used by Guitar Speak." << std::endl);
				}
			}
			// We shouldn't be reading commands here
			else { 

				// In this case, we want to re-open Guitar Speak. Use-case: User already closed Guitar Speak previously, but pressed the button again which indicates they want to re-enable us.
				if (buttonToPress == (std::string)"CLOSE") { 
					sendKeystrokesToRS2014 = true;

					if (verbose)
						LOG_INFO("(GS) Reopening Guitar Speak. " << std::endl);
				}
			}
		}
	}
	return false;
}

/// <summary>
/// Add available keybinds
/// </summary>
void GuitarSpeak::FillKeyList() {
	strKeyList[Settings::GetModSetting(Setting::GuitarSpeakDelete)]	= "DELETE";
	strKeyList[Settings::GetModSetting(Setting::GuitarSpeakSpace)]		= "SPACE";
	strKeyList[Settings::GetModSetting(Setting::GuitarSpeakEnter)]		= "ENTER";
	strKeyList[Settings::GetModSetting(Setting::GuitarSpeakTab)]		= "TAB";
	strKeyList[Settings::GetModSetting(Setting::GuitarSpeakPageUp)]	= "PGUP";
	strKeyList[Settings::GetModSetting(Setting::GuitarSpeakPageDown)]	= "PGDN";
	strKeyList[Settings::GetModSetting(Setting::GuitarSpeakUpArrow)]	= "UP";
	strKeyList[Settings::GetModSetting(Setting::GuitarSpeakDownArrow)] = "DOWN";
	strKeyList[Settings::GetModSetting(Setting::GuitarSpeakEscape)]	= "ESCAPE";
	strKeyList[Settings::GetModSetting(Setting::GuitarSpeakClose)]		= "CLOSE";
	strKeyList[Settings::GetModSetting(Setting::GuitarSpeakOBracket)]	= "OBRACKET";
	strKeyList[Settings::GetModSetting(Setting::GuitarSpeakCBracket)]	= "CBRACKET";
	strKeyList[Settings::GetModSetting(Setting::GuitarSpeakTildea)]	= "TILDEA";
	strKeyList[Settings::GetModSetting(Setting::GuitarSpeakForSlash)]	= "FORSLASH";
	strKeyList[Settings::GetModSetting(Setting::GuitarSpeakAlt)]		= "ALT";
}