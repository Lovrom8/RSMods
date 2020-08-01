#pragma once

#include <vector>
#include <string>
#include <sstream>
#include <iostream>
#include "windows.h"


#include "MemHelpers.h"
#include "Offsets.h"
#include "MemUtil.h"
#include "Settings.h"

#include "DirectX/d3dx9.h"
#include "D3D.h"

class cGuitarSpeak {
public:
	byte GetCurrentNote();
	bool TimerTick();
	void ForceNewSettings(std::string noteName, std::string keyPress);
	std::string* keyPressArray = new std::string[10]{ "DELETE", "SPACE", "ENTER", "TAB", "PGUP", "PGDN", "UP", "DOWN", "ESCAPE", "CLOSE" };
	std::string* noteNames = new std::string[12]{ "C", "C#", "D", "Eb", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
	int* octaves = new int[8]{ -1, 0, 1, 2, 3, 4, 5, 6 };
private:
	int lastNote;
	int lastNoteBuffer = 0xFF, currentNoteBuffer = 0xFF;
	int timer = 50; // Milliseconds | Original value of 50ms
	bool sendKeystrokesToRS2014 = true;
	bool newNote = false;
	std::string* strKeyList = new std::string[96]; // 12 notes in an octave, 8 octaves spanned (-1 <-> 6)
	std::string strSend;
};

extern cGuitarSpeak GuitarSpeak;