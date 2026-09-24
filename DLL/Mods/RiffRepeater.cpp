#include "../stdafx.h"
#include "RiffRepeater.hpp"

const float Divisor = 10000.f;
const float Max = 400.f;

/// <summary>
/// Gets the Time_Stretch RTPC.
/// </summary>
/// <param name="realSpeed"> - Should we return the real song speed? 400% = 4x</param>
/// <returns>Song Speed</returns>
float RiffRepeater::GetSpeed(bool realSpeed) {
	float currentTimeStretch = 0;
	RTPCValue_type type = RTPCValue_GameObject;

	// Get the current Time Stretch Value
	Wwise::SoundEngine::Query::GetRTPCValue("Time_Stretch", 0x1234, &currentTimeStretch, &type);

	// Rocksmith doesn't always set 100% speed to 100. This causes us to read 100 as 99.
	if (floorf(currentTimeStretch) == 100) 
		currentTimeStretch = 100;

	return realSpeed ? ConvertSpeed(currentTimeStretch) : currentTimeStretch;
}

/// <summary>
/// Sets the Time_Stretch RTPC to the new speed.
/// </summary>
/// <param name="newSpeed"> - New Song Speed to set Time_Stretch to.</param>
/// <param name="isRealSpeed"> - Are we using a real song speed? 400% = 4x</param>
void RiffRepeater::SetSpeed(float newSpeed, bool isRealSpeed) {
	if (isRealSpeed)
		newSpeed = ConvertSpeed(newSpeed);

	// Set Time_Stretch to newSpeed.
	Wwise::SoundEngine::SetRTPCValue("Time_Stretch", newSpeed, 0x1234, 0, AkCurveInterpolation_Linear);
	Wwise::SoundEngine::SetRTPCValue("Time_Stretch", newSpeed, AK_INVALID_GAME_OBJECT, 0, AkCurveInterpolation_Linear);

	// Update realSongSpeed with the current speed.
	realSongSpeed = ConvertSpeed(newSpeed);
}

/// <summary>
/// Converts Real Speed and Wwise RTPC back and forth
/// </summary>
/// <param name="speed"> - Speed you want to convert</param>
/// <returns>Converted Speed</returns>
float RiffRepeater::ConvertSpeed(float speed) {
	return 10000 / speed;
}

/// <summary>
/// Turns on the Actor-Mixer TimeStretch effect in slot 2.
/// </summary>
void RiffRepeater::EnableTimeStretch() {
	Wwise::SoundEngine::SetActorMixerEffect(currentSongID, 2, AK_ID_Default_Time_Stretch);
	currentlyEnabled_Above100 = true;
}

/// <summary>
/// Turns off the Actor-Mixer TimeStretch effect in slot 2.
/// </summary>
void RiffRepeater::DisableTimeStretch() {
	Wwise::SoundEngine::SetActorMixerEffect(currentSongID, 2, AK_INVALID_UNIQUE_ID);
	SetSpeed(100); // Reset TimeStretch to default.
	currentlyEnabled_Above100 = false;
}

/// <summary>
/// Gets the Actor-Mixer ID of the Play_{SongKey} so we can modify the Time_Stretch effect of it.
/// </summary>
/// <param name="songKey"> - SongKey for the current playing song.</param>
/// <returns>QueryAudioObjectIDs == AK_SUCCESS</returns>
bool RiffRepeater::LogSongID(const std::string& songKey) {
	if (songKey.empty()) return false;

	std::string playEvent = std::string("Play_") + songKey;
	AkUInt32 totalObjects = 0;

	// Gets total number of objects so we know how much memory we need to allocate.
    AKRESULT resCount = Wwise::SoundEngine::Query::QueryAudioObjectIDs(playEvent.c_str(), &totalObjects, nullptr);
	if (resCount != AK_Success && resCount != AK_PartialSuccess) {
		loggedCurrentSongID = false;
		LOG_ERROR("QueryAudioObjectIDs count failed for event " + playEvent << std::endl);
		return false;
	}
	if (totalObjects == 0) {
		loggedCurrentSongID = false;
		LOG_INFO("No audio objects for event " + playEvent << std::endl);
		return false;
	}

	std::vector<AkObjectInfo> objects(totalObjects);
	AkUInt32 capacity = totalObjects;

	// Get the Actor-Mixer ID that we need to manipulate the Time_Stretch parameter.
	resCount = Wwise::SoundEngine::Query::QueryAudioObjectIDs(playEvent.c_str(), &capacity, objects.data());
	if (resCount != AK_Success && resCount != AK_PartialSuccess) {
		loggedCurrentSongID = false;
		LOG_ERROR("QueryAudioObjectIDs fill failed for event " + playEvent << std::endl);
		return false;
	}
	if (capacity == 0) {
		loggedCurrentSongID = false;
		LOG_INFO("Zero objects returned on fill for event " + playEvent << std::endl);
		return false;
	}

	if (!objects.empty()) {
		// Save the Play_{SongKey} event and the Actor-Mixer ID to a map so we don't need to get it multiple times if the user leaves and comes back to the song.
		// These values are static, PER SONG, so we could even make a database file (and/or csv) with these IDs in it to have an even bigger cache of them.
		SongObjectIDs.try_emplace(playEvent, objects[0].objID);
		
		// The Actor-Mixer ID we need is at the very beginning of the memory block.
		currentSongID = objects[0].objID;
		loggedCurrentSongID = true;
	}

	return true;
}


/// <summary>
/// x86 ASM hook for making Riff Repeater speeds linear. 68% on the slider -> 68% song speed.
/// </summary>
void __declspec(naked) hook_timeStretchCalculations() {
	__asm {
		push EBP // Save EBP to stack

		fld Divisor // Store 10000 in ST(0).
		fdiv ST(0), ST(1) // ST(0) = 10000. ST(1) = Riff Repeater Speed. 10000 / Speed, result in ST(0).

		fld Max	// Store 400 in ST(0). ST(1) = 10000 / Speed
		fxch // Switch ST(1) and ST(0). ST(0) = 10000 / Speed. ST(1) = 400

		pop EBP // Restore EBP from stack


		push offset Offsets::ptr_timeStretchCalculationsJmpBck
		jmp MemUtil::JumpToVersioned
	}
}

/// <summary>
/// Enables the fixes that make Riff Repeater speeds linear. 68% on the slider -> 68% song speed.
/// </summary>
void RiffRepeater::EnableLinearSpeeds() {
	currentlyEnabled_LinearRR = true;
	MemUtil::PlaceHook(Offsets::ptr_timeStretchCalculations, hook_timeStretchCalculations, 6);
}

/// <summary>
/// Reverts back to the default way to handle Riff Repeater speeds. 68% on the slider -> 50% song speed.
/// </summary>
void RiffRepeater::DisableLinearSpeeds() {
	currentlyEnabled_LinearRR = false;
	MemUtil::PatchAdr(Offsets::ptr_timeStretchCalculations, "\xDD\x05\xA0\x18\x22\x01", 6);
}

void RiffRepeater::HandleSongChange(const std::string& previousSongKey) {
	// If we have cached this event, then use the cached version, and tell the other threads to enable the Riff Repeater speed mod.
	if (auto it = SongObjectIDs.find("Play_" + previousSongKey); it != SongObjectIDs.end()) {
		currentSongID = it->second;
		readyToLogSongID = false;
		loggedCurrentSongID = true;
	}
	else { // We have not seen this event yet, so we need to log the internal ID for the song and cache it.
		loggedCurrentSongID = false;
		readyToLogSongID = true; // Wait until the user gets into the song so we can grab this ID.
	}
}

/// <summary>
/// If we have recently changed the Riff Repeater speed through the mod, then save the new value to a text file.
/// This is primarily for streamers who want to make a custom overlay with the song speed.
/// </summary>
void RiffRepeater::SaveSpeedToFileOnChange() {
	if (GameState::Menus::IsInModesWithAllowedFastRiffRepeater() && RiffRepeater::saveNewRRSpeedToFile) {
		auto rrText = std::ofstream("riff_repeater_speed.txt", std::ofstream::out);
		rrText << std::to_string((int)realSongSpeed) << std::endl;
		RiffRepeater::saveNewRRSpeedToFile = false;
	}
}