#pragma once

#include <vector>
#include <string>
#include <sstream>
#include <iostream>
#include "windows.h"


#include "../MemHelpers.hpp"
#include "../Offsets.hpp"
#include "../MemUtil.hpp"
#include "../Settings.hpp"
#include "../Lib/DirectX/d3dx9core.h"

namespace GuitarSpeak {
	byte GetCurrentNote();
	bool TimerTick();
	void DrawTunerInGame();
	void ForceNewSettings(std::string noteName, std::string keyPress);

	inline std::string* keyPressArray = new std::string[14]{ "DELETE", "SPACE", "ENTER", "TAB", "PGUP", "PGDN", "UP", "DOWN", "ESCAPE", "CLOSE", "OBRACKET", "CBRACKET", "TILDEA", "FORSLASH" };
	inline std::string* noteNames = new std::string[12]{ "C", "C#", "D", "Eb", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
	inline int* octaves = new int[8]{ -1, 0, 1, 2, 3, 4, 5, 6 };

	inline int lastNote;
	inline int lastNoteBuffer = 0xFF, currentNoteBuffer = 0xFF;
	inline int timer = 50; // Milliseconds | Original value of 50ms
	inline bool sendKeystrokesToRS2014 = true;
	inline bool newNote = false;
	inline std::string* strKeyList = new std::string[96]; // 12 notes in an octave, 8 octaves spanned (-1 <-> 6)
	inline std::string strSend;

	inline std::string calibrationMenus[8] = {
		"Guitarcade_Calibration",
		"Guitarcade_WRDCalibration",
		"GECalibrationMeter",
		"NonStopPlay_CalibrationMeter_PreGame",
		"CalibrationMeter",
		"CalibrationMeter_MP",
		"LearnASong_CalibrationMeter_PreGame",
		"MixerMenu", // Not "calibration" but still a menu we shouldn't allow, as people will want to test their new volumes.
	};

	inline std::string tuningMenus[15] = { // These are all the menus where you need to tune
		"LearnASong_PreSongTuner",
		"LearnASong_PreSongTunerMP",
		"NonStopPlay_PreSongTuner",
		"NonStopPlay_PreSongTunerMP",
		"ScoreAttack_PreSongTuner",
		"SessionMode_PreSMTunerMP",
		"SessionMode_PreSMTuner",
		"UIMenu_Tuner",
		"UIMenu_TunerMP",
		"Guitarcade_Tuner",
		"Tuner",
		"Duet_PreSongTuner",
		"H2H_PreSongTuner",
		"GETuner",
		"PreGame_GETuner",
	};

	inline std::string songMenus[13] = { // These are all the menus where you would play guitar games.
		"LearnASong_Game",
		"NonStopPlay_Game",
		"ScoreAttack_Game",
		"LearnASong_Pause",
		"NonStopPlay_Pause",
		"ScoreAttack_Pause",
		"LearnASong_RiffRepeater",
		"RiffRepeater_AdvancedSettings",
		"SessionMode_Game",
		"SessionMode_PauseGame",
		"Guitarcade_Game",
		"Guitarcade_Pause",
		"HelpList", // Chords Menu
	};

	inline std::string lessonModes[2] = { // These are the Guided Experience / Lessons modes.
		"GuidedExperience_Game",
		"GuidedExperience_Pause",
	};
};