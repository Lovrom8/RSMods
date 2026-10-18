#include "../stdafx.h"
#include "MidiMod.hpp"
#include "Midi.hpp"

using Framework::SettingDefs;
using Framework::SettingDef;
using Framework::Toggle;
using Framework::Numeric;
using Framework::Enum;
using Framework::String;
using Framework::ModContext;
using Framework::GamePhase;
using Framework::KeyEdge;
using Framework::Availability;
using Framework::KeyEvent;
using Settings::When;
namespace Setting = Settings::Setting;

SettingDefs MidiMod::Settings() const {
	return {
		Toggle(Setting::AutoTuneForSong, "AutoTuneForSong", "Auto Tune For Song")
			.Hint("If you have a drop tuning pedal with a MIDI port, we will attempt to automatically tune.")
			.Heading(Framework::SettingHeading::Tuning),
		String(Setting::AutoTuneForSongDevice, "Auto Tune MIDI Output Device")
			.Hint("Select the MIDI device that goes to your drop tuning pedal.\nWe will send a signal to the pedal to try to automatically tune it.")
			.Ini("Toggle Switches", "AutoTuneForSongDevice")
			.ChoicesSource("MidiOutDevices")
			.WithVisibleWhen(Setting::AutoTuneForSong),
		String(Setting::MidiInDevice, "MIDI Input Device")
			.Hint("MIDI input device to listen to for tuning changes.")
			.Ini("Toggle Switches", "MidiInDevice")
			.ChoicesSource("MidiInDevices")
			.WithVisibleWhen(Setting::AutoTuneForSong),
		Enum(Setting::AutoTuneForSongWhen, "Auto Tune When")
			.Hint("Whether to auto-tune manually or automatically via the in-game tuner.")
			.Ini("Toggle Switches", "AutoTuneForSongWhen")
			.Choices({ "manual", "tuner" }, "manual")
			.WithVisibleWhen(Setting::AutoTuneForSong),
		Numeric(Setting::TuningPedal, "Tuning Pedal")
			.Hint("Which drop-tuning pedal you are using.")
			.Ini("Mod Settings", "TuningPedal")
			.Default("0")
			.Range(0, 5)
			.ListedUnder(Setting::AutoTuneForSong),
		Numeric(Setting::TuningOffset, "Tuning Offset")
			.Hint("What tuning is your guitar / bass set to?\nWe can adjust how we auto tune based on what you specify here.\nThis value can be changed in game by setting the \"Change Tuning Offset\" keybind.\nPress the keybind to go down in tuning; hold Shift (or Control) while pressing it to go up.")
			.Ini("Mod Settings", "TuningOffset")
			.Default("0")
			.Range(-3, 12)
			.ListedUnder(Setting::AutoTuneForSong),
		Toggle(Setting::ChordsMode, "ChordsMode", "Chords Mode")
			.Hint("If you are using the Whammy or Whammy Bass, are you using the pedal in Chords Mode or Classic Mode?\nClassic Mode = unchecked, Chords Mode = checked.")
			.WithVisibleWhen(Setting::AutoTuneForSong),
		String(Setting::AutoTuneForSoftwareSemitoneSettings, "Software Semitone Settings")
			.Hint("Semitone offsets applied by the software auto-tuner.")
			.Ini("Toggle Switches", "AutoTuneForSoftwareSemitoneSettings")
			.WithVisibleWhen(Setting::AutoTuneForSong),
		String(Setting::AutoTuneForSoftwareSemitoneTriggers, "Software Semitone Triggers")
			.Hint("Which tunings trigger the software semitone auto-tuner.")
			.Ini("Toggle Switches", "AutoTuneForSoftwareSemitoneTriggers")
			.WithVisibleWhen(Setting::AutoTuneForSong),
		String(Setting::AutoTuneForSoftwareTrueTuningSettings, "Software True Tuning Settings")
			.Hint("True-tuning (cents) values applied by the software auto-tuner.")
			.Ini("Toggle Switches", "AutoTuneForSoftwareTrueTuningSettings")
			.WithVisibleWhen(Setting::AutoTuneForSong),
		String(Setting::AutoTuneForSoftwareTrueTuningTriggers, "Software True Tuning Triggers")
			.Hint("Which tunings trigger the software true-tuning auto-tuner.")
			.Ini("Toggle Switches", "AutoTuneForSoftwareTrueTuningTriggers")
			.WithVisibleWhen(Setting::AutoTuneForSong),
		SettingDef{
			"MidiCustomEditor",
			{ "Mod Settings", "MidiCustomEditor" },
			Framework::SettingType::String,
			"",
			"MIDI Setup",
			"Open the custom MIDI and pedal setup editor",
		}.WithEditor("Midi"),
		Framework::KeyBind(Setting::Key::TuningOffset, "Change Tuning Offset", "O"),
	};
}

// Every feature here serves auto-tune, and OnDisabled reverts the pedal, so switching it off needs no latch of its
// own. Gating also keeps the tuning-controller claim honest: it only contends while auto-tune is actually on.
bool MidiMod::IsEnabled(const ModContext& c) const {
	return c.IsOn(Setting::AutoTuneForSong);
}

std::vector<std::string_view> MidiMod::ClaimsExclusive() const {
	return { "tuning-controller" };
}

void MidiMod::OnInitialize(ModContext& c) {
	Midi::tuningOffset = c.Int(Setting::TuningOffset);

	// This command changes tuning-controller state, so it must never bypass conflict suppression.
	c.Commands().BindSetting(
		Setting::Key::TuningOffset,
		KeyEdge::Up,
		Availability::Active,
		[](ModContext&, const KeyEvent& event) {
			// A higher offset is a lower tuning. Shift goes up as the hint says; Control did before and still does.
			Midi::tuningOffset += (event.shift || event.control) ? -1 : 1;
			Midi::tuningOffset = std::clamp(Midi::tuningOffset, -3, 12);
			LOG_INFO("Triggered Mod Setting: Tuning Offset is now set to " << Midi::tuningOffset << std::endl);
		},
		[](const ModContext& context, const KeyEvent&) {
			return context.IsOn(Setting::AutoTuneForSong);
		});

	// Delete is a fixed host shortcut, but its intent belongs to this tuning-controller mod.
	c.Commands().BindKey(
		"ManualMidiAutoTune",
		VK_DELETE,
		KeyEdge::Up,
		Availability::Active,
		[](ModContext&, const KeyEvent&) {
			Midi::userWantsToUseAutoTuning = true;
		},
		[](const ModContext& context, const KeyEvent&) {
			return context.When(Setting::AutoTuneForSongWhen) == When::Manual &&
				GameState::Menus::IsInTuningMenus();
		});

	c.Menu().Register("midi", "MIDI", 10, [this] { DrawMenu(); });
}

void MidiMod::OnSettingsChanged(ModContext& c) {
	Midi::tuningOffset = c.Int(Setting::TuningOffset);
}

void MidiMod::OnTick(ModContext& c) {
	if (c.phase == GamePhase::Loading)
		return;

	ScanForMidiDevices(c);
	Midi::ProcessMidiInputEvents();
}

// Two independent one-time setup steps: load the auto-tune pedal settings once AutoTuneForSong is on, 
// and spin up the MIDI-in listener once a MidiInDevice is configured (it only serves auto-tune).
void MidiMod::ScanForMidiDevices(ModContext& c) {
	if (!Midi::scannedForMidiDevices && c.IsOn(Setting::AutoTuneForSong)) {
		Midi::scannedForMidiDevices = true;
		Midi::ReadMidiSettingsFromINI(
			c.Value(Setting::ChordsMode),
			c.Int(Setting::TuningPedal),
			c.Value(Setting::AutoTuneForSongDevice),
			c.Value(Setting::MidiInDevice));
	}

	if (!midiInStarted && c.IsOn(Setting::AutoTuneForSong) && c.Value(Setting::MidiInDevice) != "") {
		midiInStarted = true;
		Midi::FindMidiInDevices(c.Value(Setting::MidiInDevice));
		midiIn = std::jthread(Midi::ListenToMidiIn);
	}
}

// While the pre-song tuner is up, tune the pedal from the tuner's tuning readout (When == Tuner only).
void MidiMod::AutoTuneInTuner(ModContext& c) {
	if (!tunerReadUntil) {
		if (!GameState::Menus::IsInPreSongTuner() ||
			!c.IsOn(Setting::AutoTuneForSong) ||
			c.When(Setting::AutoTuneForSongWhen) != When::Tuner ||
			Midi::alreadyAttemptedTuningInTuner ||
			Midi::alreadyAutomatedTuningInThisSong) {
			return;
		}

		if (!Midi::BeginTuningInTuner()) {
			Midi::appliedTunerAutoTune = true; // Lets ExtendedRange skip its tuning-settle wait.
			return;
		}

		tunerReadUntil = Clock::now() + TunerReadTime;
	}

	if (!Midi::TryTuningInTuner(Clock::now() >= *tunerReadUntil)) {
		c.RequestFastTick(); // The tuner has no tuning yet; look again soon.
		return;
	}

	tunerReadUntil.reset();
	Midi::appliedTunerAutoTune = true; // Lets ExtendedRange skip its tuning-settle wait.
}

// The tuner closed before it could be read; nothing was sent to the pedal, so there's nothing to revert.
void MidiMod::CancelTunerRead() {
	if (!tunerReadUntil)
		return;

	tunerReadUntil.reset();
	Midi::alreadyAttemptedTuningInTuner = false;
}

// In a song. Tune the pedal from the song's tuning the first time through, honouring the When setting.
void MidiMod::OnSongTick(ModContext& c) {
	CancelTunerRead(); // The song started mid-read; the song path tunes instead.
	AutoTuneInSong(c);
	PublishHud(c);
}

void MidiMod::OnMenuTick(ModContext& c) {
	if (!GameState::Menus::IsInPreSongTuner())
		CancelTunerRead();

	// The user backed out of the tuner instead of starting a song.
	if (Midi::alreadyAttemptedTuningInTuner &&
		!GameState::Menus::IsInPreSongTuner()) {
		RevertTuning();
	}

	AutoTuneInTuner(c);
	PublishHud(c);
}

void MidiMod::OnDisabled(ModContext&) {
	CancelTunerRead();
	songTuneAt.reset();
	RevertTuning();
	StopMidiIn();
}

void MidiMod::OnShutdown(ModContext&) {
	StopMidiIn();
}

// Closes the MIDI IN port (at most one 50 ms wait) and drops what it queued; the next activation listens again.
void MidiMod::StopMidiIn() {
	if (midiIn.joinable()) {
		midiIn.request_stop();
		midiIn.join();
	}

	Midi::ClearMidiInputEvents();
	midiInStarted = false;
}

void MidiMod::RevertTuning() {
	if (!Midi::alreadyAutomatedTuningInThisSong &&
		!Midi::alreadyAttemptedTuningInTuner) {
		return;
	}

	Midi::RevertAutomatedTuning();
	Midi::alreadyAttemptedTuningInTuner = false;
	Midi::userWantsToUseAutoTuning = false;
}

void MidiMod::OnSongExit(ModContext&) {
	songTuneAt.reset(); // Left before the song's tuning settled; nothing was sent to the pedal.
	RevertTuning();
	Midi::tunerAutoTuneFailed = false;
}

void MidiMod::AutoTuneInSong(ModContext& c) {
	if (songTuneAt) {
		if (Clock::now() < *songTuneAt)
			return; // The song's tuning isn't set yet.

		songTuneAt.reset();
		Midi::AutomateTuning();
		return;
	}

	// When::Tuner falls back to tuning here when the song started without a tuner (same tuning as the last song). If the tuner
	// was shown and couldn't be read, the player tuned by hand, so don't stack the pedal on top of that.
	if (c.IsOn(Setting::AutoTuneForSong) &&
		!Midi::alreadyAutomatedTuningInThisSong &&
		((c.When(Setting::AutoTuneForSongWhen) == When::Tuner && !Midi::tunerAutoTuneFailed) ||
			(c.When(Setting::AutoTuneForSongWhen) == When::Manual && Midi::userWantsToUseAutoTuning))) {
		songTuneAt = Clock::now() + SongTuningSettleTime;
	}
}

void MidiMod::PublishHud(ModContext& c) {
	Framework::HudText snapshot;
	snapshot.visible = c.IsOn(Setting::AutoTuneForSong) &&
		Settings::GetKeyBind(Setting::Key::TuningOffset) != NULL &&
		GameState::Menus::IsInTuningMenus();

	if (snapshot.visible) {
		snapshot.text = "Auto Tune For: " + Midi::GetTuningOffsetName(Midi::tuningOffset);
	}

	c.Hud().Set("autotune-tuning", { Framework::HudAnchor::TopTuning, 0 }, std::move(snapshot));
}

void MidiMod::DrawMenu() {
	static std::string previewValue = "Select a device";
	if (ImGui::BeginCombo("MIDI devices", previewValue.c_str())) {
		for (size_t i = 0; i < Midi::NumberOfOutPorts; ++i) {
			const bool isSelected = (Midi::SelectedMidiOutDevice == static_cast<int>(i));
			const auto& device = Midi::midiOutDevices[i];

			if (ImGui::Selectable(device.szPname, isSelected, ImGuiSelectableFlags_DontClosePopups)) {
				Midi::SelectedMidiOutDevice = static_cast<int>(i);
			}

			if (isSelected) {
				previewValue = device.szPname;
				ImGui::SetItemDefaultFocus();
			}
		}
		ImGui::EndCombo();
	}

	ImGui::SliderInt("Program Change", &Midi::MidiPC, 0, 127);
	ImGui::SliderInt("Control Change", &Midi::MidiCC, 0, 127);

	if (ImGui::Button("Send PC MIDI Message"))
		Midi::SendDataToThread_PC(Midi::AsMidiByte(Midi::MidiPC));

	if (ImGui::Button("Send CC MIDI Message"))
		Midi::SendDataToThread_CC(Midi::AsMidiByte(Midi::MidiCC));
}

static Framework::ModRegistrar<MidiMod> _midiReg;
