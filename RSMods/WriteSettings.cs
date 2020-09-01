using System;
using System.IO;
using System.Windows.Forms;
using System.Diagnostics;
using System.Collections.Generic;
using RSMods.Util;

namespace RSMods
{
    class WriteSettings
    {

        //public static string dumpLocation = "RSMods.ini";

        public static Dictionary<string, Dictionary<string, string>> Settings = new Dictionary<string, Dictionary<string, string>>()
        {
            // Section                           mod   default
            {"[SongListTitles]", new Dictionary<string, string> {
                { ReadSettings.Songlist1Identifier, "Define Song List 1 Here" }, // Songlist 1
                { ReadSettings.Songlist2Identifier, "Define Song List 2 Here" }, // Songlist 2
                { ReadSettings.Songlist3Identifier, "Define Song List 3 Here" }, // Songlist 3
                { ReadSettings.Songlist4Identifier, "Define Song List 4 Here" }, // Songlist 4
                { ReadSettings.Songlist5Identifier, "Define Song List 5 Here" }, // Songlist 5
                { ReadSettings.Songlist6Identifier, "Define Song List 6 Here" }, // Songlist 6
            }},
            {"[Keybinds]", new Dictionary<string, string> {
                { ReadSettings.ToggleLoftIdentifier, "" }, // Toggle Loft
                { ReadSettings.AddVolumeIdentifier, "" }, // Add Volume
                { ReadSettings.DecreaseVolumeIdentifier, "" }, // Decrease Volume
                { ReadSettings.ShowSongTimerIdentifier, "" }, // Show Song Timer
                { ReadSettings.ForceReEnumerationIdentifier, "" }, // Force ReEnumeration
                { ReadSettings.RainbowStringsIdentifier, "" }, // Rainbow Strings
                { ReadSettings.RemoveLyricsKeyIdentifier, "" }, // Remove Lyrics Key
            }},
            {"[Toggle Switches]", new Dictionary<string, string>
            {
                { ReadSettings.ToggleLoftEnabledIdentifier, "off" }, // Toggle Loft Enabled / Disabled
                { ReadSettings.AddVolumeEnabledIdentifier, "off" }, // Add Volume Enabled / Disabled
                { ReadSettings.DecreaseVolumeEnabledIdentifier, "off" }, // Decrease Volume Enabled/ Disabled
                { ReadSettings.ShowSongTimerEnabledIdentifier, "off" }, // Show Song Timer Enabled / Disabled
                { ReadSettings.ForceReEnumerationEnabledIdentifier, "off" }, // Force ReEnumeration Manual / Automatic / Disabled
                { ReadSettings.RainbowStringsEnabledIdentifier, "off" }, // Rainbow String Enabled / Disabled
                { ReadSettings.ExtendedRangeEnabledIdentifier, "off" }, // Extended Range Enabled / Disabled
                { ReadSettings.CustomStringColorNumberIndetifier, "0" }, // Custom String Colors (0 - Default, 1 - ZZ, 2 - Custom Colors)
                //{ ReadSettings.DiscoModeIdentifier, "off" }, // Disco Mode Enabled / Disabled
                { ReadSettings.RemoveHeadstockIdentifier, "off" }, // Remove Headstock Enabled / Disabled
                { ReadSettings.RemoveSkylineIdentifier, "off" }, // Remove Skyline Enabled / Disabled
                { ReadSettings.GreenScreenWallIdentifier, "off"}, // Greenscreen Back Wall Enabled / Disabled
                { ReadSettings.ForceProfileEnabledIdentifier, "off" }, // Force Load Profile On Game Boot Enabled / Disabled
                { ReadSettings.FretlessModeEnabledIdentifier, "off" }, // Fretless Mode Enabled / Disabled
                { ReadSettings.RemoveInlaysIdentifier, "off" }, // Remove Inlay Markers Enabled / Disabled
                { ReadSettings.ToggleLoftWhenIdentifier, "manual" }, // Define how or when the loft is disabled - game startup, on key command, or in song only
                { ReadSettings.RemoveLaneMarkersIdentifier, "off" }, // Remove Lane Markers Enabled / Disabled
                { ReadSettings.ToggleSkylineWhenIdentifier, "song" }, // Define how or when the skyline is disabled - game startup, or in song only
                { ReadSettings.RemoveLyricsIdentifier, "off" }, // Remove Song Lyrics Enabled / Disabled
                { ReadSettings.RemoveLyricsWhenIdentifier, "manual" }, // When should we remove lyrics Manual / Automatic
                { ReadSettings.GuitarSpeakIdentifier, "off" }, // Guitar Speak Enabled / Disabled
                { ReadSettings.RemoveHeadstockWhenIdentifier, "song" } // Remove Headstock When Startup / Song
            }},
            {"[String Colors]", new Dictionary<string, string>
            {
                { ReadSettings.String0Color_N_Identifier, "ff4f5a" }, // Default Low E String Color (HEX) | Red
                { ReadSettings.String1Color_N_Identifier, "e2c102" }, // Default A String Color (HEX) | Yellow
                { ReadSettings.String2Color_N_Identifier, "1dacf9" }, // Default D String Color (HEX) | Blue
                { ReadSettings.String3Color_N_Identifier, "ff9216" }, // Default G String Color (HEX) | Orange
                { ReadSettings.String4Color_N_Identifier, "3fcc0c" }, // Default B String Color (HEX) | Green
                { ReadSettings.String5Color_N_Identifier, "c825ed" }, // Default High E String Color (HEX) | Purple

                { ReadSettings.String0Color_CB_Identifier, "00c68e" }, // Colorblind Low E String Color (HEX) | Red
                { ReadSettings.String1Color_CB_Identifier, "ff4f5a" }, // Colorblind A String Color (HEX) | Yellow
                { ReadSettings.String2Color_CB_Identifier, "e2c102" }, // Colorblind D String Color (HEX) | Blue
                { ReadSettings.String3Color_CB_Identifier, "1dacf9" }, // Colorblind G String Color (HEX) | Orange
                { ReadSettings.String4Color_CB_Identifier, "ff9216" }, // Colorblind B String Color (HEX) | Green
                { ReadSettings.String5Color_CB_Identifier, "3fcc0c" }, // Colorblind High E String Color (HEX) | Purple
            }},
            {"[Mod Settings]", new Dictionary<string, string>
            {
                { ReadSettings.ExtendedRangeTuningIdentifier, "-5" }, // Enable Extended Range Mode When Low E Is X Below E
                { ReadSettings.CheckForNewSongIntervalIdentifier, "5000" }, // Enumerate new CDLC / ODLC every X ms
            }},

            {"[Guitar Speak]", new Dictionary<string, string>
            {
                { ReadSettings.GuitarSpeakDeleteIdentifier, "" },
                { ReadSettings.GuitarSpeakSpaceIdentifier, "" },
                { ReadSettings.GuitarSpeakEnterIdentifier, "" },
                { ReadSettings.GuitarSpeakTabIdentifier, "" },
                { ReadSettings.GuitarSpeakPGUPIdentifier, "" },
                { ReadSettings.GuitarSpeakPGDNIdentifier, "" },
                { ReadSettings.GuitarSpeakUPIdentifier, "" },
                { ReadSettings.GuitarSpeakDNIdentifier, "" },
                { ReadSettings.GuitarSpeakESCIdentifier, "" },
                { ReadSettings.GuitarSpeakCloseIdentifier, "" },
                { ReadSettings.GuitarSpeakOBracketIdentifier, "" },
                { ReadSettings.GuitarSpeakCBracketIdentifier, "" },
                { ReadSettings.GuitarSpeakTildeaIdentifier, "" },
                { ReadSettings.GuitarSpeakForSlashIdentifier, "" },
                { ReadSettings.GuitarSpeakTuningIdentifier, "off" },
            }}
        };

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

        public static void IsVoid(string installLocation) // Anti-Piracy Check (False = Real, True = Pirated) || Modified from Beat Saber Mod Assistant
        {
            if(File.Exists(Path.Combine(installLocation, "IGG-GAMES.COM.url")) || File.Exists(Path.Combine(installLocation, "SmartSteamEmu.ini")) || File.Exists(Path.Combine(installLocation, "GAMESTORRENT.CO.url")) || File.Exists(Path.Combine(installLocation, "Codex.ini")) || File.Exists(Path.Combine(installLocation, "Skidrow.ini")))
            {
                MessageBox.Show("You're pretty stupid. The DLL that works with this GUI, doesn't support pirated copies.", "ARGGGG", MessageBoxButtons.OK, MessageBoxIcon.Error);
                Process.Start("chrome.exe", "https://store.steampowered.com/app/221680/");
                Environment.Exit(1);
                return;
            }  
        }
    }
}
