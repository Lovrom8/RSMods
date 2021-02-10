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
 Riff Repeater Speed Increment - we suggest you use 5 as the minimum value here. This means every key press will increase the speed of the track by 5%. Lower values may not work as reliably.
 
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
* DirectX 9 SDK, ImGUI, GDI+, Detours - all of which are included in the project folder and should require no additional installations to compile and use the project
* Setup as C++17 / VS2019 project

## Settings:
If you want to manually create the settings file for the DLL, download the template from [here](https://pastebin.com/raw/tTW5KNdw):

And the general file structure should be as follows:

Section        | Entry             | Possible values | Info
-------------- | ----------------- | --------------- | ------ |
**SongListTitles** | &nbsp;            | &nbsp;          | &nbsp; |
 &nbsp;    | SongListTitles_1  |  _user defined string_  | &nbsp; |
 &nbsp;        |     ...           |     &nbsp;      | &nbsp; |
 &nbsp;        | SongListTitles_6  |  _user defined string_ | &nbsp; |       
 **Keybinds** | &nbsp;            | &nbsp;          | &nbsp; |
&nbsp;    | ToggleLoftKey | Function keys (F1, F10) / Media Keys (Play/Pause, Stop, NextTrack, PreviousTrack) | &nbsp; |
&nbsp;    | AddVolumeKey | Function keys (F1, F10) / Media Keys (Play/Pause, Stop, NextTrack, PreviousTrack) | &nbsp; |
&nbsp;    | DecreaseVolumeKey | Function keys (F1, F10) / Media Keys (Play/Pause, Stop, NextTrack, PreviousTrack)  | &nbsp; |
&nbsp;    | ShowSongTimerKey | Function keys (F1, F10) / Media Keys (Play/Pause, Stop, NextTrack, PreviousTrack) | &nbsp; |
&nbsp;    | ForceReEnumerationKey | Function keys (F1, F10) / Media Keys (Play/Pause, Stop, NextTrack, PreviousTrack) | &nbsp; | 
&nbsp;    | RainbowStringsKey | Function keys (F1, F10) / Media Keys (Play/Pause, Stop, NextTrack, PreviousTrack) | &nbsp; |
**Toggle Switches** | &nbsp;            | &nbsp;          | on = feature enabled, off = disabled |
&nbsp;  | ToggleLoft            | on/off       | &nbsp; |
&nbsp; | AddVolume            | on/off           | &nbsp; |
&nbsp; |DecreaseVolume             | on/off           | &nbsp; |
&nbsp; |ShowSongTimer             | on/off          | &nbsp; |
&nbsp; |ForceReEnumeration            | automatic/manual          | &nbsp; |
&nbsp; |RainbowStrings            | on/off          | &nbsp; |
&nbsp; |ExtendedRange             | on/off         | &nbsp; |
&nbsp; |CustomStringColors             | 0/1/2         | 0 = default colors, 1 = ZZ's color set, 2 = colors defined below |
&nbsp; |Headstock            | on/off          | on = headstock removed! |
&nbsp; |Skyline            | on/off        | &nbsp; |
&nbsp; |GreenScreenWall           | on/off         | &nbsp; |
&nbsp; |ForceProfileLoad             | on/off          | &nbsp; |
&nbsp; |Fretless          | on/off         | &nbsp; |
&nbsp; |Inlays           | on/off          | &nbsp; |
&nbsp; |ToggleLoftWhen             | manual/song       | &nbsp; |
&nbsp; |LaneMarkers           | on/off         | &nbsp; |
&nbsp; |ToggleSkylineWhen             | manual/song         | &nbsp; |
&nbsp; |Lyrics            | on/off          | &nbsp; |
&nbsp; |RemoveLyricsWhen           | manual/song           | &nbsp; |
&nbsp; |GuitarSpeak          | on        | &nbsp; |
&nbsp; |RemoveHeadstockWhen             | manual/song             | &nbsp; |
**String Colors** | &nbsp;            | hex defined color (eg. FF0000) | &nbsp; |
&nbsp;  | string0_N |     &nbsp;      | Colors used in non-ER songs |
&nbsp;  |     ...           |     &nbsp;      | &nbsp; |
&nbsp;  | string5_N |     &nbsp;      | &nbsp; |
&nbsp;  | string0_CB |     &nbsp;      | Colors used in ER songs |
&nbsp;  |     ...           |     &nbsp;      | &nbsp; |
&nbsp;  | string5_CB |     &nbsp;      | &nbsp; |
**Mod Settings** | &nbsp;            | &nbsp;          | &nbsp; |
ExtendedRangeModeAt         | numerical value         | Offset to E Standard (-1 = Eb, -5 = B) |
CheckForNewSongsInterval  | interval in milliseconds  | Time between each check |
**Guitar Speak** | &nbsp;            | &nbsp;          | &nbsp; |
&nbsp;  | GuitarSpeakWhileTuning     | on/off          | Is GuitarSpeak enabled while tuning (ONLY FOR ADVANCED USERS) |
&nbsp;  | GuitarSpeakDeleteWhen | Numeric index | Position in note array (using GUI recommended for this)
&nbsp;  | GuitarSpeakSpaceWhen | &nbsp;          | &nbsp; |
&nbsp;  | GuitarSpeakEnterWhen | &nbsp;          | &nbsp; |
&nbsp;  | GuitarSpeakTabWhen | &nbsp;          | &nbsp; |
&nbsp;  | GuitarSpeakPGUPWhen | &nbsp;          | &nbsp; |
&nbsp;  | GuitarSpeakPGDNWhen | &nbsp;          | &nbsp; |
&nbsp;  | GuitarSpeakUPWhen | &nbsp;          | &nbsp; |
&nbsp;  | GuitarSpeanDNWhen | &nbsp;          | &nbsp; |
&nbsp;  | GuitarSpeakESCWhen | &nbsp;          | &nbsp; | 
&nbsp;  | GuitarSpeakCloseWhen | &nbsp;          | &nbsp; |
&nbsp;  | GuitarSpeakOBracketWhen | &nbsp;          | &nbsp; |
&nbsp;  | GuitarSpeakCBracketWhen | &nbsp;          | &nbsp; |
&nbsp;  | GuitarSpeakTildeaWhen | &nbsp;          | &nbsp; | 
&nbsp;  | GuitarSpeakForSlashWhen | &nbsp;          | &nbsp; |

* The available keys for the keybinding section can be seen here: ![Visual Representation](https://i.imgur.com/lpNv3yG.png) You must follow the V-Key format available here: https://docs.microsoft.com/en-us/windows/win32/inputdev/virtual-key-codes (yes that includes the VK_ part). Ex. F3 would be VK_F3, and the music play/ pause button would be VK_MEDIA_PLAY_PAUSE. We understand that us locking down the amount of keys may aggrevate some of you but we want to allow you to search for songs without turning on/ off your mods when you search for "Slipknot". **If you are a streamer and have an Elgato Stream Deck** set your keybinds to the F13-F24 keys as most keyboards don't have those keys but we allow them to be used for keybinds.

* String Numbers go from 0-5 as this is zero-index, or how computers normally work. A translation of this is: 0. low E, 1. A, 2. D, 3. G, 4. B, 5. high E.

* Guitar Speak note values are measured from C-1 to C6 starting at 0 and moving up to 96. Ex. The low E string at the 12th fret is E3, and would translate to the number 52.

* GuitarSpeakWhileTuning is only for advanced users because if it is on and you have one of your GuitarSpeak values set wrong it can prevent you from tuning or playing this game. This decision was made to prevent people from uninstalling the mod because GuitarSpeak stops them from rocking out.

* Song list names should be below 25 characters (including spaces) because the text streches out and gets hard to read.


## FAQ

* Q: I want something added to the mod, like I want to remove XYZ from the game!!!
* A: Post a request in the [Issues tab of this Github repo](https://github.com/Lovrom8/RSCDLCEnabler-2020Edition/issues) with the name "DLL - FR - XYZ". Please follow this format so we know what we are doing, either fixing something or adding a new feature. Note: Just because you post a request for us to do something doesn't mean we can do it and/ or we will do it. Please don't keep begging us to do your request, we will get to it if we think it's worth our time spent.


* Q: I found a bug / somethings not working as it should! How do I get this fixed?
* A: Please post an issue in the [Issues tab of this Github repo](https://github.com/Lovrom8/RSCDLCEnabler-2020Edition/issues) with the name "DLL - Bug - XYZ" if the bug is in Rocksmith or "GUI - Bug - XYZ" if the bug is in the tool to mod your game. Please try to be descriptive in this, as posting "It don't work" doesn't help us. Screenshots and/ or videos can help, but steps on what you did to get it to break would be greatly appreciated. Sometimes bugs are where we get our best features :)


* Q: Who made these mods, and how can I thank you guys?
* A: Thank you for showing interest in the project. We spent a good two months working on this project trying to flesh it out so people would enjoy using it. The developers are mainly: LovroM8 (Lovro), and Ffio1 (Ffio) but we received a lot of help from ZagatoZee (ZZ), Kokolihapihvi (Koko), and L0fka. All we ask for is a quick thank you, and those can either be sent by starring this project on Github, telling your Rocksmith friends, or just coming into the Discord and saying "Thank You". I know it sounds like it's not much, but it really makes our day(s).

* If you have any questions, feel free to reach us in the [r/Rocksmith Discord](https://rocksmith.rocks/discord) in the #rsmods channel, or in the #general channel of the [CustomsForge Discord](https://discord.gg/cf)

