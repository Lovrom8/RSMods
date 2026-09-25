#pragma once

/// <summary>
/// Samples where one thread is running (EIP, plus the return address at [EBP+4]) about every millisecond,
/// and writes a histogram to RSMods_profile_<label>.txt next to the game when stopped.
/// Works on game code: the report lists Rocksmith2014+0xRVA addresses.
/// Only runs if RSMods_profiling.txt exists next to the game. One session at a time: a Start while another session runs is ignored,
/// and Stop only ends the session with the same label, so overlapping users (profile load, DLC scan) can't end each other's session.
/// </summary>
namespace SamplingProfiler {
	bool IsEnabled();
	void Start(DWORD threadId, const char* label);
	void Stop(const char* label);
}
