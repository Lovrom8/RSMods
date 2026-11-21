#include "../stdafx.h"
#include "VolumeControl.hpp"
#include "../Framework/InstructionPatch.hpp"
#include "../LnpSites.hpp"
#include "../VersioningStruct.h"

namespace {
	using namespace Framework::InstructionPatch;

	const auto readSite = [](std::uintptr_t address, auto& bytes) {
		return MemUtil::TryRead(address, bytes);
	};
	const auto writeSite = [](std::uintptr_t address, const auto& bytes) {
		return FromWriteResult(MemUtil::WritePatch(reinterpret_cast<LPVOID>(address), bytes.data(), bytes.size()),
			MemUtil::PatchResult::Installed, MemUtil::PatchResult::Original, MemUtil::PatchResult::RollbackIncomplete);
	};

	template <std::size_t Size>
	PatchedSite<Size> MakeSite(const char* name, std::uintptr_t address, const VersionedPattern<Size>& patterns,
		std::size_t writeOffset = 0, std::size_t writeSize = Size) {
		return { name, address, GameVersion::Current(), patterns, writeOffset, writeSize };
	}

	template <std::size_t Size>
	BytePattern<Size> ExpandLnpPattern(const char* text, const std::array<std::uint8_t, Size>& mask) {
		std::vector<unsigned char> expanded;
		if (!LnpSites::Expand(text, static_cast<std::uint32_t>(GameVersion::ModuleBase()), expanded)
			|| expanded.size() != Size)
			return UnavailablePattern<Size>();
		std::array<std::uint8_t, Size> bytes{};
		std::copy(expanded.begin(), expanded.end(), bytes.begin());
		return MaskedPattern(bytes, mask);
	}

	template <std::size_t Size>
	void LogOriginal(const char* name, std::uintptr_t address, const std::array<std::uint8_t, Size>& bytes) {
		std::ostringstream text;
		for (std::uint8_t byte : bytes)
			text << std::hex << static_cast<unsigned int>(byte) << ' ';
		LOG_INFO("(VOLUME CONTROL) " << name << " original at 0x" << std::hex << address << ": " << text.str() << std::endl);
	}

	const auto logOriginal = [](const char* name, std::uintptr_t address, const auto& bytes) {
		LogOriginal(name, address, bytes);
	};

	void LogResult(const char* action, const Result& result) {
		if (result.outcome == Outcome::Installed) {
			if (result.changed)
				LOG_INFO("(VOLUME CONTROL) " << action << " confirmed" << std::endl);
		}
		else if (result.outcome == Outcome::RollbackIncomplete) {
			LOG_ERROR("(VOLUME CONTROL) " << action << " outcome is indeterminate at "
				<< (result.site ? result.site : "patch group") << std::endl);
		}
		else if (result.failure != Failure::None) {
			LOG_WARNING("(VOLUME CONTROL) " << action << " left unchanged at "
				<< (result.site ? result.site : "patch group") << ": " << FailureName(result.failure) << std::endl);
		}
	}

	PatchedSite<16>& SongPreviewSite() {
		static auto site = MakeSite("song-preview event", Offsets::patch_SongPreviewWwiseEvent.GetValue(), VersionedPattern<16>{
			ExactPattern(MakeBytes(0x50, 0x6C, 0x61, 0x79, 0x5F, 0x25, 0x73, 0x5F,
				0x50, 0x72, 0x65, 0x76, 0x69, 0x65, 0x77, 0x00)),
			ExactPattern(MakeBytes(0x50, 0x6C, 0x61, 0x79, 0x5F, 0x25, 0x73, 0x5F,
				0x50, 0x72, 0x65, 0x76, 0x69, 0x65, 0x77, 0x00))
		});
		return site;
	}

	PatchedSite<8>& FocusSetterSite() {
		const std::uintptr_t immediate = Offsets::ptr_WindowNotInFocusValue.GetValue();
		constexpr auto mask = MakeBytes(0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF);
		static auto site = MakeSite("focus-state setter", immediate ? immediate - 6 : 0, VersionedPattern<8>{
			MaskedPattern(MakeBytes(0xC6, 0x05, 0x78, 0x1A, 0x25, 0x01, 0x00, 0xC3), mask),
			ExpandLnpPattern<8>("C6 05 @E52A78 00 C3", mask)
		}, 6, 1);
		return site;
	}

	PatchedSite<8>& FocusVariableSite() {
		constexpr auto mask = MakeBytes(0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF);
		static auto site = MakeSite("focus-state variable", Offsets::ptr_IsWindowInFocus.GetValue(), VersionedPattern<8>{
			MaskedPattern(MakeBytes(0x00, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00), mask),
			MaskedPattern(MakeBytes(0x01, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00), mask)
		}, 0, 1);
		return site;
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
		const auto result = SongPreviewSite().Install(readSite, writeSite,
			MakeBytes(0x50, 0x6C, 0x61, 0x79, 0x5F, 0x25, 0x73, 0x5F, 0x49, 0x6E, 0x76, 0x61, 0x6C, 0x69, 0x64, 0x00), logOriginal);
		if (result.outcome == Outcome::Installed)
			disabledSongPreviewAudio = true;
		LogResult("Song previews disabled", result);
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
	if (disabledSongPreviewAudio) {
		const auto result = SongPreviewSite().Restore(readSite, writeSite, logOriginal);
		if (result.outcome == Outcome::Installed)
			disabledSongPreviewAudio = false;
		LogResult("Song previews restored", result);
	}
	else {
		LOG_WARNING("Tried to enable song previews when they are already enabled!" << std::endl);
	}
}

/// <summary>
/// Allows the user to play music in game while Alt+Tabbed.
/// </summary>
void VolumeControl::AllowAltTabbingWithAudio() {
	auto& setter = FocusSetterSite();
	auto& variable = FocusVariableSite();
	const std::uint32_t focusAddress = static_cast<std::uint32_t>(Offsets::ptr_IsWindowInFocus.GetValue());
	const auto setterReplacement = MakeBytes(0xC6, 0x05,
		focusAddress & 0xFF, (focusAddress >> 8) & 0xFF, (focusAddress >> 16) & 0xFF, (focusAddress >> 24) & 0xFF,
		0x01, 0xC3);
	const auto variableReplacement = MakeBytes(0x01, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00);
	setter.SetReplacement(setterReplacement);
	variable.SetReplacement(variableReplacement);
	const auto result = InstallGroup(readSite, writeSite, logOriginal, setter, variable);
	if (result.outcome == Outcome::Installed) {
		focusSetterPatched = true;
		allowedAltTabbingWithAudio = true;
	}
	LogResult("Background audio", result);
}


/// <summary>
/// Pauses the audio when the user Alt+Tabs. This is the standard behavior of Rocksmith 2014.
/// </summary>
void VolumeControl::DisableAltTabbingWithAudio() {
	auto& setter = FocusSetterSite();
	auto& variable = FocusVariableSite();
	const auto result = RestoreGroup(readSite, writeSite, logOriginal, setter, variable);
	if (result.outcome == Outcome::Installed) {
		focusSetterPatched = false;
		allowedAltTabbingWithAudio = false;
	}
	LogResult("Background audio restore", result);
}
