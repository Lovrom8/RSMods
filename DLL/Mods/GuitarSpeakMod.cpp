#include "../stdafx.h"
#include "GuitarSpeakMod.hpp"
#include "GuitarSpeak.hpp"

using Framework::ModContext;
using Framework::SettingDefs;
using Framework::Toggle;
using Framework::String;
using Framework::Numeric;
namespace Setting = Settings::Setting;

SettingDefs GuitarSpeakMod::Settings() const {
	return {
		Toggle(Setting::GuitarSpeak, "GuitarSpeak", "Guitar Speak"),
		Toggle(Setting::GuitarSpeakWhileTuning, "GuitarSpeakWhileTuning", "Guitar Speak While Tuning")
			.Ini("Guitar Speak", "GuitarSpeakWhileTuning")
			.Category("Guitar Speak")
			.WithVisibleWhen(Setting::GuitarSpeak),
		String("GuitarSpeakCustomEditor", "Note Mappings")
			.Ini("Guitar Speak", "CustomEditor")
			.Category("Guitar Speak")
			.WithEditor("GuitarSpeak")
			.WithVisibleWhen(Setting::GuitarSpeak),

		Numeric(Setting::GuitarSpeakDelete, "Delete Note").Ini("Guitar Speak", "GuitarSpeakDeleteWhen").Default("0").Category("Guitar Speak").WithVisibleWhen(Setting::GuitarSpeak),
		Numeric(Setting::GuitarSpeakSpace, "Space Note").Ini("Guitar Speak", "GuitarSpeakSpaceWhen").Default("0").Category("Guitar Speak").WithVisibleWhen(Setting::GuitarSpeak),
		Numeric(Setting::GuitarSpeakEnter, "Enter Note").Ini("Guitar Speak", "GuitarSpeakEnterWhen").Default("0").Category("Guitar Speak").WithVisibleWhen(Setting::GuitarSpeak),
		Numeric(Setting::GuitarSpeakTab, "Tab Note").Ini("Guitar Speak", "GuitarSpeakTabWhen").Default("0").Category("Guitar Speak").WithVisibleWhen(Setting::GuitarSpeak),
		Numeric(Setting::GuitarSpeakPageUp, "Page Up Note").Ini("Guitar Speak", "GuitarSpeakPGUPWhen").Default("0").Category("Guitar Speak").WithVisibleWhen(Setting::GuitarSpeak),
		Numeric(Setting::GuitarSpeakPageDown, "Page Down Note").Ini("Guitar Speak", "GuitarSpeakPGDNWhen").Default("0").Category("Guitar Speak").WithVisibleWhen(Setting::GuitarSpeak),
		Numeric(Setting::GuitarSpeakUpArrow, "Up Arrow Note").Ini("Guitar Speak", "GuitarSpeakUPWhen").Default("0").Category("Guitar Speak").WithVisibleWhen(Setting::GuitarSpeak),
		Numeric(Setting::GuitarSpeakDownArrow, "Down Arrow Note").Ini("Guitar Speak", "GuitarSpeakDNWhen").Default("0").Category("Guitar Speak").WithVisibleWhen(Setting::GuitarSpeak),
		Numeric(Setting::GuitarSpeakEscape, "Escape Note").Ini("Guitar Speak", "GuitarSpeakESCWhen").Default("0").Category("Guitar Speak").WithVisibleWhen(Setting::GuitarSpeak),
		Numeric(Setting::GuitarSpeakClose, "Close Note").Ini("Guitar Speak", "GuitarSpeakCloseWhen").Default("0").Category("Guitar Speak").WithVisibleWhen(Setting::GuitarSpeak),
		Numeric(Setting::GuitarSpeakOBracket, "Open Bracket Note").Ini("Guitar Speak", "GuitarSpeakOBracketWhen").Default("0").Category("Guitar Speak").WithVisibleWhen(Setting::GuitarSpeak),
		Numeric(Setting::GuitarSpeakCBracket, "Close Bracket Note").Ini("Guitar Speak", "GuitarSpeakCBracketWhen").Default("0").Category("Guitar Speak").WithVisibleWhen(Setting::GuitarSpeak),
		Numeric(Setting::GuitarSpeakTildea, "Tilde Note").Ini("Guitar Speak", "GuitarSpeakTildeaWhen").Default("0").Category("Guitar Speak").WithVisibleWhen(Setting::GuitarSpeak),
		Numeric(Setting::GuitarSpeakForSlash, "Forward Slash Note").Ini("Guitar Speak", "GuitarSpeakForSlashWhen").Default("0").Category("Guitar Speak").WithVisibleWhen(Setting::GuitarSpeak),
		Numeric(Setting::GuitarSpeakAlt, "Alt Note").Ini("Guitar Speak", "GuitarSpeakAltWhen").Default("0").Category("Guitar Speak").WithVisibleWhen(Setting::GuitarSpeak),
	};
}

void GuitarSpeakMod::OnMenuTick(ModContext& c) {
	if (!guitarSpeakPresent && c.IsOn(Setting::GuitarSpeak)) {
		guitarSpeakPresent = true;

		if (!GuitarSpeak::RunGuitarSpeak()) { // If we are in a menu where we don't want to read bad values
			guitarSpeakPresent = false;
		}
	}
}

// Guitar Speak restarts each time the player returns to a menu, so clear the latch while in a song.
void GuitarSpeakMod::OnSongTick(ModContext&) {
	guitarSpeakPresent = false;
}


static Framework::ModRegistrar<GuitarSpeakMod> _guitarSpeakReg;
