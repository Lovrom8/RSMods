# The Rocksmith Mods that nobody asked for

[![Build status](https://ci.appveyor.com/api/projects/status/github/Lovrom8/rsmods?svg=true)](https://ci.appveyor.com/project/Lovrom8/rsmods)

## Branches:
1) [Develop](https://github.com/Lovrom8/RSCDLCEnabler-2020Edition/tree/develop) - proxy DLL for D3DX9_42.dll - [latest build](https://ci.appveyor.com/api/projects/Lovrom8/rsmods/artifacts/DLL.zip?branch=develop)
2) [GUI](https://github.com/Lovrom8/RSCDLCEnabler-2020Edition/tree/GUI) - used to customize settings for the internal mod and to add set-and-forget mods - [latest build](https://ci.appveyor.com/api/projects/Lovrom8/rsmods/artifacts/RSMods.zip?branch=GUI)
3) [One-Click-Installer](https://github.com/Lovrom8/rsmods/tree/One-Click-Installer) - one big button to start modding :) - [latest build](https://github.com/Lovrom8/RSCDLCEnabler-2020Edition/releases/latest)

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
  * Normally those are not customizable in-game and simply listed as SONG LIST 1 to SONG LIST 6. Now you can customize those, making for example, a song list for only B Standard songs, only Excercise songs, etc.
  
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

* **CDLC Enabler**
  * After all, this mod is based on the proxy DLL which bypassed the check which enabled users to play community made songs in the game.

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
  * Add your favorite tones to slot number 1 on the tone stick. This is the default tone that is applied when the game loads up. There is a seperate one saved for Lead, Rhythm and Bass. You need to have a tone saved in your profile. It does not need to be assigned to a "tone stick" slot for the GUI to be able to load it and then set it as the new default. This is not a DLL mod! This means that removing the DLL will not reverse the change of this mod, as you will need to restore the backup of your cache.psarc or verify your steam files.

  * Note: While the ability to change the tone of the Emulated Bass is available and can be added easily, we do not want to edit this tone nor do we condone anyone editting this tone. The Emulated Bass tone has a unique characteristic where it's always the same tone either in song, or in the menus. For this reason we believe that the default tone is most likely the best option to pick for the extensive range of songs / genres it can be used in.
  
* **Change Default Guitarcade tones** - **GUI Only**
  * If you just can't stand the sound of the tones in a Guitarcade game, you can change them here. This is not a DLL mod! This means that removing the DLL will not reverse the change of this mod, as you will need to restore the backup of your cache.psarc or verify your steam files.
  
* **Backup Players Profile** - **GUI Only**
  * Everytime the RSMods GUI is opened, it will make a backup of your Rockmsith Player Profiles. This is automated process to help recover from profile corruptions. Profile backups can be found in "Rocksmith2014/Profile_Backups/MM-DD-YYYY_HH-mm-ss".
  
## Installation:
* There's two options as how this mod can be installed:
1. Manually build/copy the DLL to the root folder of Rocksmith 2014 RM, create a file called RSMods.ini and fill it with options as [shown below](https://github.com/Lovrom8/RSCDLCEnabler-2020Edition#settings)
   If it is installed in the most common folder (C drive, Program Files/Steam/Steamapps), VS will attempt to copy the DLL to the folder for your convenience.
2. Use the One-Click-Installer to copy both the DLL and RSMods GUI to the game folder. If it's unable to automatically detect where Rocksmith is installed, it will ask you to point it to the correct folder.
  
## Requirements:
* Latest Steam version of Rocksmith 2014 Remastered on Windows, https://store.steampowered.com/app/221680/Rocksmith_2014_Edition__Remastered/
* MS Visual C++ 2015-2019 Redistributable for the DLL, .NET framework for GUI/One-Click-Installer
* Sorry Mac users, RS on Mac is it's own beast altogether, so we only support the Windows version
  
## Dependencies:
* DirectX 9 SDK, ImGUI, GDI+, Detours, RtMidi - all of which are included in the project folder and should require no additional installations to compile and use the project
* Setup as C++17 / VS2019 project

## Settings:
If you want to manually create the settings file for the DLL, download the template from [here](https://pastebin.com/raw/f6hf990R):

And the general file structure should be as follows:

Section        | Entry             | Possible values | Info   |
-------------- | ----------------- | --------------- | ------ |
**SongListTitles** | &nbsp; | &nbsp; | &nbsp; |
 &nbsp; | SongListTitles_1  | _user defined string_ | Songlist 1's name |
 &nbsp; | ... | &nbsp; | &nbsp; |
 &nbsp; | SongListTitles_6 |  _user defined string_ | Songlist 6's name |       
 **Keybinds** | &nbsp; | &nbsp; | &nbsp; |
&nbsp; | ToggleLoftKey | Function keys (F1, F10) / Media Keys (Play/Pause, Stop, NextTrack, PreviousTrack) in Virtual Key format.| Toggle the background of the game when this key is pressed. Only usable when Toggle Switches > ToggleLoft is on. |
&nbsp; | ShowSongTimerKey | Function keys (F1, F10) / Media Keys (Play/Pause, Stop, NextTrack, PreviousTrack) in Virtual Key format.| Shows the current time of the song being played. Only usable when Toggle Switches > ShowSongTimer is on.|
&nbsp; | ForceReEnumerationKey | Function keys (F1, F10) / Media Keys (Play/Pause, Stop, NextTrack, PreviousTrack) in Virtual Key format.| Force the game to check for new CDLC added to your DLC folder. Only usable when Toggle Switches > ForceReEnumeration is on. | 
&nbsp; | RainbowStringsKey | Function keys (F1, F10) / Media Keys (Play/Pause, Stop, NextTrack, PreviousTrack) in Virtual Key format.| Make your strings scroll through colors. Only usable when Toggle Switches > RainbowStrings is on. |
&nbsp; | RainbowNotesKey | Function keys (F1, F10) / Media Keys (Play/Pause, Stop, NextTrack, PreviousTrack) in Virtual Key format.| Make your notes scroll through colors. Only usable when Toggle Switches > RainbowNotes is on. |
&nbsp; | RemoveLyricsKey | Function keys (F1, F10) / Media Keys (Play/Pause, Stop, NextTrack, PreviousTrack) in Virtual Key format.| Toggle the lyrics when in a song. Only usable when Toggle Switches > Lyrics is on. |
&nbsp; | RRSpeedKey | Function keys (F1, F10) / Media Keys (Play/Pause, Stop, NextTrack, PreviousTrack) in Virtual Key format.| Makes Riff Repeater go past 100%. Press this key to go up by the number in Mod Settings > RRSpeedInterval, and hold shift while pressing this key to make it go down by the number in Mod Settings > RRSpeedInterval. Only usable when Toggle Switches > RRSpeedAboveOneHundred is on. |
 **Audio Keybindings** | &nbsp; | &nbsp; | &nbsp;
&nbsp; | MasterVolumeKey | Function keys (F1, F10) / Media Keys (Play/Pause, Stop, NextTrack, PreviousTrack) in Virtual Key format.| Makes the Master Volume go up by the number in Mod Settings > VolumeControlInterval. Press Control at the same time as pressing this key to make the volume go down by the number in Mod Settings > VolumeControlInterval. Only usable when Toggle Switches > VolumeControl is on. These values are not reflected in the Mixer menu. This is a value left in by Ubisoft, but never used in the Mixer menu.
&nbsp; | SongVolumeKey | Function keys (F1, F10) / Media Keys (Play/Pause, Stop, NextTrack, PreviousTrack) in Virtual Key format.| Makes the Master Volume go up by the number in Mod Settings > VolumeControlInterval. Press Control at the same time as pressing this key to make the volume go down by the number in Mod Settings > VolumeControlInterval. Only usable when Toggle Switches > VolumeControl is on. These values are not reflected in the Mixer menu.
&nbsp; | Player1VolumeKey | Function keys (F1, F10) / Media Keys (Play/Pause, Stop, NextTrack, PreviousTrack) in Virtual Key format.| Makes the volume of Player 1 go up by the number in Mod Settings > VolumeControlInterval. Press Control at the same time as pressing this key to make the volume go down by the number in Mod Settings > VolumeControlInterval. Only usable when Toggle Switches > VolumeControl is on. These values are not reflected in the Mixer menu.
&nbsp; | Player2VolumeKey | Function keys (F1, F10) / Media Keys (Play/Pause, Stop, NextTrack, PreviousTrack) in Virtual Key format.| Makes the volume of Player 2 go up by the number in Mod Settings > VolumeControlInterval. Press Control at the same time as pressing this key to make the volume go down by the number in Mod Settings > VolumeControlInterval. Only usable when Toggle Switches > VolumeControl is on. These values are not reflected in the Mixer menu.
&nbsp; | MicrophoneVolumeKey | Function keys (F1, F10) / Media Keys (Play/Pause, Stop, NextTrack, PreviousTrack) in Virtual Key format.| Makes your Microphone Volume go up by the number in Mod Settings > VolumeControlInterval. Press Control at the same time as pressing this key to make the volume go down by the number in Mod Settings > VolumeControlInterval. Only usable when Toggle Switches > VolumeControl is on. These values are not reflected in the Mixer menu.
&nbsp; | VoiceOverVolumeKey | Function keys (F1, F10) / Media Keys (Play/Pause, Stop, NextTrack, PreviousTrack) in Virtual Key format.| Makes the Voice-Over (Rocksmith Dad) Volume go up by the number in Mod Settings > VolumeControlInterval. Press Control at the same time as pressing this key to make the volume go down by the number in Mod Settings > VolumeControlInterval. Only usable when Toggle Switches > VolumeControl is on. These values are not reflected in the Mixer menu.
&nbsp; | SFXVolumeKey | Function keys (F1, F10) / Media Keys (Play/Pause, Stop, NextTrack, PreviousTrack) in Virtual Key format.| Makes the Sound Effects Volume go up by the number in Mod Settings > VolumeControlInterval. Press Control at the same time as pressing this key to make the volume go down by the number in Mod Settings > VolumeControlInterval. Only usable when Toggle Switches > VolumeControl is on. These values are not reflected in the Mixer menu.
&nbsp; | ChangedSelectedVolumeKey | Function keys (F1, F10) / Media Keys (Play/Pause, Stop, NextTrack, PreviousTrack) in Virtual Key format.| Shows you the selected volume's current value.
**Toggle Switches** | &nbsp; | &nbsp;
&nbsp; | ToggleLoft | on/off | Turns off the background of the game. This will make the background black. |
&nbsp; | VolumeControl | on/off | Allows you to change volumes by pressing a keybinding. |
&nbsp; | ShowSongTimer | on/off | Shows the time of the song you are playing. |
&nbsp; | ForceReEnumeration | automatic/manual | Forces the game to look for new CDLC added to your DLC folder. |
&nbsp; | RainbowStrings | on/off | Makes your strings cycle through a color wheel. |
&nbsp; | RainbowNotes | on/off | Makes your notes cycle through a color wheel. |
&nbsp; | ExtendedRange | on/off | Change your string colors based on the tuning of the song. This is helpful for people who play low-tuned songs with a 7-string guitar or 5-string bass. |
&nbsp; | ExtendedRangeDropTuning | on/off | An extension of ExtendedRange that will make drop tunings also trigger the color change. |
&nbsp; | CustomStringColors | 0/1/2 | 0 = default colors, 1 = ZZ's color set, 2 = Color specified in the String Colors section |
&nbsp; | Headstock | on/off | Removes the headstock to give the "headless guitar" look. |
&nbsp; | Skyline | on/off | Removes the purple and orange Dynamic Difficulty bars from the top of the screen for a cleaner-looking UI. |
&nbsp; | GreenScreenWall | on/off | Removes the wall texture of the loft if you want to have the "ToggleLoft" mode look but still want the amps / background elements to show up. |
&nbsp; | ForceProfileLoad | on/off | Spam enter when the game starts so you can go grab some coffee and come back in the main menu. |
&nbsp; | Fretless | on/off | Removes the fret wire from the guitar / bass model. |
&nbsp; | Inlays | on/off | Removes the inlays from the guitar / bass. **ONLY WORKS WITH THE STANDARD DOT INLAY** |
&nbsp; | ToggleLoftWhen | startup/song/manual | When do you want "ToggleLoft" to take effect? |
&nbsp; | LaneMarkers | on/off | Removes the excessive lines going down the noteway for the lanes you aren't currently using. |
&nbsp; | ToggleSkylineWhen | startup/song | When do you want the "Skyline" to be removed? |
&nbsp; | Lyrics | on/off | Removes the lyrics from the game for a cleaner UI. |
&nbsp; | RemoveLyricsWhen | startup/manual | When do you want the "Lyrics" to be removed? |
&nbsp; | GuitarSpeak | on/off | Allows notes being played to trigger a keypress, so you can put the keyboard away and only use your guitar / bass to control the UI. The notes that trigger keypresses are defined in the "Guitar Speak" section. |
&nbsp; | RemoveHeadstockWhen | startup/song | When do you want the "Headstock" to be removed? |
&nbsp; | ScreenShotScores | on/off | Takes a steam screenshot when you finish a song. Requires Steam's screenshot button to be set to F12. |
&nbsp; | RRSpeedAboveOneHundred | on/off | Remove the 100% speed limit from Riff Repeater to play the songs faster than intended. |
&nbsp; | AutoTuneForSong | on/off | If you have one of a few pedals we can send the pedal a signal to enable drop tuning to reduce the amount of time between playing songs. We use MIDI to send the signal. The pedals that are supported can be seen in the Q/A at the bottom of this page. |
&nbsp; | ChordsMode | on/off | Extension of "AutoTuneForSong" where some pedals have two seperate modes they can be played in. This allows us to send the correct signals according to the setting you have on the pedal. |
&nbsp; | ShowCurrentNoteOnScreen | on/off | Reads the current note being played and displays it on screen. This only works for single notes so chords will **NOT** work properly. |
&nbsp; | OnScreenFont | _font_name_ | Name of the font you want us to use when we need to show you text on screen. Default is Arial if we can't find the font you specify. |
&nbsp; | ProfileToLoad | _profile_name_ | An extension of "ForceProfileLoad" where we will look for the profile you specify in the list of all profiles. This is helpful if you have multiple profiles, or multiple users who play on the same computer. |
&nbsp; | ShowSongTimerWhen | automatic/manual | When do you want the "SongTimer" to be shown? |
&nbsp; | ShowSelectedVolumeWhen | automatic/song/manual | When do you want the "SelectedVolume" to be shown?  |
&nbsp; | SecondaryMonitor | on/off | Launch Rocksmith and move it to another monitor automatically. |
**String Colors** | &nbsp; | hex defined color (eg. FF0000) | &nbsp; |
&nbsp; | string0_N | &nbsp; | Colors for strings to be used in non-ER songs |
&nbsp; | ... | &nbsp; | &nbsp; |
&nbsp; | string5_N | &nbsp; | &nbsp; |
&nbsp; | string0_CB | &nbsp; | Colors for strings to be used in ER songs |
&nbsp; | ... | &nbsp; | &nbsp; |
&nbsp; | string5_CB | &nbsp; | &nbsp; |
&nbsp; | note0_N | &nbsp; | Colors for notes to be used in non-ER songs |
&nbsp; | ... | &nbsp; | &nbsp; |
&nbsp; | note5_N | &nbsp; | &nbsp; |
&nbsp; | note0_CB | &nbsp; | Colors for notes to be used in ER songs |
&nbsp; | ... | &nbsp; | &nbsp; |
&nbsp; | note5_CB | &nbsp; | &nbsp; |
**Mod Settings** | &nbsp; | &nbsp; | &nbsp; |
&nbsp; | ExtendedRangeModeAt | numerical value | Offset to E Standard (-1 = Eb, -5 = B) |
&nbsp; | CheckForNewSongsInterval | interval in milliseconds | Time between each Enumeration check |
&nbsp; | RRSpeedInterval | numerical value | % of speed to go up / down when pressing your RRSpeedKey |
&nbsp; | TuningPedal | numerical value | The number of the pedal you have. 0 = off, 1 = Whammy DT, 2 = Bass Whammy, 3 = Whammy |
&nbsp; | TuningOffset | numerical value | Offset for "ExtendedRangeModeAt" that shows how far your guitar is TUNED from E. (-1 = Eb, -5 = B) |
&nbsp; | VolumeControlInterval | numerical value | % of volume to go up / down when you press an audio keybinding. |
&nbsp; | SecondaryMonitorXPosition | numerical value | X position of the top-left corner of the Secondary Monitor (Virtual Screen) |
&nbsp; | SecondaryMonitorYPosition | numerical value | Y position of the top-left corner of the Secondary Monitor (Virtual Screen) |
&nbsp; | SeparateNoteColors | numerical value | 0 = Use same color as strings, 1 = Normal RS Colors, 2 = Custom Note Colors |
**Guitar Speak** | &nbsp; | &nbsp; | &nbsp; |
&nbsp; | GuitarSpeakDeleteWhen | numerical value | Midi note that will trigger Delete to be pressed. |
&nbsp; | GuitarSpeakSpaceWhen | numerical value | Midi note that will trigger Space to be pressed. |
&nbsp; | GuitarSpeakEnterWhen | numerical value | Midi note that will trigger Enter / Return to be pressed. |
&nbsp; | GuitarSpeakTabWhen | numerical value | Midi note that will trigger Tab to be pressed. |
&nbsp; | GuitarSpeakPGUPWhen | numerical value | Midi note that will trigger Page Up to be pressed. |
&nbsp; | GuitarSpeakPGDNWhen | numerical value | Midi note that will trigger Page Down to be pressed. |
&nbsp; | GuitarSpeakUPWhen | numerical value | Midi note that will trigger Up Arrow to be pressed. |
&nbsp; | GuitarSpeanDNWhen | numerical value | Midi note that will trigger Down Arrow to be pressed. |
&nbsp; | GuitarSpeakESCWhen | numerical value | Midi note that will trigger Escape to be pressed. |
&nbsp; | GuitarSpeakCloseWhen | numerical value | Midi note that will trigger Guitar Speak to stop. |
&nbsp; | GuitarSpeakOBracketWhen | numerical value | Midi note that will trigger Open Bracket [ to be pressed. |
&nbsp; | GuitarSpeakCBracketWhen | numerical value | Midi note that will trigger Close Bracket ] to be pressed. |
&nbsp; | GuitarSpeakTildeaWhen | numerical value | Midi note that will trigger Tilda / Tilde to be pressed. |
&nbsp; | GuitarSpeakForSlashWhen | numerical value | Midi note that will trigger Forward Slash to be pressed. |
&nbsp; | GuitarSpeakAltWhen | numerical value | Midi note that will trigger Alt to be pressed. |
&nbsp; | GuitarSpeakWhileTuning | on/off | Is GuitarSpeak enabled while tuning (ONLY FOR ADVANCED USERS) |
**Highway Colors** | &nbsp; | &nbsp; | &nbsp; |
&nbsp; | CustomHighwayColors | on/off | Should we use a custom note highway? |
&nbsp; | CustomHighwayNumbered | hex defined color (eg. FF0000) | &nbsp; |
&nbsp; | CustomHighwayUnNumbered | hex defined color (eg. FF0000) | &nbsp; |
&nbsp; | CustomHighwayGutter | hex defined color (eg. FF0000) | &nbsp; |
&nbsp; | CustomFretNubmers | hex defined color (eg. FF0000) | &nbsp; |
**GUI Settings** | &nbsp; | &nbsp; | &nbsp;
&nbsp; | CustomTheme | on/off | Custom Colors for GUI |
&nbsp; | ThemeBackgroundColor | hex defined color (eg. FF0000) | Background Color |
&nbsp; | ThemeTextColor | hex defined color (eg. FF0000) | Text Color |
&nbsp; | ThemeButtonColor | hex defined color (eg. FF0000) | Button Color |
&nbsp; | BackupProfile | on/off | Create backups of your Rocksmith profile / save just in-case it gets corrupted. |
&nbsp; | NumberOfBackups | numerical value | How many backups should we store? |

* The available keys for the keybinding section can be seen here: ![Visual Representation](https://i.imgur.com/lpNv3yG.png) You must follow the V-Key format available here: https://docs.microsoft.com/en-us/windows/win32/inputdev/virtual-key-codes (yes that includes the VK_ part). Ex. F3 would be VK_F3, and the music play/ pause button would be VK_MEDIA_PLAY_PAUSE. We understand that us locking down the amount of keys may aggrevate some of you but we want to allow you to search for songs without turning on/ off your mods when you search for "Slipknot". **If you are a streamer and have an Elgato Stream Deck** set your keybinds to the F13-F24 keys as most keyboards don't have those keys but we allow them to be used for keybinds.

* String Numbers go from 0-5 as this is zero-index, or how computers normally work. A translation of this is: 0. low E, 1. A, 2. D, 3. G, 4. B, 5. high E.

* Guitar Speak note values are measured from C-1 to C6 starting at 0 and moving up to 96. Ex. The low E string at the 12th fret is E3, and would translate to the number 52.

* GuitarSpeakWhileTuning is only for advanced users because if it is on and you have one of your GuitarSpeak values set wrong it can prevent you from tuning or playing this game. This decision was made to prevent people from uninstalling the mod because GuitarSpeak stops them from rocking out.

* Song list names should be below 25 characters (including spaces) because the text streches out and gets hard to read.
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
* A: Post a request in the [Issues tab of this Github repo](https://github.com/Lovrom8/RSCDLCEnabler-2020Edition/issues) with the name "DLL - FR - XYZ". Please follow this format so we know what we are doing, either fixing something or adding a new feature. Note: Just because you post a request for us to do something doesn't mean we can do it and/ or we will do it. Please don't keep begging us to do your request, we will get to it if we think it's worth our time spent.


* Q: I found a bug / somethings not working as it should! How do I get this fixed?
* A: Please post an issue in the [Issues tab of this Github repo](https://github.com/Lovrom8/RSCDLCEnabler-2020Edition/issues) with the name "DLL - Bug - XYZ" if the bug is in Rocksmith or "GUI - Bug - XYZ" if the bug is in the tool to mod your game. Please try to be descriptive in this, as posting "It don't work" doesn't help us. Screenshots and/ or videos can help, but steps on what you did to get it to break would be greatly appreciated. Sometimes bugs are where we get our best features :)


* Q: Who made these mods, and how can I thank you guys?
* A: Thank you for showing interest in the project. We spent over ten months working on this project trying to flesh it out so people would enjoy using it. The developers are mainly: LovroM8 (Lovro), and Ffio1 (Ffio) but we received a lot of help from ZagatoZee (ZZ), Kokolihapihvi (Koko), and L0fka. All we ask for is a quick thank you, and those can either be sent by starring this project on Github, telling your Rocksmith friends, or just coming into the Discord and saying "Thank You". I know it sounds like it's not much, but it really makes our day(s). 

* If you have any questions, feel free to reach us in the [r/Rocksmith Discord](https://rocksmith.rocks/discord) in the #rsmods channel, or in the #rs-mods channel of the [CustomsForge Discord](https://discord.gg/cf)

