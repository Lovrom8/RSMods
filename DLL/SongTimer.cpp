#include "stdafx.h"
#include "SongTimer.hpp"

#include <chrono>
#include <format>

/// <summary>
/// Convert time stored as a float of seconds, to h:m:s
/// </summary>
/// <param name="timeInSeconds"> - Float containing number of seconds elapsed.</param>
/// <returns>std::string of time in "h:m:s" format.</returns>
std::string SongTimer::FormatTime(float timeInSeconds)
{
	using namespace std::chrono;

	const auto dur = duration_cast<seconds>(duration<float>(timeInSeconds));
	const auto h = duration_cast<hours>(dur);
	const auto m = duration_cast<minutes>(dur % hours(1));
	const auto s = duration_cast<seconds>(dur % minutes(1));

	if (h.count() > 0)
	{
		return std::format("{:02}h:{:02}m:{:02}s", h.count(), m.count(), s.count());
	}

	return std::format("{:02}m:{:02}s", m.count(), s.count());
}

float SongTimer::SongTimer() {
	if (GameState::Menus::IsInPreSongTuner()) {
		return 0.f;
	}

	uintptr_t addrTimerBase = MemUtil::FindDMAAddy(Offsets::baseHandle + Offsets::ptr_timer, Offsets::ptr_timerBaseOffsets);
	uintptr_t addrTimerRare = MemUtil::FindDMAAddy(Offsets::baseHandle + Offsets::ptr_timerRare, Offsets::ptr_timerRareOffsets, true);

	if (!addrTimerBase) {
		LOG_ERROR("Invalid Pointer: (BASE) ShowSongTimer" << std::endl);
		return 0.f;
	}

	// At this point, we can verify that the timer is a valid time.
	if (!addrTimerRare) {
		LOG_ERROR("Invalid Pointer: (RARE) ShowSongTimer" << std::endl);
		return *(float*)addrTimerBase;
	}

	// We entered a song where the base timer does not work.
	// Cause for this is unknown but we need to check, or time based mods (looping, song timer) will break.
	// Ex: Desolate Motion, or Rocksmith 2012 Theme.
	if (GameState::Menus::IsInSongModes() && *(float*)addrTimerBase == 0.f && *(float*)addrTimerRare != 0.f) {
		return *(float*)addrTimerRare;
	}
	else {
		return *(float*)addrTimerBase; 	// This is the default case, and will be used 99.99% of the time.
	}
}

/// <summary>
/// When the user passes a note in the pause menu, notes become grey. This gets the time at which notes go from being grey (deactivated) to being colored (activated).
/// </summary>
/// <returns>Time where all notes before it are grey / deactivated.</returns>
float SongTimer::GetGreyNoteTimer() {
	uintptr_t greyNoteTimer = MemUtil::FindDMAAddy(Offsets::baseHandle + Offsets::ptr_greyOutNoteTimer, Offsets::ptr_greyOutNoteTimerOffsets);

	if (!greyNoteTimer) {
		LOG_ERROR("Invalid Pointer: GetGreyNoteTimer = NULL" << std::endl);
		return NULL;
	}

	return *(float*)greyNoteTimer;
}

/// <summary>
/// Sets the time at which all notes before it turn grey (deactivated) and all notes after it are colorful (activated)
/// </summary>
/// <param name="timeInSeconds"> - Time to set the "deactivate before" at.</param>
void SongTimer::SetGreyNoteTimer(float timeInSeconds) {
	uintptr_t greyNoteTimer = MemUtil::FindDMAAddy(Offsets::baseHandle + Offsets::ptr_greyOutNoteTimer, Offsets::ptr_greyOutNoteTimerOffsets);

	if (!greyNoteTimer) {
		LOG_ERROR("Invalid Pointer: SetGreyNoteTimer = NULL" << std::endl);
		return;
	}

	*(float*)greyNoteTimer = timeInSeconds;
}

/// <summary>
/// Gets how long the countdown will last for from start to end, between songs, in NSP.
/// </summary>
/// <returns>The amount of time it takes to go from song 1 to song 2.</returns>
double SongTimer::GetNonStopPlayTimer()
{
	return *(double*)Offsets::ptr_NonStopPlayPreSongTimer.Get();
}

/// <summary>
/// Sets the amount of time between song 1 and song 2 in NSP.
/// </summary>
/// <param name="NewTimer"> - New time to set</param>
void SongTimer::SetNonStopPlayTimer(double NewTimer)
{
	MemUtil::SetStaticValue(Offsets::ptr_NonStopPlayPreSongTimer, NewTimer, sizeof(NewTimer));
}
