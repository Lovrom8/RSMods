using System;
using System.IO;
using System.Windows.Forms;
using System.Diagnostics;
using System.Collections.Generic;
using RSMods.Util;
using System.Drawing;
using System.Security.Cryptography;
using System.Linq;

namespace RSMods
{
    class WriteSettings
    {
        // Default Colors
        public static Color defaultBackgroundColor = Color.Azure;
        public static Color defaultTextColor = Color.Black;
        public static Color defaultButtonColor = SystemColors.ControlLight;


        /// <summary>
        /// Hash for Rocksmith2014.exe for the Remastered Update | SHA256
        /// </summary>
        readonly static byte[] HASH_EXE = { 0xA7, 0x25, 0x84, 0x61, 0x10, 0x1D, 0xA0, 0x20, 0x17, 0x07, 0xF5, 0xC2, 0x72, 0xBA, 0xAA, 0x62, 0xA3, 0xD3, 0xD1, 0x0B, 0x3D, 0x22, 0x13, 0xC0, 0xD0, 0xF2, 0x1C, 0xC8, 0x3B, 0x45, 0x88, 0xDA };

        #region Save Settings

        public static Dictionary<string, Dictionary<string, string>> saveSettingsOrDefaults = new Dictionary<string, Dictionary<string, string>>()
        {
            {"[SongListTitles]", new Dictionary<string, string> {
                { ReadSettings.Songlist1Identifier, CreateDefaultOnOldINI(ReadSettings.Songlist1Identifier, "Define Song List 1 Here") }, // Songlist 1
                { ReadSettings.Songlist2Identifier, CreateDefaultOnOldINI(ReadSettings.Songlist2Identifier, "Define Song List 2 Here") }, // Songlist 2
                { ReadSettings.Songlist3Identifier, CreateDefaultOnOldINI(ReadSettings.Songlist3Identifier, "Define Song List 3 Here") }, // Songlist 3
                { ReadSettings.Songlist4Identifier, CreateDefaultOnOldINI(ReadSettings.Songlist4Identifier, "Define Song List 4 Here") }, // Songlist 4
                { ReadSettings.Songlist5Identifier, CreateDefaultOnOldINI(ReadSettings.Songlist5Identifier, "Define Song List 5 Here") }, // Songlist 5
                { ReadSettings.Songlist6Identifier, CreateDefaultOnOldINI(ReadSettings.Songlist6Identifier, "Define Song List 6 Here") }, // Songlist 6
            }},
            {"[Keybinds]", new Dictionary<string, string> {
                { ReadSettings.ToggleLoftIdentifier, CreateDefaultOnOldINI(ReadSettings.ToggleLoftIdentifier, "") }, // Toggle Loft
                { ReadSettings.ShowSongTimerIdentifier, CreateDefaultOnOldINI(ReadSettings.ShowSongTimerIdentifier, "") }, // Show Song Timer
                { ReadSettings.ForceReEnumerationIdentifier, CreateDefaultOnOldINI(ReadSettings.ForceReEnumerationIdentifier, "") }, // Force ReEnumeration
                { ReadSettings.RainbowStringsIdentifier, CreateDefaultOnOldINI(ReadSettings.RainbowStringsIdentifier, "") }, // Rainbow Strings
                { ReadSettings.RainbowNotesIdentifier, CreateDefaultOnOldINI(ReadSettings.RainbowNotesIdentifier, "") }, // Rainbow Notes
                { ReadSettings.RemoveLyricsKeyIdentifier, CreateDefaultOnOldINI(ReadSettings.RemoveLyricsKeyIdentifier, "") }, // Remove Lyrics Key
                { ReadSettings.RRSpeedKeyIdentifier, CreateDefaultOnOldINI(ReadSettings.RRSpeedKeyIdentifier, "") }, // Riff Repeater Above 100% key.
                { ReadSettings.TuningOffsetKeyIdentifier, CreateDefaultOnOldINI(ReadSettings.TuningOffsetKeyIdentifier, "") }, // Change Auto Tune For Song Offset Ingame.
                { ReadSettings.ToggleExtendedRangeKeyIdentifier, CreateDefaultOnOldINI(ReadSettings.ToggleExtendedRangeKeyIdentifier, "") }, // Toggle ER Mode on / off
            }},
            {"[Audio Keybindings]", new Dictionary<string, string>
            {
                { ReadSettings.MasterVolumeKeyIdentifier, CreateDefaultOnOldINI(ReadSettings.MasterVolumeKeyIdentifier, "") }, // Master Volume (Hidden in the in-game mixer)
                { ReadSettings.SongVolumeKeyIdentifier, CreateDefaultOnOldINI(ReadSettings.SongVolumeKeyIdentifier, "") }, // Song Volume
                { ReadSettings.Player1VolumeKeyIdentifier, CreateDefaultOnOldINI(ReadSettings.Player1VolumeKeyIdentifier, "") }, // Player 1 Guitar / Bass Volume
                { ReadSettings.Player2VolumeKeyIdentifier, CreateDefaultOnOldINI(ReadSettings.Player2VolumeKeyIdentifier, "") }, // Player 2 Guitar / Bass Volume
                { ReadSettings.MicrophoneVolumeKeyIdentifier, CreateDefaultOnOldINI(ReadSettings.MicrophoneVolumeKeyIdentifier, "") }, // Microphone Volume
                { ReadSettings.VoiceOverVolumeKeyIdentifier, CreateDefaultOnOldINI(ReadSettings.VoiceOverVolumeKeyIdentifier, "") }, // Rocksmith Dad Volume
                { ReadSettings.SFXVolumeKeyIdentifier, CreateDefaultOnOldINI(ReadSettings.SFXVolumeKeyIdentifier, "") }, // SFX Volume
                { ReadSettings.ChangeSelectedVolumeKeyIdentifier, CreateDefaultOnOldINI(ReadSettings.ChangeSelectedVolumeKeyIdentifier, "") }, // Change Selected Volume
            }},
            {"[Toggle Switches]", new Dictionary<string, string>
            {
                { ReadSettings.ToggleLoftEnabledIdentifier, CreateDefaultOnOldINI(ReadSettings.ToggleLoftEnabledIdentifier, "off") }, // Toggle Loft Enabled / Disabled
                { ReadSettings.VolumeControlEnabledIdentifier, CreateDefaultOnOldINI(ReadSettings.VolumeControlEnabledIdentifier, "off") }, // Volume Control Enabled / Disabled
                { ReadSettings.ShowSongTimerEnabledIdentifier, CreateDefaultOnOldINI(ReadSettings.ShowSongTimerEnabledIdentifier, "off") }, // Show Song Timer Enabled / Disabled
                { ReadSettings.ForceReEnumerationEnabledIdentifier, CreateDefaultOnOldINI(ReadSettings.ForceReEnumerationEnabledIdentifier, "off") }, // Force ReEnumeration Manual / Automatic / Disabled
                { ReadSettings.RainbowStringsEnabledIdentifier, CreateDefaultOnOldINI(ReadSettings.RainbowStringsEnabledIdentifier, "off") }, // Rainbow String Enabled / Disabled
                { ReadSettings.RainbowNotesEnabledIdentifier, CreateDefaultOnOldINI(ReadSettings.RainbowNotesEnabledIdentifier, "off") }, // Rainbow Notes Enabled / Disabled
                { ReadSettings.ExtendedRangeEnabledIdentifier, CreateDefaultOnOldINI(ReadSettings.ExtendedRangeEnabledIdentifier, "off") }, // Extended Range Enabled / Disabled
                { ReadSettings.ExtendedRangeDropTuningIdentifier, CreateDefaultOnOldINI(ReadSettings.ExtendedRangeDropTuningIdentifier, "off") }, // Extended Range on Drop Tunings (Drop A, Drop B, etc)
                { ReadSettings.ExtendedRangeFixBassTuningIdentifier, CreateDefaultOnOldINI(ReadSettings.ExtendedRangeFixBassTuningIdentifier, "off") }, // Fix bad chart's tuning being wrong for strings 4 and 5.
                { ReadSettings.CustomStringColorNumberIndetifier, CreateDefaultOnOldINI(ReadSettings.CustomStringColorNumberIndetifier, "0") }, // Custom String Colors (0 - Default, 1 - ZZ, 2 - Custom Colors)
                { ReadSettings.SeparateNoteColorsIdentifier, CreateDefaultOnOldINI(ReadSettings.SeparateNoteColorsIdentifier, "off") }, // Should we use Separate Note Colors?
                //{ ReadSettings.DiscoModeIdentifier, CreateDefaultOnOldINI(ReadSettings.DiscoModeIdentifier, "off") }, // Disco Mode Enabled / Disabled
                { ReadSettings.RemoveHeadstockIdentifier, CreateDefaultOnOldINI(ReadSettings.RemoveHeadstockIdentifier, "off") }, // Remove Headstock Enabled / Disabled
                { ReadSettings.RemoveSkylineIdentifier, CreateDefaultOnOldINI(ReadSettings.RemoveSkylineIdentifier, "off") }, // Remove Skyline Enabled / Disabled
                { ReadSettings.GreenScreenWallIdentifier, CreateDefaultOnOldINI(ReadSettings.GreenScreenWallIdentifier, "off") }, // Greenscreen Back Wall Enabled / Disabled
                { ReadSettings.ForceProfileEnabledIdentifier, CreateDefaultOnOldINI(ReadSettings.ForceProfileEnabledIdentifier, "off") }, // Force Load Profile On Game Boot Enabled / Disabled
                { ReadSettings.FretlessModeEnabledIdentifier, CreateDefaultOnOldINI(ReadSettings.FretlessModeEnabledIdentifier, "off") }, // Fretless Mode Enabled / Disabled
                { ReadSettings.RemoveInlaysIdentifier, CreateDefaultOnOldINI(ReadSettings.RemoveInlaysIdentifier, "off") }, // Remove Inlay Markers Enabled / Disabled
                { ReadSettings.ToggleLoftWhenIdentifier, CreateDefaultOnOldINI(ReadSettings.ToggleLoftWhenIdentifier, "manual") }, // Define how or when the loft is disabled - game startup, on key command, or in song only
                { ReadSettings.RemoveLaneMarkersIdentifier, CreateDefaultOnOldINI(ReadSettings.RemoveLaneMarkersIdentifier, "off") }, // Remove Lane Markers Enabled / Disabled
                { ReadSettings.ToggleSkylineWhenIdentifier, CreateDefaultOnOldINI(ReadSettings.ToggleSkylineWhenIdentifier, "song") }, // Define how or when the skyline is disabled - game startup, or in song only
                { ReadSettings.RemoveLyricsIdentifier, CreateDefaultOnOldINI(ReadSettings.RemoveLyricsIdentifier,"off") }, // Remove Song Lyrics Enabled / Disabled
                { ReadSettings.RemoveLyricsWhenIdentifier, CreateDefaultOnOldINI(ReadSettings.RemoveLyricsWhenIdentifier,"manual") }, // When should we remove lyrics Manual / Automatic
                { ReadSettings.GuitarSpeakIdentifier, CreateDefaultOnOldINI(ReadSettings.GuitarSpeakIdentifier, "off") }, // Guitar Speak Enabled / Disabled
                { ReadSettings.RemoveHeadstockWhenIdentifier, CreateDefaultOnOldINI(ReadSettings.RemoveHeadstockWhenIdentifier, "song") }, // Remove Headstock When Startup / Song
                { ReadSettings.ScreenShotScoresIdentifier, CreateDefaultOnOldINI(ReadSettings.ScreenShotScoresIdentifier, "off") }, // Screenshot Scores When You Finish A Song
                { ReadSettings.RiffRepeaterAboveHundredIdentifier, CreateDefaultOnOldINI(ReadSettings.RiffRepeaterAboveHundredIdentifier, "off") }, // Allow the user to go above 100% speed in LAS Riff Repeater
                { ReadSettings.MidiAutoTuningIdentifier, CreateDefaultOnOldINI(ReadSettings.MidiAutoTuningIdentifier, "off") }, // Should we be ready to send MIDI signals to a device.
                { ReadSettings.MidiAutoTuningDeviceIdentifier, CreateDefaultOnOldINI(ReadSettings.MidiAutoTuningDeviceIdentifier, "") }, // Device we should send MIDI signals to.
                { ReadSettings.MidiSoftwareSettingsIdentifier, CreateDefaultOnOldINI(ReadSettings.MidiSoftwareSettingsIdentifier, "") }, // Used to configure a Software Tuning Pedal. Format: OffChannel, PC or CC, TrueTuning Channel, CC Channel (optional | defaults to 0). Ex: "0, CC, 20, 25" will send a CC to channel 25 as defined in SoftwareTriggers, and when turning off it will send a CC 0 to channel 20. If the song has true tuning, it will send the true tuning value (0-80 -> maps to 400-480) to CC channel 20.
                { ReadSettings.MidiSoftwareTriggersIdentifier, CreateDefaultOnOldINI(ReadSettings.MidiSoftwareTriggersIdentifier, "") }, // Used to figure out Semitone <-> Control Change / Program Change with a Software Tuning Pedal. Ex: "0 66, 1 77, -2 22" will send a PC or CC, as specified in SoftwareSettings, of 66 when the tuning should be default, 77 when we need to go up 1 semitone, and 22 when going down 2 semitones.
                { ReadSettings.ChordsModeIdentifier, CreateDefaultOnOldINI(ReadSettings.ChordsModeIdentifier, "off") }, // Is the pedal using Chords Mode.
                { ReadSettings.ShowCurrentNoteOnScreenIdentifier, CreateDefaultOnOldINI(ReadSettings.ShowCurrentNoteOnScreenIdentifier, "off") }, // Display Current Note Name
                { ReadSettings.OnScreenFontIdentifier, CreateDefaultOnOldINI(ReadSettings.OnScreenFontIdentifier, "Arial") }, // Font to use when we show text over Rocksmith
                { ReadSettings.ProfileToLoadIdentifier, CreateDefaultOnOldINI(ReadSettings.ProfileToLoadIdentifier, "") }, // Default profile to load.
                { ReadSettings.ShowSongTimerWhenIdentifier, CreateDefaultOnOldINI(ReadSettings.ShowSongTimerWhenIdentifier, "manual") }, // Should we start the game with song timer on.
                { ReadSettings.ShowSelectedVolumeWhenIdentifier, CreateDefaultOnOldINI(ReadSettings.ShowSelectedVolumeWhenIdentifier, "manual") }, // Should we always show the selected volume.
                { ReadSettings.SecondaryMonitorIdentifier, CreateDefaultOnOldINI(ReadSettings.SecondaryMonitorIdentifier, "off")} // Should we allow a secondary monitor to be used for Rocksmith?
            }},
            {"[String Colors]", new Dictionary<string, string>
            {
                { ReadSettings.String0Color_N_Identifier, CreateDefaultOnOldINI(ReadSettings.String0Color_N_Identifier, "ff4f5a") }, // Default Low E String Color (HEX) | Red
                { ReadSettings.String1Color_N_Identifier, CreateDefaultOnOldINI(ReadSettings.String1Color_N_Identifier, "e2c102") }, // Default A String Color (HEX) | Yellow
                { ReadSettings.String2Color_N_Identifier, CreateDefaultOnOldINI(ReadSettings.String2Color_N_Identifier, "1dacf9") }, // Default D String Color (HEX) | Blue
                { ReadSettings.String3Color_N_Identifier, CreateDefaultOnOldINI(ReadSettings.String3Color_N_Identifier, "ff9216") }, // Default G String Color (HEX) | Orange
                { ReadSettings.String4Color_N_Identifier, CreateDefaultOnOldINI(ReadSettings.String4Color_N_Identifier, "3fcc0c") }, // Default B String Color (HEX) | Green
                { ReadSettings.String5Color_N_Identifier, CreateDefaultOnOldINI(ReadSettings.String5Color_N_Identifier, "c825ed") }, // Default High E String Color (HEX) | Purple

                { ReadSettings.String0Color_CB_Identifier, CreateDefaultOnOldINI(ReadSettings.String0Color_CB_Identifier, "00c68e") }, // Colorblind Low E String Color (HEX) | Red
                { ReadSettings.String1Color_CB_Identifier, CreateDefaultOnOldINI(ReadSettings.String1Color_CB_Identifier, "ff4f5a") }, // Colorblind A String Color (HEX) | Yellow
                { ReadSettings.String2Color_CB_Identifier, CreateDefaultOnOldINI(ReadSettings.String2Color_CB_Identifier, "e2c102") }, // Colorblind D String Color (HEX) | Blue
                { ReadSettings.String3Color_CB_Identifier, CreateDefaultOnOldINI(ReadSettings.String3Color_CB_Identifier, "1dacf9") }, // Colorblind G String Color (HEX) | Orange
                { ReadSettings.String4Color_CB_Identifier, CreateDefaultOnOldINI(ReadSettings.String4Color_CB_Identifier, "ff9216") }, // Colorblind B String Color (HEX) | Green
                { ReadSettings.String5Color_CB_Identifier, CreateDefaultOnOldINI(ReadSettings.String5Color_CB_Identifier, "3fcc0c") }, // Colorblind High E String Color (HEX) | Purple

                { ReadSettings.Note0Color_N_Identifier, CreateDefaultOnOldINI(ReadSettings.Note0Color_N_Identifier, "ff4f5a") }, // Default Low E Note Color (HEX) | Red
                { ReadSettings.Note1Color_N_Identifier, CreateDefaultOnOldINI(ReadSettings.Note1Color_N_Identifier, "e2c102") }, // Default A Note Color (HEX) | Yellow
                { ReadSettings.Note2Color_N_Identifier, CreateDefaultOnOldINI(ReadSettings.Note2Color_N_Identifier, "1dacf9") }, // Default D Note Color (HEX) | Blue
                { ReadSettings.Note3Color_N_Identifier, CreateDefaultOnOldINI(ReadSettings.Note3Color_N_Identifier, "ff9216") }, // Default G Note Color (HEX) | Orange
                { ReadSettings.Note4Color_N_Identifier, CreateDefaultOnOldINI(ReadSettings.Note4Color_N_Identifier, "3fcc0c") }, // Default B Note Color (HEX) | Green
                { ReadSettings.Note5Color_N_Identifier, CreateDefaultOnOldINI(ReadSettings.Note5Color_N_Identifier, "c825ed") }, // Default High E Note Color (HEX) | Purple

                { ReadSettings.Note0Color_CB_Identifier, CreateDefaultOnOldINI(ReadSettings.Note0Color_CB_Identifier, "00c68e") }, // Colorblind Low E Note Color (HEX) | Red
                { ReadSettings.Note1Color_CB_Identifier, CreateDefaultOnOldINI(ReadSettings.Note1Color_CB_Identifier, "ff4f5a") }, // Colorblind A Note Color (HEX) | Yellow
                { ReadSettings.Note2Color_CB_Identifier, CreateDefaultOnOldINI(ReadSettings.Note2Color_CB_Identifier, "e2c102") }, // Colorblind D Note Color (HEX) | Blue
                { ReadSettings.Note3Color_CB_Identifier, CreateDefaultOnOldINI(ReadSettings.Note3Color_CB_Identifier, "1dacf9") }, // Colorblind G Note Color (HEX) | Orange
                { ReadSettings.Note4Color_CB_Identifier, CreateDefaultOnOldINI(ReadSettings.Note4Color_CB_Identifier, "ff9216") }, // Colorblind B Note Color (HEX) | Green
                { ReadSettings.Note5Color_CB_Identifier, CreateDefaultOnOldINI(ReadSettings.Note5Color_CB_Identifier, "3fcc0c") }, // Colorblind High E Note Color (HEX) | Purple

            }},
            {"[Mod Settings]", new Dictionary<string, string>
            {
                { ReadSettings.ExtendedRangeTuningIdentifier, CreateDefaultOnOldINI(ReadSettings.ExtendedRangeTuningIdentifier, "-5")}, // Enable Extended Range Mode When Low E Is X Below E
                { ReadSettings.CheckForNewSongIntervalIdentifier, CreateDefaultOnOldINI(ReadSettings.CheckForNewSongIntervalIdentifier, "5000") }, // Enumerate new songs every X ms
                { ReadSettings.RiffRepeaterSpeedIntervalIdentifier, CreateDefaultOnOldINI(ReadSettings.RiffRepeaterSpeedIntervalIdentifier, "2") }, // The rate of how much one key press should gain the Riff Repeater speed.
                { ReadSettings.TuningPedalIdentifier, CreateDefaultOnOldINI(ReadSettings.TuningPedalIdentifier, "") }, // What tuning pedal does the user use?
                { ReadSettings.MidiTuningOffsetIdentifier, CreateDefaultOnOldINI(ReadSettings.MidiTuningOffsetIdentifier, "0") }, // Offset from 0 (E Standard) to 12 (E Standard +OCT) to adjust Midi Auto Tuning for.
                { ReadSettings.VolumeControlIntervalIdentifier, CreateDefaultOnOldINI(ReadSettings.VolumeControlIntervalIdentifier, "5") }, // By what number should we increase / decrease the volume.
                { ReadSettings.SecondaryMonitorXPositionIdentifier, CreateDefaultOnOldINI(ReadSettings.SecondaryMonitorXPositionIdentifier, "0") }, // Where should we place Rocksmith on their secondary monitor. X
                { ReadSettings.SecondaryMonitorYPositionIdentifier, CreateDefaultOnOldINI(ReadSettings.SecondaryMonitorYPositionIdentifier, "0") }, // Where should we place Rocksmith on their secondary monitor. Y
                { ReadSettings.SeparateNoteColorsModeIdentifier, CreateDefaultOnOldINI(ReadSettings.SeparateNoteColorsModeIdentifier, "0") }, // Should we display different colors on the notes, than on the strings? 0 = No | 1 = Default Colors | 2 = Custom
            }},
            {"[Guitar Speak]", new Dictionary<string, string>
            {
                { ReadSettings.GuitarSpeakDeleteIdentifier, CreateDefaultOnOldINI(ReadSettings.GuitarSpeakDeleteIdentifier, "") }, // Guitar Speak Press Delete When Midi Note = (0{C-1} - 96{C7})
                { ReadSettings.GuitarSpeakSpaceIdentifier, CreateDefaultOnOldINI(ReadSettings.GuitarSpeakSpaceIdentifier, "") }, // Guitar Speak Press Space " " When Midi Note = (0{C-1} - 96{C7})
                { ReadSettings.GuitarSpeakEnterIdentifier, CreateDefaultOnOldINI(ReadSettings.GuitarSpeakEnterIdentifier, "") }, // Guitar Speak Press Enter / Return When Midi Note = (0{C-1} - 96{C7})
                { ReadSettings.GuitarSpeakTabIdentifier, CreateDefaultOnOldINI(ReadSettings.GuitarSpeakTabIdentifier, "") }, // Guitar Speak Press Tab When Midi Note = (0{C-1} - 96{C7})
                { ReadSettings.GuitarSpeakPGUPIdentifier, CreateDefaultOnOldINI(ReadSettings.GuitarSpeakPGUPIdentifier, "") }, // Guitar Speak Press Page Up When Midi Note = (0{C-1} - 96{C7})
                { ReadSettings.GuitarSpeakPGDNIdentifier, CreateDefaultOnOldINI(ReadSettings.GuitarSpeakPGDNIdentifier, "") }, // Guitar Speak Press Page Down When Midi Note = (0{C-1} - 96{C7})
                { ReadSettings.GuitarSpeakUPIdentifier, CreateDefaultOnOldINI(ReadSettings.GuitarSpeakUPIdentifier, "") }, // Guitar Speak Press Up Arrow When Midi Note = (0{C-1} - 96{C7})
                { ReadSettings.GuitarSpeakDNIdentifier, CreateDefaultOnOldINI(ReadSettings.GuitarSpeakDNIdentifier, "") }, // Guitar Speak Press Down Arrow When Midi Note = (0{C-1} - 96{C7})
                { ReadSettings.GuitarSpeakESCIdentifier, CreateDefaultOnOldINI(ReadSettings.GuitarSpeakESCIdentifier, "") }, // Guitar Speak Press Escape When Midi Note = (0{C-1} - 96{C7})
                { ReadSettings.GuitarSpeakCloseIdentifier, CreateDefaultOnOldINI(ReadSettings.GuitarSpeakCloseIdentifier, "") }, // Stop using Guitar Speak when Midi Note = (0{C-1} - 96{C7})
                { ReadSettings.GuitarSpeakOBracketIdentifier, CreateDefaultOnOldINI(ReadSettings.GuitarSpeakOBracketIdentifier, "") }, // Guitar Speak Press Open Bracket "{" When Midi Note = (0{C-1} - 96{C7})
                { ReadSettings.GuitarSpeakCBracketIdentifier, CreateDefaultOnOldINI(ReadSettings.GuitarSpeakCBracketIdentifier, "") }, // Guitar Speak Press Close Bracket "}" When Midi Note = (0{C-1} - 96{C7})
                { ReadSettings.GuitarSpeakTildeaIdentifier, CreateDefaultOnOldINI(ReadSettings.GuitarSpeakTildeaIdentifier, "") }, // Guitar Speak Press Tilde / Tilda "~" When Midi Note = (0{C-1} - 96{C7})
                { ReadSettings.GuitarSpeakForSlashIdentifier, CreateDefaultOnOldINI(ReadSettings.GuitarSpeakForSlashIdentifier, "") }, // Guitar Speak Press Forward Slash "/" When Midi Note = (0{C-1} - 96{C7})
                { ReadSettings.GuitarSpeakAltIdentifier, CreateDefaultOnOldINI(ReadSettings.GuitarSpeakAltIdentifier, "") }, // Guitar Speak Press ALT When Midi Note = (0{C-1} - 96{C7})
                { ReadSettings.GuitarSpeakTuningIdentifier, CreateDefaultOnOldINI(ReadSettings.GuitarSpeakTuningIdentifier, "off") }, // Use Guitar Speak in Tuning Menus
            }},
            {"[Highway Colors]", new Dictionary<string, string>
            {
                { ReadSettings.CustomHighwayColorsIdentifier, CreateDefaultOnOldINI(ReadSettings.CustomHighwayColorsIdentifier, "off") }, // Should we use Custom Noteway Colors
                { ReadSettings.CustomHighwayNumberedIdentifier, CreateDefaultOnOldINI(ReadSettings.CustomHighwayNumberedIdentifier, "") }, // Numbered Frets Color
                { ReadSettings.CustomHighwayUnNumberedIdentifier, CreateDefaultOnOldINI(ReadSettings.CustomHighwayUnNumberedIdentifier, "") }, // UnNumbered Frets Color
                { ReadSettings.CustomHighwayGutterIdentifier, CreateDefaultOnOldINI(ReadSettings.CustomHighwayGutterIdentifier, "") }, // Sides of Note Highway Color
                { ReadSettings.CustomFretNubmersIdentifier, CreateDefaultOnOldINI(ReadSettings.CustomFretNubmersIdentifier, "") }, // Fret Numbers Color
            }},
            {"[GUI Settings]", new Dictionary<string, string>
            {
                { ReadSettings.CustomGUIThemeIdentifier, CreateDefaultOnOldINI(ReadSettings.CustomGUIThemeIdentifier, "off") }, // Use a Custom GUI theme.
                { ReadSettings.CustomGUIBackgroundColorIdentifier, CreateDefaultOnOldINI(ReadSettings.CustomGUIBackgroundColorIdentifier, "F0FFFF") }, // Custom Theme Background Color
                { ReadSettings.CustomGUITextColorIdentifier, CreateDefaultOnOldINI(ReadSettings.CustomGUITextColorIdentifier, "000000") }, // Custom Theme Text Color
                { ReadSettings.CustomGUIButtonColorIdentifier, CreateDefaultOnOldINI(ReadSettings.CustomGUIButtonColorIdentifier, "E3E3E3") }, // Custom Button Background Color
                { ReadSettings.BackupProfileIdentifier, CreateDefaultOnOldINI(ReadSettings.BackupProfileIdentifier, "on") }, // Should we backup profiles
                { ReadSettings.NumberOfBackupsIdentifier, CreateDefaultOnOldINI(ReadSettings.NumberOfBackupsIdentifier, "50") }, // How many backups should we store before we start deleting them.
                { ReadSettings.SpeedUpLoadupIdentifier, CreateDefaultOnOldINI(ReadSettings.SpeedUpLoadupIdentifier, "off") }, // Block Rocksmith from connecting to the internet. Speeds up enumeration.
            }},
        };

        private static string CreateDefaultOnOldINI(string settingIdentifier, string @default) => ReadSettings.DoesSettingsINIExist() && ReadSettings.ProcessSettings(settingIdentifier) != String.Empty ? ReadSettings.ProcessSettings(settingIdentifier) : @default;

        #endregion
        #region Write Settings
        public static void WriteINI(Dictionary<string, Dictionary<string, string>> DictionaryToWrite)
        {
            using (StreamWriter sw = File.CreateText(Path.Combine(GenUtil.GetRSDirectory(), "RSMods.ini")))
            {
                foreach (string section in DictionaryToWrite.Keys)
                {
                    sw.WriteLine(section);
                    foreach (KeyValuePair<string, string> entry in DictionaryToWrite[section])
                    {
                        sw.WriteLine(entry.Key + entry.Value);
                    }
                }
            }
        }
        #endregion
        #region Is RS Void
        public static void IsVoid(string installLocation) // Anti-Piracy Check (False = Real, True = Pirated) || Modified from Beat Saber Mod Assistant
        {
            if(File.Exists(Path.Combine(installLocation, "IGG-GAMES.COM.url")) || File.Exists(Path.Combine(installLocation, "SmartSteamEmu.ini")) || File.Exists(Path.Combine(installLocation, "GAMESTORRENT.CO.url")) || File.Exists(Path.Combine(installLocation, "Codex.ini")) || File.Exists(Path.Combine(installLocation, "Skidrow.ini")) || !CheckExecutable(installLocation))
            {
                MessageBox.Show("RSMods doesn't support pirated / stolen copies of Rocksmith 2014!", "ARGGGG", MessageBoxButtons.OK, MessageBoxIcon.Error);
                Process.Start("https://store.steampowered.com/app/221680/");
                Environment.Exit(1);
                return;
            }
        }

        private static bool CheckExecutable(string installLocation)
        {
            try
            {
                using (SHA256 sha256 = SHA256.Create())
                {
                    FileStream exeStream = File.Open(Path.Combine(installLocation, "Rocksmith2014.exe"), FileMode.Open);
                    exeStream.Position = 0;

                    byte[] hash = sha256.ComputeHash(exeStream);

                    return hash.SequenceEqual(HASH_EXE); // True - User is using Remastered game, False - User is using a NON-Remastered game (VOID).
                }
            }
            catch
            {
                return true; // Not ideal but if the game is open, there isn't much we can do...
            }
            
        }
        #endregion
    }
}
