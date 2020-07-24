#include "GuitarSpeak.h"

cGuitarSpeak GuitarSpeak;

/*
Modified / Translated from the Visual Basic code provided by UKLooney
Open source here: https://cdn.discordapp.com/attachments/711634414240530462/735574443035721798/G2RS_v0.3_source.zip
*/

int cGuitarSpeak::NoteToHex(char* note, int octave){
	int notePlayed = 0x0;
	octave = octave - 2; // The first full set of notes is at the 2nd octave so we do the math as if they were our zero-point.

	if (note == "C")
		notePlayed = 0x24;
	else if (note == "Db" || note == "C#")
		notePlayed = 0x25;
	else if (note == "D")
		notePlayed = 0x26;
	else if (note == "Eb" || note == "D#")
		notePlayed = 0x27;
	else if (note == "E")
		notePlayed = 0x28;
	else if (note == "F")
		notePlayed = 0x29;
	else if (note == "Gb" || note == "F#")
		notePlayed = 0x2A;
	else if (note == "G")
		notePlayed = 0x2B;
	else if (note == "Ab" || note == "G#")
		notePlayed = 0x2C;
	else if (note == "A")
		notePlayed = 0x2D;
	else if (note == "Bb" || note == "A#")
		notePlayed = 0x2E;
	else if (note == "B")
		notePlayed = 0x2F;

	if (notePlayed != 0x0)
		notePlayed = notePlayed + (octave * 0xC); // Ex. If note is G5 then 0x2B + ((5-2) * 0xC) = 4F, so our end note is 4F, not 2B;

	return notePlayed;
}

byte cGuitarSpeak::GetCurrentNote() {
	return *(byte*)MemUtil.FindDMAAddy(Offsets.ptr_guitarSpeak, Offsets.ptr_guitarSpeakOffets);
}



void cGuitarSpeak::TimerTick() {
	// Debug Commands
	strKeyList[7] = "{ENTER}";

	bool debugMode = false;


	ResetStrKeyCache();

	// Main Function
	while (true) {
		Sleep(timer);
		if (debugMode)
			std::cout << "Tick" << std::endl;

		// Pointer 1
		ReadProcessMemory(RocksmithProcessHandle, (LPVOID)Offsets.ptr_guitarSpeak, memBuffer, memBufferLength, bytesRead);
		pointer1 = memBuffer[0] || (memBuffer[1] << 0x8) || (memBuffer[2] << 0x10) || (memBuffer[3] << 0x18) + Offsets.ptr_guitarSpeakOffets[0];

		std::cout << "MemBuffer[0]: " << memBuffer[0] << std::endl;
		std::cout << "MemBuffer[1]: " << (memBuffer[1] << 0x8) << std::endl;
		std::cout << "MemBuffer[2]: " << (memBuffer[2] << 0x10) << std::endl;
		std::cout << "MemBuffer[3]: " << (memBuffer[3] << 0x18) << std::endl;

		// Pointer 2
		ReadProcessMemory(RocksmithProcessHandle, (LPVOID)pointer1, memBuffer, memBufferLength, bytesRead);
		pointer2 = memBuffer[0] || (memBuffer[1] << 0x8) || (memBuffer[2] << 0x10) || (memBuffer[3] << 0x18) + Offsets.ptr_guitarSpeakOffets[1];

		// Pointer 3
		ReadProcessMemory(RocksmithProcessHandle, (LPVOID)pointer2, memBuffer, memBufferLength, bytesRead);
		pointer3 = memBuffer[0] || (memBuffer[1] << 0x8) || (memBuffer[2] << 0x10) || (memBuffer[3] << 0x18) + Offsets.ptr_guitarSpeakOffets[2];

		// MemoryLocNote
		ReadProcessMemory(RocksmithProcessHandle, (LPVOID)pointer3, memBuffer, memBufferLength, bytesRead);
		memoryLocNote = memBuffer[0] || (memBuffer[1] << 0x8) || (memBuffer[2] << 0x10) || (memBuffer[3] << 0x18);


		/*std::cout << "Pointer 1: " << pointer1 << std::endl;
		std::cout << "Pointer 2: " << pointer2 << std::endl;
		std::cout << "Pointer 3: " << pointer3 << std::endl;
		std::cout << "Pointer 4: " << memoryLocNote << std::endl;*/



		int mem = memBuffer[0];

		if (debugMode)
			std::cout << "Memory Pointers" << std::endl;

		if (mem != 0xFF && mem >= 96) { // Is the note too high pitched?
			return;
		}

		/*std::cout << "Current Note: " << mem << std::endl;
		std::cout << "Current Note Buffer: " << currentNoteBuffer << std::endl;
		std::cout << "Previous Note Buffer: " << lastNoteBuffer << std::endl;*/

		if (mem == lastNoteBuffer && mem != currentNoteBuffer)
			currentNoteBuffer = mem;
		if (mem != lastNoteBuffer)
			lastNoteBuffer = mem;

		if (currentNoteBuffer != 0xFF && currentNoteBuffer != lastNote) { // Check if there is a new note
			if (debugMode)
				std::cout << "New Note Detected" << std::endl;
			newNote = true;
			note = currentNoteBuffer;
			lastNote = note;
		}
		if (currentNoteBuffer == 0xFF && lastNote != 0x0) { // If the note ends
			if (debugMode)
				std::cout << "Note Ended" << std::endl;
			currentNote = "-";
			currentNoteOctave = -1;
			newNote = false;
			note = 0;
			lastNote = 0;
		}

		if (newNote) {
			std::string strSendTemp = "";
			currentNote = midiNotesArray[note];
			currentNoteOctave = midiNotesOctaveArray[note];

			if (note != 0) { // A switch / case setup would be 100x better, but it kept telling me I couldn't call a "this." in this function... I'm not calling that...
				if (note == NoteToHex("A", 2))
					strSendTemp = strKey_A;
				else if (note == NoteToHex("Bb", 2))
					strSendTemp = strKey_Bb;
				else if (note == NoteToHex("B", 2))
					strSendTemp = strKey_B;
				else if (note == NoteToHex("C", 3))
					strSendTemp = strKey_C;
				else if (note == NoteToHex("C#", 3))
					strSendTemp = strKey_Cs;
				else if (note == NoteToHex("D", 3))
					strSendTemp = strKey_D;
				else if (note == NoteToHex("Eb", 3))
					strSendTemp = strKey_Eb;
				else if (note == NoteToHex("E", 3))
					strSendTemp = strKey_E;
				else if (note == NoteToHex("F", 3))
					strSendTemp = strKey_F;
				else if (note == NoteToHex("F#", 3))
					strSendTemp = strKey_Fs;
				else if (note == NoteToHex("G", 3))
					strSendTemp = strKey_G;
				else if (note == NoteToHex("G#", 3))
					strSendTemp = strKey_Gs;
				else
					strSendTemp = "";
			}

			if (!notesUsedArray->find(currentNote)) {
				return;
			}

			if (sendKeystrokesToRS2014)
				strSend = strSendTemp;
			else {
				if (newNote) {
					if (note == seqArrayNotes[seqListenCount]) {
						seqListenCount++;
						if (seqListenCount > seqArrayNotesLength - 1) {
							seqListenCount = 0;
							sendKeystrokesToRS2014 = true;
						}
					}
					else if (note == seqArrayNotes[0])
						seqListenCount = 1;
					else
						seqListenCount = 0;
				}
			}
		}
		newNote = false;

		if (sendKeystrokesToRS2014) { // Are we sending a keystroke RIGHT now
			if (strSend.find("{", 0)) {
				if (debugMode)
					std::cout << "Command Found!" << std::endl;
				// White Space Characters
				if (strSend == "{SPACE}") {
					PostMessage(FindWindow(NULL, L"Rocksmith 2014"), WM_KEYDOWN, VK_SPACE, 0);
					Sleep(30);
					PostMessage(FindWindow(NULL, L"Rocksmith 2014"), WM_KEYUP, VK_SPACE, 0);
				}
				
				else if (strSend == "{ENTER}") {
					PostMessage(FindWindow(NULL, L"Rocksmith 2014"), WM_KEYDOWN, VK_RETURN, 0);
					Sleep(30);
					PostMessage(FindWindow(NULL, L"Rocksmith 2014"), WM_KEYUP, VK_RETURN, 0);
					if (debugMode)
						std::cout << "Enter was pressed!" << std::endl;
				}
				else if (strSend == "{TAB}") {
					PostMessage(FindWindow(NULL, L"Rocksmith 2014"), WM_KEYDOWN, VK_TAB, 0);
					Sleep(30);
					PostMessage(FindWindow(NULL, L"Rocksmith 2014"), WM_KEYUP, VK_TAB, 0);
				}

				// Navigation Keys 

				else if (strSend == "{PGUP}") {
					PostMessage(FindWindow(NULL, L"Rocksmith 2014"), WM_KEYDOWN, VK_PRIOR, 0);
					Sleep(30);
					PostMessage(FindWindow(NULL, L"Rocksmith 2014"), WM_KEYUP, VK_PRIOR, 0);
				}
				else if (strSend == "{PGDN}") {
					PostMessage(FindWindow(NULL, L"Rocksmith 2014"), WM_KEYDOWN, VK_NEXT, 0);
					Sleep(30);
					PostMessage(FindWindow(NULL, L"Rocksmith 2014"), WM_KEYUP, VK_NEXT, 0);
				}
				
				
				// Arrow Keys
				else if (strSend == "{UP}") {
					PostMessage(FindWindow(NULL, L"Rocksmith 2014"), WM_KEYDOWN, VK_UP, 0);
					Sleep(30);
					PostMessage(FindWindow(NULL, L"Rocksmith 2014"), WM_KEYUP, VK_UP, 0);
				}
				else if (strSend == "{DOWN}") {
					PostMessage(FindWindow(NULL, L"Rocksmith 2014"), WM_KEYDOWN, VK_DOWN, 0);
					Sleep(30);
					PostMessage(FindWindow(NULL, L"Rocksmith 2014"), WM_KEYUP, VK_DOWN, 0);
				}

				// Misc
				else if (strSend == "{ESC}") {
					PostMessage(FindWindow(NULL, L"Rocksmith 2014"), WM_KEYDOWN, VK_ESCAPE, 0);
					Sleep(30);
					PostMessage(FindWindow(NULL, L"Rocksmith 2014"), WM_KEYUP, VK_ESCAPE, 0);
				}

				else { // Non-predetermined key
					strSend = ""; // Reset mapping
					return;
				}

			}
			else if (strSend == "<CANCEL>") // User wants to stop using GuitarSpeak mod for the current time.
				sendKeystrokesToRS2014 = false;
		}
	}
}
std::string* noteNames = new std::string[12]{ "A", "Bb", "B", "C", "C#", "D", "Eb", "E", "F", "F#", "G", "G#" };

void cGuitarSpeak::ForceNewSettings(std::string noteName, std::string keyPress) {
	for (int n = 0; n < 12; n++) {
		if (noteName == noteNames[n]) { // If the noteName variable sent is equal to an actual note name.
			strKeyList[n] = keyPress;
		}
	}
	ResetStrKeyCache(); // Refresh the strKey variables
}

void cGuitarSpeak::ResetStrKeyCache() { // Refresh the variables to their values in the strKeyList
	std::string strKey_A = strKeyList[0], strKey_Bb = strKeyList[1], strKey_B = strKeyList[2], strKey_C = strKeyList[3], strKey_Cs = strKeyList[4], strKey_D = strKeyList[5], strKey_Eb = strKeyList[6], strKey_E = strKeyList[7], strKey_F = strKeyList[8], strKey_Fs = strKeyList[9], strKey_G = strKeyList[10], strKey_Gs = strKeyList[11];
}

// ToDo: Convert Line 405-543 (checkbox-> strKey) to enable the mod. Needs some work done with ImGUI.