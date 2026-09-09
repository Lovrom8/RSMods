#include "../stdafx.h"
#include "ShowCurrentNoteMod.hpp"
#include "GuitarSpeak.hpp"

using Framework::ModContext;
namespace Setting = Settings::Setting;

bool ShowCurrentNoteMod::IsEnabled(const ModContext& c) const {
	return c.IsOn(Setting::ShowCurrentNoteOnScreen);
}

void ShowCurrentNoteMod::OnSongTick(ModContext& c) {
	Publish(c, true);
}

void ShowCurrentNoteMod::OnMenuTick(ModContext& c) {
	Publish(c, false);
}

void ShowCurrentNoteMod::Publish(ModContext& c, bool inSong) const {
	const bool enabled = c.IsOn(Setting::ShowCurrentNoteOnScreen);
	std::string noteName;
	if (enabled) {
		noteName = GuitarSpeak::GetCurrentNoteName();
	}

	const bool hasNote = !noteName.empty();

	// In song: HighwayLeft anchor
	Framework::HudText highwaySnapshot;
	highwaySnapshot.visible = enabled && inSong && hasNote;
	if (highwaySnapshot.visible) {
		highwaySnapshot.text = noteName;
	}
	c.Hud().Set("current-note-song", { Framework::HudAnchor::HighwayLeft, 0 }, std::move(highwaySnapshot));

	// Outside song (in menus): MenuBanner anchor
	Framework::HudText menuSnapshot;
	menuSnapshot.visible = enabled && !inSong && hasNote;
	if (menuSnapshot.visible) {
		menuSnapshot.text = "Current Note: " + noteName;
	}
	c.Hud().Set("current-note-menu", { Framework::HudAnchor::MenuBanner, 0 }, std::move(menuSnapshot));
}

static Framework::ModRegistrar<ShowCurrentNoteMod> _showCurrentNoteReg;
