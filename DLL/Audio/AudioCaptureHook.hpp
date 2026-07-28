#pragma once

#include "CaptureFormat.h"
#include "IInputProcessor.hpp"

// Intercepts the buffer Rocksmith reads guitar input from, so the signal can be altered
// before note detection and the tone chain ever see it. This is the v2 approach to drop
// tuning: DropPedal retunes a MultiPitch pedal already in the player's tone, which costs
// no latency but only covers tones that contain one. Pre-processing the input costs the
// shifter's algorithmic delay but covers every tone unconditionally.
//
// The interception point is the game-facing IAudioCaptureClient. RS_ASIO hands the game a
// fake WASAPI device backed by an ASIO driver, so hooking the interface the game holds
// covers the ASIO path and the plain WASAPI path with one hook.
//
// Reaching that interface means walking the chain the game builds:
// IMMDeviceEnumerator -> IMMDevice -> IAudioClient -> IAudioCaptureClient. Each step is a
// vtable patch, applied once per implementation class.
//
// Known gap: RS_ASIO patches the game's call site rather than the CoCreateInstance export,
// so it never reaches our detour and the chain starts at an enumerator we never see. The
// log this probe writes is what tells us which case we are in. See docs/input-pitch-shift.md.
namespace Audio::CaptureHook
{
	void Install();

	// Ownership stays with the caller, which must keep the processor alive for as long as
	// the game's audio engine runs.
	void SetProcessor(IInputProcessor* inputProcessor);

	// Which discovered capture client carries the guitar, in discovery order. The game
	// opens a microphone alongside one or two guitar inputs and nothing in the WASAPI
	// interface distinguishes them, so this is chosen deliberately rather than guessed.
	// A negative index disables processing.
	void SetCaptureClientIndex(int index);

	void SetProcessingEnabled(bool enabled);
	bool IsProcessingEnabled();

	int GetDiscoveredCaptureClientCount();
	const CaptureFormat& GetActiveFormat();
}
