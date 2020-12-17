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
	std::string StringCurrentNote();
	bool RunGuitarSpeak();
	void FillKeyList();

	inline bool verbose = false;

	inline int lastNote;
	inline int lastNoteBuffer = 0xFF, currentNoteBuffer = 0xFF; // 0xFF = End Note
	inline int timer = 50; // Milliseconds | Original value of 50ms
	inline bool sendKeystrokesToRS2014 = true, newNote = false;
	inline std::string* strKeyList = new std::string[96]; // 12 notes in an octave, 8 octaves spanned (-1 <-> 6) || Limit C-1 <-> C7
	inline int noNote = 0x0, endOfNote = 0xFF; // Used so we can call these instead of 0xFF and 0x0 in the file.

	inline std::map<std::string, int> keyToVKey{
		{"DELETE", VK_DELETE},	// Delete
		{"SPACE", VK_SPACE},	// Space
		{"ENTER", VK_RETURN},	// Enter
		{"TAB", VK_TAB},		// Tab
		{"PGUP", VK_PRIOR},		// Page Up
		{"PGDN", VK_NEXT},		// Page Down
		{"UP", VK_UP},			// Up Arrow
		{"DOWN", VK_DOWN},		// Down Arrow
		{"ESCAPE", VK_ESCAPE},	// Escape
		{"OBRACKET", VK_OEM_4}, // Open Bracket
		{"CBRACKET", VK_OEM_6}, // Close Bracket
		{"TILDEA", VK_OEM_3},	// Tilde/a
		{"FORSLASH", VK_OEM_2}, // Forward Slash
	};

	inline std::vector<std::string> calibrationMenus = {
		"Guitarcade_Calibration",
		"Guitarcade_WRDCalibration",
		"GECalibrationMeter",
		"NonStopPlay_CalibrationMeter_PreGame",
		"CalibrationMeter",
		"CalibrationMeter_MP",
		"LearnASong_CalibrationMeter_PreGame",
	};

	inline std::vector<std::string> tuningMenus = { // These are all the menus where you need to tune
		"SelectionListDialog",
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
		"PreGame_GETuner"
	};

	inline std::vector<std::string> songMenus = { // These are all the menus where you would play guitar games.
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
		"MixerMenu",
	};

	inline std::vector<std::string> lessonModes = { // These are the Guided Experience / Lessons modes.
		"GuidedExperience_Game",
		"GuidedExperience_Pause",
	};

	inline std::vector<std::string> noteLetters = {
		"C", "C#", "D", "Eb", "E", "F", "F#", "G", "Ab", "A", "Bb", "B"
	};
};