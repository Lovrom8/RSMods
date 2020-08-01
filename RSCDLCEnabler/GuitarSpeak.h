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


	std::string cGuitarSpeak::tuningMenus[17] = { // These are all the menus where you need to tune
	(std::string)"SelectionListDialog",
	(std::string)"LearnASong_PreSongTuner",
	(std::string)"LearnASong_PreSongTunerMP",
	(std::string)"LearnASong_SongOptions",
	(std::string)"NonStopPlay_PreSongTuner",
	(std::string)"NonStopPlay_PreSongTunerMP",
	(std::string)"ScoreAttack_PreSongTuner",
	(std::string)"SessionMode_PreSMTunerMP",
	(std::string)"SessionMode_PreSMTuner",
	(std::string)"UIMenu_Tuner",
	(std::string)"UIMenu_TunerMP",
	(std::string)"Guitarcade_Tuner",
	(std::string)"Tuner",
	(std::string)"Duet_PreSongTuner",
	(std::string)"H2H_PreSongTuner",
	(std::string)"GETuner",
	(std::string)"PreGame_GETuner"
	};

	std::string  cGuitarSpeak::songModes[8] = { // These are all the menus where you would play guitar games.
		(std::string)"LearnASong_Game",
		(std::string)"NonStopPlay_Game",
		(std::string)"ScoreAttack_Game",
		(std::string)"LearnASong_Pause",
		(std::string)"NonStopPlay_Pause",
		(std::string)"ScoreAttack_Pause",
	};

	std::string  cGuitarSpeak::lessonModes[2] = { // These are the Guided Experience / Lessons modes.
		(std::string)"GuidedExperience_Game",
		(std::string)"GuidedExperience_Pause"
	};
};

extern cGuitarSpeak GuitarSpeak;