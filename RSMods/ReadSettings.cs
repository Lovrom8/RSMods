using System.IO;
using RSMods.Util;

namespace RSMods
{
    class ReadSettings
    {
        public static string Songlist1Name, Songlist2Name, Songlist3Name, Songlist4Name, Songlist5Name, Songlist6Name,
                             ToggleLoftKey, AddVolumeKey, DecreaseVolumeKey, ShowSongTimerKey, ForceReEnumerationKey, RainbowStringsKey,
                             ToggleLoftEnabled, AddVolumeEnabled, DecreaseVolumeEnabled, ShowSongTimerEnabled, ForceReEnumerationEnabled, RainbowStringsEnabled, ExtendedRangeEnabled, CustomStringColorsNumber, DiscoModeEnabled, RemoveHeadstockEnabled, RemoveSkylineEnabled, GreenscreenWallEnabled, ForceProfileEnabled, FretlessEnabled, RemoveInlaysEnabled, ToggleLoftWhen, ToggleSkylineWhen, RemoveLaneMarkersEnabled, RemoveLyricsEnabled,
                             String0Color_N, String1Color_N, String2Color_N, String3Color_N, String4Color_N, String5Color_N, String0Color_CB, String1Color_CB, String2Color_CB, String3Color_CB, String4Color_CB, String5Color_CB,
                             ExtendedRangeTuning, CheckForNewSongInterval, 
                             RocksmithInstallLocation;

        public static string
                // Song List Identifiers
                Songlist1Identifier = "SongListTitle_1 = ",
                Songlist2Identifier = "SongListTitle_2 = ",
                Songlist3Identifier = "SongListTitle_3 = ",
                Songlist4Identifier = "SongListTitle_4 = ",
                Songlist5Identifier = "SongListTitle_5 = ",
                Songlist6Identifier = "SongListTitle_6 = ",
                // Toggle On Key Identifiers
                ToggleLoftIdentifier = "ToggleLoftKey = ",
                AddVolumeIdentifier = "AddVolumeKey = ",
                DecreaseVolumeIdentifier = "DecreaseVolumeKey = ",
                ShowSongTimerIdentifier = "ShowSongTimerKey = ",
                ForceReEnumerationIdentifier = "ForceReEnumerationKey = ",
                RainbowStringsIdentifier = "RainbowStringsKey = ",
                // Toggle Effects (Change Names to easier naming scheme before public launch)
                ToggleLoftEnabledIdentifier = "ToggleLoft = ",
                AddVolumeEnabledIdentifier = "AddVolume = ",
                DecreaseVolumeEnabledIdentifier = "DecreaseVolume = ",
                ShowSongTimerEnabledIdentifier = "ShowSongTimer = ",
                ForceReEnumerationEnabledIdentifier = "ForceReEnumeration = ",
                RainbowStringsEnabledIdentifier = "RainbowStrings = ",
                ExtendedRangeEnabledIdentifier = "ExtendedRange = ",
                CustomStringColorNumberIndetifier = "CustomStringColors = ",
                DiscoModeIdentifier = "DiscoMode = ",
                RemoveHeadstockIdentifier = "Headstock = ",
                RemoveSkylineIdentifier = "Skyline = ",
                GreenScreenWallIdentifier = "GreenScreenWall = ",
                ForceProfileEnabledIdentifier = "ForceProfileLoad = ",
                FretlessModeEnabledIdentifier = "Fretless = ",
                RemoveInlaysIdentifier = "Inlays = ",
                ToggleLoftWhenIdentifier = "ToggleLoftWhen = ",
                ToggleSkylineWhenIdentifier = "ToggleSkylineWhen = ",
                RemoveLaneMarkersIdentifier = "LaneMarkers = ",
                RemoveLyricsIdentifier = "Lyrics = ",
                    // String Colors (Normal {N} & Colorblind {CB})
                    // Normal String Colors
                    String0Color_N_Identifier = "string0_N = ",
                    String1Color_N_Identifier = "string1_N = ",
                    String2Color_N_Identifier = "string2_N = ",
                    String3Color_N_Identifier = "string3_N = ",
                    String4Color_N_Identifier = "string4_N = ",
                    String5Color_N_Identifier = "string5_N = ",
                    // Colorblind String Colors
                    String0Color_CB_Identifier = "string0_CB = ",
                    String1Color_CB_Identifier = "string1_CB = ",
                    String2Color_CB_Identifier = "string2_CB = ",
                    String3Color_CB_Identifier = "string3_CB = ",
                    String4Color_CB_Identifier = "string4_CB = ",
                    String5Color_CB_Identifier = "string5_CB = ",
                // Mod Settings
                ExtendedRangeTuningIdentifier = "ExtendedRangeModeAt = ",
                CheckForNewSongIntervalIdentifier = "CheckForNewSongsInterval = ";


        public static string ProcessSettings(string identifierToGrab)
        {
            if (!File.Exists(Path.Combine(GenUtil.GetRSDirectory(), "RSMods.ini")))
            {
                if (!File.Exists(Path.Combine(GenUtil.GetRSDirectory(), "RSMods.ini")))
                    WriteSettings.WriteINI(WriteSettings.Settings); // Creates Settings File
                return "";
            }
            else
            {


                foreach (string currentLine in File.ReadLines(Path.Combine(GenUtil.GetRSDirectory(), "RSMods.ini")))
                {
                    // Song Lists
                    {
                        if (currentLine.Contains(Songlist1Identifier))
                        {
                            Songlist1Name = currentLine.Substring(Songlist1Identifier.Length, (currentLine.Length - Songlist1Identifier.Length));
                            if (identifierToGrab == Songlist1Identifier)
                            {
                                return Songlist1Name;
                            }
                        }
                        if (currentLine.Contains(Songlist2Identifier))
                        {
                            Songlist2Name = currentLine.Substring(Songlist2Identifier.Length, (currentLine.Length - Songlist2Identifier.Length));
                            if (identifierToGrab == Songlist2Identifier)
                            {
                                return Songlist2Name;
                            }
                        }
                        if (currentLine.Contains(Songlist3Identifier))
                        {
                            Songlist3Name = currentLine.Substring(Songlist3Identifier.Length, (currentLine.Length - Songlist3Identifier.Length));
                            if (identifierToGrab == Songlist3Identifier)
                            {
                                return Songlist3Name;
                            }
                        }
                        if (currentLine.Contains(Songlist4Identifier))
                        {
                            Songlist4Name = currentLine.Substring(Songlist4Identifier.Length, (currentLine.Length - Songlist4Identifier.Length));
                            if (identifierToGrab == Songlist4Identifier)
                            {
                                return Songlist4Name;
                            }
                        }
                        if (currentLine.Contains(Songlist5Identifier))
                        {
                            Songlist5Name = currentLine.Substring(Songlist5Identifier.Length, (currentLine.Length - Songlist5Identifier.Length));
                            if (identifierToGrab == Songlist5Identifier)
                            {
                                return Songlist5Name;
                            }
                        }
                        if (currentLine.Contains(Songlist6Identifier))
                        {
                            Songlist6Name = currentLine.Substring(Songlist6Identifier.Length, (currentLine.Length - Songlist6Identifier.Length));
                            if (identifierToGrab == Songlist6Identifier)
                            {
                                return Songlist6Name;
                            }
                        }
                    }

                    // Mods
                    {
                        if (currentLine.Contains(ToggleLoftIdentifier))
                        {
                            ToggleLoftKey = currentLine.Substring(ToggleLoftIdentifier.Length, (currentLine.Length - ToggleLoftIdentifier.Length));
                            if (KeyConversion.VirtualKey(ToggleLoftKey) != "")
                            {
                                ToggleLoftKey = KeyConversion.VirtualKey(ToggleLoftKey);
                            }
                            if (identifierToGrab == ToggleLoftIdentifier)
                            {
                                return ToggleLoftKey;
                            }
                        }

                        if (currentLine.Contains(AddVolumeIdentifier))
                        {
                            AddVolumeKey = currentLine.Substring(AddVolumeIdentifier.Length, (currentLine.Length - AddVolumeIdentifier.Length));
                            if (KeyConversion.VirtualKey(AddVolumeKey) != "")
                            {
                                AddVolumeKey = KeyConversion.VirtualKey(AddVolumeKey);
                            }
                            if (identifierToGrab == AddVolumeIdentifier)
                            {
                                return AddVolumeKey;
                            }
                        }
                        if (currentLine.Contains(DecreaseVolumeIdentifier))
                        {
                            DecreaseVolumeKey = currentLine.Substring(DecreaseVolumeIdentifier.Length, (currentLine.Length - DecreaseVolumeIdentifier.Length));
                            if (KeyConversion.VirtualKey(DecreaseVolumeKey) != "")
                            {
                                DecreaseVolumeKey = KeyConversion.VirtualKey(DecreaseVolumeKey);
                            }
                            if (identifierToGrab == DecreaseVolumeIdentifier)
                            {
                                return DecreaseVolumeKey;
                            }
                        }
                        if (currentLine.Contains(ShowSongTimerIdentifier))
                        {
                            ShowSongTimerKey = currentLine.Substring(ShowSongTimerIdentifier.Length, (currentLine.Length - ShowSongTimerIdentifier.Length));
                            if (KeyConversion.VirtualKey(ShowSongTimerKey) != "")
                            {
                                ShowSongTimerKey = KeyConversion.VirtualKey(ShowSongTimerKey);
                            }
                            if (identifierToGrab == ShowSongTimerIdentifier)
                            {
                                return ShowSongTimerKey;
                            }
                        }
                        if (currentLine.Contains(ForceReEnumerationIdentifier))
                        {
                            ForceReEnumerationKey = currentLine.Substring(ForceReEnumerationIdentifier.Length, (currentLine.Length - ForceReEnumerationIdentifier.Length));
                            if (KeyConversion.VirtualKey(ForceReEnumerationKey) != "")
                            {
                                ForceReEnumerationKey = KeyConversion.VirtualKey(ForceReEnumerationKey);

                            }
                            if (identifierToGrab == ForceReEnumerationIdentifier)
                            {
                                return ForceReEnumerationKey;
                            }
                        }
                        if (currentLine.Contains(RainbowStringsIdentifier))
                        {
                            RainbowStringsKey = currentLine.Substring(RainbowStringsIdentifier.Length, (currentLine.Length - RainbowStringsIdentifier.Length));
                            if (KeyConversion.VirtualKey(RainbowStringsKey) != "")
                            {
                                RainbowStringsKey = KeyConversion.VirtualKey(RainbowStringsKey);
                            }
                            if (identifierToGrab == RainbowStringsIdentifier)
                            {
                                return RainbowStringsKey;
                            }
                        }
                    }

                    // Mods Enabled / Disabled
                    {
                        if (currentLine.Contains(ToggleLoftEnabledIdentifier))
                        {
                            if (currentLine.Substring(ToggleLoftEnabledIdentifier.Length, (currentLine.Length - ToggleLoftEnabledIdentifier.Length)) == "on")
                            {
                                ToggleLoftEnabled = "on";
                            }
                            else
                            {
                                ToggleLoftEnabled = "off";
                            }

                            if (identifierToGrab == ToggleLoftEnabledIdentifier)
                            {
                                return ToggleLoftEnabled;
                            }
                        }
                        if (currentLine.Contains(AddVolumeEnabledIdentifier))
                        {
                            if (currentLine.Substring(AddVolumeEnabledIdentifier.Length, (currentLine.Length - AddVolumeEnabledIdentifier.Length)) == "on")
                            {
                                AddVolumeEnabled = "on";
                            }
                            else
                            {
                                AddVolumeEnabled = "off";
                            }

                            if (identifierToGrab == AddVolumeEnabledIdentifier)
                            {
                                return AddVolumeEnabled;
                            }
                        }
                        if (currentLine.Contains(DecreaseVolumeEnabledIdentifier))
                        {
                            if (currentLine.Substring(DecreaseVolumeEnabledIdentifier.Length, (currentLine.Length - DecreaseVolumeEnabledIdentifier.Length)) == "on")
                            {
                                DecreaseVolumeEnabled = "on";
                            }
                            else
                            {
                                DecreaseVolumeEnabled = "off";
                            }

                            if (identifierToGrab == DecreaseVolumeEnabledIdentifier)
                            {
                                return DecreaseVolumeEnabled;
                            }
                        }
                        if (currentLine.Contains(ShowSongTimerEnabledIdentifier))
                        {
                            if (currentLine.Substring(ShowSongTimerEnabledIdentifier.Length, (currentLine.Length - ShowSongTimerEnabledIdentifier.Length)) == "on")
                            {
                                ShowSongTimerEnabled = "on";
                            }
                            else
                            {
                                ShowSongTimerEnabled = "off";
                            }

                            if (identifierToGrab == ShowSongTimerEnabledIdentifier)
                            {
                                return ShowSongTimerEnabled;
                            }
                        }
                        if (currentLine.Contains(ForceReEnumerationEnabledIdentifier))
                        {
                            if (currentLine.Substring(ForceReEnumerationEnabledIdentifier.Length, (currentLine.Length - ForceReEnumerationEnabledIdentifier.Length)) == "manual")
                            {
                                ForceReEnumerationEnabled = "manual";
                            }
                            if (currentLine.Substring(ForceReEnumerationEnabledIdentifier.Length, (currentLine.Length - ForceReEnumerationEnabledIdentifier.Length)) == "automatic")
                            {
                                ForceReEnumerationEnabled = "automatic";
                            }
                            if (currentLine.Substring(ForceReEnumerationEnabledIdentifier.Length, (currentLine.Length - ForceReEnumerationEnabledIdentifier.Length)) == "off")
                            {
                                ForceReEnumerationEnabled = "off";
                            }

                            if (identifierToGrab == ForceReEnumerationEnabledIdentifier)
                            {
                                return ForceReEnumerationEnabled;
                            }
                        }
                        if (currentLine.Contains(RainbowStringsEnabledIdentifier))
                        {
                            if (currentLine.Substring(RainbowStringsEnabledIdentifier.Length, (currentLine.Length - RainbowStringsEnabledIdentifier.Length)) == "on")
                            {
                                RainbowStringsEnabled = "on";
                            }
                            else
                            {
                                RainbowStringsEnabled = "off";
                            }

                            if (identifierToGrab == RainbowStringsEnabledIdentifier)
                            {
                                return RainbowStringsEnabled;
                            }
                        }
                        if (currentLine.Contains(ExtendedRangeEnabledIdentifier))
                        {
                            if (currentLine.Substring(ExtendedRangeEnabledIdentifier.Length, (currentLine.Length - ExtendedRangeEnabledIdentifier.Length)) == "on")
                            {
                                ExtendedRangeEnabled = "on";
                            }
                            else
                            {
                                ExtendedRangeEnabled = "off";
                            }

                            if (identifierToGrab == ExtendedRangeEnabledIdentifier)
                            {
                                return ExtendedRangeEnabled;
                            }
                        }
                        if (currentLine.Contains(CustomStringColorNumberIndetifier))
                        {
                            CustomStringColorsNumber = currentLine.Substring(CustomStringColorNumberIndetifier.Length, (currentLine.Length - CustomStringColorNumberIndetifier.Length));

                            if (identifierToGrab == CustomStringColorNumberIndetifier)
                            {
                                return CustomStringColorsNumber;
                            }
                        }
                        if (currentLine.Contains(DiscoModeIdentifier))
                        {
                            if (currentLine.Substring(DiscoModeIdentifier.Length, (currentLine.Length - DiscoModeIdentifier.Length)) == "on")
                            {
                                DiscoModeEnabled = "on";
                            }
                            else
                            {
                                DiscoModeEnabled = "off";
                            }

                            if (identifierToGrab == DiscoModeIdentifier)
                            {
                                return DiscoModeEnabled;
                            }
                        }
                        if (currentLine.Contains(RemoveHeadstockIdentifier))
                        {
                            if (currentLine.Substring(RemoveHeadstockIdentifier.Length, (currentLine.Length - RemoveHeadstockIdentifier.Length)) == "on")
                            {
                                RemoveHeadstockEnabled = "on";
                            }
                            else
                            {
                                RemoveHeadstockEnabled = "off";
                            }

                            if (identifierToGrab == RemoveHeadstockIdentifier)
                            {
                                return RemoveHeadstockEnabled;
                            }
                        }
                        if (currentLine.Contains(RemoveSkylineIdentifier))
                        {
                            if (currentLine.Substring(RemoveSkylineIdentifier.Length, (currentLine.Length - RemoveSkylineIdentifier.Length)) == "on")
                            {
                                RemoveSkylineEnabled = "on";
                            }
                            else
                            {
                                RemoveSkylineEnabled = "off";
                            }

                            if (identifierToGrab == RemoveSkylineIdentifier)
                            {
                                return RemoveSkylineEnabled;
                            }
                        }
                        if (currentLine.Contains(GreenScreenWallIdentifier))
                        {
                            if (currentLine.Substring(GreenScreenWallIdentifier.Length, (currentLine.Length - GreenScreenWallIdentifier.Length)) == "on")
                            {
                                GreenscreenWallEnabled = "on";
                            }
                            else
                            {
                                GreenscreenWallEnabled = "off";
                            }

                            if (identifierToGrab == GreenScreenWallIdentifier)
                            {
                                return GreenscreenWallEnabled;
                            }
                        }

                        if (currentLine.Contains(ForceProfileEnabledIdentifier))
                        {
                            if (currentLine.Substring(ForceProfileEnabledIdentifier.Length, (currentLine.Length - ForceProfileEnabledIdentifier.Length)) == "on")
                            {
                                ForceProfileEnabled = "on";
                            }
                            else
                            {
                                ForceProfileEnabled = "off";
                            }
                            if (identifierToGrab == ForceProfileEnabledIdentifier)
                            {
                                return ForceProfileEnabled;
                            }
                        }
                        if (currentLine.Contains(FretlessModeEnabledIdentifier))
                        {
                            if (currentLine.Substring(FretlessModeEnabledIdentifier.Length, (currentLine.Length - FretlessModeEnabledIdentifier.Length)) == "on")
                            {
                                FretlessEnabled = "on";
                            }
                            else
                            {
                                FretlessEnabled = "off";
                            }

                            if (identifierToGrab == FretlessModeEnabledIdentifier)
                            {
                                return FretlessEnabled;
                            }
                        }
                        if (currentLine.Contains(RemoveInlaysIdentifier))
                        {
                            if (currentLine.Substring(RemoveInlaysIdentifier.Length, (currentLine.Length - RemoveInlaysIdentifier.Length)) == "on")
                            {
                                RemoveInlaysEnabled = "on";
                            }
                            else
                            {
                                RemoveInlaysEnabled = "off";
                            }

                            if (identifierToGrab == RemoveInlaysIdentifier)
                            {
                                return RemoveInlaysEnabled;
                            }
                        }
                        if (currentLine.Contains(ToggleLoftWhenIdentifier))
                        {
                            if (currentLine.Substring(ToggleLoftWhenIdentifier.Length, (currentLine.Length - ToggleLoftWhenIdentifier.Length)) == "manual")
                            {
                                ToggleLoftWhen = "manual";
                            }
                            if (currentLine.Substring(ToggleLoftWhenIdentifier.Length, (currentLine.Length - ToggleLoftWhenIdentifier.Length)) == "startup")
                            {
                                ToggleLoftWhen = "startup";
                            }
                            if (currentLine.Substring(ToggleLoftWhenIdentifier.Length, (currentLine.Length - ToggleLoftWhenIdentifier.Length)) == "song")
                            {
                                ToggleLoftWhen = "song";
                            }

                            if (identifierToGrab == ToggleLoftWhenIdentifier)
                            {
                                return ToggleLoftWhen;
                            }
                        }
                        if (currentLine.Contains(RemoveLaneMarkersIdentifier))
                        {
                            if (currentLine.Substring(RemoveLaneMarkersIdentifier.Length, (currentLine.Length - RemoveLaneMarkersIdentifier.Length)) == "on")
                            {
                                RemoveLaneMarkersEnabled = "on";
                            }
                            else
                            {
                                RemoveLaneMarkersEnabled = "off";
                            }

                            if (identifierToGrab == RemoveLaneMarkersIdentifier)
                            {
                                return RemoveLaneMarkersEnabled;
                            }
                        }
                        if (currentLine.Contains(ToggleSkylineWhenIdentifier))
                        {
                            if (currentLine.Substring(ToggleSkylineWhenIdentifier.Length, (currentLine.Length - ToggleSkylineWhenIdentifier.Length)) == "song")
                            {
                                ToggleSkylineWhen = "song";
                            }
                            if (currentLine.Substring(ToggleSkylineWhenIdentifier.Length, (currentLine.Length - ToggleSkylineWhenIdentifier.Length)) == "startup")
                            {
                                ToggleSkylineWhen = "startup";
                            }

                            if (identifierToGrab == ToggleSkylineWhenIdentifier)
                            {
                                return ToggleSkylineWhen;
                            }
                        }

                        if (currentLine.Contains(RemoveLyricsIdentifier))
                            if (currentLine.Substring(RemoveLyricsIdentifier.Length, currentLine.Length - RemoveLyricsIdentifier.Length) == "on")
                            {
                                RemoveLyricsEnabled = "on";
                            }
                            else
                            {
                                RemoveLyricsEnabled = "off";
                            }

                        if (identifierToGrab == RemoveLyricsIdentifier)
                        {
                            return RemoveLyricsEnabled;
                        }
                    }

                    // String Colors (Normal {N} & Colorblind {CB})
                    {
                        // Normal Colors
                        {
                            if (currentLine.Contains(String0Color_N_Identifier))
                            {
                                String0Color_N = currentLine.Substring(String0Color_N_Identifier.Length, (currentLine.Length - String0Color_N_Identifier.Length));

                                if (identifierToGrab == String0Color_N_Identifier)
                                {
                                    return String0Color_N;
                                }
                            }
                            if (currentLine.Contains(String1Color_N_Identifier))
                            {
                                String1Color_N = currentLine.Substring(String1Color_N_Identifier.Length, (currentLine.Length - String1Color_N_Identifier.Length));

                                if (identifierToGrab == String1Color_N_Identifier)
                                {
                                    return String1Color_N;
                                }
                            }
                            if (currentLine.Contains(String2Color_N_Identifier))
                            {
                                String2Color_N = currentLine.Substring(String2Color_N_Identifier.Length, (currentLine.Length - String2Color_N_Identifier.Length));

                                if (identifierToGrab == String2Color_N_Identifier)
                                {
                                    return String2Color_N;
                                }
                            }
                            if (currentLine.Contains(String3Color_N_Identifier))
                            {
                                String3Color_N = currentLine.Substring(String3Color_N_Identifier.Length, (currentLine.Length - String3Color_N_Identifier.Length));

                                if (identifierToGrab == String3Color_N_Identifier)
                                {
                                    return String3Color_N;
                                }
                            }
                            if (currentLine.Contains(String4Color_N_Identifier))
                            {
                                String4Color_N = currentLine.Substring(String4Color_N_Identifier.Length, (currentLine.Length - String4Color_N_Identifier.Length));

                                if (identifierToGrab == String4Color_N_Identifier)
                                {
                                    return String4Color_N;
                                }
                            }
                            if (currentLine.Contains(String5Color_N_Identifier))
                            {
                                String5Color_N = currentLine.Substring(String5Color_N_Identifier.Length, (currentLine.Length - String5Color_N_Identifier.Length));

                                if (identifierToGrab == String5Color_N_Identifier)
                                {
                                    return String5Color_N;
                                }
                            }
                        }

                        // Color Blind Colors
                        {
                            if (currentLine.Contains(String0Color_CB_Identifier))
                            {
                                String0Color_CB = currentLine.Substring(String0Color_CB_Identifier.Length, (currentLine.Length - String0Color_CB_Identifier.Length));

                                if (identifierToGrab == String0Color_CB_Identifier)
                                {
                                    return String0Color_CB;
                                }
                            }
                            if (currentLine.Contains(String1Color_CB_Identifier))
                            {
                                String1Color_CB = currentLine.Substring(String1Color_CB_Identifier.Length, (currentLine.Length - String1Color_CB_Identifier.Length));

                                if (identifierToGrab == String1Color_CB_Identifier)
                                {
                                    return String1Color_CB;
                                }
                            }
                            if (currentLine.Contains(String2Color_CB_Identifier))
                            {
                                String2Color_CB = currentLine.Substring(String2Color_CB_Identifier.Length, (currentLine.Length - String2Color_CB_Identifier.Length));

                                if (identifierToGrab == String2Color_CB_Identifier)
                                {
                                    return String2Color_CB;
                                }
                            }
                            if (currentLine.Contains(String3Color_CB_Identifier))
                            {
                                String3Color_CB = currentLine.Substring(String3Color_CB_Identifier.Length, (currentLine.Length - String3Color_CB_Identifier.Length));

                                if (identifierToGrab == String3Color_CB_Identifier)
                                {
                                    return String3Color_CB;
                                }
                            }
                            if (currentLine.Contains(String4Color_CB_Identifier))
                            {
                                String4Color_CB = currentLine.Substring(String4Color_CB_Identifier.Length, (currentLine.Length - String4Color_CB_Identifier.Length));

                                if (identifierToGrab == String4Color_CB_Identifier)
                                {
                                    return String4Color_CB;
                                }
                            }
                            if (currentLine.Contains(String5Color_CB_Identifier))
                            {
                                String5Color_CB = currentLine.Substring(String5Color_CB_Identifier.Length, (currentLine.Length - String5Color_CB_Identifier.Length));

                                if (identifierToGrab == String5Color_CB_Identifier)
                                {
                                    return String5Color_CB;
                                }
                            }
                        }

                    }

                    // Mod Settings
                    {
                        if (currentLine.Contains(ExtendedRangeTuningIdentifier))
                        {
                            ExtendedRangeTuning = currentLine.Substring(ExtendedRangeTuningIdentifier.Length, (currentLine.Length - ExtendedRangeTuningIdentifier.Length));

                            if (identifierToGrab == ExtendedRangeTuningIdentifier)
                            {
                                return ExtendedRangeTuning;
                            }
                        }
                        if (currentLine.Contains(CheckForNewSongIntervalIdentifier))
                        {
                            CheckForNewSongInterval = currentLine.Substring(CheckForNewSongIntervalIdentifier.Length, (currentLine.Length - CheckForNewSongIntervalIdentifier.Length));

                            if (identifierToGrab == CheckForNewSongIntervalIdentifier)
                            {
                                return CheckForNewSongInterval;
                            }
                        }

                    }
                }
                return "";
            }
        }
    }
}
