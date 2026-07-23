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
	void InstallHooks();
	void Poll();

	bool IsEnabled();
	int GetTargetSemitones();
	std::string GetTuningName();
}
