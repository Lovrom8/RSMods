#include "../stdafx.h"
#include "VolumeControl.hpp"
#include "../SitePatchGroup.hpp"
#include "../VersioningStruct.h"

#include <memory>

namespace {
	using namespace Framework::InstructionPatch;
	using namespace SitePatch;

	// These groups are changed by mod lifecycle callbacks on MainThread.
	PatchedSiteGroup songPreviewPatch("VOLUME CONTROL");
	PatchedSiteGroup focusPatch("VOLUME CONTROL");

	SiteDescriptor<16> SongPreviewSite() {
		// "Play_%s_Preview" is the same 16 bytes on both builds, and the replacement is the same length.
		constexpr auto preview = MakeBytes(0x50, 0x6C, 0x61, 0x79, 0x5F, 0x25, 0x73, 0x5F,
			0x50, 0x72, 0x65, 0x76, 0x69, 0x65, 0x77, 0x00);
		return MakeSite("song-preview event", Offsets::patch_SongPreviewWwiseEvent.GetValue(), VersionedPattern<16>{
			ExactPattern(preview),
			ExactPattern(preview)
		}, MakeBytes(0x50, 0x6C, 0x61, 0x79, 0x5F, 0x25, 0x73, 0x5F,
			0x49, 0x6E, 0x76, 0x61, 0x6C, 0x69, 0x64, 0x00));
	}

	SiteDescriptor<8> FocusSetterSite(const std::array<std::uint8_t, 8>& replacement) {
		const std::uintptr_t immediate = Offsets::ptr_WindowNotInFocusValue.GetValue();
		return MakeSite("focus-state setter", immediate ? immediate - 6 : 0, VersionedPattern<8>{
			ExactPattern(MakeBytes(0xC6, 0x05, 0x78, 0x1A, 0x25, 0x01, 0x00, 0xC3)),
			ExpandLnpPattern<8>("C6 05 @E52A78 00 C3")
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

	SiteDescriptor<6> FocusLostPauseSite() {
		// Suppress only the focus-loss caller; cable errors use the same native event.
		return MakeSite("focus-loss pause call", Offsets::patch_FocusLostPauseCall.GetValue(), VersionedPattern<6>{
			ExactPattern(MakeBytes(0x56, 0xE8, 0x0F, 0x63, 0xE5, 0xFF)),
			ExactPattern(MakeBytes(0x56, 0xE8, 0x1F, 0x65, 0xE5, 0xFF))
		}, MakeBytes(0x90, 0x90, 0x90, 0x90, 0x90, 0x90));
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
MemUtil::PatchResult VolumeControl::DisableSongPreviewAudio() {
	if (disabledSongPreviewAudio)
		return MemUtil::PatchResult::Installed;

	const auto result = songPreviewPatch.Install("Song previews disabled", ReadSite, SongPreviewSite());
	if (result == MemUtil::PatchResult::Installed)
		disabledSongPreviewAudio = true;
	return result;
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
MemUtil::PatchResult VolumeControl::EnableSongPreviewAudio() {
	const auto result = songPreviewPatch.Restore("Song previews restored");
	if (result == MemUtil::PatchResult::Original)
		disabledSongPreviewAudio = false;
	return result;
}

/// <summary>
/// Allows the user to play music in game while Alt+Tabbed.
/// </summary>
MemUtil::PatchResult VolumeControl::AllowAltTabbingWithAudio() {
	const std::uint32_t focusAddress = static_cast<std::uint32_t>(Offsets::ptr_IsWindowInFocus.GetValue());
	const auto setterReplacement = MakeBytes(0xC6, 0x05,
		focusAddress & 0xFF, (focusAddress >> 8) & 0xFF, (focusAddress >> 16) & 0xFF, (focusAddress >> 24) & 0xFF,
		0x01, 0xC3);
	const auto setter = FocusSetterSite(setterReplacement);
	const auto variable = FocusVariableSite();
	const auto result = focusPatch.Install("Background audio", ReadSite, setter, variable, FocusLostPauseSite());
	if (result == MemUtil::PatchResult::Installed) {
		focusSetterPatched = true;
		allowedAltTabbingWithAudio = true;
	}
	return result;
}


/// <summary>
/// Pauses the audio when the user Alt+Tabs. This is the standard behavior of Rocksmith 2014.
/// </summary>
MemUtil::PatchResult VolumeControl::DisableAltTabbingWithAudio() {
	const auto result = focusPatch.Restore("Background audio restore");
	if (result == MemUtil::PatchResult::Original) {
		focusSetterPatched = false;
		allowedAltTabbingWithAudio = false;
	}
	return result;
}
