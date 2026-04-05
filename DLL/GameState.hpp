#pragma once

#include "stdafx.h"

/// <param name="valueToCheckIfInsideArray"> - Input</param>
/// <param name="vec"> - Is input in list | VECTOR? (NULLABLE)</param>
template <typename T>
bool Contains(const T& valueToCheckIfInsideArray, const std::vector<T>& vec) {
	return std::find(vec.begin(), vec.end(), valueToCheckIfInsideArray) != vec.end();
}

namespace GameState {
	bool IsInSong();
	bool IsMultiplayer();
	std::string CurrentSelectedUser();
	std::string GetSongKey();
	std::string GetArrangementID();

	inline std::string lastSongKey = "";
	inline std::string lastArrangementID = "";

	std::string GetCurrentMenu(bool GameNotLoaded = false);
	void ToggleCB(bool enabled);

	inline static std::string lastMenu = "";
	inline static bool canGetRealMenu = false;

	inline std::string previousMenu, currentMenu; // What is the last menu, and the current menu?
	inline bool GameLoaded = false; // Has the game gotten to the main menu where you can pick the gamemodes?
	inline bool LessonMode = false; // Is the user in LessonMode?

	inline bool lowPerformancePC = false; // Does your game lag with all of our mods? Toggle on to disable us running mods for an "original DLL" experience.
	inline bool GameClosing = false; // Inform the threads that their work is done just before the game closes

	const std::vector<std::string> calibrationMenus = {
		"Guitarcade_Calibration",
		"Guitarcade_WRDCalibration",
		"GECalibrationMeter",
		"NonStopPlay_CalibrationMeter_PreGame",
		"CalibrationMeter",
		"CalibrationMeter_MP",
		"LearnASong_CalibrationMeter_PreGame",
	};

	const std::vector<std::string> tuningMenus = { // These are all the menus where you need to tune
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

	const std::vector<std::string> songMenus = { // These are all the menus where you would play guitar games.
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

	const std::vector<std::string> lessonModes = { // These are the Guided Experience / Lessons modes.
		"GuidedExperience_Game",
		"GuidedExperience_Pause",
	};

	const std::vector<std::string> preSongTuners = { // Tuning menus where we aren't in the song
		"SelectionListDialog",
		"LearnASong_PreSongTuner",
		"LearnASong_PreSongTunerMP",
		"NonStopPlay_PreSongTuner",
		"NonStopPlay_PreSongTunerMP",
		"ScoreAttack_PreSongTuner",
		"SessionMode_PreSMTunerMP",
		"SessionMode_PreSMTuner",
		"Duet_PreSongTuner",
		"H2H_PreSongTuner",
		"PreGame_GETuner"
	};

	const std::vector<std::string> songModes = { // These are all the menus where you would play guitar games.
		"LearnASong_Game",
		"NonStopPlay_Game",
		"ScoreAttack_Game",
		"LearnASong_Pause",
		"NonStopPlay_Pause",
		"ScoreAttack_Pause",

		// Riff Repeater (RR)
		"RiffRepeater",
		"LearnASong_RiffRepeater",
		"RiffRepeater_AdvancedSettings",
		"RiffRepeater_Pause",

		// Misc menus
		"Tuner",
		"MixerMenu",
		"HelpList",
		"SideList",
		"CalibrationMeter",

	};

	const std::vector<std::string> multiplayerTuners = {
		"LearnASong_PreSongTunerMP",
		"NonStopPlay_PreSongTunerMP",
		"SessionMode_PreSMTunerMP",
		"UIMenu_TunerMP",
		"Duet_PreSongTuner",
		"H2H_PreSongTuner",
	};

	const std::vector<std::string> learnASongModes = {
		"LearnASong_Game",
		"NonStopPlay_Game",
		"LearnASong_Pause",
		"NonStopPlay_Pause",
	};

	const std::vector<std::string> learnASongPlaying = {
		"LearnASong_Game",
		"NonStopPlay_Game"
	};

	const std::vector<std::string> lasPauseMenus = {
		"LearnASong_Pause",
		"NonStopPlay_Pause",
	};

	const std::vector<std::string> fastRRModes = {
		"LearnASong_Game",
		"NonStopPlay_Game",
		"LearnASong_Pause",
		"NonStopPlay_Pause",
		"LearnASong_RiffRepeater",
		"RiffRepeater",
		"RiffRepeater_AdvancedSettings",
		"RiffRepeater_Pause",
	};

	const std::vector<std::string> scoreAttackModes = {
		"ScoreAttack_Game",
		"ScoreAttack_Pause",
	};

	const std::vector<std::string> scoreScreens = { // Screens for us to take score screenshots in
		"LearnASong_SongReview",
		"ScoreAttack_SongReview",
		"Duet_SongReview",
		"H2H_SongReview",
	};

	const std::vector<std::string> onlineModes = {
		"ScoreAttack_Game",
		"Guitarcade_Game",
	};

	const std::vector<std::string> dontAutoEnter = { // Don't use the auto load last profile mode if we are on these screens.
		// First time Player
		"TextEntryDialog", // Prompts to enter profile name / uplay name / etc.
		"PlayedRS1Select", // Did you play the original Rocksmith?
		"ExperienceSelect", // How good at guitar do you think you are?
		"PathSelect", // What path do you want to play? (Lead, Rhythm, Bass)
		"HandSelect", // Are you left handed or right handed? (Left, Right)
		"HeadstockSelect", // Select between (3+3 or 6-inline) (2+2 or 4-inline).
		"FE_InputSelect", // What cable method do you have? (RTC, Mic, Disconnected)
		"FECalibrationMeter", // First time calibrating.
		"VideoPlayer", // Intro videos on how to calibrate, tune, play the game.
		"FETuner", // First time tuning.
		"FirstEncounter_Game", // Intro to game, teaches basic UI elements.
		//"SelectionListDialog", // Occasional Yes/ No Prompts.

		// Played RS1 before
		"RefresherSelect", // User has played Rocksmith before, but do they need some touchup on their knowledge.

		// Disconnected Mode
		"ImageDialog", // Disconnected Mode Pictures.

		// Uplay section
		//"UplayLoginDialog", // User needs to login to uPlay.
		"UplayAccountCreationDialog" // User needs to create a uPlay account.
	};

	namespace Menus {
		bool IsInMultiplayerTunerMenus();
		bool IsInScoreMenus();
		bool IsInTuningMenus();
		bool IsInPreSongTuner();
		bool IsInSongModes();
		bool IsInScoreAttackModes();
		bool IsInLearnASongModes();
		bool IsInLearnASongPauseModes();
		bool IsInModesWithAllowedFastRiffRepeater();
		bool IsInOnlineModes();
		bool IsInLASPlayingModes();
		bool IsOnScoreScreens();
		bool IsInLessonModes();
		bool IsInMenusWithDisallowedAutoEnter();
		bool IsInCalibrationMenus();
	}
};
