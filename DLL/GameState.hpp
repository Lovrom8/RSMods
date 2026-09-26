#pragma once

#include <algorithm>
#include <array>
#include <atomic>
#include <concepts>
#include <ranges>
#include <string>
#include <string_view>
#include <vector>

/// <param name="valueToCheckIfInsideArray"> - Input</param>
/// <param name="vec"> - Is input in list | VECTOR? (NULLABLE)</param>
template <typename T, std::ranges::input_range R>
	requires std::convertible_to<T, std::ranges::range_value_t<R>>
constexpr bool Contains(const T& value, const R& range)
{
	const auto searchValue = static_cast<std::ranges::range_value_t<R>>(value);
	for (const auto& element : range)
		if (element == searchValue) return true;
	return false;
}

namespace GameState {
	using namespace std::string_view_literals;

	bool IsInSong();
	bool IsMultiplayer();
	std::string CurrentSelectedUser();
	std::string GetSongKey();

	inline std::string lastSongKey = "";

	std::string GetCurrentMenu(bool GameNotLoaded = false);
	void ToggleCB(bool enabled);

	inline static std::string lastMenu = "";
	inline static bool canGetRealMenu = false;

	inline std::string previousMenu, currentMenu; // What is the last menu, and the current menu?
	inline std::atomic_bool GameLoaded = false; // Has the game gotten to the main menu where you can pick the gamemodes?
	inline bool LessonMode = false; // Is the user in LessonMode?

	inline bool lowPerformancePC = false; // Does your game lag with all of our mods? Toggle on to disable us running mods for an "original DLL" experience.
	inline std::atomic_bool GameClosing = false; // Inform the threads that their work is done just before the game closes

	inline constexpr std::array calibrationMenus = {
		"Guitarcade_Calibration"sv,
		"Guitarcade_WRDCalibration"sv,
		"GECalibrationMeter"sv,
		"NonStopPlay_CalibrationMeter_PreGame"sv,
		"CalibrationMeter"sv,
		"CalibrationMeter_MP"sv,
		"LearnASong_CalibrationMeter_PreGame"sv
	};

	inline constexpr std::array tuningMenus = { // These are all the menus where you need to tune
		"SelectionListDialog"sv,
		"LearnASong_PreSongTuner"sv,
		"LearnASong_PreSongTunerMP"sv,
		"NonStopPlay_PreSongTuner"sv,
		"NonStopPlay_PreSongTunerMP"sv,
		"ScoreAttack_PreSongTuner"sv,
		"SessionMode_PreSMTunerMP"sv,
		"SessionMode_PreSMTuner"sv,
		"UIMenu_Tuner"sv,
		"UIMenu_TunerMP"sv,
		"Guitarcade_Tuner"sv,
		"Tuner"sv,
		"Duet_PreSongTuner"sv,
		"H2H_PreSongTuner"sv,
		"GETuner"sv,
		"PreGame_GETuner"sv
	};

	inline constexpr std::array songMenus = { // These are all the menus where you would play guitar games.
		"LearnASong_Game"sv,
		"NonStopPlay_Game"sv,
		"ScoreAttack_Game"sv,
		"LearnASong_Pause"sv,
		"NonStopPlay_Pause"sv,
		"ScoreAttack_Pause"sv,
		"LearnASong_RiffRepeater"sv,
		"RiffRepeater_AdvancedSettings"sv,
		"SessionMode_Game"sv,
		"SessionMode_PauseGame"sv,
		"Guitarcade_Game"sv,
		"Guitarcade_Pause"sv,
		"HelpList"sv, // Chords Menu
		"MixerMenu"sv
	};

	inline constexpr std::array lessonModes = { // These are the Guided Experience / Lessons modes.
		"GuidedExperience_Game"sv,
		"GuidedExperience_Pause"sv
	};

	inline constexpr std::array preSongTuners = { // Tuning menus where we aren't in the song
		"SelectionListDialog"sv,
		"LearnASong_PreSongTuner"sv,
		"LearnASong_PreSongTunerMP"sv,
		"NonStopPlay_PreSongTuner"sv,
		"NonStopPlay_PreSongTunerMP"sv,
		"ScoreAttack_PreSongTuner"sv,
		"SessionMode_PreSMTunerMP"sv,
		"SessionMode_PreSMTuner"sv,
		"Duet_PreSongTuner"sv,
		"H2H_PreSongTuner"sv,
		"PreGame_GETuner"sv
	};

	inline constexpr std::array songModes = { // These are all the menus where you would play guitar games.
		"LearnASong_Game"sv,
		"NonStopPlay_Game"sv,
		"ScoreAttack_Game"sv,
		"LearnASong_Pause"sv,
		"NonStopPlay_Pause"sv,
		"ScoreAttack_Pause"sv,

		// Riff Repeater (RR)
		"RiffRepeater"sv,
		"LearnASong_RiffRepeater"sv,
		"RiffRepeater_AdvancedSettings"sv,
		"RiffRepeater_Pause"sv,

		// Misc menus
		"Tuner"sv,
		"MixerMenu"sv,
		"HelpList"sv,
		"SideList"sv,
		"CalibrationMeter"sv

	};

	inline constexpr std::array multiplayerTuners = {
		"LearnASong_PreSongTunerMP"sv,
		"NonStopPlay_PreSongTunerMP"sv,
		"SessionMode_PreSMTunerMP"sv,
		"UIMenu_TunerMP"sv,
		"Duet_PreSongTuner"sv,
		"H2H_PreSongTuner"sv
	};

	inline constexpr std::array learnASongModes = {
		"LearnASong_Game"sv,
		"NonStopPlay_Game"sv,
		"LearnASong_Pause"sv,
		"NonStopPlay_Pause"sv
	};

	inline constexpr std::array learnASongPlaying = {
		"LearnASong_Game"sv,
		"NonStopPlay_Game"sv
	};

	inline constexpr std::array lasPauseMenus = {
		"LearnASong_Pause"sv,
		"NonStopPlay_Pause"sv
	};

	inline constexpr std::array fastRRModes = {
		"LearnASong_Game"sv,
		"NonStopPlay_Game"sv,
		"LearnASong_Pause"sv,
		"NonStopPlay_Pause"sv,
		"LearnASong_RiffRepeater"sv,
		"RiffRepeater"sv,
		"RiffRepeater_AdvancedSettings"sv,
		"RiffRepeater_Pause"sv
	};

	inline constexpr std::array scoreAttackModes = {
		"ScoreAttack_Game"sv,
		"ScoreAttack_Pause"sv
	};

	inline constexpr std::array scoreScreens = { // Screens for us to take score screenshots in
		"LearnASong_SongReview"sv,
		"ScoreAttack_SongReview"sv,
		"Duet_SongReview"sv,
		"H2H_SongReview"sv
	};

	// A Guitarcade minigame is being played or paused. Used by the ultrawide
	// correction: the 2D minigames draw their scene with an orthographic
	// projection that the correction's affine test cannot tell apart from a 2D
	// interface element, so the scene gets confined to 16:9. The correction skips
	// that clamp here while keeping it for Scaleform interface draws.
	inline constexpr std::array guitarcadeGameModes = {
		"Guitarcade_Game"sv,
		"Guitarcade_Pause"sv
	};

	inline constexpr std::array onlineModes = {
		"ScoreAttack_Game"sv,
		"Guitarcade_Game"sv
	};

	inline constexpr std::array dontAutoEnter = { // Don't use the auto load last profile mode if we are on these screens.
		// First time Player
		"TextEntryDialog"sv, // Prompts to enter profile name / uplay name / etc.
		"PlayedRS1Select"sv, // Did you play the original Rocksmith?
		"ExperienceSelect"sv, // How good at guitar do you think you are?
		"PathSelect"sv, // What path do you want to play? (Lead, Rhythm, Bass)
		"HandSelect"sv, // Are you left handed or right handed? (Left, Right)
		"HeadstockSelect"sv, // Select between (3+3 or 6-inline) (2+2 or 4-inline).
		"FE_InputSelect"sv, // What cable method do you have? (RTC, Mic, Disconnected)
		"FECalibrationMeter"sv, // First time calibrating.
		"VideoPlayer"sv, // Intro videos on how to calibrate, tune, play the game.
		"FETuner"sv, // First time tuning.
		"FirstEncounter_Game"sv, // Intro to game, teaches basic UI elements.
		//"SelectionListDialog"sv, // Occasional Yes/ No Prompts.

		// Played RS1 before
		"RefresherSelect"sv, // User has played Rocksmith before, but do they need some touchup on their knowledge.

		// Disconnected Mode
		"ImageDialog"sv, // Disconnected Mode Pictures.

		// Uplay section
		//"UplayLoginDialog"sv, // User needs to login to uPlay.
		"UplayAccountCreationDialog"sv // User needs to create a uPlay account.
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
		bool IsInGuitarcadeGame();
	}
};
