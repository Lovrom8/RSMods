#pragma once

/// <summary>
/// Finds what the game is doing during FPS dips.
/// Samples the render thread about every millisecond and remembers which frame each sample landed in. Frames that take
/// much longer than the recent average count as dips, and their samples go into their own histogram.
/// Writes RSMods_profile_frames.txt next to the game every 10 seconds (Rocksmith2014+0xRVA addresses).
/// Only runs if RSMods_frame_profiling.txt exists next to the game.
/// </summary>
namespace FrameProfiler {
	/// Call once per EndScene from the render thread. Starts the sampler on the first call.
	void OnFrame();
}
