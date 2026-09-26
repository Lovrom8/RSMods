#pragma once

/// <summary>
/// Samples where one thread is running (EIP, plus the return address at [EBP+4]) about every millisecond,
/// and writes a histogram to RSMods_profile_<label>.txt next to the game when stopped.
/// Works on game code: the report lists Rocksmith2014+0xRVA addresses.
/// Only runs if RSMods_profiling.txt exists next to the game. One session at a time.
/// </summary>
namespace SamplingProfiler {
	bool Enabled();
	void Start(DWORD threadId, const char* label);
	void Stop();
}
