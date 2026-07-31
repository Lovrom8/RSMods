#pragma once

#include "QualityOfLife.hpp"
#include "Mods/VolumeControl.hpp"
#include "Mods/AudioDevices.hpp"
#include "Audio/AsioHook.hpp"
#include "Mods/BugPrevention.hpp"
#include "Mods/LaunchOnExternalMonitor.hpp"
#include "Keyboard.hpp"
#include "Mods/Loft.hpp"
#include "Keybindings.hpp"
#include "Mods/ExtendedRangeMode.hpp"
#include "Mods/CustomSongTitles.hpp"
#include "D3DInfo.h"
#include <string>

struct GameLoopState {
    bool movedToExternalDisplay = false; // User wants to move the display to a specific location on boot.
    bool skipERSleep = false; // If using RR past 100%, remove the 1.5s sleep on ER mode, to stop flickering colors.
    bool forkInToasterNewProfile = false; // If Auto Load Profile has a specified profile, and we can't find it, then this will be true.
    bool automatedSongTimer = false; // If true, we will always show the song timer.
    bool loftOff = false; // Is the loft disabled right now? Toggles when loft turns off (True - No Loft, False - Loft)
    bool guitarSpeakPresent = false; // If true, read the notes inputted and press the key combo provided. (True - On, False - Off)
    std::string selectedUser;
};

namespace ModManager {
    inline const double DefaultNSPTimeLimit = 10.9899997711182; // The default time for NSP.

    void InitializeConfiguration();
    void InitializeMods(bool debug);
    void ApplyStartupMods();
    void HandlePostGameLoadedMods(GameLoopState& state);
    void UpdateGameLoadingState(GameLoopState& state);

    void EnableRiffRepeaterFeatures();
    void LogSongIDForRiffRepeater();
    void EnableRiffRepeaterFeatures();
    void HandleInSongVisualMods(GameLoopState& state);
    void HandleMidiAutoTuningInSong();
    void HandleSongTimerDisplay(GameLoopState& state);
    void HandleExtendedRangeInSong(const GameLoopState& state);
    void HandleAutoLoadProfile(GameLoopState& state);
    void HandleSpecificProfileLoad(GameLoopState& state);
    void HandleAlwaysOnMods(GameLoopState& state);

    // Runs on the fast hotkey thread. Samples the drop pedal keys and keeps the input
    // shifter's semitones in step, since the 250ms main loop is too slow to catch a tap.
    void PollDropPedalHotkeys();
    void HandleInMenuState(GameLoopState& state);
    void HandleInSongState(GameLoopState& state);
    void HandleTwoRTCBypassToggle();
    void HandleNonStopPlayTimer();
    void HandleLinearRiffRepeaterToggle();
    void HandleMidiDeviceScanning();
    void HandleRainbowEffects();
    void HandleExternalMonitor(GameLoopState& state);
    void HandleMenuVisualMods(GameLoopState& state);
    void HandleMenuFeatures(GameLoopState& state);
    void HandleHeadstockCacheReset(GameLoopState& state);
    void HandleExtendedRangeInTuner(const GameLoopState& state);
    void CleanupSongSpecificStates(GameLoopState& state);
    void ConfigureAlternativeSampleRate();
    void CheckIfGameHasLoaded();
    void ApplyAudioDeviceConfiguration();
    void HandleMicrophoneVolumeOverride();
    void HandleAudioBackgroundToggle();
    void HandleTwoRTCBypassToggle();
}