#include "GuitarSpeak.h"

cGuitarSpeak GuitarSpeak;

/*
Modified / Translated from the Visual Basic code provided by UKLooney
Open source here: https://cdn.discordapp.com/attachments/711634414240530462/735574443035721798/G2RS_v0.3_source.zip
*/

int cGuitarSpeak::NoteToInt(char* note, int octave){
	int notePlayed = 0x0;
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
}

byte cGuitarSpeak::GetCurrentNote() {
	return *(byte*)MemUtil.FindDMAAddy(Offsets.ptr_guitarSpeak, Offsets.ptr_guitarSpeakOffets);
}



void cGuitarSpeak::TimerTick() {
	// Debug Commands
	strKeyList[7] = "{ENTER}";

	bool debugMode = true;

	sendKeystrokesToRS2014 = true;

	//(LPVOID)Offsets.ptr_guitarSpeak
	//void* baseAddress = VirtualAllocEx(RocksmithProcessHandle, NULL, sizeof(byte) + 500, MEM_RESERVE, PAGE_READWRITE);
	// Main Function
	while (true) {
		Sleep(timer);
		/*if (debugMode)
			std::cout << "Tick" << std::endl;*/
		strKeyList[7] = "{ENTER}";

		int mem = GetCurrentNote();


		if (debugMode)
			std::cout << "Memory Pointers: " << mem << std::endl;

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
			currentNote = "-";
			currentNoteOctave = -1;
			newNote = false;
			mem = 0;
			lastNote = 0;
		}

		

		if (newNote) {
			std::string strSendTemp = "";
			currentNote = midiNotesArray[mem];
			currentNoteOctave = midiNotesOctaveArray[mem];
			//std::cout << "You need a " << NoteToInt("E", 3) << std::endl;

			if (mem != 0) { // A switch / case setup would be 100x better, but it kept telling me I couldn't call a "this." in this function... I'm not calling that...
				if (mem == NoteToInt("A", 2))
					strSendTemp = strKeyList[0];
				else if (mem == NoteToInt("Bb", 2))
					strSendTemp = strKeyList[1];
				else if (mem == NoteToInt("B", 2))
					strSendTemp = strKeyList[2];
				else if (mem == NoteToInt("C", 3))
					strSendTemp = strKeyList[3];
				else if (mem == NoteToInt("C#", 3))
					strSendTemp = strKeyList[4];
				else if (mem == NoteToInt("D", 3))
					strSendTemp = strKeyList[5];
				else if (mem == NoteToInt("Eb", 3))
					strSendTemp = strKeyList[6];
				else if (mem == NoteToInt("E", 3)) {
					strSendTemp = strKeyList[7];
					std::cout << "!!!!E3!!!!!" << std::endl;
				}
					
				else if (mem == NoteToInt("F", 3))
					strSendTemp = strKeyList[8];
				else if (mem == NoteToInt("F#", 3))
					strSendTemp = strKeyList[9];
				else if (mem == NoteToInt("G", 3))
					strSendTemp = strKeyList[10];
				else if (mem == NoteToInt("G#", 3))
					strSendTemp = strKeyList[11];
				else
					strSendTemp = "";
			}

			if (sendKeystrokesToRS2014)
				strSend = strSendTemp;
			else {
				if (newNote) {
					if (mem == seqArrayNotes[seqListenCount]) {
						seqListenCount++;
						if (seqListenCount > seqArrayNotesLength - 1) {
							seqListenCount = 0;
							sendKeystrokesToRS2014 = true;
						}
					}
					else if (mem == seqArrayNotes[0])
						seqListenCount = 1;
					else
						seqListenCount = 0;
				}
			}
		}
		newNote = false;

		if (sendKeystrokesToRS2014 && strSend != "") { // Are we sending a keystroke RIGHT now
			if (debugMode)
				std::cout << "Command Found!" << std::endl;

			// White Space Characters
			if (strSend == keyPressArray[6]) {
				PostMessage(FindWindow(NULL, L"Rocksmith 2014"), WM_KEYDOWN, VK_SPACE, 0);
				Sleep(30);
				PostMessage(FindWindow(NULL, L"Rocksmith 2014"), WM_KEYUP, VK_SPACE, 0);
			}
				
			else if (strSend == keyPressArray[2]) {
				PostMessage(FindWindow(NULL, L"Rocksmith 2014"), WM_KEYDOWN, VK_RETURN, 0);
				Sleep(30);
				PostMessage(FindWindow(NULL, L"Rocksmith 2014"), WM_KEYUP, VK_RETURN, 0);
				if (debugMode)
					std::cout << "Enter was pressed!" << std::endl;
			}
			else if (strSend == keyPressArray[7]) {
				PostMessage(FindWindow(NULL, L"Rocksmith 2014"), WM_KEYDOWN, VK_TAB, 0);
				Sleep(30);
				PostMessage(FindWindow(NULL, L"Rocksmith 2014"), WM_KEYUP, VK_TAB, 0);
			}

			// Navigation Keys 
			else if (strSend == keyPressArray[5]) {
				PostMessage(FindWindow(NULL, L"Rocksmith 2014"), WM_KEYDOWN, VK_PRIOR, 0);
				Sleep(30);
				PostMessage(FindWindow(NULL, L"Rocksmith 2014"), WM_KEYUP, VK_PRIOR, 0);
			}
			else if (strSend == keyPressArray[4]) {
				PostMessage(FindWindow(NULL, L"Rocksmith 2014"), WM_KEYDOWN, VK_NEXT, 0);
				Sleep(30);
				PostMessage(FindWindow(NULL, L"Rocksmith 2014"), WM_KEYUP, VK_NEXT, 0);
			}
				
			// Arrow Keys
			else if (strSend == keyPressArray[8]) {
				PostMessage(FindWindow(NULL, L"Rocksmith 2014"), WM_KEYDOWN, VK_UP, 0);
				Sleep(30);
				PostMessage(FindWindow(NULL, L"Rocksmith 2014"), WM_KEYUP, VK_UP, 0);
			}
			else if (strSend == keyPressArray[1]) {
				PostMessage(FindWindow(NULL, L"Rocksmith 2014"), WM_KEYDOWN, VK_DOWN, 0);
				Sleep(30);
				PostMessage(FindWindow(NULL, L"Rocksmith 2014"), WM_KEYUP, VK_DOWN, 0);
			}

			// Misc
			else if (strSend == keyPressArray[3]) {
				PostMessage(FindWindow(NULL, L"Rocksmith 2014"), WM_KEYDOWN, VK_ESCAPE, 0);
				Sleep(30);
				PostMessage(FindWindow(NULL, L"Rocksmith 2014"), WM_KEYUP, VK_ESCAPE, 0);
			}
			else if (strSend == keyPressArray[9]) // User wants to stop using GuitarSpeak mod for the current time.
				sendKeystrokesToRS2014 = false;
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