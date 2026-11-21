#include "../stdafx.h"
#include "../Framework/InstructionPatch.hpp"
#include "../LnpSites.hpp"
#include "../VersioningStruct.h"
#include "RiffRepeater.hpp"

#include <memory>

namespace {
	using namespace Framework::InstructionPatch;

	const auto readSite = [](std::uintptr_t address, auto& bytes) {
		return MemUtil::TryRead(address, bytes);
	};

	template <std::size_t Size>
	SiteDescriptor<Size> MakeSite(const char* name, std::uintptr_t address, const VersionedPattern<Size>& expected,
		const std::array<std::uint8_t, Size>& replacement,
		typename SiteDescriptor<Size>::ReplacementBuilder buildReplacement = nullptr) {
		return Framework::InstructionPatch::MakeSite(name, address, ::GameVersion::Current(),
			expected, replacement, buildReplacement);
	}

	template <std::size_t Size>
	BytePattern<Size> ExpandLnpPattern(const char* text) {
		std::vector<unsigned char> expanded;
		if (!LnpSites::Expand(text, static_cast<std::uint32_t>(GameVersion::ModuleBase()), expanded)
			|| expanded.size() != Size)
			return UnavailablePattern<Size>();
		std::array<std::uint8_t, Size> bytes{};
		std::copy(expanded.begin(), expanded.end(), bytes.begin());
		return ExactPattern(bytes);
	}

	void LogOriginal(const char* name, std::uintptr_t address, const std::vector<std::uint8_t>& bytes) {
		std::ostringstream text;
		for (std::uint8_t byte : bytes)
			text << std::hex << static_cast<unsigned int>(byte) << ' ';
		LOG_INFO("(RIFF REPEATER) " << name << " original at 0x" << std::hex << address << ": " << text.str() << std::endl);
	}

	bool AppendSites(MemUtil::PreparedPatchSet& patches, const CaptureResult& captured) {
		return AppendCaptured(captured, [&](const CapturedSite& site) {
			MemUtil::PreparedPatchRequest request{};
			request.address = reinterpret_cast<LPVOID>(site.address);
			request.kind = MemUtil::PreparedPatchKind::Bytes;
			request.expectedBytes = site.original;
			request.replacementBytes = site.replacement;
			request.length = site.original.size();
			return patches.Append(request);
		});
	}

	void LogValidationFailure(const char* action, const CaptureResult& captured) {
		if (captured.failure != ValidationFailure::None)
			LOG_WARNING("(RIFF REPEATER) " << action << " left unchanged at " << captured.site
				<< ": " << FailureName(captured.failure) << std::endl);
	}

	void LogUnavailable(const char* action, const char* site) {
		LOG_WARNING("(RIFF REPEATER) " << action << " left unchanged at " << site << ": "
			<< FailureName(ValidationFailure::Unavailable) << std::endl);
	}

	void LogResult(const char* action, const CaptureResult& captured, MemUtil::PatchResult result,
		std::uintptr_t failureAddress, bool restoring) {
		const char* site = SiteNameAt(captured, failureAddress);
		if ((!restoring && result == MemUtil::PatchResult::Installed)
			|| (restoring && result == MemUtil::PatchResult::Original))
			LOG_INFO("(RIFF REPEATER) " << action << " confirmed" << std::endl);
		else if (result == MemUtil::PatchResult::RollbackIncomplete) {
			LOG_ERROR("(RIFF REPEATER) " << action << " outcome is indeterminate at "
				<< (site ? site : "linear-speed site") << std::endl);
		}
		else {
			LOG_WARNING("(RIFF REPEATER) " << action << " left unchanged at "
				<< (site ? site : "linear-speed site") << ": write was not applied" << std::endl);
		}
	}

	using RiffInstruction = std::array<std::uint8_t, 6>;
	std::unique_ptr<MemUtil::PreparedPatchSet> riffPatch;
	CaptureResult riffSites;

	SiteDescriptor<6> RiffSite(const RiffInstruction& replacement) {
		return MakeSite("linear-speed instruction", Offsets::ptr_timeStretchCalculations.GetValue(), VersionedPattern<6>{
			ExactPattern(MakeBytes(0xDD, 0x05, 0xB8, 0x43, 0x22, 0x01)),
			ExpandLnpPattern<6>("DD 05 @E25C50")
		}, replacement);
	}
}

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

	// Rocksmith doesn't always set 100% speed to exactly 100 (can land slightly above or below).
	// 0.5 is well inside the smallest real step (101% = 99.01, 99% = 101.01).
	if (fabsf(currentTimeStretch - 100.f) < 0.5f)
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
		newSpeed = ConvertSpeed(std::clamp(newSpeed, MinRealSpeed, MaxRealSpeed));

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
/// Only touches Wwise when the effect isn't already attached to this song, since every SetActorMixerEffect call rebuilds the effect on the playing voice.
/// </summary>
void RiffRepeater::EnableTimeStretch() {
	if (currentlyEnabled_Above100 && timeStretchSongID == currentSongID)
		return;

	Wwise::SoundEngine::SetActorMixerEffect(currentSongID, 2, AK_ID_Default_Time_Stretch);
	timeStretchSongID = currentSongID;
	currentlyEnabled_Above100 = true;
}

/// <summary>
/// Turns off the Actor-Mixer TimeStretch effect in slot 2, without changing the song speed.
/// </summary>
void RiffRepeater::RemoveTimeStretch() {
	if (!currentlyEnabled_Above100)
		return;

	Wwise::SoundEngine::SetActorMixerEffect(timeStretchSongID, 2, AK_INVALID_UNIQUE_ID);
	currentlyEnabled_Above100 = false;
}

/// <summary>
/// Turns off the Actor-Mixer TimeStretch effect in slot 2, and resets the song speed to 100%.
/// </summary>
void RiffRepeater::DisableTimeStretch() {
	RemoveTimeStretch();
	SetSpeed(100); // Reset TimeStretch to default.
}

/// <summary>
/// Mirrors what the game does for Riff Repeater (RSAudioService::SetTimeStretchName):
/// the Time Stretch effect is only in the chain while the song isn't playing at 100%.
/// Even at 100% the effect still runs the audio through its phase vocoder, which colours the sound.
/// </summary>
void RiffRepeater::SyncTimeStretch() {
	if (GetSpeed() != 100.f)
		EnableTimeStretch();
	else
		RemoveTimeStretch();
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
	if (riffPatch) {
		const auto result = MemUtil::ToPatchResult(riffPatch->Outcome());
		if (result == MemUtil::PatchResult::Installed)
			currentlyEnabled_LinearRR = true;
		else
			LogResult("Linear-speed install", riffSites, result, riffPatch->FailureAddress(), false);
		return;
	}
	const auto address = Offsets::ptr_timeStretchCalculations.GetValue();
	RiffInstruction hookBytes{};
	if (!BuildRelativeJump(address, reinterpret_cast<std::uintptr_t>(&hook_timeStretchCalculations), hookBytes)) {
		LogUnavailable("Linear-speed install", "linear-speed instruction");
		return;
	}
	const auto site = RiffSite(hookBytes);
	auto captured = CaptureSites(readSite, site);
	if (captured.failure != ValidationFailure::None) {
		LogValidationFailure("Linear-speed install", captured);
		return;
	}
	auto patches = std::make_unique<MemUtil::PreparedPatchSet>();
	if (!AppendSites(*patches, captured)) {
		LogResult("Linear-speed install", captured, MemUtil::PatchResult::Original, patches->FailureAddress(), false);
		return;
	}
	for (const CapturedSite& capturedSite : captured.sites)
		LogOriginal(capturedSite.name, capturedSite.address, capturedSite.original);
	const auto result = patches->Commit();
	const auto failureAddress = patches->FailureAddress();
	if (result == MemUtil::PatchResult::Installed)
		currentlyEnabled_LinearRR = true;
	if (result == MemUtil::PatchResult::Original)
		LogResult("Linear-speed install", captured, result, failureAddress, false);
	else {
		riffSites = std::move(captured);
		riffPatch = std::move(patches);
		LogResult("Linear-speed install", riffSites, result, failureAddress, false);
	}
}

/// <summary>
/// Reverts back to the default way to handle Riff Repeater speeds. 68% on the slider -> 50% song speed.
/// </summary>
void RiffRepeater::DisableLinearSpeeds() {
	if (!riffPatch)
		return;
	for (const CapturedSite& capturedSite : riffSites.sites)
		LogOriginal(capturedSite.name, capturedSite.address, capturedSite.original);
	const auto result = riffPatch->Rollback();
	const auto failureAddress = riffPatch->FailureAddress();
	if (result == MemUtil::PatchResult::Original) {
		currentlyEnabled_LinearRR = false;
		LogResult("Linear-speed restore", riffSites, result, failureAddress, true);
		riffPatch.reset();
		riffSites = {};
	}
	else
		LogResult("Linear-speed restore", riffSites, result, failureAddress, true);
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
