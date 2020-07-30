using System;
using System.IO;
using System.Windows.Forms;
using System.Diagnostics;
using System.Collections.Generic;

namespace RSMods
{
    class WriteSettings
    {

        public static string dumpLocation = "RSMods.ini";
        public static string guiSettings = "GUI_Settings.ini";
        public static string[] StringArray = new String[50];

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
                { ReadSettings.DiscoModeIdentifier, "off" }, // Disco Mode Enabled / Disabled
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
            }},
            {"[String Colors]", new Dictionary<string, string> 
            {
                { ReadSettings.String0Color_N_Identifier, "bf5f5f" }, // Default Low E String Color (HEX) | Red
                { ReadSettings.String1Color_N_Identifier, "bfb75f" }, // Default A String Color (HEX) | Yellow
                { ReadSettings.String2Color_N_Identifier, "5e9dbc" }, // Default D String Color (HEX) | Blue
                { ReadSettings.String3Color_N_Identifier, "bf8d5f" }, // Default G String Color (HEX) | Orange
                { ReadSettings.String4Color_N_Identifier, "5fbf97" }, // Default B String Color (HEX) | Green
                { ReadSettings.String5Color_N_Identifier, "ac5ebc" }, // Default High E String Color (HEX) | Purple

                { ReadSettings.String0Color_CB_Identifier, "bf5f5f" }, // Colorblind Low E String Color (HEX) | Red
                { ReadSettings.String1Color_CB_Identifier, "9dbc5e" }, // Colorblind A String Color (HEX) | Yellow
                { ReadSettings.String2Color_CB_Identifier, "5f9fbf" }, // Colorblind D String Color (HEX) | Blue
                { ReadSettings.String3Color_CB_Identifier, "bf875f" }, // Colorblind G String Color (HEX) | Orange
                { ReadSettings.String4Color_CB_Identifier, "5fbfa4" }, // Colorblind B String Color (HEX) | Green
                { ReadSettings.String5Color_CB_Identifier, "8c5fbf" }, // Colorblind High E String Color (HEX) | Purple
            }},
            {"[Mod Settings]", new Dictionary<string, string>
            {
                { ReadSettings.ExtendedRangeTuningIdentifier, "-5" }, // Enable Extended Range Mode When Low E Is X Below E
                { ReadSettings.CheckForNewSongIntervalIdentifier, "5000" }, // Enumerate new CDLC / ODLC every X ms
            }},
        };

        public static void WriteINI(Dictionary<string, Dictionary<string, string>> DictionaryToWrite)
        {
            using (StreamWriter sw = File.CreateText(WhereIsRocksmith()))
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

        public static string WhereIsRocksmith()
        {
            if (File.Exists(@guiSettings)) // If there is already a save file
            {
                if (ReadSettings.SavedRocksmithLocation() != "")
                {
                    dumpLocation = Path.Combine(ReadSettings.SavedRocksmithLocation(), @dumpLocation);
                    return dumpLocation;
                }
            }

            if (File.Exists(@"C:\\Program Files (x86)\\Steam\\steamapps\\common\\Rocksmith2014\\Rocksmith2014.exe")) { // If Rocksmith is in the default location
                WriteRocksmithLocation(@"C:\\Program Files (x86)\\Steam\\steamapps\\common\\Rocksmith2014\\");
                return Path.Combine("C:\\Program Files (x86)\\Steam\\steamapps\\common\\Rocksmith2014\\", @dumpLocation);
            }
            else // User has Rocksmith not in the default spot
            {
                FolderBrowserDialog AskUserLocation = new FolderBrowserDialog();
                AskUserLocation.RootFolder = Environment.SpecialFolder.MyComputer;
                AskUserLocation.Description = "Where is your Rocksmith Installed at?";
                if (AskUserLocation.ShowDialog() == DialogResult.OK)
                {
                    if (File.Exists(@AskUserLocation.SelectedPath + "\\Rocksmith2014.exe"))
                    {
                        WriteRocksmithLocation(@AskUserLocation.SelectedPath + "\\");
                        dumpLocation = Path.Combine(@AskUserLocation.SelectedPath, @dumpLocation);
                        return dumpLocation;
                    }
                    else
                    {
                        MessageBox.Show("The folder you selected ''" + AskUserLocation.SelectedPath + "'' does not contain Rocksmith 2014.", "Rocksmith Not Found", MessageBoxButtons.OK, MessageBoxIcon.Error);
                        WhereIsRocksmith();
                    }                    
                }
                else
                {
                    Environment.Exit(1);
                    return "exit";
                }
                return WhereIsRocksmith();
            }
        }
        private static void WriteRocksmithLocation(string rocksmithLocation)
        {
            if (!IsVoid(rocksmithLocation))
            {
                var dumpGUI = File.Create(@guiSettings);
                dumpGUI.Close();
                File.WriteAllText(@guiSettings, (ReadSettings.RocksmithInstallLocationIdentifier + rocksmithLocation));
                return;
            }
            else
            {
                MessageBox.Show("You're pretty stupid. The DLL that works with this GUI, doesn't support pirated copies.", "ARGGGG", MessageBoxButtons.OK, MessageBoxIcon.Error);
                Process.Start("chrome.exe", "https://store.steampowered.com/app/221680/");
                Environment.Exit(1);
                return;
            }
        }

        private static bool IsVoid(string installLocation) // Anti-Piracy Check (False = Real, True = Pirated) || Modified from Beat Saber Mod Assistant
        {
            return
                File.Exists(Path.Combine(installLocation, "IGG-GAMES.COM.url")) ||
                File.Exists(Path.Combine(installLocation, "SmartSteamEmu.ini")) ||
                File.Exists(Path.Combine(installLocation, "GAMESTORRENT.CO.url")) ||
                File.Exists(Path.Combine(installLocation, "Codex.ini")) ||
                File.Exists(Path.Combine(installLocation, "Skidrow.ini"));
        }
    }
}
