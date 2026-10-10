#pragma once

#include "../Framework/Framework.hpp"

// Drives the MIDI auto-tuning cluster: scans for MIDI devices, auto-tunes the pedal in the pre-song tuner and
// again once the song starts, and reverts the pedal when leaving a song. The Midi namespace helper does the
// real work; this mod owns the per-tick scheduling that used to live in ModManager's game loop.
class MidiMod : public Framework::IMod {
public:
	MOD_ID(MidiMod)

	bool IsEnabled(const Framework::ModContext& c) const override; // AutoTuneForSong

	// Reserves pedal tuning at the default priority 0. Another mod that retunes the guitar (DropPedal) claims the
	// same resource; when both are on, the higher priority runs and the other is suppressed.
	std::vector<std::string_view> ClaimsExclusive() const override;
	Framework::SettingDefs Settings() const override;

	void OnInitialize(Framework::ModContext& c) override;
	void OnTick(Framework::ModContext& c) override;     // Device scanning (post-load only).
	void OnMenuTick(Framework::ModContext& c) override; // Pedal revert + tuner auto-tune.
	void OnSongTick(Framework::ModContext& c) override; // In-song auto-tune.
	void OnDisabled(Framework::ModContext& c) override;
	void OnSongExit(Framework::ModContext& c) override;
	void OnSettingsChanged(Framework::ModContext& c) override;

private:
	void ScanForMidiDevices(Framework::ModContext& c);
	void RevertTuningWhenLeavingSong();
	void AutoTuneInTuner(Framework::ModContext& c);
	void AutoTuneInSong(Framework::ModContext& c);
	void RevertTuning();
	void PublishHud(Framework::ModContext& c);
	void DrawMenu();
};
