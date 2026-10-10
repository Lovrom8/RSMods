#pragma once

// In-game editor for every schema-declared mod setting. SettingEdits applies and saves what it changes.
namespace SettingsMenu {
	// MainThread, after every mod has registered: snapshots the schema and registers the menu entry.
	void Register();
}
