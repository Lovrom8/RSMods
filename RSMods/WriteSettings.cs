using System;
using System.IO;

namespace RSModsConsole
{
    class WriteSettings
    {
        public readonly static string dumpLocation = "RSMods.ini";
        readonly static string[] StringArray = new String[23];
        public static void ModifyINI(string[] StringArray)
        {
            var dumpINI = File.Create(@dumpLocation);
            dumpINI.Close();
            File.WriteAllLines(@dumpLocation, StringArray);
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
                StringArray[20] = ReadSettings.RainbowStringsEnabledIdentifier + "true";
                StringArray[21] = ReadSettings.ExtendedRangeEnabledIdentifier + "true";
                StringArray[22] = ReadSettings.ExtendedRangeTuningIdentifier + "DropB";
                ModifyINI(StringArray);
            }
        }
    }
}
