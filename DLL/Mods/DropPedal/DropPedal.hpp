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

	// Tracks the active arrangement's authored tuning reference through tuner and song.
	void HandleArrangementTuning();
	void ResetSongState();

	bool IsEnabled();
	int GetTargetSemitones();
	std::string GetTuningName();
	bool TryGetAuthoredTrueTuning(float& trueTuning);

	// Selects which engine realises the pitch. With the ASIO input shifter active, the
	// game-side MultiPitch driving is suppressed: the input itself is retuned, so audio
	// and detection hear the same shifted notes while Rocksmith keeps the arrangement's
	// authored tuning reference. The hotkeys and overlay stay live either way.
	void SetInputShifterActive(bool active);
	bool IsInputShifterActive();
	bool ConsumeInputShifterTransitionFailure();

	// Tick of the last engine decision or change, for the on-screen engine notice.
	// Zero until hooks are installed.
	unsigned long long GetEngineNoticeTick();

	// The tuning the guitar is physically in, and which way the shift is going, so the
	// overlay can name and colour the state without duplicating the arithmetic.
	int GetBaseTuningSemitones();
	std::string GetBaseTuningName();
	int GetShiftDirection();
}
