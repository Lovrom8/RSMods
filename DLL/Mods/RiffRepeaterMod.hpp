#pragma once

#include "../Framework/Framework.hpp"
#include "RiffRepeater.hpp"
#include "../Keybindings.hpp"

// Consolidates the Riff Repeater features that used to live in ModManager:
//   - LinearRiffRepeater: patches the RR speed logic to be linear, kept in sync with the setting.
//   - RRSpeedAboveOneHundred: enables >100% time stretch in a song and drops it when leaving to a non-score menu.
// Also logs the current song id the first time it is seen so the >100% speed table is ready.
class RiffRepeaterMod : public Framework::IMod {
public:
	MOD_ID(RiffRepeaterMod)

	std::vector<std::string_view> ClaimsExclusive() const override { return { "song-speed" }; }

	void OnInitialize(Framework::ModContext& c) override;
	void OnTick(Framework::ModContext& c) override;     // Linear-speed patch sync (all phases, incl. startup/loading).
	void OnMenuTick(Framework::ModContext& c) override; // Drop >100% time stretch outside the score menus.
	void OnSongTick(Framework::ModContext& c) override; // Log the song id + apply >100% time stretch.

private:
	static void Rewind(const Framework::ModContext& c);
	static void SetLoopStart(const Framework::KeyEvent& event);
	static void SetLoopEnd(const Framework::KeyEvent& event);
	static void ChangeSpeed(const Framework::ModContext& c, const Framework::KeyEvent& event);
	void SyncLinearSpeeds(Framework::ModContext& c);
};
