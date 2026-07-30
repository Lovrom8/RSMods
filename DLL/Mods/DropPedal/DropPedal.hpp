#pragma once

// Retunes a pitch shifter pedal in the player's tone chain, so a guitar in
// standard tuning can be heard in the tuning a song was written for.
//
// The player adds a MultiPitch pedal to their tone in the Tone Designer. The game
// delivers that pedal's pitch through SetParam on the plugin's param object, in
// cents, re-sending it on every tone load and tone switch. Overriding that value
// is the whole mod: no tone files are modified, and a tone without a pitch pedal
// is left untouched.
//
// A pitch shifter moves the whole signal, so this covers uniform tunings such as
// Eb or D standard. It cannot produce drop tunings, where only one string differs.
//
// See docs/wwise-plugin-internals.md for how this was found and what was ruled out.
namespace DropPedal
{
	void LoadSettings();
	bool IsConfiguredEnabled();
	bool ShouldInstallInputHooks();
	bool RequiresInputShifter();
	void ReportInputShifterUnavailable();

	void InstallHooks();
	void Poll();

	// Key sampling, split from Poll so a fast thread can drive it. Poll runs on the mod's
	// 250ms loop, and a key tap is shorter than that gap, so sampling there drops presses.
	void PollHotkeys();

	// The pitch shifter is downstream of note detection, so detection has to be told
	// what the player's guitar is actually tuned to or every note reads wrong.
	void HandleTuningInSong();
	void ResetSongState();

	bool IsEnabled();
	int GetTargetSemitones();
	std::string GetTuningName();

	// Selects which engine realises the pitch. With the ASIO input shifter active, the
	// game-side MultiPitch driving and tuner reference changes are suppressed: the input
	// itself is retuned, so detection hears shifted notes against a 440 reference. The
	// hotkeys and overlay stay live either way; only the output stage switches.
	void SetInputShifterActive(bool active);
	bool IsInputShifterActive();

	// Tick of the last engine decision or change, for the on-screen engine notice.
	// Zero until hooks are installed.
	unsigned long long GetEngineNoticeTick();

	// The tuning the guitar is physically in, and which way the shift is going, so the
	// overlay can name and colour the state without duplicating the arithmetic.
	std::string GetBaseTuningName();
	int GetShiftDirection();
}
