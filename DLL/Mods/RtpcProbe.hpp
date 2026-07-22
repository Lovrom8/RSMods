#pragma once

// Drop pedal mod, discovery build.
//
// Emulated bass is the Wwise Harmonizer (plugin 138). Each voice stores its pitch
// as a frequency ratio 2^(cents / 1200) in the param object, and emulated bass sets
// voice one to 0.5, exactly an octave down. Writing that float on the objects
// returned by the create-param callback changed nothing audible, which points at
// Wwise cloning the ShareSet param object per effect instance: the running effect
// reads a clone we never saw. So this build also hooks Clone on the param vtable
// and retunes the clones.
//
// F9 lowers and F10 raises the tracked voices by 100 cents.
namespace RtpcProbe
{
	void InstallHooks();
	void Poll();
}
