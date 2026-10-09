#include "../stdafx.h"
#include "VolumeDisplayMod.hpp"
#include "VolumeControl.hpp"

using Framework::ModContext;
using Framework::KeyEdge;
using Framework::Availability;
using Framework::KeyEvent;
using Framework::SettingDefs;
using Framework::Toggle;
using Framework::Numeric;
namespace Setting = Settings::Setting;

namespace {
	Framework::SettingDef AudioKey(std::string_view key, std::string_view label, std::string_view defaultKey) {
		return Framework::KeyBind(key, label, defaultKey).Ini("Audio Keybindings", key).Category("Audio Keybindings");
	}
}

SettingDefs VolumeDisplayMod::Settings() const {
	return {
		Toggle(Setting::VolumeControlEnabled, "VolumeControl", "Volume Control")
			.Hint("Allows you to control how loud the game is using the in-game mixer without needing to open it.\nAlso includes a hidden \"Master Volume\" control.")
			.Heading(Framework::SettingHeading::Audio),
		Numeric(Setting::VolumeControlInterval, "Volume Control Interval")
			.Hint("How many volume steps each key press adjusts the volume by.")
			.Ini("Mod Settings", "VolumeControlInterval")
			.Default("5")
			.Range(1, 100)
			.WithVisibleWhen(Setting::VolumeControlEnabled),
		AudioKey(Setting::Key::MasterVolume, "Master Volume", "5"),
		AudioKey(Setting::Key::SongVolume, "Song Volume", "6"),
		AudioKey(Setting::Key::Player1Volume, "Player 1 Volume", "7"),
		AudioKey(Setting::Key::Player2Volume, "Player 2 Volume", "8"),
		AudioKey(Setting::Key::MicrophoneVolume, "Microphone Volume", "9"),
		AudioKey(Setting::Key::VoiceOverVolume, "Voice-Over Volume", "0"),
		AudioKey(Setting::Key::SFXVolume, "SFX Volume", "S"),
		AudioKey(Setting::Key::DisplayMixer, "Display Mixer", "P"),
		AudioKey(Setting::Key::MutePlayer1, "Mute / Unmute Player 1", "X"),
		AudioKey(Setting::Key::MutePlayer2, "Mute / Unmute Player 2", "C"),
		AudioKey(Setting::Key::ChangedSelectedVolume, "Change Selected Volume", "B"),
	};
}

void VolumeDisplayMod::OnInitialize(ModContext& c) {
	auto commands = c.Commands();
	const auto volumeEnabled = [](const ModContext& context, const KeyEvent&) { return context.IsOn(Setting::VolumeControlEnabled); };

	commands.BindSetting(Setting::Key::MutePlayer1, KeyEdge::Up,
		Availability::Active,
		[this](ModContext&, const KeyEvent&) { ToggleMute(false); }, {}, "Mute Player 1");

	commands.BindSetting(Setting::Key::MutePlayer2, KeyEdge::Up,
		Availability::Active,
		[this](ModContext&, const KeyEvent&) { ToggleMute(true); }, {}, "Mute Player 2");

	commands.BindSetting(Setting::Key::DisplayMixer, KeyEdge::Up,
		Availability::Active,
		[this](ModContext&, const KeyEvent&) { showMixer = false; });

	commands.BindSetting(Setting::Key::ChangedSelectedVolume, KeyEdge::Up,
		Availability::Active,
		[this](ModContext&, const KeyEvent&) {
			currentIndex = (currentIndex + 1) % static_cast<int>(channels.size());
		}, volumeEnabled);

	commands.BindSetting(Setting::Key::DisplayMixer, KeyEdge::Down,
		Availability::Active,
		[this](ModContext&, const KeyEvent&) { showMixer = true; },
		volumeEnabled, "Display Mixer");

	for (int index = 0; index < static_cast<int>(channels.size()); ++index) {
		commands.BindSetting(channels[index].key, KeyEdge::Down,
			Availability::Active,
			[this, index](ModContext& context, const KeyEvent& event) {
				ChangeVolume(context, event, index);
			}, volumeEnabled);
	}
}

void VolumeDisplayMod::ToggleMute(bool player2) {
	bool& muted = player2 ? VolumeControl::player2Muted : VolumeControl::player1Muted;

	if (muted) {
		VolumeControl::UnmutePlayer(player2);
	}
	else {
		VolumeControl::MutePlayer(player2);
	}

	RaisePopup(player2 ? 3 : 2); // Player 2 / Player 1 rows in `channels`.
}

void VolumeDisplayMod::ChangeVolume(const ModContext& c, const KeyEvent& event, int index) {
	const int interval = c.Int(Setting::VolumeControlInterval);
	const std::string channel(channels[index].channel);

	if (event.control) {
		VolumeControl::DecreaseVolume(interval, channel);
	}
	else {
		VolumeControl::IncreaseVolume(interval, channel);
	}

	RaisePopup(index);
}

void VolumeDisplayMod::RaisePopup(int index) {
	currentIndex = index;
	showPopup = true;
	popupRaised = std::chrono::steady_clock::now();
}

void VolumeDisplayMod::OnMenuTick(ModContext& c) {
	SyncPopup(c);
	SyncMixer(c);
}

void VolumeDisplayMod::OnSongTick(ModContext& c) {
	SyncPopup(c);
	SyncMixer(c);
}

// Published at order 0, above the mixer band.
void VolumeDisplayMod::SyncPopup(ModContext& c) {
	const bool enabled = c.IsOn(Setting::VolumeControlEnabled);

	if (enabled && showPopup && PopupExpired()) {
		showPopup = false;
	}

	Framework::HudText snapshot;
	snapshot.visible = enabled && showPopup && !showMixer; // the held mixer already lists every channel
	if (snapshot.visible) {
		snapshot.text = LineFor(currentIndex);
	}

	c.Hud().Set("current-volume", { Framework::HudAnchor::TopLeft, 0 }, std::move(snapshot));
}

void VolumeDisplayMod::SyncMixer(ModContext& c) {
	const bool visible = c.IsOn(Setting::VolumeControlEnabled) && showMixer;

	for (int index = 0; index < static_cast<int>(channels.size()); ++index) {
		Framework::HudText line;
		line.visible = visible;
		if (visible) {
			line.text = LineFor(index);
		}

		c.Hud().Set("mixer-" + std::to_string(index), { Framework::HudAnchor::TopLeft, 10 + index }, std::move(line));
	}
}

std::string VolumeDisplayMod::LineFor(int index) const {
	const int volume = static_cast<int>(VolumeControl::CurrentVolume(channels[index].channel));
	return std::string(channels[index].label) + std::to_string(volume) + "%";
}

bool VolumeDisplayMod::PopupExpired() const {
	return std::chrono::steady_clock::now() - popupRaised > std::chrono::seconds(3);
}

static Framework::ModRegistrar<VolumeDisplayMod> _volumeDisplayReg;
