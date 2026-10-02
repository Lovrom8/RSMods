#pragma once

#include <chrono>

#include "../Framework/Framework.hpp"
#include "MetronomeBeats.hpp"
#include "MetronomeClickMixer.hpp"

// Plays a click on every beat of the song's chart, accenting the first beat of each measure.
// The master setting enables the mod; the key toggles the clicks on and off while playing, and a
// top-left line shows which it is.
class MetronomeMod : public Framework::IMod {
public:
	MOD_ID(MetronomeMod)
	Framework::SettingDefs Settings() const override;
	bool IsEnabled(const Framework::ModContext& c) const override;

	void OnInitialize(Framework::ModContext& c) override;
	void OnSettingsChanged(Framework::ModContext& c) override;
	void OnSongEnter(Framework::ModContext& c) override;
	void OnSongExit(Framework::ModContext& c) override;
	void OnMenuTick(Framework::ModContext& c) override;
	void OnSongTick(Framework::ModContext& c) override;

private:
	Metronome::BeatMapSource beatMapSource;
	Metronome::ClickMixer clickMixer;
	std::chrono::steady_clock::time_point songIndicatorHideTime;
	std::chrono::steady_clock::time_point nextAudioReportTime;
	bool warnedAudioUnhooked = false;

	void ApplySettings(const Framework::ModContext& c);
	void RefreshBeats();
	void WarnIfAudioUnhooked();
	void ReportAudioActivity();
	void ToggleClicks();
	void ShowIndicator(Framework::ModContext& c) const;
	void HideIndicator(Framework::ModContext& c) const;
};
