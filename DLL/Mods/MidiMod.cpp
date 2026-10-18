#include "../stdafx.h"
#include "MidiMod.hpp"
#include "Midi.hpp"

#include <atomic>

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

namespace {
	std::atomic<bool> gateActive = false, tunesInTuner = false;

	// The game's functions, through their Detours trampolines.
	uintptr_t hasTuningChanged = 0, hasCentsOffsetChanged = 0;

	constexpr double kOctaveDownCents = -1200.0;

	// The game's tuning objects: string0..string5 and CentOffset as numbers.
	struct JsonNode {
		struct VTable {
			void(__thiscall* addRef)(JsonNode*);
			void(__thiscall* release)(JsonNode*);
			void* unused[3];
			JsonNode* (__thiscall* child)(JsonNode*, const char* key); // +0x14
			void* unused2[14];
			JsonNode* (__thiscall* asNumber)(JsonNode*);                 // +0x50
		}* vtable;
	};

	struct GameTuning {
		int offsets[6] = {};
		double cents = 0.0;
	};

	JsonNode* ResolveTuning(unsigned player, bool previous) {
		JsonNode* tuning = nullptr;
		if (previous) {
			const uintptr_t address = Offsets::func_resolvePreviousTuning.GetValue();
			__asm {
				lea eax, tuning
				push eax
				mov eax, player
				call address
				add esp, 4
			}
		}
		else {
			const uintptr_t address = Offsets::func_resolveCurrentTuning.GetValue();
			__asm {
				push edi
				mov eax, player
				lea edi, tuning
				call address
				pop edi
			}
		}
		return tuning;
	}

	bool ReadNumber(JsonNode* tuning, const char* key, double* value) {
		JsonNode* child = tuning->vtable->child(tuning, key);
		JsonNode* number = child ? child->vtable->asNumber(child) : nullptr;
		if (!number)
			return false;
		*value = *reinterpret_cast<const double*>(reinterpret_cast<const char*>(number) + 8);
		return true;
	}

	// An A220 chart raises every string by 12 to sound the same as at A440, so it comes back as A440 offsets.
	bool ReadTuning(unsigned player, bool previous, GameTuning* out) {
		bool read = false;
		__try {
			JsonNode* tuning = ResolveTuning(player, previous);
			if (!tuning)
				return false;

			read = true;
			for (int i = 0; i < 6 && read; i++) {
				char key[] = "string0";
				key[6] = static_cast<char>('0' + i);
				double value = 0.0;
				read = ReadNumber(tuning, key, &value);
				out->offsets[i] = static_cast<int>(value);
			}
			if (!ReadNumber(tuning, "CentOffset", &out->cents))
				out->cents = 0.0;
			tuning->vtable->release(tuning);
		}
		__except (EXCEPTION_EXECUTE_HANDLER) {
			return false;
		}

		if (out->cents == kOctaveDownCents) {
			for (int& offset : out->offsets)
				offset -= 12;
		}
		return read;
	}

	bool IsOctaveReference(double cents) {
		return cents == 0.0 || cents == kOctaveDownCents;
	}

	bool PedalPlaysCurrentTuning(unsigned player, GameTuning* current) {
		return gateActive && tunesInTuner && player == 0 && ReadTuning(player, false, current) && Midi::PedalCoversTuning(current->offsets);
	}

	// Each is asked only once the game found a change since the player last tuned. When the pedal alone plays the new
	// tuning, the guitar itself hasn't changed. The game still requires the tuner after a reboot, for a capo, and for a
	// cent offset other than A220, which only moves the chart's numbers, not the pitches.
	bool __cdecl PedalPlaysTuningChange(unsigned player) {
		GameTuning current;
		if (!PedalPlaysCurrentTuning(player, &current))
			return false;

		LOG_INFO("(MIDI) No tuner needed: the pedal plays this tuning" << std::endl);
		return true;
	}

	bool __cdecl PedalPlaysReferenceChange(unsigned player) {
		GameTuning current, previous;
		if (!PedalPlaysCurrentTuning(player, &current) || !ReadTuning(player, true, &previous))
			return false;

		return IsOctaveReference(current.cents) && IsOctaveReference(previous.cents);
	}

	__declspec(naked) void HasTuningChanged() {
		__asm {
			push ecx
			call dword ptr [hasTuningChanged]
			pop ecx
			test al, al
			jz done
			push ecx
			call PedalPlaysTuningChange
			add esp, 4
			xor al, 1
		done:
			ret
		}
	}

	__declspec(naked) void HasCentsOffsetChanged() {
		__asm {
			push eax
			call dword ptr [hasCentsOffsetChanged]
			pop ecx
			test al, al
			jz done
			push ecx
			call PedalPlaysReferenceChange
			add esp, 4
			xor al, 1
		done:
			ret
		}
	}

	uintptr_t Hook(uintptr_t target, void* hook) {
		return reinterpret_cast<uintptr_t>(DetourFunction(reinterpret_cast<PBYTE>(target), reinterpret_cast<PBYTE>(hook)));
	}

	// Stays in place once installed; with the gate off it answers exactly as the game would.
	void InstallTunerGate() {
		static bool attempted = false;
		if (std::exchange(attempted, true))
			return;

		const uintptr_t tuningChanged = Offsets::func_hasTuningChanged.GetValue();
		const uintptr_t centsChanged = Offsets::func_hasCentsOffsetChanged.GetValue();
		const uintptr_t current = Offsets::func_resolveCurrentTuning.GetValue();
		const uintptr_t previous = Offsets::func_resolvePreviousTuning.GetValue();
		if (!tuningChanged || !centsChanged || !current || !previous) {
			LOG_WARNING("(MIDI) Tuner skip not installed: offsets not ported for this version" << std::endl);
			return;
		}

		static constexpr unsigned char tuningChangedPrologue[] = { 0x55, 0x8B, 0xEC, 0x83, 0xEC, 0x2C, 0xA1 };
		static constexpr unsigned char centsChangedPrologue[] = { 0x55, 0x8B, 0xEC, 0x83, 0xEC, 0x08, 0x56, 0x57 };
		static constexpr unsigned char currentPrologue[] = { 0x55, 0x8B, 0xEC, 0x83, 0xEC, 0x14, 0x56, 0x8B, 0xF0 };
		static constexpr unsigned char previousPrologue[] = { 0x55, 0x8B, 0xEC, 0x83, 0xEC, 0x10, 0x53, 0x56, 0x57, 0x8B, 0xF8 };
		if (!MemUtil::MatchesBytes(tuningChanged, tuningChangedPrologue) || !MemUtil::MatchesBytes(centsChanged, centsChangedPrologue) ||
			!MemUtil::MatchesBytes(current, currentPrologue) || !MemUtil::MatchesBytes(previous, previousPrologue)) {
			LOG_WARNING("(MIDI) Tuner skip not installed: unexpected bytes in the game's tuning checks" << std::endl);
			return;
		}

		hasTuningChanged = Hook(tuningChanged, &HasTuningChanged);
		hasCentsOffsetChanged = hasTuningChanged ? Hook(centsChanged, &HasCentsOffsetChanged) : 0;
		if (!hasCentsOffsetChanged) {
			LOG_ERROR("(MIDI) Tuner skip not installed: hooking the game's tuning checks failed" << std::endl);
			return;
		}

		LOG_INFO("(MIDI) Tuner skip installed" << std::endl);
	}
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
	tunesInTuner = c.When(Setting::AutoTuneForSongWhen) == When::Tuner;
}

// The pedal is tuned in the song when no tuner opens (AutoTuneInSong), so only When == Tuner can pass the tuner.
void MidiMod::OnEnabled(ModContext& c) {
	InstallTunerGate();
	tunesInTuner = c.When(Setting::AutoTuneForSongWhen) == When::Tuner;
	gateActive = true;
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

namespace {
	// Menus the game passes through between the pre-song tuner and the song (the calibration meter when it asks for one).
	// Leaving the tuner for these is starting the song, not backing out, so the tuned pedal stays as it is.
	bool IsSongLaunchTransition() {
		return GameState::currentMenu == "LearnASong_CalibrationMeter_PreGame" ||
			GameState::currentMenu == "NonStopPlay_CalibrationMeter_PreGame";
	}
}

void MidiMod::OnMenuTick(ModContext& c) {
	if (!GameState::Menus::IsInPreSongTuner())
		CancelTunerRead();

	// The user backed out of the tuner instead of starting a song.
	if (Midi::alreadyAttemptedTuningInTuner &&
		!GameState::Menus::IsInPreSongTuner() &&
		!IsSongLaunchTransition()) {
		RevertTuning();
	}

	AutoTuneInTuner(c);
	PublishHud(c);
}

void MidiMod::OnDisabled(ModContext&) {
	gateActive = false;
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
