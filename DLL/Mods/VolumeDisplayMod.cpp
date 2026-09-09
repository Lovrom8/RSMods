#include "../stdafx.h"
#include "VolumeDisplayMod.hpp"
#include "VolumeControl.hpp"

using Framework::ModContext;
using Framework::KeyEdge;
using Framework::Availability;
using Framework::KeyEvent;
namespace Setting = Settings::Setting;

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
