#pragma once

#include "windows.h"
#include "MemUtil.h"
#include "Offsets.h"
#include <vector>
#include <string>
#include <sstream>
#include <iostream>
#include "imgui_includeMe.h"

class cGuitarSpeak {
public:
	int NoteToHex(char* note, int octave);
	byte GetCurrentNote();
	void TimerTick();
	void ForceNewSettings(std::string noteName, std::string keyPress);
	void ResetStrKeyCache();
	std::string* keyPressArray = new std::string[12]{ "{DELETE}", "{DOWN}", "{ENTER}", "{ESC}", "{PGDN}", "{PGUP}", "{SPACE}", "{TAB}", "{UP}", "<CANCEL>", "<none>", "<TBD>" };
	std::string* noteNames = new std::string[12]{ "A", "Bb", "B", "C", "C#", "D", "Eb", "E", "F", "F#", "G", "G#" };
private:
	int pointer1, pointer2, pointer3, memoryLocNote, bytesRead;
	int lastNoteBuffer = 0xFF, currentNoteBuffer = 0xFF;
	int timer = 50; // Milliseconds | Original value of 50ms
	int* memBuffer = new int[memBufferLength]{ 0, 0, 0, 0 };
	int memBufferLength = 4;
	bool isRocksmithFocused = false;
	bool isRocksmithOpen = false;
	bool sendKeystrokesToRS2014 = false;
	bool isThisNewNote = false;
	bool newNote = false;
	HANDLE RocksmithProcessHandle;
	HWND lastForegroundWindow, currentForegroundWindow;
	byte note, lastNote;
	LPSTR windowTitle;


	// Potentially garbage values we can break down to save space / memory

	int timerCount, memberCount;
	int seqListenCount = 0;
	std::string* keyComboArray = new std::string[12]{ "DELETE", "DOWN", "ENTER", "ESCAPE", "PAGE DN", "PAGE UP", "SPACE", "TAB", "UP", "CANCEL", "none", "TBD" };
	//std::string* keyPressArray = new std::string[12]{ "{DELETE}", "{DOWN}", "{ENTER}", "{ESC}", "{PGDN}", "{PGUP}", "{SPACE}", "{TAB}", "{UP}", "<CANCEL>", "<none>", "<TBD>" };
	std::string* notesUsedArray = new std::string[6]{ "A", "B", "C#", "Eb", "F", "G" };
	std::string* notesSeqTextArray = new std::string[12]{ "A 2", "B 2", "C# 3", "Eb 3", "F 3", "G 3", "A 3", "B 3", "C# 4", "Eb 4", "F 4", "G 4" };
	int* notesSeqMidiArray = new int[12]{0x2D, 0x2F, 0x31, 0x33, 0x35, 0x37, 0x39, 0x3B, 0x3D, 0x3F, 0x41, 0x43};
	int* seqArrayNotes = new int[seqArrayNotesLength]{ 0, 0, 0, 0, 0 };
	int seqArrayNotesLength = 5;
	std::string strSend = "";
	int* midiArray = new int[96]{ 0x0, 0x1, 0x2, 0x3, 0x4, 0x5, 0x6, 0x7, 0x8, 0x9, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0x60, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79, 0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89, 0x90, 0x91, 0x92, 0x93, 0x94, 0x95 }; // C-1 <-> C6
	std::string* midiNotesArray = new std::string[96]{ "C", "C#", "D", "Eb", "E", "F", "F#", "G", "G#", "A", "Bb", "B", "C", "C#", "D", "Eb", "E", "F", "F#", "G", "G#", "A", "Bb", "B", "C", "C#", "D", "Eb", "E", "F", "F#", "G", "G#", "A", "Bb", "B", "C", "C#", "D", "Eb", "E", "F", "F#", "G", "G#", "A", "Bb", "B", "C", "C#", "D", "Eb", "E", "F", "F#", "G", "G#", "A", "Bb", "B", "C", "C#", "D", "Eb", "E", "F", "F#", "G", "G#", "A", "Bb", "B", "C", "C#", "D", "Eb", "E", "F", "F#", "G", "G#", "A", "Bb", "B", "C", "C#", "D", "Eb", "E", "F", "F#", "G", "G#", "A", "Bb", "B" }; // Note Names
	std::string* midiNotesFullArray = new std::string[96]{ "C-1", "C#-1", "D-1", "Eb-1", "E-1", "F-1", "F#-1", "G-1", "G#-1", "A-1", "Bb-1", "B-1", "C0", "C#0", "D0", "Eb0", "E0", "F0", "F#0", "G0", "G#0", "A0", "Bb0", "B0", "C1", "C#1", "D1", "Eb1", "E1", "F1", "F#1", "G1", "G#1", "A1", "Bb1", "B1", "C2", "C#2", "D2", "Eb2", "E2", "F2", "F#2", "G2", "G#2", "A2", "Bb2", "B2", "C3", "C#3", "D3", "Eb3", "E3", "F3", "F#3", "G3", "G#3", "A3", "Bb3", "B3", "C4", "C#4", "D4", "Eb4", "E4", "F4", "F#4", "G4", "G#4", "A4", "Bb4", "B4", "C5", "C#5", "D5", "Eb5", "E5", "F5", "F#5", "G5", "G#5", "A5", "Bb5", "B5", "C6", "C#6", "D6", "Eb6", "E6", "F6", "F#6", "G6", "G#6", "A6", "Bb6", "B6" };
	int* midiNotesOctaveArray = new int[96]{ -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6 };
	int indexKey_A, indexKey_Bb, indexKey_B, indexKey_C, indexKey_Cs, indexKey_D, indexKey_Eb, indexKey_E, indexKey_F, indexKey_Fs, indexKey_G, indexKey_Gs;
	std::string* strKeyList = new std::string[12]{ "", "", "", "", "", "", "", "", "", "", "", "" };
	std::string strKey_A = strKeyList[0], strKey_Bb = strKeyList[1], strKey_B = strKeyList[2], strKey_C = strKeyList[3], strKey_Cs = strKeyList[4], strKey_D = strKeyList[5], strKey_Eb = strKeyList[6], strKey_E = strKeyList[7], strKey_F = strKeyList[8], strKey_Fs = strKeyList[9], strKey_G = strKeyList[10], strKey_Gs = strKeyList[11];
	std::string reqSeq1, reqSeq2, reqSeq3, reqSeq4, reqSeq5;
	std::string currentNote = "-";
	int currentNoteOctave = -1;
	

};

extern cGuitarSpeak GuitarSpeak;