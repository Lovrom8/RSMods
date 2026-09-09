#pragma once

#include <array>
#include <chrono>
#include <string>

#include "../Framework/Framework.hpp"

class VolumeDisplayMod : public Framework::IMod {
public:
	MOD_ID(VolumeDisplayMod)

	void OnInitialize(Framework::ModContext& c) override;
	void OnMenuTick(Framework::ModContext& c) override;
	void OnSongTick(Framework::ModContext& c) override;

private:
	struct Channel { const char* key; const char* channel; const char* label; };

	// The channels this mod adjusts and shows, in mixer order. The row index is the selected-channel index.
	static constexpr std::array<Channel, 7> channels = { {
		{ Settings::Setting::Key::MasterVolume,     Settings::Setting::Channel::Master,    "Master Volume: "     },
		{ Settings::Setting::Key::SongVolume,       Settings::Setting::Channel::Music,     "Song Volume: "       },
		{ Settings::Setting::Key::Player1Volume,    Settings::Setting::Channel::Player1,   "Player 1 Volume: "   }, // Guitar + Bass, P1
		{ Settings::Setting::Key::Player2Volume,    Settings::Setting::Channel::Player2,   "Player 2 Volume: "   }, // Guitar + Bass, P2
		{ Settings::Setting::Key::MicrophoneVolume, Settings::Setting::Channel::Mic,       "Microphone Volume: " },
		{ Settings::Setting::Key::VoiceOverVolume,  Settings::Setting::Channel::VoiceOver, "Voice-Over Volume: " }, // Rocksmith Dad VO
		{ Settings::Setting::Key::SFXVolume,        Settings::Setting::Channel::SFX,       "SFX Volume: "        },
	} };

	// View state, owned by the mod. The render thread sees nothing but the HudElements this mod publishes.
	int currentIndex = 0;                                // selected channel (index into `channels`)
	bool showMixer = false;                              // full mixer held via the DisplayMixer key
	bool showPopup = false;                              // transient popup after a volume change
	std::chrono::steady_clock::time_point popupRaised{}; // when the popup was last raised

	void ToggleMute(bool player2);
	void ChangeVolume(const Framework::ModContext& c, const Framework::KeyEvent& event, int index);
	void RaisePopup(int index);
	void SyncPopup(Framework::ModContext& c);
	void SyncMixer(Framework::ModContext& c);
	std::string LineFor(int index) const;
	bool PopupExpired() const;
};
