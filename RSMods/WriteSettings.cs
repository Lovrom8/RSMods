using System;
using System.IO;
using System.Windows.Forms;
using System.Diagnostics;

namespace RSModsConsole
{
    class WriteSettings
    {
        public static string dumpLocation = "RSMods.ini";
        public static string guiSettings = "GUI_Settings.ini";
        public readonly static string[] StringArray = new String[25];

        public static void ModifyINI(string[] StringArray)
        {
            var dumpINI = File.Create(WhereIsRocksmith());
            dumpINI.Close();
            File.WriteAllLines(WhereIsRocksmith(), StringArray);
        }

        public static void NoSettingsDetected()
        {
            if (!File.Exists(@dumpLocation))
            {
                StringArray[0] = "[SongListTitles]";
                StringArray[1] = ReadSettings.Songlist1Identifier + "Define Song List 1 Here"; // Songlist 1
                StringArray[2] = ReadSettings.Songlist2Identifier + "Define Song List 2 Here"; // Songlist 2
                StringArray[3] = ReadSettings.Songlist3Identifier + "Define Song List 3 Here"; // Songlist 3
                StringArray[4] = ReadSettings.Songlist4Identifier + "Define Song List 4 Here"; // Songlist 4
                StringArray[5] = ReadSettings.Songlist5Identifier + "Define Song List 5 Here"; // Songlist 5
                StringArray[6] = ReadSettings.Songlist6Identifier + "Define Song List 6 Here"; // Songlist 6
                StringArray[7] = "[Keybinds]";
                StringArray[8] = ReadSettings.ToggleLoftIdentifier; // Toggle Loft
                StringArray[9] = ReadSettings.AddVolumeIdentifier; // Add Volume
                StringArray[10] = ReadSettings.DecreaseVolumeIdentifier; // Decrease Volume
                StringArray[11] = ReadSettings.ShowSongTimerIdentifier; // Show Song Timer
                StringArray[12] = ReadSettings.ForceReEnumerationIdentifier; // Force ReEnumeration
                StringArray[13] = ReadSettings.RainbowStringsIdentifier; // Rainbow Strings
                StringArray[14] = "[Toggle Switches]";
                StringArray[15] = ReadSettings.ToggleLoftEnabledIdentifier + "true"; // Toggle Loft Enabled / Disabled
                StringArray[16] = ReadSettings.AddVolumeEnabledIdentifier + "true"; // Add Volume Enabled / Disabled
                StringArray[17] = ReadSettings.DecreaseVolumeEnabledIdentifier + "true"; // Decrease Volume Enabled/ Disabled
                StringArray[18] = ReadSettings.ShowSongTimerEnabledIdentifier + "true"; // Show Song Timer Enabled / Disabled
                StringArray[19] = ReadSettings.ForceReEnumerationEnabledIdentifier + "manual"; // Force ReEnumeration Manual / Automatic / Disabled
                StringArray[20] = ReadSettings.RainbowStringsEnabledIdentifier + "true"; // Rainbow String Enabled / Disabled
                StringArray[21] = ReadSettings.ExtendedRangeEnabledIdentifier + "true"; // Extended Range Enabled / Disabled
                StringArray[22] = ReadSettings.ExtendedRangeTuningIdentifier + "B"; // Extended Range Starts At X Tuning (lowest string)
                StringArray[23] = ReadSettings.DiscoModeIdentifier + "false"; // Disco Mode Enabled / Disabled
                StringArray[24] = ReadSettings.RemoveHeadstockIdentifier + "false"; // Remove Headstock Enabled / Disabled
                ModifyINI(StringArray);
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
