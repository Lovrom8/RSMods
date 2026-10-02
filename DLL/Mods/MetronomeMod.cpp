#include "../stdafx.h"
#include "MetronomeMod.hpp"

using Framework::ModContext;
using Framework::SettingDef;
using Framework::SettingDefs;
using Framework::KeyEdge;
using Framework::Availability;
using Framework::KeyEvent;
using namespace std::chrono_literals;

namespace {
	// Every mod's settings share one key space, so keys carry the mod's name.
	constexpr char kEnabled[] = "MetronomeEnabled";
	constexpr char kAccentVolume[] = "MetronomeAccentVolume";
	constexpr char kBeatVolume[] = "MetronomeBeatVolume";
	constexpr char kAccentSound[] = "MetronomeAccentSound";
	constexpr char kBeatSound[] = "MetronomeBeatSound";
	constexpr char kOffset[] = "MetronomeOffsetMs";
	constexpr char kToggleKey[] = "MetronomeToggleKey";

	constexpr int kMaxVolume = 100;
	constexpr int kMaxOffsetMs = 500;

	constexpr char kIndicatorId[] = "metronome";
	constexpr int kIndicatorOrder = 100; // Below the volume popup and mixer, which share the top-left anchor.
	constexpr int kClicksOnColor = static_cast<int>(0xFF66DD66);
	constexpr int kClicksOffColor = static_cast<int>(0xFF9E9E9E);

	// In a song the indicator only appears briefly after a toggle, to keep the highway clear.
	constexpr auto kSongIndicatorDuration = 3s;

	SettingDef Volume(std::string_view key, std::string_view label) {
		return SettingDef::Numeric(key, label)
			.Range(0, kMaxVolume)
			.Default(std::to_string(kMaxVolume))
			.WithVisibleWhen(kEnabled);
	}

	SettingDef SoundFile(std::string_view key, std::string_view label) {
		return SettingDef::String(key, label)
			.Ini("Mod Settings", key)
			.Category("Mod Settings")
			.Hint("Path to a WAV file. Leave empty for the built-in click.")
			.WithVisibleWhen(kEnabled);
	}

	float VolumeToLevel(const ModContext& c, std::string_view key) {
		return static_cast<float>(c.Int(key)) / kMaxVolume;
	}
}

SettingDefs MetronomeMod::Settings() const {
	return {
		SettingDef::Toggle(kEnabled, "Metronome")
			.Hint("Plays a click on every beat of the song, with a different sound on the first beat of each measure.\n"
				"Toggle the clicks in game with the Metronome key; the top left corner shows whether they're on."),
		Volume(kAccentVolume, "Metronome Accent Volume")
			.Hint("Volume of the click on the first beat of each measure."),
		Volume(kBeatVolume, "Metronome Beat Volume")
			.Hint("Volume of the click on every other beat."),
		SoundFile(kAccentSound, "Metronome Accent Sound"),
		SoundFile(kBeatSound, "Metronome Beat Sound"),
		SettingDef::Numeric(kOffset, "Metronome Offset (ms)")
			.Range(-kMaxOffsetMs, kMaxOffsetMs)
			.Hint("Moves every click later (positive) or earlier (negative) if they don't line up with the song.")
			.WithVisibleWhen(kEnabled),
		Framework::KeyBind(kToggleKey, "Toggle Metronome", "M"),
	};
}

// Stays Active in menus too: the indicator is a HUD element, and those are cleared when a mod deactivates.
bool MetronomeMod::IsEnabled(const ModContext& c) const {
	return c.IsOn(kEnabled);
}

void MetronomeMod::OnInitialize(ModContext& c) {
	ApplySettings(c);

	c.Commands().BindSetting(
		kToggleKey,
		KeyEdge::Up,
		Availability::Active,
		[this](ModContext&, const KeyEvent&) { ToggleClicks(); },
		{},
		"Toggle Metronome");
}

void MetronomeMod::OnSettingsChanged(ModContext& c) {
	ApplySettings(c);
}

void MetronomeMod::ApplySettings(const ModContext& c) {
	clickMixer.SetLevels({ VolumeToLevel(c, kAccentVolume), VolumeToLevel(c, kBeatVolume) });
	clickMixer.SetOffset(std::chrono::milliseconds(c.Int(kOffset)));
	// TODO: Load the kAccentSound / kBeatSound WAVs (off the audio thread) and hand them to the mixer.
}

void MetronomeMod::ToggleClicks() {
	if (clickMixer.IsMuted())
		clickMixer.Unmute();
	else
		clickMixer.Mute();

	songIndicatorHideTime = std::chrono::steady_clock::now() + kSongIndicatorDuration;
}

// The chart can finish loading after the song starts, so OnSongTick keeps polling for it.
void MetronomeMod::OnSongEnter(ModContext&) {
	beatMapSource.Forget();
	RefreshBeats();
}

void MetronomeMod::OnSongExit(ModContext&) {
	beatMapSource.Forget();
	clickMixer.ClearBeats();
}

void MetronomeMod::OnMenuTick(ModContext& c) {
	ShowIndicator(c);
}

void MetronomeMod::RefreshBeats() {
	std::optional<Metronome::BeatMap> beats = beatMapSource.PollChanges();
	if (!beats) return;

	LOG_INFO("(Metronome) Loaded " << beats->size() << " beats of " << GameState::GetSongKey()
		<< ", " << beats->front().seconds << " s to " << beats->back().seconds << " s" << std::endl);
	clickMixer.SetBeats(std::move(*beats));
}

void MetronomeMod::OnSongTick(ModContext& c) {
	RefreshBeats();

	if (std::chrono::steady_clock::now() < songIndicatorHideTime)
		ShowIndicator(c);
	else
		HideIndicator(c);
}

void MetronomeMod::ShowIndicator(ModContext& c) const {
	const bool clicksOn = !clickMixer.IsMuted();

	Framework::HudText indicator;
	indicator.visible = true;
	indicator.text = clicksOn ? "Metronome: ON" : "Metronome: OFF";
	indicator.colorHex = clicksOn ? kClicksOnColor : kClicksOffColor;

	c.Hud().Set(kIndicatorId, { Framework::HudAnchor::TopLeft, kIndicatorOrder }, std::move(indicator));
}

void MetronomeMod::HideIndicator(ModContext& c) const {
	c.Hud().Set(kIndicatorId, { Framework::HudAnchor::TopLeft, kIndicatorOrder }, {});
}

static Framework::ModRegistrar<MetronomeMod> _metronomeModReg;
