#include "GuitarSpeak.h"

cGuitarSpeak GuitarSpeak;

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

