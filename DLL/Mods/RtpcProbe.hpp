#pragma once

// Spy hooks on the game's own Wwise setter functions. The RTPC name sweep proved
// none of the guessable pitch RTPC names exist, so instead of asking the engine,
// we watch what the game itself calls when the Emulated Bass tone (a -1200 cent
// Pitch Shifter instance) loads. Whatever fires there is the runtime pitch control.
namespace RtpcProbe
{
	void InstallHooks();
}
