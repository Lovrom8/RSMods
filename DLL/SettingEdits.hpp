#pragma once

#include <optional>
#include <string>

// Edits made in the in-game settings window: applied live at once, then saved to RSMods.ini.
//
// Saving keeps one writer at a time. While the RSMods GUI runs it owns the file (it rewrites it whole from memory),
// so the edit goes to the GUI over WM_COPYDATA; with no GUI, or a GUI set up for another game folder, the game
// patches the one line itself.
//
// Until the file has the edit, rereading it (the GUI's "update all", Ctrl+A) would bring the old value back, so an
// edit stays pending and every reread applies it again until the file shows it.
namespace SettingEdits {
	struct Edit {
		std::string key;      // In-code key, which the GUI's fields are keyed by too
		std::string section;  // Bare INI section name
		std::string name;     // INI key
		std::string value;    // As the INI stores it
		std::string label;    // For the status line
		bool isInt = false;   // Int settings live in customSettings, the rest in modSettings
	};

	// Any thread; returns at once.
	void Apply(Edit edit);

	// The edit's value while it's pending, for the menu to show instead of what the game had a moment ago.
	std::optional<std::string> PendingValue(const std::string& key);

	// MainThread, straight after RSMods.ini is reread.
	void ReapplyPending();

	struct Status {
		std::string text;
		bool error = false;
	};

	// Result of the most recent save, empty before the first.
	Status LastStatus();
}
