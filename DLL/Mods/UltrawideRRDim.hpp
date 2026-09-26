#pragma once

// Keeps the venue lit in Riff Repeater on ultrawide displays.
//
// Riff Repeater pushes loft state 0x13, whose post-effects darken the venue. At 16:9 that
// darkening covers the whole screen; at ultrawide it blacks out the side extensions the
// correction has just revealed. Rather than reshape the render passes that apply it, this
// substitutes the post-effects state at the point where the game selects it: the CALL from
// LoftManager::SetLoftState into LoftPostEffectsManager::SetLoftState. While the correction
// is active and the requested state is 0x13, the thunk hands the manager its previous state
// instead, so the post-effects transition becomes "previous to previous" and nothing
// darkens. The original function still runs, so lighting, audio and the manager's own
// bookkeeping stay stock; only the state argument changes.
//
// The centre keeps its normal Riff Repeater look and the side extensions stay lit. At 16:9,
// or with the Ultrawide setting off, nothing changes because the gate follows
// UltrawideState::active.
//
// Install() validates the CALL at the site, the callee's prologue and its return, and
// refuses on any mismatch, so an unknown build is never patched.
namespace UltrawideRRDim {
	// Redirect the call once. Later calls are no-ops.
	void Install();

	// Restore the original call only while it still targets our thunk.
	void Uninstall();

	// Publish whether the thunk may substitute. False makes it a pass-through.
	void SetActive(bool active);
}
