#include "RiffRepeater.hpp"

float Divisor = 10000.f;
float Max = 400.f;

/// <summary>
/// Gets the Time_Stretch RTPC.
/// </summary>
/// <param name="realSpeed"> - Should we return the real song speed? 400% = 4x</param>
/// <returns>Song Speed</returns>
float RiffRepeater::GetSpeed(bool realSpeed) {
	float currentTimeStretch = 0;
	RTPCValue_type type = RTPCValue_GameObject;

	WwiseVariables::Wwise_Sound_Query_GetRTPCValue_Char("Time_Stretch", 0x1234, &currentTimeStretch, &type);

	if (floorf(currentTimeStretch) == 100) // Rocksmith doesn't always set 100% speed to 100. This causes us to read 100 as 99.
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

	WwiseVariables::Wwise_Sound_SetRTPCValue_Char("Time_Stretch", newSpeed, 0x1234, 0, AkCurveInterpolation_Linear);
	WwiseVariables::Wwise_Sound_SetRTPCValue_Char("Time_Stretch", newSpeed, AK_INVALID_GAME_OBJECT, 0, AkCurveInterpolation_Linear);

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
	WwiseVariables::Wwise_Sound_SetActorMixerEffect(currentSongID, 2, AK_ID_Default_Time_Stretch);
	currentlyEnabled = true;
}

/// <summary>
/// Turns off the Actor-Mixer TimeStretch effect in slot 2.
/// </summary>
void RiffRepeater::DisableTimeStretch() {
	WwiseVariables::Wwise_Sound_SetActorMixerEffect(currentSongID, 2, AK_INVALID_UNIQUE_ID);
	SetSpeed(100); // Reset TimeStretch to default.
	currentlyEnabled = false;
}

/// <summary>
/// Gets the Actor-Mixer ID of the Play_{SongKey} so we can modify the Time_Stretch effect of it.
/// </summary>
/// <param name="songKey"> - SongKey for the current playing song.</param>
/// <returns>QueryAudioObjectIDs == AK_SUCCESS</returns>
bool RiffRepeater::LogSongID(std::string songKey) {
	std::string playEvent = "Play_" + songKey;

	AkUInt32 totalObjects = 0;

	WwiseVariables::Wwise_Sound_Query_QueryAudioObjectIDs_Char(playEvent.c_str(), &totalObjects, NULL); // Gets total number of objects so we know how much memory we need to allocate.

	void* memoryBlock = malloc(totalObjects * 0xC); // Allocate a memory block, 12 wide per object. Should only ever be 1 object, but just to be sure.

	AKRESULT soundObjects = WwiseVariables::Wwise_Sound_Query_QueryAudioObjectIDs_Char(playEvent.c_str(), &totalObjects, (AkObjectInfo*)memoryBlock); // Get the Actor-Mixer ID that we need to manipulate the Time_Stretch parameter.

	if (totalObjects > 0 && memoryBlock != NULL) {
		SongObjectIDs.insert({ playEvent, *(AkUInt32*)memoryBlock }); // Save the Play_{SongKey} event and the Actor-Mixer ID to a map so we don't need to get it multiple times if the user leaves and comes back to the song.
																	  // These values are static, PER SONG, so we could even make a database file (and/or csv) with these IDs in it to have an even bigger cache of them.
		currentSongID = *(AkUInt32*)memoryBlock; // The Actor-Mixer ID we need is at the very beginning of the memory block.
		loggedCurrentSongID = true;
	}

	free(memoryBlock); // Free the memory we allocated earlier in this function.

	return soundObjects == AK_Success;
}

void __declspec(naked) hook_timeStretchCalulations() {
	__asm {
		push EBP // Save EBP

		fld Divisor // Store 10000 in ST(0).
		fdiv ST(0), ST(1) // ST(0) = 10000. ST(1) = Riff Repeater Speed. 10000 / Speed, result in ST(0).

		fld Max	// Store 400 in ST(0). ST(1) = 10000 / Speed
		fxch // Switch ST(1) and ST(0). ST(0) = 10000 / Speed. ST(1) = 400

		pop EBP // Restore EBP

		jmp Offsets::ptr_timeStretchCalculationsJmpBck  // Skip to the validation checks.
	}
}

void RiffRepeater::FixSpeedPercents() {
	MemUtil::PlaceHook((void*)Offsets::ptr_timeStretchCalculations, hook_timeStretchCalulations, 6);
}

void RiffRepeater::ReverseSpeedPercents() {
	MemUtil::PatchAdr((LPVOID)Offsets::ptr_timeStretchCalculations, "\xDD\x05\xA0\x18\x22\x01", 6);
}
