#pragma once

// Lets the background asset loader finish more than one load per tick while the DLC scan runs.
//
// The asset loader's step function (stdcall, this on the stack, returns AL = made progress) walks the pending list and stops
// after the first entry that completes or starts, so the loader thread's tick finishes at most one asset per tick.
// After the DLC scan the game loads one manifest header per package through it (844 packages took 12 s, 3796 took ~30 s), and
// the main menu's tone load waits on that. The CALL in the loader thread's tick is redirected to a loop that calls it again while it reports
// progress, up to FastEnumerationAssetLoadsPerTick times (hidden in RSMods.ini [Fast Enumeration], default 64, 1 = stock).
//
// Only while the DLC scan drains: batching every background load changed the loft (the elevator cage sat down over a night lit
// loft at Path/Input), so something downstream doesn't like several responses landing in one frame.
//
// Based on ffio's private AssetLoadDrain.
namespace AssetLoadDrain {
	void Install();
	void SetActive(bool active);
}
