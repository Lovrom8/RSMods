# The Rocksmith Mods that nobody asked for

[![Build status](https://ci.appveyor.com/api/projects/status/github/Lovrom8/rsmods?svg=true)](https://ci.appveyor.com/project/Lovrom8/rsmods)

## IMPORTANT NOTE:
If you are trying to play on a Learn & Play build from Steam that was released on December 19th, 2024, you cannot use the 1.2.7.4 version of the mods without additional work. Please see the instructions on this github issue on how to get the mods to work: https://github.com/Lovrom8/RSMods/issues/196#issuecomment-2564077160.

Feel free to try the 1.2.8.0 version if you are on the Learn & Play version.

## Features:

* **Extended Range Mode**
  * Rocksmith by default doesn't officialy support 7 (or more) string guitars or 5 string bass, even though a few low-tuned songs have been released as official DLC. 
    Hence with the power of DX9, GDI+ and some smart reverse engineering, when it detects that a song is in a tuning lower than the set threshold, the color scheme    will change accordingly. What that means is that no more will your brain be confused because the lowest string is colored red (usually indicating the low E string), while you actually need to play the low B! So effectively all the strings will be shifted one place down.
   * **Options**
   1. ZZ's color set - this mode will replicate the default color of high B string (the teal / green used in colorblind mode) and make it match the color of the low B string (7-th string) - [See the ER mode in action](https://www.youtube.com/watch?v=FPjFwt-Dpdo). Note this video is from an older mod method, but the way it displays in game is the same, the enabling of it is now just all done automatically based on tuning, rather than enabling "Colorblind" mode.
   2. Custom color set - defined by you as the colorblind color set in the settings, it will be used only in extended range mode songs, while regular songs will use the normal colors
   ** Known bugs: Some highlights for accented open strings or HO / PO notations, flicker between the default colors and the modified ER mode colors.
                  The string pegs shown in the tuner are not changed correctly. The colors change when the song starts, not in the tuner, meaning on first ER use, default colors will be shown in the tuner. It also means that if you last played an ER song, then go back to a standard tuned song - the pegs in the tuner will still show in ER colors.
      
* **Custom Song List Titles**
  * Normally those are not customizable in-game and simply listed as SONG LIST 1 to SONG LIST 6. Now you can customize those, making for example, a song list for only B Standard songs, only Exercise songs, etc.
  
* **Add/Decrease Song Volume**
  * In case you are playing a song which is unusually low in volume, or is ear-piercingly loud, by invoking the functions of AudioKinetic audio engine used by the game, you can now modify volume of the music on the fly, without going in to the mixer. 
  
* **Toggle Loft**
  * If you are a streamer, this one may especially come in handy. The background behind the noteway (be it a crowd in the venue or just a plain wall), can now be removed on the fly and replaced with a dark background. It is suggested that you also turn off "Venue Mode" in the game settings as this will help with performance a little as well as make sure you don't have the colored showlights being displayed. The all black background can then be "keyed out" in OBS using a Luma key set for black (luma key works better than chroma key for this), effectively giving you the ability to make the note highway "float" over whatever background you use. To you while playing, or recording game footage locally, it will just appear as an all black background.
  * **Options** - loft can be automatically toggled off as soon as the game is started or only when in a song, or by pressing the hotkey that you define.
  
* **Greenscreen Wall**
  * Similar to Toggle Loft, but only applied to the background wall. This keeps the amps visible and a few of the other UI elements that are removed with Toggle Loft.
  
* **Force ReEnumeration**
  * Normally after adding a new song to your collection, you would have had to either restart the game, or enter the in-game Shop in order for the game to recognize the new songs.
  * **Options**
  1. Automatic - check if any new songs have been added every X seconds (even while a song is playing!)
  2. By going into Enumerate menu - SHOP in the menu has now been replaced with **Enumerate** and conveniently moved to the second place **(GUI only)**
  3. By hotkey - press a hotkey to force the game to enumerate your songs.

* **Removal of certain visual game elements**
  * Again something which may come in handy if you are a streamer (or just prefer your screen to be as uncluttered as possible), you can now remove certain elements from the screen
  * **Options**
  1. Headstock
  2. Skyline (Dynamic Difficulty bars at the top)
  3. Frets
  4. Inlays
  5. Lane Markers
  6. Lyrics
  
* **Rainbow Strings**
  * A fun mod which will continously hue-shift the colors of your strings, making them look like a rainbow! 

* **Custom String Colors**
  * It doesn't all have to be fun and games, so instead of a rainbow, your strings can also be permanently changed in your own color set.

* **Remove song previews**
  * In case you don't like hearing song previews while scrolling through your song list, they can also be disabled.

* **Play audio in background**
  *  Allows you to listen to Rocksmith with the game in the background (alt-tabbed out of the game).
  
* **Linear Riff Repeater**
  * By default, the speed for Riff Repeater is not linear - In standard Rocksmith 2014: 68% speed in Riff Repeater = 50% real speed. With this mod: 68% speed in Riff Repeater = 68% real speed.

* **Enable looping**
  * Allows you to loop sections of songs. This differs from Riff Repeater as we let you pick sections by the amount of time using specialized keybindings to set the loop beginning and the loop end.

* **Allow rewinding** 
  * If you mess up a section and want to retry it, you can go back in time for a set amount of seconds. 

* **Custom Non-Stop Play timer**
  * The timer between songs in Non-stop play is 10.9 seconds by default and a lot of people find this timer to be too long. With this mod, you can change the amount of time between each song (down to 2 seconds due to technical limitations).

* **Start RS on secondary monitor**
  * Makes Rocksmith run on your second monitor. Though, make sure to pay attention to the tooltips!

* **Bypass 2+ RealToneCable popups**
  * Allows you to have two Real Tone Cables plugged in while playing singleplayer. Without the mod Rocksmith will stop you from doing this. 

* **Alternative sample rates for sound output**
  * Rocksmith normally needs sample rate of 48kHz, but this mod tells the game to look for a sample rate that you set. Doing this enables you to use headphones/speakers that don't support 48kHz (eg. some Bluetooth headphones). This won't save you from latency, but at least BT headphones will work in the game.

* **Prevent buggy tones**
  * When playing some songs, the tone system may just give up and constantly play the clean tone, no matter the currently enabled tone. To fix this, you normally have to restart your game to get tones working again, but this mod may just help the game recover without restarts.

* **GuitarSpeak**
  * What hides behind this mysterious name is an exciting feature that will let you control the game by playing certain notes on your guitar (fully customizable!). Forget your keyboard and mouse, guitar is where it's at! There is an option to continue to use it while in the tuner - it is off by default as it can cause issues, but if you're happy to continue using it there - hit that button. It is OFF while in a song, tuning menus, and calibration menus by default.

* **Auto enter last used profile**
  * Also know as the Fork-in-the-toaster mod, due to it's simple but effective nature, it is best used in conjunction with the Fast Load mod. The DLL will spam Enter key in order to automatically enter the game. It may cause potential issues if UPLAY servers are unavailable, but in general it makes your life quite a bit easier.

* **Auto tune your Whammy DT**
  * If you own a Digitech Whammy DT and have a device capable of sending MIDI program controls, such as a simple USB-to-MIDI cable or a MIDI output port on an interface, you may find this useful - automatically change the tuning of your guitar to the tuning of the current song without touching the pedal (works even with odd tunings like A443 or similar). Connect your MIDI cable to the MIDI IN port on the Whammy DT, select the MIDI device name in the GUI settings and when on the Pre-Song tuning screen, press the DELETE key to skip tuning and auto activate the drop tuning of the Whammy DT. This assumes your guitar is in E Standard or Drop D when setting the amount of steps the pedal needs to shift, be that pitching up or down. You'll see the lights on the Whammy DT turn on when the mod is activated and it will auto de-activate when on the post song results screen.
 
* **Allow Riff Repeater Speed Above 100** 
  * For whatever reason TTFAF is not fast enough for you, you can now play it in Riff Repeater with speeds over 100%. :)
  * **Options**
 Riff Repeater Speed Increment - we suggest you use 2 as the minimum value here. This means every key press will increase the speed of the track by 2%. This gives you the most flexibility on how fast you want the song to be.
 
* **Screenshot Scores**
  * If keeping track of your improvements is your jam, you may want to use this option which will tell Steam to take a screenshot of your latest playthrough for you. It uses the default key bind in Steam of F12, to take a screenshot when the post song results screen is displayed. 
  
* **Show Current Note**
  * While we still don't have a way of forcing the game to show the pause menu tuner while you are playing, you can enable this budget version to see which note you are currently hitting, so that you can finally hit 100% instead of missing those pesky bends due to _bad note detection_.

* **Show Song Timer**
  * Show a timestamp of your current position in a song. 

* **Override input volume**
  * Now you can turn your guitar or bass up to 11! Rocksmith sets what volume it wants to listen to your cable at, but this mod allows you to bypass that restriction by changing it to whatever you set.

* **Fix Oculus Crash**
  * When you try to open Rocksmith with a Oculus / Meta headset connected to your PC, it typically crashes. This mod tries to avoid the crash by preventing the bad code from running. It may also fix other audio-related crashes when Rocksmith opens.

* **Fast Load** - **GUI Only**
  * If you are running the game from an SSD or especially an NVMe SSD drive, you will enjoy this one - it skips the intro screens and lets you load the game in a matter of seconds. It can be fairly unstable, but in general it should work provided you don't try to use it on a good old mechanical HDD. This is not a DLL mod! This means that removing the DLL will not reverse the change of this mod, as you will need to restore the backup of your cache.psarc or verify your steam files.
  
* **Custom Tunings** - **GUI Only**
  * By default, the game has a fairly limited set of tunings it can recongnize and in cases where it doesn't find in the list, it will just display CUSTOM TUNING. And that isn't of much help, is it? But don't worry, you can now make the game know that a B Standard song is actually B Standard, and not just _Custom Tuning_ :(
  Together with the list we include, you can add your own tunings if you find some which aren't included in the list. This is not a DLL mod! This means that removing the DLL will not reverse the change of this mod, as you will need to restore the backup of your cache.psarc or verify your steam files.
  
* **EXIT GAME in the menu** - **GUI Only**
  * As useful of a device your mouse is, it is not really the most convenient option when you want to exit the game. And until now, you had to use the said mouse to do that, but fret no more. It took only six and a half years, but now you can exit the game by pressing EXIT GAME in the menu (which replaces the UPLAY button, and let's be honest, no-one used that). This is not a DLL mod! This means that removing the DLL will not reverse the change of this mod, as you will need to restore the backup of your cache.psarc or verify your steam files.
  
* **Enable Direct Connect Mode** - **GUI Only** - https://youtu.be/H6nAB5ogfeU
  * This mod enables a hidden input mode that UbiSoft made - but for unknown reasons disabled for release. It is basically Microphone Mode - but with the tone simulations enabled. This is not a DLL mod! This means that removing the DLL will not reverse the change of this mod, as you will need to restore the backup of your cache.psarc or verify your steam files.
  ** Known issues; Some interfaces report the guitar input channel as one that the game isn't expecting, in this case - Direct Connected mode may not work for you particularly well. If you want to test before applying - go into Microphone Mode and see if your interface lets you have some note detection. If it does - then DC mode should work for you once enabled.
  
* **Change Default tones** - **GUI Only**
  * Add your favorite tones to slot number 1 on the tone stick. This is the default tone that is applied when the game loads up. There is a separate one saved for Lead, Rhythm and Bass. You need to have a tone saved in your profile. It does not need to be assigned to a "tone stick" slot for the GUI to be able to load it and then set it as the new default. This is not a DLL mod! This means that removing the DLL will not reverse the change of this mod, as you will need to restore the backup of your cache.psarc or verify your steam files.

  * Note: While the ability to change the tone of the Emulated Bass is available and can be added easily, we do not want to edit this tone nor do we condone anyone editing this tone. The Emulated Bass tone has a unique characteristic where it's always the same tone either in song, or in the menus. For this reason we believe that the default tone is most likely the best option to pick for the extensive range of songs / genres it can be used in.
  
* **Change Default Guitarcade tones** - **GUI Only**
  * If you just can't stand the sound of the tones in a Guitarcade game, you can change them here. This is not a DLL mod! This means that removing the DLL will not reverse the change of this mod, as you will need to restore the backup of your cache.psarc or verify your steam files.
  
* **Backup Players Profile** - **GUI Only**
  * Every time the RSMods GUI is opened, it will make a backup of your Rocksmith Player Profiles. This is an automated process to help recover from profile corruptions. Profile backups can be found in "Rocksmith2014/Profile_Backups/MM-DD-YYYY_HH-mm-ss".
  
## Installation:
* There's two options as how this mod can be installed:
1. Manually build/copy the DLL to the root folder of Rocksmith 2014 RM, create a file called RSMods.ini and fill it with options as [shown below](https://github.com/Lovrom8/RSMods#settings)
   If it is installed in the most common folder (C drive, Program Files/Steam/Steamapps), VS will attempt to copy the DLL to the folder for your convenience.
2. Use the One-Click-Installer to copy both the DLL and RSMods GUI to the game folder. If it's unable to automatically detect where Rocksmith is installed, it will ask you to point it to the correct folder.

### Running under Wine / Proton
Install RSMods into the Rocksmith 2014 wineprefix using protontricks, then add this to the Rocksmith 2014 launch options in Steam:

```
WINEDLLOVERRIDES="xinput1_3=n,b" %command%
```

## Requirements:
* Latest Steam version of Rocksmith 2014 Remastered on Windows, https://store.steampowered.com/app/221680/Rocksmith_2014_Edition__Remastered/
* MS Visual C++ 2015-2022 Redistributable for the DLL, .NET framework 4.7.2 for GUI/One-Click-Installer
* Sorry Mac users, RS on Mac is its own beast altogether, so we only support the Windows version
  
## Dependencies:
* DirectX 9 SDK, ImGUI, GDI+, Detours, RtMidi - all of which are included in the project folder and should require no additional installations to compile and use the project
* Setup as C++20 / VS2022 project

## Settings (`RSMods.ini`)

The DLL (`DLL/Settings.cpp`) and GUI both read/write the same `RSMods.ini` in the Rocksmith install folder. Prefer the GUI, but you can edit by hand. Keys and defaults below match the DLL parser (`Settings::ReadKeyBinds` / `ReadModSettings` / `ReadStringColors` / `ReadNotewayColors`) and the GUI first-run writer (`WriteSettings.LoadSettingsFromINI`).

Format is standard INI: `[Section]` then `Key = Value` (spaces around `=` are fine).

### `[SongListTitles]`
| Key | Default | Values | Info |
| --- | ------- | ------ | ---- |
| `SongListTitle_1` … `SongListTitle_20` | `SONG LIST` (GUI first-run uses `Define Song List N Here`) | string | Custom names for the 6 stock song lists plus 14 extra lists (profile-modified). Keep under ~25 characters. |

### `[Keybinds]`
All keybinds use [Virtual-Key names](https://docs.microsoft.com/en-us/windows/win32/inputdev/virtual-key-codes) (include the `VK_` prefix), e.g. `VK_F3`, `VK_MEDIA_PLAY_PAUSE`. Leave empty to disable. DLL fallbacks if a key is missing are shown in parentheses.

| Key | DLL fallback | Info |
| --- | ------------ | ---- |
| `ToggleLoftKey` | `T` | Toggle loft / black background. Requires `ToggleLoft = on`. |
| `ShowSongTimerKey` | `N` | Toggle song timer. Requires `ShowSongTimer = on`. |
| `ForceReEnumerationKey` | `F` | Force DLC re-enumeration. Requires ForceReEnumeration not `off`. |
| `RainbowStringsKey` | `V` | Toggle rainbow strings. Requires `RainbowStrings = on`. |
| `RainbowNotesKey` | `N` | Toggle rainbow notes. Requires `RainbowNotes = on`. |
| `RemoveLyricsKey` | `L` | Toggle lyrics. Requires `Lyrics = on`. |
| `RRSpeedKey` | `R` | Adjust RR speed by `RRSpeedInterval` (hold Shift to decrease). Requires `RRSpeedAboveOneHundred = on`. |
| `TuningOffsetKey` | `O` | Change MIDI auto-tune offset in-game. |
| `ToggleExtendedRangeKey` | `E` | Toggle Extended Range mode on/off. |
| `LoopStartKey` | `Y` | Mark loop start. Requires `AllowLooping = on`. |
| `LoopEndKey` | `U` | Mark loop end. Requires `AllowLooping = on`. |
| `RewindKey` | `Z` | Rewind by `RewindBy` ms. Requires `AllowRewind = on`. |

Visual key reference: ![Virtual keys](https://i.imgur.com/lpNv3yG.png). Stream Deck users often use `VK_F13`–`VK_F24`.

### `[Audio Keybindings]`
Same VKey format. Require `VolumeControl = on`. Use Control with the key to decrease volume. Step size is `VolumeControlInterval`.

| Key | DLL fallback | Info |
| --- | ------------ | ---- |
| `MasterVolumeKey` | `5` | Master volume (not shown in in-game mixer). |
| `SongVolumeKey` | `6` | Song / music volume. |
| `Player1VolumeKey` | `7` | Player 1 instrument volume. |
| `Player2VolumeKey` | `8` | Player 2 instrument volume. |
| `MicrophoneVolumeKey` | `9` | Microphone volume. |
| `VoiceOverVolumeKey` | `0` | Voice-over volume. |
| `SFXVolumeKey` | `S` | SFX volume. |
| `DisplayMixerKey` | `P` | Show selected volume / mixer display. |
| `MutePlayer1Key` | `X` | Mute / unmute player 1. |
| `MutePlayer2Key` | `C` | Mute / unmute player 2. |

### `[Toggle Switches]`
| Key | Default | Values | Info |
| --- | ------- | ------ | ---- |
| `ToggleLoft` | `off` | on/off | Black out venue loft / background. |
| `VolumeControl` | `off` | on/off | Enable audio keybinds. |
| `ShowSongTimer` | `off` | on/off | Enable song timer. |
| `ForceReEnumeration` | `off` | `off` / `manual` / `automatic` | Re-scan DLC for new songs. |
| `RainbowStrings` | `off` | on/off | Rainbow string colors. |
| `RainbowNotes` | `off` | on/off | Rainbow note colors. |
| `ExtendedRange` | `off` | on/off | ER color remap for low tunings. |
| `ExtendedRangeDropTuning` | `off` | on/off | Also trigger ER on drop tunings. |
| `ExtendedRangeFixBassTuning` | `off` | on/off | Fix bad bass string 4/5 tuning on some charts. |
| `CustomStringColors` | `0` | `0` / `1` / `2` | `0` = stock colors, `1` = ZZ set, `2` = `[String Colors]`. |
| `SeparateNoteColors` | `off` | on/off | Use separate note colors (see `SeparateNoteColorsMode`). |
| `DiscoMode` | `off` | on/off | Deprecated / unused in current GUI. |
| `Headstock` | `off` | on/off | Remove headstock. |
| `Skyline` | `off` | on/off | Remove DD skyline bars. |
| `GreenScreenWall` | `off` | on/off | Remove back wall only (keep amps). |
| `ForceProfileLoad` | `off` | on/off | Auto-spam Enter at boot to load profile. |
| `Fretless` | `off` | on/off | Remove fret wire. |
| `Inlays` | `off` | on/off | Remove inlays (stock dots only). |
| `RemoveFingerprints` | `off` | on/off | Remove fingerprint / hand indicators. |
| `ToggleLoftWhen` | `manual` | `startup` / `song` / `manual` | When loft toggles. |
| `ToggleSkylineWhen` | `song` | `startup` / `song` | When skyline is removed. |
| `LaneMarkers` | `off` | on/off | Remove unused lane markers. |
| `Lyrics` | `off` | on/off | Remove lyrics. |
| `RemoveLyricsWhen` | `manual` | `startup` / `manual` | When lyrics are removed. |
| `GuitarSpeak` | `off` | on/off | Guitar notes as key presses (see Guitar Speak). |
| `RemoveHeadstockWhen` | `song` | `startup` / `song` | When headstock is removed. |
| `ScreenShotScores` | `off` | on/off | Steam screenshot on song end (Steam screenshot key = F12). |
| `RRSpeedAboveOneHundred` | `off` | on/off | Allow Riff Repeater speed above 100%. |
| `AutoTuneForSong` | `off` | on/off | MIDI auto-tune for supported pedals. |
| `AutoTuneForSongDevice` | _(empty)_ | string | MIDI out device name. |
| `MidiInDevice` | _(empty)_ | string | MIDI in device name. |
| `AutoTuneForSongWhen` | `manual` | `manual` / `tuner` | When auto-tune fires. |
| `AutoTuneForSoftwareSemitoneSettings` | _(empty)_ | string | Software pedal drop-tune config. Format: `OffChannel, PC\|CC, CC Channel`. |
| `AutoTuneForSoftwareSemitoneTriggers` | _(empty)_ | string | Semitone ↔ PC/CC map, e.g. `0 66, 1 77, -2 22`. |
| `AutoTuneForSoftwareTrueTuningSettings` | _(empty)_ | string | Software pedal true-tuning config. |
| `AutoTuneForSoftwareTrueTuningTriggers` | _(empty)_ | string | True tuning ↔ PC/CC map, e.g. `432 32, 455 2`. |
| `ChordsMode` | `off` | on/off | Pedal chords mode (with AutoTuneForSong). |
| `ShowCurrentNoteOnScreen` | `off` | on/off | Show current note name (single notes only). |
| `OnScreenFont` | `Arial` | font name | Font for on-screen text. |
| `ProfileToLoad` | _(empty)_ | profile name | Profile for ForceProfileLoad. |
| `ShowSongTimerWhen` | `manual` | `automatic` / `manual` | When song timer is shown. |
| `ShowSelectedVolumeWhen` | `manual` | `automatic` / `manual` | When selected volume is shown. |
| `SecondaryMonitor` | `off` | on/off | Launch Rocksmith on secondary monitor. |
| `SongPreviews` | `off` | on/off | Disable song previews in the song list. |
| `OverrideInputVolumeEnabled` | `off` | on/off | Override RealTone / input volume. |
| `OverrideInputVolumeDevice` | _(empty)_ | device name | Input device for volume override. |
| `AllowAudioInBackground` | `off` | on/off | Keep audio when alt-tabbed. |
| `BypassTwoRTCMessageBox` | `off` | on/off | Allow two RTCs in singleplayer without popup. |
| `LinearRiffRepeater` | `off` | on/off | Linear RR speed (68% UI = 68% real). |
| `AltOutputSampleRate` | `off` | on/off | Use non-48 kHz output sample rate. |
| `AllowLooping` | `off` | on/off | Custom loop markers (LoopStart/End keys). |
| `AllowRewind` | `off` | on/off | Rewind key. |
| `FixOculusCrash` | `off` | on/off | Mitigate crash with Oculus/Meta headsets. |
| `FixBrokenTones` | `off` | on/off | Try to recover dead tone system. |
| `UseCustomNSPTimer` | `off` | on/off | Custom Non-stop Play inter-song timer. |
| `DisplayCurrentAccuracy` | `off` | on/off | Show current accuracy on screen. |
| `PreventMidSongPause` | `off` | on/off | Prevent accidental mid-song pause. |
| `Ultrawide` | `off` | on/off | Ultrawide (21:9 / 32:9) display support. The interface stays 16:9. |

### `[String Colors]`
Hex colors without `#` (e.g. `FF4F5A`). Indices `0`–`5` = low E … high E. `_N` = normal songs; `_CB` = Extended Range / colorblind palette (DLL defaults below).

| Key | Default (N) | Default (CB / ER, DLL) |
| --- | ----------- | ---------------------- |
| `string0_N` / `string0_CB` | `FF4F5A` | `C12A2A` |
| `string1_N` / `string1_CB` | `E2C102` | `A3F400` |
| `string2_N` / `string2_CB` | `1DACF9` | `1DACF9` |
| `string3_N` / `string3_CB` | `FF9216` | `DB7F41` |
| `string4_N` / `string4_CB` | `3FCC0C` | `00C68E` |
| `string5_N` / `string5_CB` | `C825ED` | `7648A8` |
| `note0_N` … `note5_N` | same as strings_N | |
| `note0_CB` … `note5_CB` | same as strings_CB | |

### `[Mod Settings]`
Numeric values used by the DLL (`customSettings`). Times are **milliseconds** unless noted.

| Key | Default | Values / range | Info |
| --- | ------- | -------------- | ---- |
| `ExtendedRangeModeAt` | `-5` | `-12` … `-2` | Semitones below E Standard when ER triggers (`-2` = D, `-5` = B, `-12` = octave down). |
| `CheckForNewSongsInterval` | `5000` | ms | Poll interval for automatic re-enumeration. |
| `RRSpeedInterval` | `2` | e.g. `-50` … `50` | % change per RRSpeedKey press. |
| `TuningPedal` | `0` | `0`–`4` | `0` = none, `1` = Whammy DT, `2` = Bass Whammy, `3` = Whammy, `4` = software pedal. |
| `TuningOffset` | `0` | `-3` … `12` | MIDI auto-tune offset from E Standard (not ER threshold). |
| `VolumeControlInterval` | `5` | e.g. `1`–`100` | % step for volume keys. |
| `SecondaryMonitorXPosition` | `0` | int | Virtual desktop X of secondary monitor top-left. |
| `SecondaryMonitorYPosition` | `0` | int | Virtual desktop Y of secondary monitor top-left. |
| `SeparateNoteColorsMode` | `0` | `0` / `1` / `2` | `0` = same as strings, `1` = stock note colors, `2` = custom note colors. |
| `OverrideInputVolume` | `17` | `0`–`100` | Input volume override (Rocksmith’s stock “default” is 17). |
| `AlternativeOutputSampleRate` | `48000` | e.g. `44100`, `48000` | Used when `AltOutputSampleRate = on`. |
| `LoopingLeadUp` | `0` | ms | Lead-in before custom loops. |
| `RewindBy` | `5000` | ms | How far rewind jumps. |
| `RewindLeadup` | `2000` | ms | Grey note / lead-up adjust after rewind. |
| `CustomNSPTimeLimit` | `10000` | ms | Non-stop Play wait between songs (UI min ~2 s). |
| `OnScreenFontSize` | `24` | e.g. `8`–`80` | On-screen text size. |

### `[Guitar Speak]`
MIDI note numbers `0`–`96` (C-1 … C7). Example: low E 12th fret (E3) ≈ `52`.

| Key | Default | Info |
| --- | ------- | ---- |
| `GuitarSpeakDeleteWhen` | `0` (off) | Delete |
| `GuitarSpeakSpaceWhen` | `0` | Space |
| `GuitarSpeakEnterWhen` | `0` | Enter |
| `GuitarSpeakTabWhen` | `0` | Tab |
| `GuitarSpeakPGUPWhen` | `0` | Page Up |
| `GuitarSpeakPGDNWhen` | `0` | Page Down |
| `GuitarSpeakUPWhen` | `0` | Up arrow |
| `GuitarSpeakDNWhen` | `0` | Down arrow |
| `GuitarSpeakESCWhen` | `0` | Escape |
| `GuitarSpeakCloseWhen` | `0` | Stop GuitarSpeak |
| `GuitarSpeakOBracketWhen` | `0` | `[` |
| `GuitarSpeakCBracketWhen` | `0` | `]` |
| `GuitarSpeakTildeaWhen` | `0` | `~` |
| `GuitarSpeakForSlashWhen` | `0` | `/` |
| `GuitarSpeakAltWhen` | `0` | Alt |
| `GuitarSpeakWhileTuning` | `off` | on/off - **advanced only**; wrong notes can block tuning |

### `[Highway Colors]`
| Key | Default | Values | Info |
| --- | ------- | ------ | ---- |
| `CustomHighwayColors` | `off` | on/off | Enable custom noteway colors. |
| `CustomHighwayNumbered` | _(empty)_ | hex | Numbered frets. |
| `CustomHighwayUnNumbered` | _(empty)_ | hex | Unnumbered frets. |
| `CustomHighwayGutter` | _(empty)_ | hex | Highway sides. |
| `CustomFretNubmers` | _(empty)_ | hex | Fret number text (**spelling is intentional** - matches DLL key). |

### `[GUI Settings]`
Used by the RSMods GUI only (not read by the game DLL).

| Key | Default | Values | Info |
| --- | ------- | ------ | ---- |
| `CustomTheme` | `off` | on/off | Custom GUI colors. |
| `ThemeBackgroundColor` | `F0FFFF` | hex | Background. |
| `ThemeTextColor` | `000000` | hex | Text. |
| `ThemeButtonColor` | `E3E3E3` | hex | Buttons. |
| `BackupProfile` | `on` | on/off | Profile backups. |
| `NumberOfBackups` | `50` | int | Max backups to keep. |

### Notes
* Prefer saving from the GUI so key names and defaults stay consistent with the DLL.

## FAQ

* Q: How do I setup my stream so it is transparent / black / etc?
* A: 

  0. Download RSMods from the releases page, run the installer, and click the button to install the mods.
  1. Go into the "Enable / Disable Mods" tab.
  2. Check the checkbox with the name "Toggle Loft".
  3. Go into the "Disable UI Elements" sub-tab.
  4. In the "Toggle Loft Off When" section, set it to "Always". This will make your game have a black background ALWAYS (after you get into the game, not counting the profile screen / login screens / etc).
  5. In your OBS / SLOBS you need to setup a key. In OBS it is a Luma Key and in SLOBS it's a Color key. You can close RSMods if you don't want to mess with any other mods.

  OBS:
  1. Right click on your capture, and click "Filters".
  2. Click the "+" button and pick "Luma Key" and hit "Okay".
  3. Leave everything at 0.00, except for "Luma Max" which should be at 1.00
  4. Hit close to save your changes.
  5. Add a background so it's not just black by default. For me, I just drew a quick image and put it under the game.
  * Result: https://i.imgur.com/MX5GQNU.png

  SLOBS:
  1. Right click on your capture, and click "Filters".
  2. Click the "+" button and pick "Color Key", name it whatever you want, and click "Done".
  3. In "Key Color Type" pick "Custom Color", click the new "Key Color" box and drag it to the bottom-left hand corner. It should say "#00000000" if done correctly.
  4. Set "Similarity" to 1.
  5. Set "Smoothness" to 150, leave everything else default, and click "Done".
  6. Add a background so it's not just black by default. For me, I just drew a quick image and put it under the game.
  * Result: https://cdn.discordapp.com/attachments/758715497352396860/822917699088154664/unknown.png

* Q: What pedals can be automatically tuned with MIDI?
* A: The current pedals that support drop tuning automatically are the Digitech Whammy DT, Digitech Whammy, and the Digitech Bass Whammy. Thank PoizenJam for the work they did getting those last two pedals to work as intended.

* Q: I want something added to the mod, like I want to remove XYZ from the game!!!
* A: Post a request in the [Issues tab of this Github repo](https://github.com/Lovrom8/RSMods/issues) with the name "DLL - FR - XYZ". Please follow this format so we know what we are doing, either fixing something or adding a new feature. Note: Just because you post a request for us to do something doesn't mean we can do it and/ or we will do it. Please don't keep begging us to do your request, we will get to it if we think it's worth our time spent.


* Q: I found a bug / somethings not working as it should! How do I get this fixed?
* A: Please post an issue in the [Issues tab of this Github repo](https://github.com/Lovrom8/RSMods/issues) with the name "DLL - Bug - XYZ" if the bug is in Rocksmith or "GUI - Bug - XYZ" if the bug is in the tool to mod your game. Please try to be descriptive in this, as posting "It don't work" doesn't help us. Screenshots and/ or videos can help, but steps on what you did to get it to break would be greatly appreciated. Sometimes bugs are where we get our best features :)


* Q: Who made these mods, and how can I thank you guys?
* A: Thank you for showing interest in the project. We spent over ten months working on this project trying to flesh it out so people would enjoy using it. The developers are mainly: LovroM8 (Lovro), and Ffio1 (Ffio) but we received a lot of help from ZagatoZee (ZZ), Kokolihapihvi (Koko), and L0fka. All we ask for is a quick thank you, and those can either be sent by starring this project on Github, telling your Rocksmith friends, or just coming into the Discord and saying "Thank You". I know it sounds like it's not much, but it really makes our day(s). 

* If you have any questions, feel free to reach us in the [r/Rocksmith Discord](https://rocksmith.rocks/discord) in the #rsmods channel.
