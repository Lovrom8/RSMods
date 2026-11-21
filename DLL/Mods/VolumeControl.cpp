#include "../stdafx.h"
#include "VolumeControl.hpp"
#include "../Framework/InstructionPatch.hpp"
#include "../LnpSites.hpp"
#include "../VersioningStruct.h"

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

	void LogOriginal(const char* name, std::uintptr_t address, const std::vector<std::uint8_t>& bytes) {
		std::ostringstream text;
		for (std::uint8_t byte : bytes)
			text << std::hex << static_cast<unsigned int>(byte) << ' ';
		LOG_INFO("(VOLUME CONTROL) " << name << " original at 0x" << std::hex << address << ": " << text.str() << std::endl);
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
			LOG_WARNING("(VOLUME CONTROL) " << action << " left unchanged at " << captured.site
				<< ": " << FailureName(captured.failure) << std::endl);
	}

	void LogUnavailable(const char* action, const char* site) {
		LOG_WARNING("(VOLUME CONTROL) " << action << " left unchanged at " << site << ": "
			<< FailureName(ValidationFailure::Unavailable) << std::endl);
	}

	void LogResult(const char* action, const CaptureResult& captured, MemUtil::PatchResult result,
		std::uintptr_t failureAddress, bool restoring) {
		const char* site = SiteNameAt(captured, failureAddress);
		if ((!restoring && result == MemUtil::PatchResult::Installed)
			|| (restoring && result == MemUtil::PatchResult::Original))
			LOG_INFO("(VOLUME CONTROL) " << action << " confirmed" << std::endl);
		else if (result == MemUtil::PatchResult::RollbackIncomplete) {
			LOG_ERROR("(VOLUME CONTROL) " << action << " outcome is indeterminate at "
				<< (site ? site : "patch group") << std::endl);
		}
		else {
			LOG_WARNING("(VOLUME CONTROL) " << action << " left unchanged at "
				<< (site ? site : "patch group") << ": write was not applied" << std::endl);
		}
	}

	std::unique_ptr<MemUtil::PreparedPatchSet> songPreviewPatch;
	CaptureResult songPreviewSites;
	std::unique_ptr<MemUtil::PreparedPatchSet> focusPatch;
	CaptureResult focusSites;

	SiteDescriptor<16> SongPreviewSite() {
		return MakeSite("song-preview event", Offsets::patch_SongPreviewWwiseEvent.GetValue(), VersionedPattern<16>{
			ExactPattern(MakeBytes(0x50, 0x6C, 0x61, 0x79, 0x5F, 0x25, 0x73, 0x5F,
				0x50, 0x72, 0x65, 0x76, 0x69, 0x65, 0x77, 0x00)),
			UnavailablePattern<16>()
		}, MakeBytes(0x50, 0x6C, 0x61, 0x79, 0x5F, 0x25, 0x73, 0x5F,
			0x49, 0x6E, 0x76, 0x61, 0x6C, 0x69, 0x64, 0x00));
	}

	SiteDescriptor<8> FocusSetterSite(const std::array<std::uint8_t, 8>& replacement) {
		const std::uintptr_t immediate = Offsets::ptr_WindowNotInFocusValue.GetValue();
		return MakeSite("focus-state setter", immediate ? immediate - 6 : 0, VersionedPattern<8>{
			ExactPattern(MakeBytes(0xC6, 0x05, 0x78, 0x1A, 0x25, 0x01, 0x00, 0xC3)),
			UnavailablePattern<8>()
		}, replacement);
	}

	SiteDescriptor<8> FocusVariableSite() {
		constexpr auto mask = MakeBytes(0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF);
		// The first byte is the live focus flag and changes while the game runs.
		return MakeSite("focus-state variable", Offsets::ptr_IsWindowInFocus.GetValue(), VersionedPattern<8>{
			MaskedPattern(MakeBytes(0x00, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00), mask),
			MaskedPattern(MakeBytes(0x01, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00), mask)
		}, MakeBytes(0x01, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00));
	}
}

namespace Setting = Settings::Setting;

/// <summary>
/// Increase Volume of Mixer's Backend
/// </summary>
/// <param name="amountToIncrease"> - How much should we increase by?</param>
/// <param name="mixerToIncrease"> - Name of Mixer Value</param>
void VolumeControl::IncreaseVolume(int amountToIncrease, std::string mixerToIncrease) {
	float volume = 0;
	RTPCValue_type type = RTPCValue_GameObject;

	// Fill Volume Variable With Current Volume
	Wwise::SoundEngine::Query::GetRTPCValue(mixerToIncrease.c_str(), AK_INVALID_GAME_OBJECT, &volume, &type); 

	// Increase the volume by the amountToIncrease.
	if (volume <= (100.0f - amountToIncrease))
		volume += amountToIncrease;
	else
		volume = 100.0f; // In case the volume is within the amountToIncrease we can't throw it over 100.
	
	// Set Volume
	Wwise::SoundEngine::SetRTPCValue(mixerToIncrease.c_str(), (float)volume, AK_INVALID_GAME_OBJECT, 0, AkCurveInterpolation_Linear);
	Wwise::SoundEngine::SetRTPCValue(mixerToIncrease.c_str(), (float)volume, 0x1234, 0, AkCurveInterpolation_Linear);

	LOG_INFO("Increase volume of " << mixerToIncrease << " by " << amountToIncrease << " with a new volume of " << volume << std::endl);
}

/// <summary>
/// Decrease Volume of Mixer's Backend
/// </summary>
/// <param name="amountToDecrease"> - How much show we decrease by?</param>
/// <param name="mixerToDecrease"> - Name of Mixer Value</param>
void VolumeControl::DecreaseVolume(int amountToDecrease, std::string mixerToDecrease) {
	float volume = 0;
	RTPCValue_type type = RTPCValue_GameObject;

	// Fill Volume Variable With Current Volume
	Wwise::SoundEngine::Query::GetRTPCValue(mixerToDecrease.c_str(), AK_INVALID_GAME_OBJECT, &volume, &type); 

	// Decrease the volume by the amountToDecrease.
	if (volume >= (0.0f + amountToDecrease))
		volume -= amountToDecrease;
	else
		volume = 0.0f; // In case the volume is within the amountToDecrease we can't throw it below 0.

	// Set Volume
	Wwise::SoundEngine::SetRTPCValue(mixerToDecrease.c_str(), (float)volume, AK_INVALID_GAME_OBJECT, 0, AkCurveInterpolation_Linear);
	Wwise::SoundEngine::SetRTPCValue(mixerToDecrease.c_str(), (float)volume, 0x1234, 0, AkCurveInterpolation_Linear);
	
	LOG_INFO("Decrease volume of " << mixerToDecrease << " by " << amountToDecrease << " with a new volume of " << volume << std::endl);
}

/// <summary>
/// Reads the live Wwise volume (0-100) for a mixer channel.
/// </summary>
/// <param name="mixer"> - Name of the Mixer Value (see Settings::Setting::Channel).</param>
float VolumeControl::CurrentVolume(const char* mixer) {
	float volume = 0;
	RTPCValue_type type = RTPCValue_GameObject;

	Wwise::SoundEngine::Query::GetRTPCValue(mixer, AK_INVALID_GAME_OBJECT, &volume, &type);
	return volume;
}

/// <summary>
/// Disables the song previews when hovering over a song.
/// </summary>
void VolumeControl::DisableSongPreviewAudio() {
	if (!disabledSongPreviewAudio) {
		if (songPreviewPatch) {
			const auto result = MemUtil::ToPatchResult(songPreviewPatch->Outcome());
			if (result == MemUtil::PatchResult::Installed)
				disabledSongPreviewAudio = true;
			LogResult("Song previews disabled", songPreviewSites, result, songPreviewPatch->FailureAddress(), false);
			return;
		}
		auto captured = CaptureSites(readSite, SongPreviewSite());
		if (captured.failure != ValidationFailure::None) {
			LogValidationFailure("Song previews disabled", captured);
			return;
		}
		auto patches = std::make_unique<MemUtil::PreparedPatchSet>();
		if (!AppendSites(*patches, captured)) {
			LogResult("Song previews disabled", captured, MemUtil::PatchResult::Original, patches->FailureAddress(), false);
			return;
		}
		for (const CapturedSite& site : captured.sites)
			LogOriginal(site.name, site.address, site.original);
		const auto result = patches->Commit();
		if (result == MemUtil::PatchResult::Installed)
			disabledSongPreviewAudio = true;
		if (result != MemUtil::PatchResult::Original) {
			songPreviewSites = std::move(captured);
			songPreviewPatch = std::move(patches);
		}
		LogResult("Song previews disabled", songPreviewSites.sites.empty() ? captured : songPreviewSites,
			result, patches ? patches->FailureAddress() : songPreviewPatch->FailureAddress(), false);
	}
	else {
		LOG_WARNING("Tried to disable song previews when they are already disabled!" << std::endl);
	}
}

void VolumeControl::MutePlayer(bool player2) {
	RTPCValue_type type = RTPCValue_GameObject;

	const char* mixer = player2 ? Setting::Channel::Player2 : Setting::Channel::Player1;

	// Save current volume.
	if (player2)
	{
		Wwise::SoundEngine::Query::GetRTPCValue(mixer, AK_INVALID_GAME_OBJECT, &player2VolumeBeforeMute, &type);
		player2Muted = true;
	}
	else {
		Wwise::SoundEngine::Query::GetRTPCValue(mixer, AK_INVALID_GAME_OBJECT, &player1VolumeBeforeMute, &type);
		player1Muted = true;
	}

	// Mute (set volume to 0).
	Wwise::SoundEngine::SetRTPCValue(mixer, 0.f, AK_INVALID_GAME_OBJECT, 0, AkCurveInterpolation_Linear);
	Wwise::SoundEngine::SetRTPCValue(mixer, 0.f, 0x1234, 0, AkCurveInterpolation_Linear);

	LOG_INFO("Muted " << mixer << std::endl);
}

void VolumeControl::UnmutePlayer(bool player2)
{
	const char* mixer = player2 ? Setting::Channel::Player2 : Setting::Channel::Player1;

	// Unmute
	if (player2)
	{
		Wwise::SoundEngine::SetRTPCValue(mixer, player2VolumeBeforeMute, AK_INVALID_GAME_OBJECT, 0, AkCurveInterpolation_Linear);
		Wwise::SoundEngine::SetRTPCValue(mixer, player2VolumeBeforeMute, 0x1234, 0, AkCurveInterpolation_Linear);
		player2Muted = false;
	}
	else {
		Wwise::SoundEngine::SetRTPCValue(mixer, player1VolumeBeforeMute, AK_INVALID_GAME_OBJECT, 0, AkCurveInterpolation_Linear);
		Wwise::SoundEngine::SetRTPCValue(mixer, player1VolumeBeforeMute, 0x1234, 0, AkCurveInterpolation_Linear);
		player1Muted = false;
	}

	LOG_INFO("Unmuted " << mixer << std::endl);
}

/// <summary>
/// Enables the song previews when hovering over a song.
/// </summary>
void VolumeControl::EnableSongPreviewAudio() {
	if (songPreviewPatch) {
		for (const CapturedSite& site : songPreviewSites.sites)
			LogOriginal(site.name, site.address, site.original);
		const auto result = songPreviewPatch->Rollback();
		if (result == MemUtil::PatchResult::Original) {
			disabledSongPreviewAudio = false;
			songPreviewPatch.reset();
			songPreviewSites = {};
		}
		LogResult("Song previews restored", songPreviewSites, result,
			songPreviewPatch ? songPreviewPatch->FailureAddress() : 0, true);
	}
	else {
		LOG_WARNING("Tried to enable song previews when they are already enabled!" << std::endl);
	}
}

/// <summary>
/// Allows the user to play music in game while Alt+Tabbed.
/// </summary>
void VolumeControl::AllowAltTabbingWithAudio() {
	if (focusPatch)
		return;
	const std::uint32_t focusAddress = static_cast<std::uint32_t>(Offsets::ptr_IsWindowInFocus.GetValue());
	const auto setterReplacement = MakeBytes(0xC6, 0x05,
		focusAddress & 0xFF, (focusAddress >> 8) & 0xFF, (focusAddress >> 16) & 0xFF, (focusAddress >> 24) & 0xFF,
		0x01, 0xC3);
	const auto setter = FocusSetterSite(setterReplacement);
	const auto variable = FocusVariableSite();
	auto captured = CaptureSites(readSite, setter, variable);
	if (captured.failure != ValidationFailure::None) {
		LogValidationFailure("Background audio", captured);
		return;
	}
	auto patches = std::make_unique<MemUtil::PreparedPatchSet>();
	if (!AppendSites(*patches, captured)) {
		LogResult("Background audio", captured, MemUtil::PatchResult::Original, patches->FailureAddress(), false);
		return;
	}
	for (const CapturedSite& site : captured.sites)
		LogOriginal(site.name, site.address, site.original);
	const auto result = patches->Commit();
	if (result == MemUtil::PatchResult::Installed) {
		focusSetterPatched = true;
		allowedAltTabbingWithAudio = true;
	}
	if (result != MemUtil::PatchResult::Original) {
		focusSites = std::move(captured);
		focusPatch = std::move(patches);
	}
	LogResult("Background audio", focusSites.sites.empty() ? captured : focusSites, result,
		focusPatch ? focusPatch->FailureAddress() : patches->FailureAddress(), false);
}


/// <summary>
/// Pauses the audio when the user Alt+Tabs. This is the standard behavior of Rocksmith 2014.
/// </summary>
void VolumeControl::DisableAltTabbingWithAudio() {
	if (!focusPatch)
		return;
	for (const CapturedSite& site : focusSites.sites)
		LogOriginal(site.name, site.address, site.original);
	const auto result = focusPatch->Rollback();
	if (result == MemUtil::PatchResult::Original) {
		focusSetterPatched = false;
		allowedAltTabbingWithAudio = false;
		focusPatch.reset();
		focusSites = {};
	}
	LogResult("Background audio restore", focusSites, result,
		focusPatch ? focusPatch->FailureAddress() : 0, true);
}
