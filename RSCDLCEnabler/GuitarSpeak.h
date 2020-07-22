#pragma once

#include "windows.h"
#include "Offsets.h"
#include <vector>
#include <string>
#include <iostream>

class cGuitarSpeak {
public:
	int NoteToHex(char* note, int octave);
private:
	int pointer1, pointer2, pointer3, memoryLocNote, bytesread;
	int lastNoteBuffer = 0xFF, currentNoteBuffer = 0xFF;
	byte memBuffer;
};