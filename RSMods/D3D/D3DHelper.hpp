#pragma once
#include <vector>
#include <string>
#include <fstream>

namespace D3DHelper {
	inline char dlldir[320];
	inline char* GetDirFile(char* name) {
		static char pldir[320];
		strcpy_s(pldir, dlldir);
		strcat_s(pldir, name);
		return pldir;
	}

	inline void Log(const char* fmt, ...)
	{
		if (!fmt)	return;

		char		text[4096];
		va_list		ap;
		va_start(ap, fmt);
		vsprintf_s(text, fmt, ap);
		va_end(ap);

		std::ofstream logfile;
		logfile.open(GetDirFile((PCHAR)"log.txt"), std::ios::app);
		if (logfile.is_open() && text)	logfile << text << std::endl;
		logfile.close();
	}
}

inline std::vector<std::string> tuningMenus = { // These are all the menus where you need to tune
	"SelectionListDialog",
	"LearnASong_PreSongTuner",
	"LearnASong_PreSongTunerMP",
	"LearnASong_SongOptions",
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

inline std::vector<std::string> preSongTuners = { // Tuning menus where we aren't in the song
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

inline std::vector<std::string> songModes = { // These are all the menus where you would play guitar games.
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

inline std::vector<std::string> multiplayerTuners = {
	"LearnASong_PreSongTunerMP",
	"NonStopPlay_PreSongTunerMP",
	"SessionMode_PreSMTunerMP",
	"UIMenu_TunerMP",
	"Duet_PreSongTuner",
	"H2H_PreSongTuner",
};

inline std::vector<std::string> learnASongModes = {
	"LearnASong_Game",
	"NonStopPlay_Game",
	"LearnASong_Pause",
	"NonStopPlay_Pause",
};

inline std::vector<std::string> lasPauseMenus = {
	"LearnASong_Pause",
	"NonStopPlay_Pause",
};

inline std::vector<std::string> fastRRModes = {
	"LearnASong_Game",
	"NonStopPlay_Game",
	"LearnASong_Pause",
	"NonStopPlay_Pause",
	"RiffRepeater",
	"RiffRepeater_AdvancedSettings",
	"RiffRepeater_Pause",
};

inline std::vector<std::string> scoreAttackModes = {
	"ScoreAttack_Game",
	"ScoreAttack_Pause",
};

inline std::vector<std::string> lessonModes = { // These are the Guided Experience / Lessons modes.
	"GuidedExperience_Game",
	"GuidedExperience_Pause"
};

inline std::vector<std::string> scoreScreens = { // Screens for us to take score screenshots in
	"LearnASong_SongReview",
	"ScoreAttack_SongReview",
	"Duet_SongReview",
	"H2H_SongReview",
};

inline std::vector<std::string> onlineModes = {
	"ScoreAttack_Game",
	"Guitarcade_Game",
};

inline std::vector<std::string> dontAutoEnter = { // Don't use the auto load last profile mode if we are on these screens.
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
	"SelectionListDialog", // Occasional Yes/ No Prompts.

	// Played RS1 before
	"RefresherSelect", // User has played Rocksmith before, but do they need some touchup on their knowledge.

	// Disconnected Mode
	"ImageDialog", // Disconnected Mode Pictures.

	// Uplay section
	"UplayLoginDialog", // User needs to login to uPlay.
	"UplayAccountCreationDialog" // User needs to create a uPlay account.
};


struct Mesh {
	UINT Stride;
	UINT PrimCount;
	UINT NumVertices;

	Mesh(UINT s, UINT p, UINT n) {
		Stride = s;
		PrimCount = p;
		NumVertices = n;
	}

	bool operator ==(const Mesh& meshB) const {
		return (Stride == meshB.Stride
			&& PrimCount == meshB.PrimCount
			&& NumVertices == meshB.NumVertices);
	}
};

struct ThiccMesh {
	UINT Stride;
	UINT PrimCount;
	UINT NumVertices;
	UINT StartIndex;
	UINT StartRegister;
	UINT PrimType;
	UINT DeclType;
	UINT VectorCount;
	UINT NumElements;

	ThiccMesh(UINT s, UINT p, UINT n, UINT si, UINT mSR, UINT pt, UINT dt, UINT mvc, UINT nm) {
		Stride = s;
		PrimCount = p;
		NumVertices = n;
		StartIndex = si;
		StartRegister = mSR;
		PrimType = pt;
		DeclType = dt;
		VectorCount = mvc;
		NumElements = nm;
	}

	std::string ToString() {
		std::string ret = "{ ";

		ret += std::to_string(Stride);
		ret += ",";
		ret += std::to_string(PrimCount);
		ret += ",";
		ret += std::to_string(NumVertices);
		ret += ",";
		ret += std::to_string(StartIndex);
		ret += ",";
		ret += std::to_string(StartRegister);
		ret += ",";
		ret += std::to_string(PrimType);
		ret += ",";
		ret += std::to_string(DeclType);
		ret += ",";
		ret += std::to_string(VectorCount);
		ret += ",";
		ret += std::to_string(NumElements);

		ret += " },";

		return ret;
	}

	bool operator ==(const ThiccMesh& meshB) const {
		return (Stride == meshB.Stride
			&& PrimCount == meshB.PrimCount
			&& NumVertices == meshB.NumVertices
			&& StartIndex == meshB.StartIndex
			&& StartRegister == meshB.StartRegister
			&& PrimType == meshB.PrimType
			&& DeclType == meshB.DeclType
			&& VectorCount == meshB.VectorCount
			&& NumElements == meshB.NumElements);
	}
};

inline bool IsExtraRemoved(std::vector<ThiccMesh> list, ThiccMesh mesh) {
	for (const auto& currentMesh : list)
		if (currentMesh == mesh)
			return true;

	return false;
}

inline bool IsToBeRemoved(std::vector<Mesh> list, Mesh mesh) {
	for (const auto& currentMesh : list)
		if (currentMesh == mesh)
			return true;

	return false;
}

/*--------------------------- GENERAL D3D -----------------------------*/


/*--------------------------- LOG -----------------------------*/

