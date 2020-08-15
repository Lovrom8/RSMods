#include "GuitarSpeak.h"

cGuitarSpeak GuitarSpeak;

/*
Modified / Translated from the Visual Basic code provided by UKLooney
Open source here: https://cdn.discordapp.com/attachments/711634414240530462/735574443035721798/G2RS_v0.3_source.zip
*/

byte cGuitarSpeak::GetCurrentNote() {
	uintptr_t noteAdr = MemUtil.FindDMAAddy(Offsets.ptr_guitarSpeak, Offsets.ptr_guitarSpeakOffets); // Copied & Modified from GetCurrentMenu()

	if (!noteAdr)
		return -1;

	return *(byte*)noteAdr;
}

void cGuitarSpeak::DrawTunerInGame() {
	HDC RocksmithHDC = GetDC(FindWindow(NULL, L"Rocksmith 2014"));

	RECT* TextRectangle = new RECT();

	TextRectangle->left = 0;
	TextRectangle->top = 100;
	TextRectangle->right = 300;
	TextRectangle->bottom = 130;

	SetTextColor(RocksmithHDC, 0x00FFFFFF); // White Text
	SetBkMode(RocksmithHDC, TRANSPARENT);

	while (true)
	{
		int currentNoteImport = GetCurrentNote();

		if (currentNoteImport >= 96)
		{
			Sleep(timer);
			cGuitarSpeak::DrawTunerInGame();
		}

		int octaveNumber = (currentNoteImport / 12) - 1;

		std::string noteName = noteNames[currentNoteImport % 12];

		std::string TextToDraw = "Current Note: " + noteName + std::to_string(octaveNumber);

		DrawTextA(RocksmithHDC, TextToDraw.c_str(), TextToDraw.size(), TextRectangle, DT_NOCLIP);

		
	}

}


bool cGuitarSpeak::TimerTick() {

	bool debugMode = true;
		
	strKeyList[Settings.GetModSetting("GuitarSpeakDelete")] = (std::string)"DELETE";
	strKeyList[Settings.GetModSetting("GuitarSpeakSpace")] = (std::string)"SPACE";
	strKeyList[Settings.GetModSetting("GuitarSpeakEnter")] = (std::string)"ENTER";
	strKeyList[Settings.GetModSetting("GuitarSpeakTab")] = (std::string)"TAB";
	strKeyList[Settings.GetModSetting("GuitarSpeakPageUp")] = (std::string)"PGUP";
	strKeyList[Settings.GetModSetting("GuitarSpeakPageDown")] = (std::string)"PGDN";
	strKeyList[Settings.GetModSetting("GuitarSpeakUpArrow")] = (std::string)"UP";
	strKeyList[Settings.GetModSetting("GuitarSpeakDownArrow")] = (std::string)"DOWN";
	strKeyList[Settings.GetModSetting("GuitarSpeakEscape")] = (std::string)"ESCAPE";
	strKeyList[Settings.GetModSetting("GuitarSpeakClose")] = (std::string)"CLOSE";
	strKeyList[Settings.GetModSetting("GuitarSpeakOBracket")] = (std::string)"OBRACKET";
	strKeyList[Settings.GetModSetting("GuitarSpeakCBracket")] = (std::string)"CBRACKET";
	strKeyList[Settings.GetModSetting("GuitarSpeakTildea")] = (std::string)"TILDEA";
	strKeyList[Settings.GetModSetting("GuitarSpeakForSlash")] = (std::string)"FORSLASH";



	while (true) {
		Sleep(timer);
		/*if (debugMode)
			std::cout << "Tick" << std::endl;*/

		std::string currentMenu = MemHelpers.GetCurrentMenu();

		
		if (std::find(std::begin(tuningMenus), std::end(tuningMenus), currentMenu.c_str()) != std::end(tuningMenus) && Settings.ReturnSettingValue("GuitarSpeakWhileTuning") == "off") { // If someone wants to tune in the setting menu they skip the check
			std::cout << "Entered Tuning Menu!" << std::endl;
			return false;
		}

		if (std::find(std::begin(lessonModes), std::end(lessonModes), currentMenu.c_str()) != std::end(lessonModes) || std::find(std::begin(songMenus), std::end(songMenus), currentMenu.c_str()) != std::end(songMenus) || std::find(std::begin(calibrationMenus), std::end(calibrationMenus), currentMenu.c_str()) != std::end(calibrationMenus)) {
			std::cout << "Entered Song Menu!" << std::endl;
			return false;
		}


		int mem = GetCurrentNote();

		if (mem > 96) // The limit of Rocksmith
			mem = 0;

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
			else if (strSend == keyPressArray[10]) { // Open Bracket
				PostMessage(FindWindow(NULL, L"Rocksmith 2014"), WM_KEYDOWN, VK_OEM_4, 0);
				Sleep(30);
				PostMessage(FindWindow(NULL, L"Rocksmith 2014"), WM_KEYUP, VK_OEM_4, 0);
				if (debugMode)
					std::cout << "Open Bracket was pressed!" << std::endl;
			}
			else if (strSend == keyPressArray[11]) { // Close Bracket
				PostMessage(FindWindow(NULL, L"Rocksmith 2014"), WM_KEYDOWN, VK_OEM_6, 0);
				Sleep(30);
				PostMessage(FindWindow(NULL, L"Rocksmith 2014"), WM_KEYUP, VK_OEM_6, 0);
				if (debugMode)
					std::cout << "Close Bracket was pressed!" << std::endl;
			}
			else if (strSend == keyPressArray[12]) { // Tilde/a
				PostMessage(FindWindow(NULL, L"Rocksmith 2014"), WM_KEYDOWN, VK_OEM_3, 0);
				Sleep(30);
				PostMessage(FindWindow(NULL, L"Rocksmith 2014"), WM_KEYUP, VK_OEM_3, 0);
				if (debugMode)
					std::cout << "Tilde/ Tilda was pressed!" << std::endl;
			}
			else if (strSend == keyPressArray[13]) { // Forward Slash
				PostMessage(FindWindow(NULL, L"Rocksmith 2014"), WM_KEYDOWN, VK_OEM_2, 0);
				Sleep(30);
				PostMessage(FindWindow(NULL, L"Rocksmith 2014"), WM_KEYUP, VK_OEM_2, 0);
				if (debugMode)
					std::cout << "Forward Slash was pressed!" << std::endl;
			}
		}
		strSend = ""; // Reset Mapping || Prevents spam
	}
	return false;
}
std::string* noteNames = new std::string[12]{ "A", "Bb", "B", "C", "C#", "D", "Eb", "E", "F", "F#", "G", "G#" };

void cGuitarSpeak::ForceNewSettings(std::string noteName, std::string keyPress) {
	for (int n = 0; n < 12; n++) {
		if (noteName == noteNames[n]) { // If the noteName variable sent is equal to an actual note name.
			strKeyList[n] = keyPress;
		}
	}
}