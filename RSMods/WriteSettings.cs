using System;
using System.IO;
using System.Windows.Forms;
using System.Diagnostics;
using System.Security.Cryptography;

namespace RSMods
{
    class WriteSettings
    {
        public static string dumpLocation = "RSMods.ini";
        public static string guiSettings = "GUI_Settings.ini";
        public static string[] StringArray = new String[49];

        public static void ModifyINI(string[] StringArray)
        {
            var dumpINI = File.Create(WhereIsRocksmith());
            dumpINI.Close();
            File.WriteAllLines(WhereIsRocksmith(), StringArray);
        }

        public static void NoSettingsDetected()
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
            StringArray[15] = ReadSettings.ToggleLoftEnabledIdentifier + "off"; // Toggle Loft Enabled / Disabled
            StringArray[16] = ReadSettings.AddVolumeEnabledIdentifier + "off"; // Add Volume Enabled / Disabled
            StringArray[17] = ReadSettings.DecreaseVolumeEnabledIdentifier + "off"; // Decrease Volume Enabled/ Disabled
            StringArray[18] = ReadSettings.ShowSongTimerEnabledIdentifier + "off"; // Show Song Timer Enabled / Disabled
            StringArray[19] = ReadSettings.ForceReEnumerationEnabledIdentifier + "off"; // Force ReEnumeration Manual / Automatic / Disabled
            StringArray[20] = ReadSettings.RainbowStringsEnabledIdentifier + "off"; // Rainbow String Enabled / Disabled
            StringArray[21] = ReadSettings.ExtendedRangeEnabledIdentifier + "off"; // Extended Range Enabled / Disabled
            StringArray[22] = ReadSettings.CustomStringColorNumberIndetifier + "0"; // Custom String Colors (0 - Default, 1 - ZZ, 2 - Custom Colors)
            StringArray[23] = ReadSettings.DiscoModeIdentifier + "off"; // Disco Mode Enabled / Disabled
            StringArray[24] = ReadSettings.RemoveHeadstockIdentifier + "off"; // Remove Headstock Enabled / Disabled
            StringArray[25] = ReadSettings.RemoveSkylineIdentifier + "off"; // Remove Skyline Enabled / Disabled
            StringArray[26] = ReadSettings.GreenScreenWallIdentifier + "off"; // Greenscreen Back Wall Enabled / Disabled
            StringArray[27] = ReadSettings.ForceProfileEnabledIdentifier + "off"; // Force Load Profile On Game Boot Enabled / Disabled
            StringArray[28] = ReadSettings.FretlessModeEnabledIdentifier + "off"; // Fretless Mode Enabled / Disabled
            StringArray[29] = ReadSettings.RemoveInlaysIdentifier + "off"; // Remove Inlay Markers Enabled / Disabled
            StringArray[30] = ReadSettings.ToggleLoftWhenIdentifier + "manual"; // Define how or when the loft is disabled - game startup, on key command or in song only
            StringArray[31] = ReadSettings.RemoveLaneMarkersIdentifier + "off"; // Remove Inlays  Enabled / Disabled
            StringArray[32] = ReadSettings.ToggleSkylineWhenIdentifier + "song"; // Define how or when the skyline is disabled - game startup, or in song only
            StringArray[33] = "[String Colors]";
            StringArray[34] = ReadSettings.String0Color_N_Identifier + "bf5f5f"; // Default Low E String Color (HEX) | Red
            StringArray[35] = ReadSettings.String1Color_N_Identifier + "bfb75f"; // Default A String Color (HEX) | Yellow
            StringArray[36] = ReadSettings.String2Color_N_Identifier + "5e9dbc"; // Default D String Color (HEX) | Blue
            StringArray[37] = ReadSettings.String3Color_N_Identifier + "bf8d5f"; // Default G String Color (HEX) | Orange
            StringArray[38] = ReadSettings.String4Color_N_Identifier + "5fbf97"; // Default B String Color (HEX) | Green
            StringArray[39] = ReadSettings.String5Color_N_Identifier + "ac5ebc"; // Default High E String Color (HEX) | Purple
            StringArray[40] = ReadSettings.String0Color_CB_Identifier + "bf5f5f"; // Colorblind Low E String Color (HEX) | Red
            StringArray[41] = ReadSettings.String1Color_CB_Identifier + "9dbc5e"; // Colorblind A String Color (HEX) | Yellow
            StringArray[42] = ReadSettings.String2Color_CB_Identifier + "5f9fbf"; // Colorblind D String Color (HEX) | Blue
            StringArray[43] = ReadSettings.String3Color_CB_Identifier + "bf875f"; // Colorblind G String Color (HEX) | Orange
            StringArray[44] = ReadSettings.String4Color_CB_Identifier + "5fbfa4"; // Colorblind B String Color (HEX) | Green
            StringArray[45] = ReadSettings.String5Color_CB_Identifier + "8c5fbf"; // Colorblind High E String Color (HEX) | Purple
            StringArray[46] = "[Mod Settings]";
            StringArray[47] = ReadSettings.ExtendedRangeTuningIdentifier + "-5"; // Enable Extended Range Mode When Low E Is X Below E
            StringArray[48] = ReadSettings.CheckForNewSongIntervalIdentifier + "5000"; // Enumerate new CDLC / ODLC every X ms

            ModifyINI(StringArray);
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


        public static byte[] Sha256Hash()
        {
            // Should output: A7258461101DA0201707F5C272BAAA62A3D3D10B3D2213C0D0F21CC83B4588DA | and does for me
            using (SHA256 mySHA256 = SHA256.Create())
            {
                        // Create a fileStream for the file.
                        FileStream fileStream = File.Open(WhereIsRocksmith().Substring(0, WhereIsRocksmith().Length - 10) + "Rocksmith2014.exe", FileMode.Open);
                        // Be sure it's positioned to the beginning of the stream.
                        fileStream.Position = 0;
                        // Compute the hash of the fileStream.
                        byte[] hashValue = mySHA256.ComputeHash(fileStream);
                        // Write the name and hash value of the file to the console.
                        PrintByteArray(hashValue);
                        // Close the file.
                        fileStream.Close();

                return hashValue;
            }
        }


        public static string PrintByteArray(byte[] array)
        {
            string output = "";
            for (int i = 0; i < array.Length; i++)
            {
                output +=($"{array[i]:X2}");
            }
            MessageBox.Show(output, "Hash", MessageBoxButtons.OK, MessageBoxIcon.Information);
            return output; // Outputs hash
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
                File.Exists(Path.Combine(installLocation, "Skidrow.ini")) ||
                !(PrintByteArray(Sha256Hash()).Equals("A7258461101DA0201707F5C272BAAA62A3D3D10B3D2213C0D0F21CC83B4588DA"));
        }
    }
}
