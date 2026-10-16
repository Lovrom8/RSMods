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
		Toggle(Setting::GuitarSpeak, "GuitarSpeak", "Guitar Speak")
			.Hint("Use your guitar to control the menus!")
			.Heading(Framework::SettingHeading::GameAndWindow),
		Toggle(Setting::GuitarSpeakWhileTuning, "GuitarSpeakWhileTuning", "Guitar Speak While Tuning")
			.Hint("For advanced users only!\nUse Guitar Speak in tuning menus.\nThis can potentially stop you from tuning, or playing songs, if set up improperly.")
			.Ini("Guitar Speak", "GuitarSpeakWhileTuning")
			.Category("Guitar Speak")
			.WithVisibleWhen(Setting::GuitarSpeak),
		String("GuitarSpeakCustomEditor", "Note Mappings")
			.Hint("Open the Guitar Speak note-mapping editor.")
			.Ini("Guitar Speak", "CustomEditor")
			.Category("Guitar Speak")
			.WithEditor("GuitarSpeak")
			.WithVisibleWhen(Setting::GuitarSpeak),

		Numeric(Setting::GuitarSpeakDelete, "Delete Note").Hint("Note that acts as the Delete key while Guitar Speak is active.").Ini("Guitar Speak", "GuitarSpeakDeleteWhen").Default("0").Category("Guitar Speak").WithVisibleWhen(Setting::GuitarSpeak).EditedBy("GuitarSpeak"),
		Numeric(Setting::GuitarSpeakSpace, "Space Note").Hint("Note that acts as the Space key while Guitar Speak is active.").Ini("Guitar Speak", "GuitarSpeakSpaceWhen").Default("0").Category("Guitar Speak").WithVisibleWhen(Setting::GuitarSpeak).EditedBy("GuitarSpeak"),
		Numeric(Setting::GuitarSpeakEnter, "Enter Note").Hint("Note that acts as the Enter key while Guitar Speak is active.").Ini("Guitar Speak", "GuitarSpeakEnterWhen").Default("0").Category("Guitar Speak").WithVisibleWhen(Setting::GuitarSpeak).EditedBy("GuitarSpeak"),
		Numeric(Setting::GuitarSpeakTab, "Tab Note").Hint("Note that acts as the Tab key while Guitar Speak is active.").Ini("Guitar Speak", "GuitarSpeakTabWhen").Default("0").Category("Guitar Speak").WithVisibleWhen(Setting::GuitarSpeak).EditedBy("GuitarSpeak"),
		Numeric(Setting::GuitarSpeakPageUp, "Page Up Note").Hint("Note that acts as the Page Up key while Guitar Speak is active.").Ini("Guitar Speak", "GuitarSpeakPGUPWhen").Default("0").Category("Guitar Speak").WithVisibleWhen(Setting::GuitarSpeak).EditedBy("GuitarSpeak"),
		Numeric(Setting::GuitarSpeakPageDown, "Page Down Note").Hint("Note that acts as the Page Down key while Guitar Speak is active.").Ini("Guitar Speak", "GuitarSpeakPGDNWhen").Default("0").Category("Guitar Speak").WithVisibleWhen(Setting::GuitarSpeak).EditedBy("GuitarSpeak"),
		Numeric(Setting::GuitarSpeakUpArrow, "Up Arrow Note").Hint("Note that acts as the Up Arrow key while Guitar Speak is active.").Ini("Guitar Speak", "GuitarSpeakUPWhen").Default("0").Category("Guitar Speak").WithVisibleWhen(Setting::GuitarSpeak).EditedBy("GuitarSpeak"),
		Numeric(Setting::GuitarSpeakDownArrow, "Down Arrow Note").Hint("Note that acts as the Down Arrow key while Guitar Speak is active.").Ini("Guitar Speak", "GuitarSpeakDNWhen").Default("0").Category("Guitar Speak").WithVisibleWhen(Setting::GuitarSpeak).EditedBy("GuitarSpeak"),
		Numeric(Setting::GuitarSpeakEscape, "Escape Note").Hint("Note that acts as the Escape key while Guitar Speak is active.").Ini("Guitar Speak", "GuitarSpeakESCWhen").Default("0").Category("Guitar Speak").WithVisibleWhen(Setting::GuitarSpeak).EditedBy("GuitarSpeak"),
		Numeric(Setting::GuitarSpeakClose, "Close Note").Hint("Note that closes Guitar Speak while it is active.").Ini("Guitar Speak", "GuitarSpeakCloseWhen").Default("0").Category("Guitar Speak").WithVisibleWhen(Setting::GuitarSpeak).EditedBy("GuitarSpeak"),
		Numeric(Setting::GuitarSpeakOBracket, "Open Bracket Note").Hint("Note that acts as the Open Bracket key while Guitar Speak is active.").Ini("Guitar Speak", "GuitarSpeakOBracketWhen").Default("0").Category("Guitar Speak").WithVisibleWhen(Setting::GuitarSpeak).EditedBy("GuitarSpeak"),
		Numeric(Setting::GuitarSpeakCBracket, "Close Bracket Note").Hint("Note that acts as the Close Bracket key while Guitar Speak is active.").Ini("Guitar Speak", "GuitarSpeakCBracketWhen").Default("0").Category("Guitar Speak").WithVisibleWhen(Setting::GuitarSpeak).EditedBy("GuitarSpeak"),
		Numeric(Setting::GuitarSpeakTildea, "Tilde Note").Hint("Note that acts as the Tilde key while Guitar Speak is active.").Ini("Guitar Speak", "GuitarSpeakTildeaWhen").Default("0").Category("Guitar Speak").WithVisibleWhen(Setting::GuitarSpeak).EditedBy("GuitarSpeak"),
		Numeric(Setting::GuitarSpeakForSlash, "Forward Slash Note").Hint("Note that acts as the Forward Slash key while Guitar Speak is active.").Ini("Guitar Speak", "GuitarSpeakForSlashWhen").Default("0").Category("Guitar Speak").WithVisibleWhen(Setting::GuitarSpeak).EditedBy("GuitarSpeak"),
		Numeric(Setting::GuitarSpeakAlt, "Alt Note").Hint("Note that acts as the Alt key while Guitar Speak is active.").Ini("Guitar Speak", "GuitarSpeakAltWhen").Default("0").Category("Guitar Speak").WithVisibleWhen(Setting::GuitarSpeak).EditedBy("GuitarSpeak"),
	};
}

void GuitarSpeakMod::OnMenuTick(ModContext& c) {
	if (!c.IsOn(Setting::GuitarSpeak)) {
		StopListening();
		return;
	}

	if (listener.joinable())
		return;

	GuitarSpeak::keysDirty = true;
	listener = std::jthread([](std::stop_token stop) {
		try {
			GuitarSpeak::RunGuitarSpeak(stop);
		}
		catch (const std::exception& e) {
			LOG_ERROR("(GS) Guitar Speak stopped: " << e.what() << std::endl);
		}
	});
}

void GuitarSpeakMod::OnSettingsChanged(ModContext&) {
	GuitarSpeak::keysDirty = true;
}

void GuitarSpeakMod::OnDisabled(ModContext&) {
	StopListening();
}

void GuitarSpeakMod::OnShutdown(ModContext&) {
	StopListening();
}

// Waits out at most one 50 ms poll, plus a key press in progress.
void GuitarSpeakMod::StopListening() {
	if (!listener.joinable())
		return;

	listener.request_stop();
	listener.join();
}


static Framework::ModRegistrar<GuitarSpeakMod> _guitarSpeakReg;
