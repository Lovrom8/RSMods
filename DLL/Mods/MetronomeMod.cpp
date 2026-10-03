#include "../stdafx.h"
#include "MetronomeMod.hpp"

#include "../SongTimer.hpp"
#include "MetronomeAudioHook.hpp"
#include "MetronomeWav.hpp"

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

	constexpr int kFullVolume = 100;
	constexpr int kMaxVolume = 300;
	constexpr int kMaxOffsetMs = 500;

	constexpr char kIndicatorId[] = "metronome";
	constexpr int kIndicatorOrder = 100; // Below the volume popup and mixer, which share the top-left anchor.
	constexpr int kClicksOnColor = static_cast<int>(0xFFFFFFFF);
	constexpr int kClicksOffColor = static_cast<int>(0xFF9E9E9E);

	// In a song the indicator only appears briefly after a toggle, to keep the highway clear.
	constexpr auto kSongIndicatorDuration = 3s;

	SettingDef Volume(std::string_view key, std::string_view label) {
		return SettingDef::Numeric(key, label)
			.Range(0, kMaxVolume)
			.Default(std::to_string(kFullVolume))
			.WithVisibleWhen(kEnabled);
	}

	SettingDef SoundFile(std::string_view key, std::string_view label) {
		return SettingDef::String(key, label)
			.Ini("Mod Settings", key)
			.Category("Mod Settings")
			.Hint("Path to a WAV file. Leave empty for the built-in click.")
			.WithVisibleWhen(kEnabled);
	}

	// Pasted paths often keep their quotes. Relative paths are relative to the game folder.
	std::filesystem::path SoundPath(std::string path) {
		std::erase(path, '"');
		const auto first = path.find_first_not_of(" 	");
		const auto last = path.find_last_not_of(" 	");
		if (first == std::string::npos) return {};

		std::filesystem::path soundPath = std::filesystem::path(path.substr(first, last - first + 1));
		if (soundPath.is_relative()) {
			wchar_t exePath[MAX_PATH] = {};
			GetModuleFileNameW(nullptr, exePath, MAX_PATH);
			soundPath = std::filesystem::path(exePath).parent_path() / soundPath;
		}
		return soundPath;
	}

	// Empty when the setting is empty or the file can't be used; either way the built-in click plays.
	std::optional<Metronome::MonoSound> LoadCustomSound(const std::string& setting, std::string_view which) {
		const std::filesystem::path path = SoundPath(setting);
		if (path.empty()) return std::nullopt;

		auto loaded = Metronome::LoadWav(path);
		if (auto* error = std::get_if<std::string>(&loaded)) {
			LOG_ERROR("(Metronome) Couldn't use " << path.string() << " as the " << which << " sound (" << *error
				<< "); playing the built-in click" << std::endl);
			return std::nullopt;
		}

		const auto& sound = std::get<Metronome::MonoSound>(loaded);
		LOG_INFO("(Metronome) Using " << path.string() << " as the " << which << " sound (" << sound.samples.size()
			<< " samples at " << sound.sampleRate << " Hz)" << std::endl);
		return sound;
	}

	float VolumeToLevel(const ModContext& c, std::string_view key) {
		return static_cast<float>(c.Int(key)) / kFullVolume;
	}

}

SettingDefs MetronomeMod::Settings() const {
	return {
		SettingDef::Toggle(kEnabled, "Metronome")
			.Hint("Plays a click on every beat of the song, with a different sound on the first beat of each measure.\n"
				"Toggle the clicks in game with the Metronome key; the top left corner shows whether they're on.\n"
				"Turning this on takes effect after restarting the game.\n"
				"Untested on the Learn & Play edition."),
		Volume(kAccentVolume, "Metronome Accent Volume")
			.Hint("Volume of the click on the first beat of each measure. 100 is the normal level; up to 300 for loud songs."),
		Volume(kBeatVolume, "Metronome Beat Volume")
			.Hint("Volume of the click on every other beat. 100 is the normal level; up to 300 for loud songs."),
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

// The audio hook has to be in place before the game sets up its audio, which happens once, at startup.
void MetronomeMod::OnInitialize(ModContext& c) {
	ApplySettings(c);
	if (c.IsOn(kEnabled))
		Metronome::AudioHook::Install(clickMixer);

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
	ApplySounds(c);
}

// Only reloads when a path changes: settings changes of any kind land here.
void MetronomeMod::ApplySounds(const ModContext& c) {
	const std::string accentPath = c.Value(kAccentSound);
	const std::string beatPath = c.Value(kBeatSound);
	if (accentPath == loadedAccentPath && beatPath == loadedBeatPath && soundsLoaded) return;

	clickMixer.SetSounds(LoadCustomSound(accentPath, "accent"), LoadCustomSound(beatPath, "beat"));
	loadedAccentPath = accentPath;
	loadedBeatPath = beatPath;
	soundsLoaded = true;
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
	WarnIfAudioUnhooked();
}

void MetronomeMod::OnSongExit(ModContext&) {
	Metronome::AudioHook::ClearChartTime();
	beatMapSource.Forget();
	clickMixer.ClearBeats();
}

void MetronomeMod::OnMenuTick(ModContext& c) {
	Metronome::AudioHook::LogStatus();
	ShowIndicator(c);
}

void MetronomeMod::WarnIfAudioUnhooked() {
	if (Metronome::AudioHook::IsHooked() || warnedAudioUnhooked) return;

	LOG_WARNING("(Metronome) No music decoder hooked yet, so no clicks. If this persists, the game set up its audio "
		"before the metronome could hook it." << std::endl);
	warnedAudioUnhooked = true;
}

void MetronomeMod::RefreshBeats() {
	std::optional<Metronome::ChartBeats> chart = beatMapSource.PollChanges();
	if (!chart) return;

	std::ostringstream countIn;
	if (chart->countIn)
		countIn << ", silent through the chart's own count-in (" << chart->countIn->start << " s to " << chart->countIn->end << " s)";
	LOG_INFO("(Metronome) Loaded " << chart->beats.size() << " beats of " << GameState::GetSongKey() << countIn.str() << std::endl);
	clickMixer.SetBeats(std::move(chart->beats));
}

void MetronomeMod::OnSongTick(ModContext& c) {
	RefreshBeats();
	Metronome::AudioHook::SetChartTime(SongTimer::SongTimer());
	Metronome::AudioHook::LogStatus();

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
