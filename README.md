# Part of RSCDLCEnabler-2020Edition that nobody asked for

[![Build status](https://ci.appveyor.com/api/projects/status/3ivrpj6l6ntv35gp?svg=true)](https://ci.appveyor.com/project/Lovrom8/rscdlcenabler-2020edition)

## Branches:
1) [Future](https://github.com/Lovrom8/RSCDLCEnabler-2020Edition/tree/Future) - proxy DLL for D3DX9_42.dll - [latest build](https://ci.appveyor.com/api/projects/Lovrom8/rscdlcenabler-2020edition/artifacts/DLL.zip?branch=Future)
2) [GUI](https://github.com/Lovrom8/tree/RSCDLCEnabler-2020Edition/tree/GUI) - used to customize settings for the internal mod and to add set-and-forget mods - [latest build](https://ci.appveyor.com/api/projects/Lovrom8/rscdlcenabler-2020edition/artifacts/RSMods.zip?branch=GUI)
3) [One-Click-Installer](https://github.com/Lovrom8/RSCDLCEnabler-2020Edition/tree/One-Click-Installer) - one big button :) - [latest build](https://github.com/Lovrom8/RSCDLCEnabler-2020Edition/releases/latest)

## Features:

* **Extended Range Mode**
  * Rocksmith by default doesn't officialy support 7 (or more) string guitars, even though a few low-tuned songs have been released as official DLC. 
    Hence with the power of DX9, GDI+ and some smart reverse engineering, when it detects that a song is in a tuning lower than the set treshold, the color scheme    will change accordingly. What that means is that no more will your brain be confused because the lowest string is colored red (usually indicating the low E string), while you actually need to play the low B! So effectively all the strings will be shifted one place down.
   * **Options**
   1. ZZ's color set - this mode will replicate the default color of high B string (green) and make it match the color of the low B string (7-th string) - [See the ER mode in action](https://www.youtube.com/watch?v=FPjFwt-Dpdo)
   2. Custom color set - defined by you as the colorblind color set in the settings, it will be used only in extended range mode songs, while regular songs will use the normal colors
      
* **Custom Song List Titles**
  * Normally those are not customizable in-game and simply listed as SONG LIST 1 to SONG LIST 6. Now you can customize those, in for example, a song list for only B Standard songs, only Excercise songs, etc.
  
* **Add/Decrease Song Volume**
  * In case you are playing a song which is unusually low in volume, or is ear-piercingly loud, by invoking the functions of AudioKinetic audio engine used by the game, you can now modify volume of the music on the fly, without going in to the mixer 
  
* **Toggle Loft**
  * If you are a streamer, this one will especially come in handy. The background behind the noteway (be it a crowd in the venue or just a plain wall), can now be removed on the fly with a dark background
  * **Options** - loft can be automatically toggled off on startup or when entering a song, or by pressing the hotkey
  
* **Force ReEnumeration**
  * Normally after adding a new song to your collection, you would have had to either restart the game, or enter the in-game Shop in order for the game to recognize the new songs.
  * **Options**
  1. Automatic - check if any new songs have been added every X seconds (even while a song is playing!)
  2. By going into Enumerate menu - SHOP in the menu has now been replaced with **Enumerate** and conveniently moved to the second place **(GUI only)**
  3. By hotkey - press a hotkey to force the game to enumerate your songs

* **Removal of certain visual game elements**
  * Again something which may come in handy if you are a streamer (or just prefer your screen to be cluttered the least possible), you can now remove certain elements from the screen
  * **Options**
  1. Headstock
  2. Skyline
  3. Frets
  4. Inlays
  5. Lane Markers
  6. Lyrics
  
* **Rainbow Strings**
  * A fun mod which will continously hue-shift the colors of your strings, making them look like rainbow! 
  
* **GuitarSpeak**
  * What hides behind this mysterious name is an exciting feature that will let you control the game by playing certain notes on your guitar (fully customizable!). Forget your keyboard and mouse, guitar is where it's at!  

* **Auto enter last used profile**
  * Also know as the Fork-in-the-toaster mod, due to it's simple but effective nature, it is best used in conjunction with the Fast Load mod. The DLL will spam Enter key in order to automatically enter the game. It may cause potential issues if UPLAY servers are unavailable, but in general it makes your life quite a bit easier.

* **CDLC Enabler**
  * After all, this mod is based on the proxy DLL which bypassed the check which enabled users to play non-official songs in the game.

* **Fast Load** - **GUI Only**
  * If you are running the game of a SSD or especially a NVMe SSD drive, you will enjoy this one - it skips the intro screens and lets you load the game in matter of seconds. It can be fairly unstable, but in general it should work provided you don't used it on a good old HDD
  
* **Custom Tunings** - **GUI Only**
  * By default, the game has a fairly limited set of tunings it can recongnize and in cases where it doesn't find in the list, it will just display CUSTOM TUNING. And that isn't of much help, is it? But don't worry, you can now make the game know that a B Standard song is actually B Standard, and not just _Custom Tuning_ :(
  Together with the list we include, you can add your own tunings if you find some which aren't included in the list.
  
* **EXIT GAME in the menu** - **GUI Only**
  * As useful of a device your mouse is, it is not really the most convenient option when you want to exit the game. And until now, you had to use the said mouse to do that, but fret no more. It took only six and a half years, but now you can exit the game by pressing EXIT GAME in the menu (which replaces the UPLAY button, and let's be honest, nooone used that)
  
* **Change Default tones** - **GUI Only**
  * Add your favorite tones to the tone wheel, and by enabling this mod, your game will use the tone you picked amongst the four as soon as you enter the game. Nifty, isn't it?

## Installation:
* There's two options as how this mod can be installed:
1. Manually build/copy the DLL to the root folder of Rocksmith 2014 RM, create a file called RS.ini and fill it with options as [shown below](https://github.com/Lovrom8/RSCDLCEnabler-2020Edition#settings)
   If it is installed in the most common folder (C drive, Program Files/Steam/Steamapps), VS will attempt to copy the DLL to the folder for your convenience.
2. Use the One-Click-Installer to copy both the DLL and RSMods GUI to the game folder. If it's unable to automatically detect where Rocksmith is installed, it will ask you to point it to the correct folder.
  
## Requirements:
* Latest Steam version of Rocksmith 2014 Remastered, https://store.steampowered.com/app/221680/Rocksmith_2014_Edition__Remastered/
* MS Visual C++ 2015-2019 Redistributable for the DLL, .NET framework for GUI/One-Click-Installer
* Sorry Mac users, RS on Mac is it's own beast altogether, so we only support the Windows version
  
## Dependencies:
* DirectX 9 SDK, ImGUI, GDI+, Detours - all of which are included in the project folder and should require no additional installations to compile and use the project
* Setup as C++17 / VS2019 project

## Settings:

If you want to manually create the settings file for the DLL, the general structure should be as follows:

Section        | Entry             | Possible values | Info
-------------- | ----------------- | --------------- | ------ |
**SongListTitles** | &nbsp;            | &nbsp;          | &nbsp; |
 &nbsp;    | SongListTitles_1  |  _user defined string_  | &nbsp; |
 &nbsp;        |     ...           |     &nbsp;      | &nbsp; |
 &nbsp;        | SongListTitles_6  |  _user defined string_ | &nbsp; |       
 **Keybinds** | &nbsp;            | &nbsp;          | &nbsp; |
&nbsp;    | ToggleLoftKey | letter (F, G, H) / function key (F1, F10) | &nbsp; |
&nbsp;    | AddVolumeKey | letter (F, G, H) / function key (F1, F10) | &nbsp; |
&nbsp;    | DecreaseVolumeKey | letter (F, G, H) / function key (F1, F10)  | &nbsp; |
&nbsp;    | ShowSongTimerKey | letter (F, G, H) / function key (F1, F10) | &nbsp; |
&nbsp;    | ForceReEnumerationKey | letter (F, G, H) / function key (F1, F10) | &nbsp; | 
&nbsp;    | RainbowStringsKey | letter (F, G, H) / function key (F1, F10) | &nbsp; |
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
&nbsp;  | string6_N |     &nbsp;      | &nbsp; |
&nbsp;  | string0_CB |     &nbsp;      | Colors used in ER songs |
&nbsp;  |     ...           |     &nbsp;      | &nbsp; |
&nbsp;  | string6_CB |     &nbsp;      | &nbsp; |
**Mod Settings** | &nbsp;            | &nbsp;          | &nbsp; |
ExtendedRangeModeAt         | numerical value         | Offset to E Standard (-1 = EB, -5 = B) |
CheckForNewSongsInterval  | interval in milliseconds  | Time between each check |
**Guitar Speak** | &nbsp;            | &nbsp;          | &nbsp; |
&nbsp;  | GuitarSpeakWhileTuning     | &nbsp;          | Is GuitarSpeak enabled while tuning |
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

## FAQ

If you have any questions, feel free to reach us on [Rocksmith Discord](https://rocksmith.rocks/discord) and #rsmods channel.

