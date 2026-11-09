#pragma once

#include <string>

// Drain native installation and loading within a shared game-thread frame budget.
// Workers prepare file data only; engine mutation stays on the game thread.
// Unsupported archives use native installation with file warming.
namespace EnumerationDrain {
	// Seconds since launch; zero until menu readiness.
	double MainMenuReadySeconds();
	struct Progress {
		bool active{};
		bool completed{};
		bool upToDate{};
		unsigned int detected{};
		unsigned int processed{};
		unsigned int remaining{};
		unsigned int registered{};
		unsigned int notRegistered{};
		unsigned int newDlc{};
		unsigned int totalDlc{};
		double elapsedSeconds{};
		double completionElapsedSeconds{};
	};

	void Install();
	bool IsInstalled();
	Progress GetProgress();

	// Explicit user requests must bypass sign-in scan suppression.
	void NoteManualRequest();

	// Publish menu state from the mod thread; the game thread must not read currentMenu directly.
	void NoteMenu(const std::string& menu);
}
