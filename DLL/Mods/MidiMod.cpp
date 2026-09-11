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
		Toggle(Setting::AutoTuneForSong, "AutoTuneForSong", "Auto Tune For Song"),
		String(Setting::AutoTuneForSongDevice, "Auto Tune MIDI Output Device")
			.Ini("Toggle Switches", "AutoTuneForSongDevice")
			.ChoicesSource("MidiOutDevices")
			.WithVisibleWhen(Setting::AutoTuneForSong),
		String(Setting::MidiInDevice, "MIDI Input Device")
			.Ini("Toggle Switches", "MidiInDevice")
			.ChoicesSource("MidiInDevices"),
		Enum(Setting::AutoTuneForSongWhen, "Auto Tune When")
			.Ini("Toggle Switches", "AutoTuneForSongWhen")
			.Choices({ "manual", "tuner" }, "manual")
			.WithVisibleWhen(Setting::AutoTuneForSong),
		Numeric(Setting::TuningPedal, "Tuning Pedal")
			.Ini("Mod Settings", "TuningPedal")
			.Default("0")
			.Range(0, 5),
		Numeric(Setting::TuningOffset, "Tuning Offset")
			.Ini("Mod Settings", "TuningOffset")
			.Default("0")
			.Range(-3, 12),
		Toggle(Setting::ChordsMode, "ChordsMode", "Chords Mode")
			.WithVisibleWhen(Setting::AutoTuneForSong),
		String(Setting::AutoTuneForSoftwareSemitoneSettings, "Software Semitone Settings")
			.Ini("Toggle Switches", "AutoTuneForSoftwareSemitoneSettings")
			.WithVisibleWhen(Setting::AutoTuneForSong),
		String(Setting::AutoTuneForSoftwareSemitoneTriggers, "Software Semitone Triggers")
			.Ini("Toggle Switches", "AutoTuneForSoftwareSemitoneTriggers")
			.WithVisibleWhen(Setting::AutoTuneForSong),
		String(Setting::AutoTuneForSoftwareTrueTuningSettings, "Software True Tuning Settings")
			.Ini("Toggle Switches", "AutoTuneForSoftwareTrueTuningSettings")
			.WithVisibleWhen(Setting::AutoTuneForSong),
		String(Setting::AutoTuneForSoftwareTrueTuningTriggers, "Software True Tuning Triggers")
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
	};
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
			Midi::tuningOffset += event.control ? -1 : 1;
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
}

// Two independent one-time setup steps: load the auto-tune pedal settings once AutoTuneForSong is on, 
// and spin up the MIDI-in listener once a MidiInDevice is configured.
void MidiMod::ScanForMidiDevices(ModContext& c) {
	if (!Midi::scannedForMidiDevices && c.IsOn(Setting::AutoTuneForSong)) {
		Midi::scannedForMidiDevices = true;
		Midi::ReadMidiSettingsFromINI(
			c.Value(Setting::ChordsMode),
			c.Int(Setting::TuningPedal),
			c.Value(Setting::AutoTuneForSongDevice),
			c.Value(Setting::MidiInDevice));
	}

	if (!Midi::attemptedToDetachMidiInThread && c.Value(Setting::MidiInDevice) != "") {
		Midi::attemptedToDetachMidiInThread = true;
		Midi::FindMidiInDevices(c.Value(Setting::MidiInDevice));
		std::thread(Midi::ListenToMidiInThread).detach();
	}
}

// Once we have left the song (and are not in its tuner), tell the pedal to drop the tuning we applied.
void MidiMod::RevertTuningWhenLeavingSong() {
	if ((Midi::alreadyAutomatedTuningInThisSong || Midi::alreadyAttemptedTuningInTuner) &&
		!GameState::Menus::IsInPreSongTuner()) {
		Midi::RevertAutomatedTuning();
		Midi::alreadyAttemptedTuningInTuner = false;
		Midi::userWantsToUseAutoTuning = false;
	}
}

// While the pre-song tuner is up, tune the pedal from the tuner's tuning readout (When == Tuner only).
void MidiMod::AutoTuneInTuner(ModContext& c) {
	if (GameState::Menus::IsInPreSongTuner() &&
		c.IsOn(Setting::AutoTuneForSong) &&
		c.When(Setting::AutoTuneForSongWhen) == When::Tuner &&
		!Midi::alreadyAttemptedTuningInTuner &&
		!Midi::alreadyAutomatedTuningInThisSong) {
		Midi::AttemptTuningInTuner();
		Midi::appliedTunerAutoTune = true; // Lets ExtendedRange skip its tuning-settle sleep.
	}
}

// In a song. Tune the pedal from the song's tuning the first time through, honouring the When setting.
void MidiMod::OnSongTick(ModContext& c) {
	AutoTuneInSong(c);
	PublishHud(c);
}

void MidiMod::OnMenuTick(ModContext& c) {
	// The user backed out of the tuner instead of starting a song.
	if (Midi::alreadyAttemptedTuningInTuner &&
		!GameState::Menus::IsInPreSongTuner()) {
		RevertTuning();
	}

	AutoTuneInTuner(c);
	PublishHud(c);
}

void MidiMod::OnDisabled(ModContext&) {
	RevertTuning();
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
	RevertTuning();
}

void MidiMod::AutoTuneInSong(ModContext& c) {
	if (c.IsOn(Setting::AutoTuneForSong) &&
		!Midi::alreadyAutomatedTuningInThisSong &&
		(c.When(Setting::AutoTuneForSongWhen) == When::Tuner ||
			(c.When(Setting::AutoTuneForSongWhen) == When::Manual && Midi::userWantsToUseAutoTuning))) {
		Midi::AutomateTuning();
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
			const bool isSelected = (Midi::SelectedMidiOutDevice == i);
			const auto& device = Midi::midiOutDevices[i];

			if (ImGui::Selectable(device.szPname, isSelected, ImGuiSelectableFlags_DontClosePopups)) {
				Midi::SelectedMidiOutDevice = i;
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
		Midi::SendDataToThread_PC(Midi::MidiPC);

	if (ImGui::Button("Send CC MIDI Message"))
		Midi::SendDataToThread_CC(Midi::MidiCC);
}

static Framework::ModRegistrar<MidiMod> _midiReg;
