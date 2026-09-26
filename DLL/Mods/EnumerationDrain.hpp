#pragma once

#include <string>

// Faster DLC scan (enumeration), for large song libraries.
//
// The DLC service's per-frame tick runs one install step, which pops one path off the scan queue and installs it, so a scan of
// N packages takes at least N frames. After the install, each package waits on the game's package load list, and the service's
// load step moves ONE of those into its load slot per tick, so the second stage is also one per frame.
//
// This keeps the game's own code doing all the work, on the game thread, and just calls it more often:
//   - After the service's own tick, the install step and the load step are called again within a time budget (a vtable slot
//     swap on the service's tick, no game bytes changed).
//   - Every psarc is opened with fopen from MSVCR100, whose open-stream cap is 512. Mounts keep their stream until the load stage
//     is done, so going faster than one per frame runs out of streams, fopen fails, and the game drops the package as not found.
//     The cap is raised to MSVCR100's max of 2048, and the drain pauses while the open CRT handles are near it.
//   - The per-package shader cache scan in package registration is skipped for packages the DLC scan
//     installs. It was ~11 of the ~12 ms per install, and song packages don't ship generated shaders.
//   - A rescan of an unchanged library pops the entries that registered last time (same path, size and write time) without
//     opening them again.
//   - The native "Enumerating downloadable content..." banner is replaced with a progress bar (Menu::RenderImGuiMenu).
//   - The background asset loader handles more than one load per tick while the scan runs (AssetLoadDrain).
//
// Based on ffio's private EnumerationDrain. Every address was re-checked on both supported builds.
//
// On with FastEnumeration (Toggle Switches). Tuning is hidden in RSMods.ini under [Fast Enumeration], not in the GUI:
//   FastEnumerationInstallsPerTick      extra installs per frame (default 16, 0 turns the install drain off)
//   FastEnumerationLoadsPerTick         extra load-stage steps per frame (default 32)
//   FastEnumerationBudgetMs             time budget per frame (default 24)
//   FastEnumerationMenuBudgetMs         time budget per frame once the main menu is up (default 6, the loft streams on the same thread)
//   FastEnumerationSkipUnchanged        on/off, rescans skip unchanged packages (default on)
//   FastEnumerationSkipShaderScan       on/off, skip the per-package shader cache scan (default on)
//   FastEnumerationStreamLimit          CRT stream cap, 512..2048 (default 2048)
//   FastEnumerationFileCeiling          open CRT handles at which the scan pauses (default 1792)
//   FastEnumerationPrefetchKB           read-ahead per package on a background thread (default 128, 0 off)
//   FastEnumerationPrefetchHddOnly      on/off, only read ahead when a DLC volume is a spinning disk (default off)
//   FastEnumerationEarlyScan            on/off, scan at the DLC service's first tick instead of after the profile screens (default off)
//   FastEnumerationEarlyScanMax         largest library the early scan runs for (default 1500)
//   FastEnumerationBootBudgetMs / FastEnumerationBootInstallsPerTick   budget and installs per frame for the early scan (default 30 / 32)
//   FastEnumerationAssetLoadsPerTick    background asset loads per loader tick while the scan runs (default 64, 1 = stock)
namespace EnumerationDrain {
	struct Progress {
		bool active{};
		bool completed{};
		bool upToDate{};		// Completed with nothing new. Shown briefly as "UP TO DATE".
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

	// Called by Enumeration::ForceEnumeration so a user's request is never taken for the game's own sign-in request.
	void NoteManualRequest();

	// Called from the mod thread with GameState::currentMenu. The drain runs on the game thread and must not read that string.
	void NoteMenu(const std::string& menu);
}
