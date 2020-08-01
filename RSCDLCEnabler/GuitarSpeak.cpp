#include "GuitarSpeak.h"

cGuitarSpeak GuitarSpeak;

/*
Modified / Translated from the Visual Basic code provided by UKLooney
Open source here: https://cdn.discordapp.com/attachments/711634414240530462/735574443035721798/G2RS_v0.3_source.zip
*/

/* Moved over to the RSMods GUI to make INI reading easier with ints instead of parasing out the int (octave) out of the char* note.
int cGuitarSpeak::NoteToInt(char* note, int octave){
	int notePlayed = 0;
	octave = octave - 2; // The first full set of notes is at the 2nd octave so we do the math as if they were our zero-point.

	if (note == "C")
		notePlayed = 36;
	else if (note == "Db" || note == "C#")
		notePlayed = 37;
	else if (note == "D")
		notePlayed = 38;
	else if (note == "Eb" || note == "D#")
		notePlayed = 39;
	else if (note == "E")
		notePlayed = 40;
	else if (note == "F")
		notePlayed = 41;
	else if (note == "Gb" || note == "F#")
		notePlayed = 42;
	else if (note == "G")
		notePlayed = 43;
	else if (note == "Ab" || note == "G#")
		notePlayed = 44;
	else if (note == "A")
		notePlayed = 45;
	else if (note == "Bb" || note == "A#")
		notePlayed = 46;
	else if (note == "B")
		notePlayed = 47;

	if (notePlayed != 0x0)
		notePlayed = notePlayed + (octave * 12); // Ex. If note is G5 then 43 + ((5-2) * 12) = 79, so our end note is 79, not 43;

	return notePlayed;
}*/

byte cGuitarSpeak::GetCurrentNote() {
	return *(byte*)MemUtil.FindDMAAddy(Offsets.ptr_guitarSpeak, Offsets.ptr_guitarSpeakOffets);
}



void cGuitarSpeak::TimerTick() {
		bool debugMode = true;
		/*
		Template:
		strKeyList[noteID] = (std::string)"X-Variable" // Where "X-Variable" is a string in keyPressArray[].
		*/
		
	strKeyList[Settings.GetModSetting("Delete")] = (std::string)"DELETE";
	strKeyList[Settings.GetModSetting("Space")] = (std::string)"SPACE";
	strKeyList[Settings.GetModSetting("Enter")] = (std::string)"ENTER";
	strKeyList[Settings.GetModSetting("Tab")] = (std::string)"TAB";
	strKeyList[Settings.GetModSetting("PageUp")] = (std::string)"PGUP";
	strKeyList[Settings.GetModSetting("PageDown")] = (std::string)"PGDN";
	strKeyList[Settings.GetModSetting("UpArrow")] = (std::string)"UP";
	strKeyList[Settings.GetModSetting("DownArrow")] = (std::string)"DOWN";
	strKeyList[Settings.GetModSetting("Escape")] = (std::string)"ESCAPE";
	strKeyList[Settings.GetModSetting("CloseGuitarSpeak")] = (std::string)"CLOSE";
	



	while (true) {
		Sleep(timer);
		/*if (debugMode)
			std::cout << "Tick" << std::endl;*/

		int mem = GetCurrentNote();


		if (debugMode && mem != 0xFF)
			std::cout << "Note: " << mem << std::endl;

		lastNoteBuffer = currentNoteBuffer;
		currentNoteBuffer = mem;



		if (mem == lastNoteBuffer && mem != currentNoteBuffer)
			currentNoteBuffer = mem;
		if (mem != lastNoteBuffer)
			lastNoteBuffer = mem;

		if (currentNoteBuffer != 0xFF && currentNoteBuffer != lastNote) { // Check if there is a new note
			if (debugMode)
				std::cout << "New Note Detected" << std::endl;
			newNote = true;
			lastNote = mem;
		}
		if (currentNoteBuffer == 0xFF && lastNote != 0x0) { // If the note ends
			if (debugMode)
				std::cout << "Note Ended" << std::endl;
			newNote = false;
			mem = 0;
			lastNote = 0;
		}

		

		if (newNote) {
			std::string strSendTemp = "";

			if (mem != 0) {
				strSendTemp = strKeyList[mem];
			}

			if (sendKeystrokesToRS2014)
				strSend = strSendTemp;
		}
		newNote = false;

		if (sendKeystrokesToRS2014 && strSend != "") { // We sending a keystroke right?
			if (debugMode)
				std::cout << "Command Found!" << std::endl;

			if (strSend == keyPressArray[0]) { // Delete
				PostMessage(FindWindow(NULL, L"Rocksmith 2014"), WM_KEYDOWN, VK_DELETE, 0);
				Sleep(30);
				PostMessage(FindWindow(NULL, L"Rocksmith 2014"), WM_KEYUP, VK_DELETE, 0);
				if (debugMode)
					std::cout << "Delete was pressed!" << std::endl;
			}

			else if (strSend == keyPressArray[1]) { // Space
				PostMessage(FindWindow(NULL, L"Rocksmith 2014"), WM_KEYDOWN, VK_SPACE, 0);
				Sleep(30);
				PostMessage(FindWindow(NULL, L"Rocksmith 2014"), WM_KEYUP, VK_SPACE, 0);
				if (debugMode)
					std::cout << "Space was pressed!" << std::endl;
			}
				
			else if (strSend == keyPressArray[2]) { // Enter
				PostMessage(FindWindow(NULL, L"Rocksmith 2014"), WM_KEYDOWN, VK_RETURN, 0);
				Sleep(30);
				PostMessage(FindWindow(NULL, L"Rocksmith 2014"), WM_KEYUP, VK_RETURN, 0);
				if (debugMode)
					std::cout << "Enter was pressed!" << std::endl;
			}
			else if (strSend == keyPressArray[3]) { // Tab
				PostMessage(FindWindow(NULL, L"Rocksmith 2014"), WM_KEYDOWN, VK_TAB, 0);
				Sleep(30);
				PostMessage(FindWindow(NULL, L"Rocksmith 2014"), WM_KEYUP, VK_TAB, 0);
				if (debugMode)
					std::cout << "Tab was pressed!" << std::endl;
			}

			else if (strSend == keyPressArray[4]) { // Page UP
				PostMessage(FindWindow(NULL, L"Rocksmith 2014"), WM_KEYDOWN, VK_PRIOR, 0);
				Sleep(30);
				PostMessage(FindWindow(NULL, L"Rocksmith 2014"), WM_KEYUP, VK_PRIOR, 0);
				if (debugMode)
					std::cout << "Page Up was pressed!" << std::endl;
			}
			else if (strSend == keyPressArray[5]) { // Page DOWN
				PostMessage(FindWindow(NULL, L"Rocksmith 2014"), WM_KEYDOWN, VK_NEXT, 0);
				Sleep(30);
				PostMessage(FindWindow(NULL, L"Rocksmith 2014"), WM_KEYUP, VK_NEXT, 0);
				if (debugMode)
					std::cout << "Page Down was pressed!" << std::endl;
			}
				
			else if (strSend == keyPressArray[6]) { // Up Arrow
				PostMessage(FindWindow(NULL, L"Rocksmith 2014"), WM_KEYDOWN, VK_UP, 0);
				Sleep(30);
				PostMessage(FindWindow(NULL, L"Rocksmith 2014"), WM_KEYUP, VK_UP, 0);
				if (debugMode)
					std::cout << "Up Arrow was pressed!" << std::endl;
			}
			else if (strSend == keyPressArray[7]) { // Down Arrow
				PostMessage(FindWindow(NULL, L"Rocksmith 2014"), WM_KEYDOWN, VK_DOWN, 0);
				Sleep(30);
				PostMessage(FindWindow(NULL, L"Rocksmith 2014"), WM_KEYUP, VK_DOWN, 0);
				if (debugMode)
					std::cout << "Down Arrow was pressed!" << std::endl;
			}

			else if (strSend == keyPressArray[8]) { // Escape
				PostMessage(FindWindow(NULL, L"Rocksmith 2014"), WM_KEYDOWN, VK_ESCAPE, 0);
				Sleep(30);
				PostMessage(FindWindow(NULL, L"Rocksmith 2014"), WM_KEYUP, VK_ESCAPE, 0);
				if (debugMode)
					std::cout << "Escape was pressed!" << std::endl;
			}
			else if (strSend == keyPressArray[9]) { // User wants to stop using GuitarSpeak mod for the current time.
				sendKeystrokesToRS2014 = false;
				if (debugMode)
					std::cout << "CLOSE!!!" << std::endl;
			}
		}
		strSend = ""; // Reset Mapping || Prevents spam
	}
}
std::string* noteNames = new std::string[12]{ "A", "Bb", "B", "C", "C#", "D", "Eb", "E", "F", "F#", "G", "G#" };

void cGuitarSpeak::ForceNewSettings(std::string noteName, std::string keyPress) {
	for (int n = 0; n < 12; n++) {
		if (noteName == noteNames[n]) { // If the noteName variable sent is equal to an actual note name.
			strKeyList[n] = keyPress;
		}
	}
}

// ToDo: Convert Line 405-543 (checkbox-> strKey) to enable the mod. Needs some work done with ImGUI.