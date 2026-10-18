#pragma once

#include <chrono>
#include <optional>
#include <thread>

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
	void OnShutdown(Framework::ModContext& c) override;
	void OnSongExit(Framework::ModContext& c) override;
	void OnSettingsChanged(Framework::ModContext& c) override;

private:
	using Clock = std::chrono::steady_clock;

	// The game reports the song menu when its intro starts, before it sets the song's tuning, and the tuner menu
	// before the tuner has ticked. Both waits used to Sleep on MainThread, freezing keybinds and every other mod;
	// now they're deadlines checked on later ticks, like ExtendedRangeMod's settle wait.
	static constexpr std::chrono::milliseconds SongTuningSettleTime{ 1500 };
	static constexpr std::chrono::milliseconds TunerReadTime{ 2000 };

	// Set while a wait is pending; cleared once it runs or the menu or song it belongs to is left.
	std::optional<Clock::time_point> songTuneAt;
	std::optional<Clock::time_point> tunerReadUntil;

	// The MIDI IN listener: started once a MidiInDevice is configured, stopped when the mod leaves Active.
	std::jthread midiIn;
	bool midiInStarted = false; // One attempt per activation; a device that failed to open isn't retried every tick.

	void StopMidiIn();
	void CancelTunerRead();
	void ScanForMidiDevices(Framework::ModContext& c);
	void AutoTuneInTuner(Framework::ModContext& c);
	void AutoTuneInSong(Framework::ModContext& c);
	void RevertTuning();
	void PublishHud(Framework::ModContext& c);
	void DrawMenu();
};
