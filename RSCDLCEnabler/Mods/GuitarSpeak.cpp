#include "GuitarSpeak.hpp"

/*
Modified / Translated from the Visual Basic code provided by UKLooney
Open source here: https://cdn.discordapp.com/attachments/711634414240530462/735574443035721798/G2RS_v0.3_source.zip
*/

byte GuitarSpeak::GetCurrentNote() {
	uintptr_t noteAdr = MemUtil::FindDMAAddy(Offsets::baseHandle + Offsets::ptr_guitarSpeak, Offsets::ptr_guitarSpeakOffets);

	if (!noteAdr) {
		std::cout << "(GS) Note Address can't be found!" << std::endl;
		return (BYTE)noNote;
	}

	return *(byte*)noteAdr;
}

std::string GuitarSpeak::StringCurrentNote() {
	byte currentNote = GetCurrentNote();

	if (currentNote == noNote || currentNote == endOfNote || currentNote >= 96)
		return "";

	int octave = (currentNote / 12) - 1; // The game starts reading at -1 instead of 0 so we need to offset by -1.
	int note = currentNote % 12;

	return noteLetters.at(note) + std::to_string(octave);
}

bool GuitarSpeak::RunGuitarSpeak() {
	FillKeyList();

	while (true) {
		Sleep(timer);

		std::string currentMenu = MemHelpers::GetCurrentMenu();

		if (MemHelpers::IsInStringArray(currentMenu, NULL, tuningMenus) && Settings::ReturnSettingValue("GuitarSpeakWhileTuning") == "off") { // If someone wants to tune in the setting menu they skip the check
			if (verbose)
				std::cout << "(GS) Entered Tuning Menu! Stopping Guitar Speak." << std::endl;
			break; // We aren't needed here anymore.
		}

		if (MemHelpers::IsInStringArray(currentMenu, NULL, lessonModes) || MemHelpers::IsInStringArray(currentMenu, NULL, songMenus) || MemHelpers::IsInStringArray(currentMenu, NULL, calibrationMenus)) {
			if (verbose)
				std::cout << "(GS) Entered Song Menu! Stopping Guitar Speak." << std::endl;
			break; // We aren't needed here anymore.
		}

		int currentNote = GetCurrentNote();
		std::string buttonToPress = "";

		if (currentNote >= 96) // The limit of Rocksmith | If you go over 96, the values will fall out of the array and the game will crash as it's looking for a value that doesn't exist | C7
			currentNote = noNote;

		if (verbose && currentNote != endOfNote && currentNote != noNote)
			std::cout << "(GS) Note: " << currentNote << std::endl;

		lastNoteBuffer = currentNoteBuffer;
		currentNoteBuffer = currentNote;

		if (currentNote == lastNoteBuffer && currentNote != currentNoteBuffer)
			currentNoteBuffer = currentNote;

		if (currentNote != lastNoteBuffer)
			lastNoteBuffer = currentNote;

		if (currentNoteBuffer != endOfNote && currentNoteBuffer != lastNote) { // New note detected
			newNote = true;
			lastNote = currentNote;

			if (verbose)
				std::cout << "(GS) New Note Detected" << std::endl;
		}
		if (currentNoteBuffer == endOfNote && lastNote != noNote) { // If the note ends
			newNote = false;
			currentNote = 0;
			lastNote = 0;

			if (verbose)
				std::cout << "(GS) Note Ended" << std::endl;
		}

		if (newNote && currentNote != noNote)
			buttonToPress = strKeyList[currentNote];

		newNote = false;

		if (buttonToPress.c_str() != "") {
			if (sendKeystrokesToRS2014) { // We should send a keystroke, and the key being pressed isn't null.

				if (buttonToPress == (std::string)"CLOSE") {
					sendKeystrokesToRS2014 = false;

					if (verbose)
						std::cout << "(GS) Closing Guitar Speak." << std::endl;

					break; // We aren't needed here anymore.
				}

				else if(keyToVKey.find(buttonToPress) != keyToVKey.end()) {
					PostMessage(FindWindow(NULL, L"Rocksmith 2014"), WM_KEYDOWN, keyToVKey.find(buttonToPress)->second, 0);
					Sleep(30);
					PostMessage(FindWindow(NULL, L"Rocksmith 2014"), WM_KEYUP, keyToVKey.find(buttonToPress)->second, 0);

					if (verbose)
						std::cout << "(GS) " << keyToVKey.find(buttonToPress)->first << " was used by Guitar Speak." << std::endl;
				}
			}
			else { // We shouldn't be reading commands here
				if (buttonToPress == (std::string)"CLOSE") { // In this case, we want to re-open Guitar Speak. Use-case: User already closed Guitar Speak previously, but pressed the button again which indicates they want to re-enable us.
					sendKeystrokesToRS2014 = true;

					if (verbose)
						std::cout << "(GS) Reopening Guitar Speak. " << std::endl;
				}
			}
		}
	}
	return false;
}

void GuitarSpeak::FillKeyList() {
	strKeyList[Settings::GetModSetting("GuitarSpeakDelete")] = "DELETE";
	strKeyList[Settings::GetModSetting("GuitarSpeakSpace")] = "SPACE";
	strKeyList[Settings::GetModSetting("GuitarSpeakEnter")] = "ENTER";
	strKeyList[Settings::GetModSetting("GuitarSpeakTab")] = "TAB";
	strKeyList[Settings::GetModSetting("GuitarSpeakPageUp")] = "PGUP";
	strKeyList[Settings::GetModSetting("GuitarSpeakPageDown")] = "PGDN";
	strKeyList[Settings::GetModSetting("GuitarSpeakUpArrow")] = "UP";
	strKeyList[Settings::GetModSetting("GuitarSpeakDownArrow")] = "DOWN";
	strKeyList[Settings::GetModSetting("GuitarSpeakEscape")] = "ESCAPE";
	strKeyList[Settings::GetModSetting("GuitarSpeakClose")] = "CLOSE";
	strKeyList[Settings::GetModSetting("GuitarSpeakOBracket")] = "OBRACKET";
	strKeyList[Settings::GetModSetting("GuitarSpeakCBracket")] = "CBRACKET";
	strKeyList[Settings::GetModSetting("GuitarSpeakTildea")] = "TILDEA";
	strKeyList[Settings::GetModSetting("GuitarSpeakForSlash")] = "FORSLASH";
}