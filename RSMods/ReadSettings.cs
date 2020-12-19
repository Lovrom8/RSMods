using System.IO;
using RSMods.Util;

namespace RSMods
{
    class ReadSettings
    {
        public static string Songlist1Name, Songlist2Name, Songlist3Name, Songlist4Name, Songlist5Name, Songlist6Name,
                             ToggleLoftKey, AddVolumeKey, DecreaseVolumeKey, ChangeSelectedVolumeKey, ShowSongTimerKey, ForceReEnumerationKey, RainbowStringsKey, RemoveLyricsKey, RRSpeedKey,
                             ToggleLoftEnabled, AddVolumeEnabled, DecreaseVolumeEnabled, ShowSongTimerEnabled, ForceReEnumerationEnabled, RainbowStringsEnabled, ExtendedRangeEnabled, ExtendedRangeDropTuning, CustomStringColorsNumber,
                             DiscoModeEnabled, RemoveHeadstockEnabled, RemoveSkylineEnabled, GreenscreenWallEnabled, ForceProfileEnabled, FretlessEnabled, RemoveInlaysEnabled, ToggleLoftWhen,
                             ToggleSkylineWhen, RemoveLaneMarkersEnabled, RemoveLyricsEnabled, RemoveLyricsWhen, GuitarSpeakEnabled, RemoveHeadstockWhen, ScreenShotScores,
                                                RiffRepeaterAboveHundred, MidiAutoTuning, MidiAutoTuningDevice, TuningPedal, ChordsMode, ShowCurrentNoteOnScreen, OnScreenFont,
                             String0Color_N, String1Color_N, String2Color_N, String3Color_N, String4Color_N, String5Color_N, String0Color_CB, String1Color_CB, String2Color_CB, String3Color_CB, String4Color_CB, String5Color_CB,
                             ExtendedRangeTuning, CheckForNewSongInterval, RiffRepeaterSpeedInterval,
                             GuitarSpeakDelete, GuitarSpeakSpace, GuitarSpeakEnter, GuitarSpeakTab, GuitarSpeakPGUP, GuitarSpeakPGDN, GuitarSpeakUP, GuitarSpeakDN, GuitarSpeakESC,
                                                GuitarSpeakClose, GuitarSpeakOBracket, GuitarSpeakCBracket, GuitarSpeakTildea, GuitarSpeakForSlash, GuitarSpeakWhileTuning,
                             CustomGUITheme, CustomGUIBackgroundColor, CustomGUITextColor,

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
                RemoveLyricsKeyIdentifier = "RemoveLyricsKey = ",
                ChangeSelectedVolumeKeyIdentifier = "ChangedSelectedVolumeKey = ",
                RRSpeedKeyIdentifier = "RRSpeedKey = ",

                // Toggle Effects
                ToggleLoftEnabledIdentifier = "ToggleLoft = ",
                AddVolumeEnabledIdentifier = "AddVolume = ",
                DecreaseVolumeEnabledIdentifier = "DecreaseVolume = ",
                ShowSongTimerEnabledIdentifier = "ShowSongTimer = ",
                ForceReEnumerationEnabledIdentifier = "ForceReEnumeration = ",
                RainbowStringsEnabledIdentifier = "RainbowStrings = ",
                ExtendedRangeEnabledIdentifier = "ExtendedRange = ",
                ExtendedRangeDropTuningIdentifier = "ExtendedRangeDropTuning = ",
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
                RemoveLyricsWhenIdentifier = "RemoveLyricsWhen = ",
                GuitarSpeakIdentifier = "GuitarSpeak = ",
                RemoveHeadstockWhenIdentifier = "RemoveHeadstockWhen = ",
                ScreenShotScoresIdentifier = "ScreenShotScores = ",
                RiffRepeaterAboveHundredIdentifier = "RRSpeedAboveOneHundred = ",
                MidiAutoTuningIdentifier = "AutoTuneForSong = ",
                MidiAutoTuningDeviceIdentifier = "AutoTuneForSongDevice = ",
                ChordsModeIdentifier = "ChordsMode = ",
                ShowCurrentNoteOnScreenIdentifier = "ShowCurrentNoteOnScreen = ",
                OnScreenFontIdentifier = "OnScreenFont = ",

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
                CheckForNewSongIntervalIdentifier = "CheckForNewSongsInterval = ",
                RiffRepeaterSpeedIntervalIdentifier = "RRSpeedInterval = ",
                TuningPedalIdentifier = "TuningPedal = ",

                // Guitar Speak
                GuitarSpeakDeleteIdentifier = "GuitarSpeakDeleteWhen = ",
                GuitarSpeakSpaceIdentifier = "GuitarSpeakSpaceWhen = ",
                GuitarSpeakEnterIdentifier = "GuitarSpeakEnterWhen = ",
                GuitarSpeakTabIdentifier = "GuitarSpeakTabWhen = ",
                GuitarSpeakPGUPIdentifier = "GuitarSpeakPGUPWhen = ",
                GuitarSpeakPGDNIdentifier = "GuitarSpeakPGDNWhen = ",
                GuitarSpeakUPIdentifier = "GuitarSpeakUPWhen = ",
                GuitarSpeakDNIdentifier = "GuitarSpeanDNWhen = ",
                GuitarSpeakESCIdentifier = "GuitarSpeakESCWhen = ",
                GuitarSpeakCloseIdentifier = "GuitarSpeakCloseWhen = ",
                GuitarSpeakOBracketIdentifier = "GuitarSpeakOBracketWhen = ",
                GuitarSpeakCBracketIdentifier = "GuitarSpeakCBracketWhen = ",
                GuitarSpeakTildeaIdentifier = "GuitarSpeakTildeaWhen = ",
                GuitarSpeakForSlashIdentifier = "GuitarSpeakForSlashWhen = ",
                GuitarSpeakTuningIdentifier = "GuitarSpeakWhileTuning = ",


                // GUI Settings
                CustomGUIThemeIdentifier = "CustomTheme = ",
                CustomGUIBackgroundColorIdentifier = "ThemeBackgroundColor = ",
                CustomGUITextColorIdentifier = "ThemeTextColor = ";

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
                    if (currentLine.Contains(Songlist1Identifier))
                    {
                        Songlist1Name = currentLine.Substring(Songlist1Identifier.Length, (currentLine.Length - Songlist1Identifier.Length));
                        if (identifierToGrab == Songlist1Identifier)
                            return Songlist1Name;
                    }
                    if (currentLine.Contains(Songlist2Identifier))
                    {
                        Songlist2Name = currentLine.Substring(Songlist2Identifier.Length, (currentLine.Length - Songlist2Identifier.Length));
                        if (identifierToGrab == Songlist2Identifier)
                            return Songlist2Name;
                    }
                    if (currentLine.Contains(Songlist3Identifier))
                    {
                        Songlist3Name = currentLine.Substring(Songlist3Identifier.Length, (currentLine.Length - Songlist3Identifier.Length));
                        if (identifierToGrab == Songlist3Identifier)
                            return Songlist3Name;
                    }
                    if (currentLine.Contains(Songlist4Identifier))
                    {
                        Songlist4Name = currentLine.Substring(Songlist4Identifier.Length, (currentLine.Length - Songlist4Identifier.Length));
                        if (identifierToGrab == Songlist4Identifier)
                            return Songlist4Name;
                    }
                    if (currentLine.Contains(Songlist5Identifier))
                    {
                        Songlist5Name = currentLine.Substring(Songlist5Identifier.Length, (currentLine.Length - Songlist5Identifier.Length));
                        if (identifierToGrab == Songlist5Identifier)
                            return Songlist5Name;
                    }
                    if (currentLine.Contains(Songlist6Identifier))
                    {
                        Songlist6Name = currentLine.Substring(Songlist6Identifier.Length, (currentLine.Length - Songlist6Identifier.Length));
                        if (identifierToGrab == Songlist6Identifier)
                            return Songlist6Name;
                    }

                    // Mods
                    if (currentLine.Contains(ToggleLoftIdentifier))
                    {
                        ToggleLoftKey = currentLine.Substring(ToggleLoftIdentifier.Length, (currentLine.Length - ToggleLoftIdentifier.Length));
                        if (KeyConversion.VirtualKey(ToggleLoftKey) != "")
                            ToggleLoftKey = KeyConversion.VirtualKey(ToggleLoftKey);
                        if (identifierToGrab == ToggleLoftIdentifier)
                            return ToggleLoftKey;
                    }

                    if (currentLine.Contains(AddVolumeIdentifier))
                    {
                        AddVolumeKey = currentLine.Substring(AddVolumeIdentifier.Length, (currentLine.Length - AddVolumeIdentifier.Length));
                        if (KeyConversion.VirtualKey(AddVolumeKey) != "")
                            AddVolumeKey = KeyConversion.VirtualKey(AddVolumeKey);
                        if (identifierToGrab == AddVolumeIdentifier)
                            return AddVolumeKey;
                    }
                    if (currentLine.Contains(DecreaseVolumeIdentifier))
                    {
                        DecreaseVolumeKey = currentLine.Substring(DecreaseVolumeIdentifier.Length, (currentLine.Length - DecreaseVolumeIdentifier.Length));
                        if (KeyConversion.VirtualKey(DecreaseVolumeKey) != "")
                            DecreaseVolumeKey = KeyConversion.VirtualKey(DecreaseVolumeKey);
                        if (identifierToGrab == DecreaseVolumeIdentifier)
                            return DecreaseVolumeKey;
                    }
                    if (currentLine.Contains(ChangeSelectedVolumeKeyIdentifier))
                    {
                        ChangeSelectedVolumeKey = currentLine.Substring(ChangeSelectedVolumeKeyIdentifier.Length, (currentLine.Length - ChangeSelectedVolumeKeyIdentifier.Length));
                        if (KeyConversion.VirtualKey(ChangeSelectedVolumeKey) != "")
                            ChangeSelectedVolumeKey = KeyConversion.VirtualKey(ChangeSelectedVolumeKey);
                        if (identifierToGrab == ChangeSelectedVolumeKeyIdentifier)
                            return ChangeSelectedVolumeKey;
                    }
                    if (currentLine.Contains(ShowSongTimerIdentifier))
                    {
                        ShowSongTimerKey = currentLine.Substring(ShowSongTimerIdentifier.Length, (currentLine.Length - ShowSongTimerIdentifier.Length));
                        if (KeyConversion.VirtualKey(ShowSongTimerKey) != "")
                            ShowSongTimerKey = KeyConversion.VirtualKey(ShowSongTimerKey);
                        if (identifierToGrab == ShowSongTimerIdentifier)
                            return ShowSongTimerKey;
                    }
                    if (currentLine.Contains(ForceReEnumerationIdentifier))
                    {
                        ForceReEnumerationKey = currentLine.Substring(ForceReEnumerationIdentifier.Length, (currentLine.Length - ForceReEnumerationIdentifier.Length));
                        if (KeyConversion.VirtualKey(ForceReEnumerationKey) != "")
                            ForceReEnumerationKey = KeyConversion.VirtualKey(ForceReEnumerationKey);
                        if (identifierToGrab == ForceReEnumerationIdentifier)
                            return ForceReEnumerationKey;
                    }
                    if (currentLine.Contains(RainbowStringsIdentifier))
                    {
                        RainbowStringsKey = currentLine.Substring(RainbowStringsIdentifier.Length, (currentLine.Length - RainbowStringsIdentifier.Length));
                        if (KeyConversion.VirtualKey(RainbowStringsKey) != "")
                            RainbowStringsKey = KeyConversion.VirtualKey(RainbowStringsKey);
                        if (identifierToGrab == RainbowStringsIdentifier)
                            return RainbowStringsKey;
                    }

                    if (currentLine.Contains(RemoveLyricsKeyIdentifier))
                    {
                        RemoveLyricsKey = currentLine.Substring(RemoveLyricsKeyIdentifier.Length, (currentLine.Length - RemoveLyricsKeyIdentifier.Length));
                        if (KeyConversion.VirtualKey(RemoveLyricsKey) != "")
                            RemoveLyricsKey = KeyConversion.VirtualKey(RemoveLyricsKey);
                        if (identifierToGrab == RemoveLyricsKeyIdentifier)
                            return RemoveLyricsKey;
                    }
                    
                    if (currentLine.Contains(RRSpeedKeyIdentifier))
                    {
                        RRSpeedKey = currentLine.Substring(RRSpeedKeyIdentifier.Length, (currentLine.Length - RRSpeedKeyIdentifier.Length));
                        if (KeyConversion.VirtualKey(RRSpeedKey) != "")
                            RRSpeedKey = KeyConversion.VirtualKey(RRSpeedKey);
                        if (identifierToGrab == RRSpeedKeyIdentifier)
                            return RRSpeedKey;
                    }

                    // Mods Enabled / Disabled
                    if (currentLine.Contains(ToggleLoftEnabledIdentifier))
                    {
                        if (currentLine.Substring(ToggleLoftEnabledIdentifier.Length, (currentLine.Length - ToggleLoftEnabledIdentifier.Length)) == "on")
                            ToggleLoftEnabled = "on";
                        else
                            ToggleLoftEnabled = "off";

                        if (identifierToGrab == ToggleLoftEnabledIdentifier)
                            return ToggleLoftEnabled;
                    }
                    if (currentLine.Contains(AddVolumeEnabledIdentifier))
                    {
                        if (currentLine.Substring(AddVolumeEnabledIdentifier.Length, (currentLine.Length - AddVolumeEnabledIdentifier.Length)) == "on")
                            AddVolumeEnabled = "on";
                        else
                            AddVolumeEnabled = "off";

                        if (identifierToGrab == AddVolumeEnabledIdentifier)
                            return AddVolumeEnabled;
                    }
                    if (currentLine.Contains(DecreaseVolumeEnabledIdentifier))
                    {
                        if (currentLine.Substring(DecreaseVolumeEnabledIdentifier.Length, (currentLine.Length - DecreaseVolumeEnabledIdentifier.Length)) == "on")
                            DecreaseVolumeEnabled = "on";
                        else
                            DecreaseVolumeEnabled = "off";

                        if (identifierToGrab == DecreaseVolumeEnabledIdentifier)
                            return DecreaseVolumeEnabled;
                    }
                    if (currentLine.Contains(ShowSongTimerEnabledIdentifier))
                    {
                        if (currentLine.Substring(ShowSongTimerEnabledIdentifier.Length, (currentLine.Length - ShowSongTimerEnabledIdentifier.Length)) == "on")
                            ShowSongTimerEnabled = "on";
                        else
                            ShowSongTimerEnabled = "off";

                        if (identifierToGrab == ShowSongTimerEnabledIdentifier)
                            return ShowSongTimerEnabled;
                    }
                    if (currentLine.Contains(ForceReEnumerationEnabledIdentifier))
                    {
                        if (currentLine.Substring(ForceReEnumerationEnabledIdentifier.Length, (currentLine.Length - ForceReEnumerationEnabledIdentifier.Length)) == "manual")
                            ForceReEnumerationEnabled = "manual";
                        if (currentLine.Substring(ForceReEnumerationEnabledIdentifier.Length, (currentLine.Length - ForceReEnumerationEnabledIdentifier.Length)) == "automatic")
                            ForceReEnumerationEnabled = "automatic";
                        if (currentLine.Substring(ForceReEnumerationEnabledIdentifier.Length, (currentLine.Length - ForceReEnumerationEnabledIdentifier.Length)) == "off")
                            ForceReEnumerationEnabled = "off";

                        if (identifierToGrab == ForceReEnumerationEnabledIdentifier)
                            return ForceReEnumerationEnabled;
                    }
                    if (currentLine.Contains(RainbowStringsEnabledIdentifier))
                    {
                        if (currentLine.Substring(RainbowStringsEnabledIdentifier.Length, (currentLine.Length - RainbowStringsEnabledIdentifier.Length)) == "on")
                            RainbowStringsEnabled = "on";
                        else
                            RainbowStringsEnabled = "off";

                        if (identifierToGrab == RainbowStringsEnabledIdentifier)
                            return RainbowStringsEnabled;
                    }
                    if (currentLine.Contains(ExtendedRangeEnabledIdentifier))
                    {
                        if (currentLine.Substring(ExtendedRangeEnabledIdentifier.Length, (currentLine.Length - ExtendedRangeEnabledIdentifier.Length)) == "on")
                            ExtendedRangeEnabled = "on";
                        else
                            ExtendedRangeEnabled = "off";

                        if (identifierToGrab == ExtendedRangeEnabledIdentifier)
                            return ExtendedRangeEnabled;
                    }
                    if (currentLine.Contains(ExtendedRangeDropTuningIdentifier))
                    {
                        if (currentLine.Substring(ExtendedRangeDropTuningIdentifier.Length, (currentLine.Length - ExtendedRangeDropTuningIdentifier.Length)) == "on")
                            ExtendedRangeDropTuning = "on";
                        else
                            ExtendedRangeDropTuning = "off";

                        if (identifierToGrab == ExtendedRangeDropTuningIdentifier)
                            return ExtendedRangeDropTuning;
                    }
                    if (currentLine.Contains(CustomStringColorNumberIndetifier))
                    {
                        CustomStringColorsNumber = currentLine.Substring(CustomStringColorNumberIndetifier.Length, (currentLine.Length - CustomStringColorNumberIndetifier.Length));

                        if (identifierToGrab == CustomStringColorNumberIndetifier)
                            return CustomStringColorsNumber;
                    }
                    if (currentLine.Contains(DiscoModeIdentifier))
                    {
                        if (currentLine.Substring(DiscoModeIdentifier.Length, (currentLine.Length - DiscoModeIdentifier.Length)) == "on")
                            DiscoModeEnabled = "on";
                        else
                            DiscoModeEnabled = "off";

                        if (identifierToGrab == DiscoModeIdentifier)
                            return DiscoModeEnabled;
                    }
                    if (currentLine.Contains(RemoveHeadstockIdentifier))
                    {
                        if (currentLine.Substring(RemoveHeadstockIdentifier.Length, (currentLine.Length - RemoveHeadstockIdentifier.Length)) == "on")
                            RemoveHeadstockEnabled = "on";
                        else
                            RemoveHeadstockEnabled = "off";

                        if (identifierToGrab == RemoveHeadstockIdentifier)
                            return RemoveHeadstockEnabled;
                    }
                    if (currentLine.Contains(RemoveSkylineIdentifier))
                    {
                        if (currentLine.Substring(RemoveSkylineIdentifier.Length, (currentLine.Length - RemoveSkylineIdentifier.Length)) == "on")
                            RemoveSkylineEnabled = "on";
                        else
                            RemoveSkylineEnabled = "off";

                        if (identifierToGrab == RemoveSkylineIdentifier)
                            return RemoveSkylineEnabled;
                    }
                    if (currentLine.Contains(GreenScreenWallIdentifier))
                    {
                        if (currentLine.Substring(GreenScreenWallIdentifier.Length, (currentLine.Length - GreenScreenWallIdentifier.Length)) == "on")
                            GreenscreenWallEnabled = "on";
                        else
                            GreenscreenWallEnabled = "off";

                        if (identifierToGrab == GreenScreenWallIdentifier)
                            return GreenscreenWallEnabled;
                    }

                    if (currentLine.Contains(ForceProfileEnabledIdentifier))
                    {
                        if (currentLine.Substring(ForceProfileEnabledIdentifier.Length, (currentLine.Length - ForceProfileEnabledIdentifier.Length)) == "on")
                            ForceProfileEnabled = "on";
                        else
                            ForceProfileEnabled = "off";
                        if (identifierToGrab == ForceProfileEnabledIdentifier)
                            return ForceProfileEnabled;
                    }
                    if (currentLine.Contains(FretlessModeEnabledIdentifier))
                    {
                        if (currentLine.Substring(FretlessModeEnabledIdentifier.Length, (currentLine.Length - FretlessModeEnabledIdentifier.Length)) == "on")
                            FretlessEnabled = "on";
                        else
                            FretlessEnabled = "off";

                        if (identifierToGrab == FretlessModeEnabledIdentifier)
                            return FretlessEnabled;
                    }
                    if (currentLine.Contains(RemoveInlaysIdentifier))
                    {
                        if (currentLine.Substring(RemoveInlaysIdentifier.Length, (currentLine.Length - RemoveInlaysIdentifier.Length)) == "on")
                            RemoveInlaysEnabled = "on";
                        else
                            RemoveInlaysEnabled = "off";

                        if (identifierToGrab == RemoveInlaysIdentifier)
                            return RemoveInlaysEnabled;
                    }
                    if (currentLine.Contains(ToggleLoftWhenIdentifier))
                    {
                        if (currentLine.Substring(ToggleLoftWhenIdentifier.Length, (currentLine.Length - ToggleLoftWhenIdentifier.Length)) == "manual")
                            ToggleLoftWhen = "manual";
                        if (currentLine.Substring(ToggleLoftWhenIdentifier.Length, (currentLine.Length - ToggleLoftWhenIdentifier.Length)) == "startup")
                            ToggleLoftWhen = "startup";
                        if (currentLine.Substring(ToggleLoftWhenIdentifier.Length, (currentLine.Length - ToggleLoftWhenIdentifier.Length)) == "song")
                            ToggleLoftWhen = "song";

                        if (identifierToGrab == ToggleLoftWhenIdentifier)
                            return ToggleLoftWhen;
                    }
                    if (currentLine.Contains(RemoveLaneMarkersIdentifier))
                    {
                        if (currentLine.Substring(RemoveLaneMarkersIdentifier.Length, (currentLine.Length - RemoveLaneMarkersIdentifier.Length)) == "on")
                            RemoveLaneMarkersEnabled = "on";
                        else
                            RemoveLaneMarkersEnabled = "off";

                        if (identifierToGrab == RemoveLaneMarkersIdentifier)
                            return RemoveLaneMarkersEnabled;
                    }
                    if (currentLine.Contains(ToggleSkylineWhenIdentifier))
                    {
                        if (currentLine.Substring(ToggleSkylineWhenIdentifier.Length, (currentLine.Length - ToggleSkylineWhenIdentifier.Length)) == "song")
                            ToggleSkylineWhen = "song";
                        if (currentLine.Substring(ToggleSkylineWhenIdentifier.Length, (currentLine.Length - ToggleSkylineWhenIdentifier.Length)) == "startup")
                            ToggleSkylineWhen = "startup";

                        if (identifierToGrab == ToggleSkylineWhenIdentifier)
                            return ToggleSkylineWhen;
                    }

                    if (currentLine.Contains(RemoveLyricsIdentifier))
                    {
                        if (currentLine.Substring(RemoveLyricsIdentifier.Length, currentLine.Length - RemoveLyricsIdentifier.Length) == "on")
                            RemoveLyricsEnabled = "on";
                        else
                            RemoveLyricsEnabled = "off";

                        if (identifierToGrab == RemoveLyricsIdentifier)
                            return RemoveLyricsEnabled;
                    }

                    if(currentLine.Contains(RemoveLyricsWhenIdentifier))
                    {
                        if(currentLine.Substring(RemoveLyricsWhenIdentifier.Length, currentLine.Length - RemoveLyricsWhenIdentifier.Length) == "startup")
                            RemoveLyricsWhen = "startup";
                        else
                            RemoveLyricsWhen = "manual";

                        if(identifierToGrab == RemoveLyricsWhenIdentifier)
                            return RemoveLyricsWhen;
                    }
                    if(currentLine.Contains(GuitarSpeakIdentifier))
                    {
                        if (currentLine.Substring(GuitarSpeakIdentifier.Length, (currentLine.Length - GuitarSpeakIdentifier.Length)) == "on")
                            GuitarSpeakEnabled = "on";
                        else
                            GuitarSpeakEnabled = "off";

                        if(identifierToGrab == GuitarSpeakIdentifier)
                            return GuitarSpeakEnabled;
                    }
                    if (currentLine.Contains(RemoveHeadstockWhenIdentifier))
                    {
                        if (currentLine.Substring(RemoveHeadstockWhenIdentifier.Length, currentLine.Length - RemoveHeadstockWhenIdentifier.Length) == "startup")
                            RemoveHeadstockWhen = "startup";
                        else
                            RemoveHeadstockWhen = "song";

                        if (identifierToGrab == RemoveHeadstockWhenIdentifier)
                            return RemoveHeadstockWhen;
                    }

                    if (currentLine.Contains(ScreenShotScoresIdentifier))
                    {
                        if (currentLine.Substring(ScreenShotScoresIdentifier.Length, (currentLine.Length - ScreenShotScoresIdentifier.Length)) == "on")
                            ScreenShotScores = "on";
                        else
                            ScreenShotScores = "off";

                        if (identifierToGrab == ScreenShotScoresIdentifier)
                            return ScreenShotScores;
                    }

                    if (currentLine.Contains(RiffRepeaterAboveHundredIdentifier))
                    {
                        if (currentLine.Substring(RiffRepeaterAboveHundredIdentifier.Length, (currentLine.Length - RiffRepeaterAboveHundredIdentifier.Length)) == "on")
                            RiffRepeaterAboveHundred = "on";
                        else
                            RiffRepeaterAboveHundred = "off";

                        if (identifierToGrab == RiffRepeaterAboveHundredIdentifier)
                            return RiffRepeaterAboveHundred;
                    }

                    if (currentLine.Contains(MidiAutoTuningIdentifier))
                    {
                        if (currentLine.Substring(MidiAutoTuningIdentifier.Length, (currentLine.Length - MidiAutoTuningIdentifier.Length)) == "on")
                            MidiAutoTuning = "on";
                        else
                            MidiAutoTuning = "off";

                        if (identifierToGrab == MidiAutoTuningIdentifier)
                            return MidiAutoTuning;
                    }

                    if (currentLine.Contains(MidiAutoTuningDeviceIdentifier))
                    {
                        MidiAutoTuningDevice = currentLine.Substring(MidiAutoTuningDeviceIdentifier.Length, (currentLine.Length - MidiAutoTuningDeviceIdentifier.Length));

                        if (identifierToGrab == MidiAutoTuningDeviceIdentifier)
                            return MidiAutoTuningDevice;
                    }

                    if (currentLine.Contains(ChordsModeIdentifier))
                    {
                        if (currentLine.Substring(ChordsModeIdentifier.Length, (currentLine.Length - ChordsModeIdentifier.Length)) == "on")
                            ChordsMode = "on";
                        else
                            ChordsMode = "off";

                        if (identifierToGrab == ChordsModeIdentifier)
                            return ChordsMode;
                    }

                    if (currentLine.Contains(ShowCurrentNoteOnScreenIdentifier))
                    {
                        if (currentLine.Substring(ShowCurrentNoteOnScreenIdentifier.Length, (currentLine.Length - ShowCurrentNoteOnScreenIdentifier.Length)) == "on")
                            ShowCurrentNoteOnScreen = "on";
                        else
                            ShowCurrentNoteOnScreen = "off";

                        if (identifierToGrab == ShowCurrentNoteOnScreenIdentifier)
                            return ShowCurrentNoteOnScreen;
                    }

                    if (currentLine.Contains(OnScreenFontIdentifier))
                    {
                        OnScreenFont = currentLine.Substring(OnScreenFontIdentifier.Length, (currentLine.Length - OnScreenFontIdentifier.Length));

                        if (identifierToGrab == OnScreenFontIdentifier)
                            return OnScreenFont;
                    }

                    // String Colors (Normal {N} & Colorblind {CB})

                    // Normal Colors
                        if (currentLine.Contains(String0Color_N_Identifier))
                        {
                            String0Color_N = currentLine.Substring(String0Color_N_Identifier.Length, (currentLine.Length - String0Color_N_Identifier.Length));

                            if (identifierToGrab == String0Color_N_Identifier)
                                return String0Color_N;
                        }
                        if (currentLine.Contains(String1Color_N_Identifier))
                        {
                            String1Color_N = currentLine.Substring(String1Color_N_Identifier.Length, (currentLine.Length - String1Color_N_Identifier.Length));

                            if (identifierToGrab == String1Color_N_Identifier)
                                return String1Color_N;
                        }
                        if (currentLine.Contains(String2Color_N_Identifier))
                        {
                            String2Color_N = currentLine.Substring(String2Color_N_Identifier.Length, (currentLine.Length - String2Color_N_Identifier.Length));

                            if (identifierToGrab == String2Color_N_Identifier)
                                return String2Color_N;
                        }
                        if (currentLine.Contains(String3Color_N_Identifier))
                        {
                            String3Color_N = currentLine.Substring(String3Color_N_Identifier.Length, (currentLine.Length - String3Color_N_Identifier.Length));

                            if (identifierToGrab == String3Color_N_Identifier)
                                return String3Color_N;
                        }
                        if (currentLine.Contains(String4Color_N_Identifier))
                        {
                            String4Color_N = currentLine.Substring(String4Color_N_Identifier.Length, (currentLine.Length - String4Color_N_Identifier.Length));

                            if (identifierToGrab == String4Color_N_Identifier)
                                return String4Color_N;
                        }
                        if (currentLine.Contains(String5Color_N_Identifier))
                        {
                            String5Color_N = currentLine.Substring(String5Color_N_Identifier.Length, (currentLine.Length - String5Color_N_Identifier.Length));

                            if (identifierToGrab == String5Color_N_Identifier)
                                return String5Color_N;
                        }

                    // Color Blind Colors
                        if (currentLine.Contains(String0Color_CB_Identifier))
                        {
                            String0Color_CB = currentLine.Substring(String0Color_CB_Identifier.Length, (currentLine.Length - String0Color_CB_Identifier.Length));

                            if (identifierToGrab == String0Color_CB_Identifier)
                                return String0Color_CB;
                        }
                        if (currentLine.Contains(String1Color_CB_Identifier))
                        {
                            String1Color_CB = currentLine.Substring(String1Color_CB_Identifier.Length, (currentLine.Length - String1Color_CB_Identifier.Length));

                            if (identifierToGrab == String1Color_CB_Identifier)
                                return String1Color_CB;
                        }
                        if (currentLine.Contains(String2Color_CB_Identifier))
                        {
                            String2Color_CB = currentLine.Substring(String2Color_CB_Identifier.Length, (currentLine.Length - String2Color_CB_Identifier.Length));

                            if (identifierToGrab == String2Color_CB_Identifier)
                                return String2Color_CB;
                        }
                        if (currentLine.Contains(String3Color_CB_Identifier))
                        {
                            String3Color_CB = currentLine.Substring(String3Color_CB_Identifier.Length, (currentLine.Length - String3Color_CB_Identifier.Length));

                            if (identifierToGrab == String3Color_CB_Identifier)
                                return String3Color_CB;
                        }
                        if (currentLine.Contains(String4Color_CB_Identifier))
                        {
                            String4Color_CB = currentLine.Substring(String4Color_CB_Identifier.Length, (currentLine.Length - String4Color_CB_Identifier.Length));

                            if (identifierToGrab == String4Color_CB_Identifier)
                                return String4Color_CB;
                        }
                        if (currentLine.Contains(String5Color_CB_Identifier))
                        {
                            String5Color_CB = currentLine.Substring(String5Color_CB_Identifier.Length, (currentLine.Length - String5Color_CB_Identifier.Length));

                            if (identifierToGrab == String5Color_CB_Identifier)
                                return String5Color_CB;
                        }

                    // Mod Settings
                    if (currentLine.Contains(ExtendedRangeTuningIdentifier))
                    {
                        ExtendedRangeTuning = currentLine.Substring(ExtendedRangeTuningIdentifier.Length, (currentLine.Length - ExtendedRangeTuningIdentifier.Length));

                        if (identifierToGrab == ExtendedRangeTuningIdentifier)
                            return ExtendedRangeTuning;
                    }

                    if (currentLine.Contains(CheckForNewSongIntervalIdentifier))
                    {
                        CheckForNewSongInterval = currentLine.Substring(CheckForNewSongIntervalIdentifier.Length, (currentLine.Length - CheckForNewSongIntervalIdentifier.Length));

                        if (identifierToGrab == CheckForNewSongIntervalIdentifier)
                            return CheckForNewSongInterval;
                    }

                    if (currentLine.Contains(RiffRepeaterSpeedIntervalIdentifier))
                    {
                        RiffRepeaterSpeedInterval = currentLine.Substring(RiffRepeaterSpeedIntervalIdentifier.Length, (currentLine.Length - RiffRepeaterSpeedIntervalIdentifier.Length));

                        if (identifierToGrab == RiffRepeaterSpeedIntervalIdentifier)
                            return RiffRepeaterSpeedInterval;
                    }

                    if (currentLine.Contains(TuningPedalIdentifier))
                    {
                        TuningPedal = currentLine.Substring(TuningPedalIdentifier.Length, (currentLine.Length - TuningPedalIdentifier.Length));

                        if (identifierToGrab == TuningPedalIdentifier)
                            return TuningPedal;
                    }

                    // Guitar Speak
                    if(currentLine.Contains(GuitarSpeakDeleteIdentifier))
                    {
                        GuitarSpeakDelete = currentLine.Substring(GuitarSpeakDeleteIdentifier.Length, (currentLine.Length - GuitarSpeakDeleteIdentifier.Length));

                        if (identifierToGrab == GuitarSpeakDeleteIdentifier)
                            return GuitarSpeakDelete;
                    }
                    if(currentLine.Contains(GuitarSpeakSpaceIdentifier))
                    {
                        GuitarSpeakSpace = currentLine.Substring(GuitarSpeakSpaceIdentifier.Length, (currentLine.Length - GuitarSpeakSpaceIdentifier.Length));

                        if (identifierToGrab == GuitarSpeakSpaceIdentifier)
                            return GuitarSpeakSpace;
                    }
                    if(currentLine.Contains(GuitarSpeakEnterIdentifier))
                    {
                        GuitarSpeakEnter = currentLine.Substring(GuitarSpeakEnterIdentifier.Length, (currentLine.Length - GuitarSpeakEnterIdentifier.Length));

                        if (identifierToGrab == GuitarSpeakEnterIdentifier)
                            return GuitarSpeakEnter;
                    }
                    if(currentLine.Contains(GuitarSpeakTabIdentifier))
                    {
                        GuitarSpeakTab = currentLine.Substring(GuitarSpeakTabIdentifier.Length, (currentLine.Length - GuitarSpeakTabIdentifier.Length));

                        if (identifierToGrab == GuitarSpeakTabIdentifier)
                            return GuitarSpeakTab;
                    }
                    if(currentLine.Contains(GuitarSpeakPGUPIdentifier))
                    {
                        GuitarSpeakPGUP = currentLine.Substring(GuitarSpeakPGUPIdentifier.Length, (currentLine.Length - GuitarSpeakPGUPIdentifier.Length));

                        if (identifierToGrab == GuitarSpeakPGUPIdentifier)
                            return GuitarSpeakPGUP;
                    }
                    if(currentLine.Contains(GuitarSpeakPGDNIdentifier))
                    {
                        GuitarSpeakPGDN = currentLine.Substring(GuitarSpeakPGDNIdentifier.Length, (currentLine.Length - GuitarSpeakPGDNIdentifier.Length));

                        if (identifierToGrab == GuitarSpeakPGDNIdentifier)
                            return GuitarSpeakPGDN;
                    }
                    if(currentLine.Contains(GuitarSpeakUPIdentifier))
                    {
                        GuitarSpeakUP = currentLine.Substring(GuitarSpeakUPIdentifier.Length, (currentLine.Length - GuitarSpeakUPIdentifier.Length));

                        if (identifierToGrab == GuitarSpeakUPIdentifier)
                            return GuitarSpeakUP;
                    }
                    if(currentLine.Contains(GuitarSpeakDNIdentifier))
                    {
                        GuitarSpeakDN = currentLine.Substring(GuitarSpeakDNIdentifier.Length, (currentLine.Length - GuitarSpeakDNIdentifier.Length));

                        if (identifierToGrab == GuitarSpeakDNIdentifier)
                            return GuitarSpeakDN;
                    }
                    if(currentLine.Contains(GuitarSpeakESCIdentifier))
                    {
                        GuitarSpeakESC = currentLine.Substring(GuitarSpeakESCIdentifier.Length, (currentLine.Length - GuitarSpeakESCIdentifier.Length));

                        if (identifierToGrab == GuitarSpeakESCIdentifier)
                            return GuitarSpeakESC;
                    }
                    if(currentLine.Contains(GuitarSpeakCloseIdentifier))
                    {
                        GuitarSpeakClose = currentLine.Substring(GuitarSpeakCloseIdentifier.Length, (currentLine.Length - GuitarSpeakCloseIdentifier.Length));

                        if (identifierToGrab == GuitarSpeakCloseIdentifier)
                            return GuitarSpeakClose;
                    }
                    if (currentLine.Contains(GuitarSpeakOBracketIdentifier))
                    {
                        GuitarSpeakOBracket = currentLine.Substring(GuitarSpeakOBracketIdentifier.Length, (currentLine.Length - GuitarSpeakOBracketIdentifier.Length));

                        if (identifierToGrab == GuitarSpeakOBracketIdentifier)
                            return GuitarSpeakOBracket;
                    }
                    if (currentLine.Contains(GuitarSpeakCBracketIdentifier))
                    {
                        GuitarSpeakCBracket = currentLine.Substring(GuitarSpeakCBracketIdentifier.Length, (currentLine.Length - GuitarSpeakCBracketIdentifier.Length));

                        if (identifierToGrab == GuitarSpeakCBracketIdentifier)
                            return GuitarSpeakCBracket;
                    }
                    if (currentLine.Contains(GuitarSpeakTildeaIdentifier))
                    {
                        GuitarSpeakTildea = currentLine.Substring(GuitarSpeakTildeaIdentifier.Length, (currentLine.Length - GuitarSpeakTildeaIdentifier.Length));

                        if (identifierToGrab == GuitarSpeakTildeaIdentifier)
                            return GuitarSpeakTildea;
                    }
                    if (currentLine.Contains(GuitarSpeakForSlashIdentifier))
                    {
                        GuitarSpeakForSlash = currentLine.Substring(GuitarSpeakForSlashIdentifier.Length, (currentLine.Length - GuitarSpeakForSlashIdentifier.Length));

                        if (identifierToGrab == GuitarSpeakForSlashIdentifier)
                            return GuitarSpeakForSlash;
                    }

                    if (currentLine.Contains(GuitarSpeakTuningIdentifier))
                    {
                        if (currentLine.Substring(GuitarSpeakTuningIdentifier.Length, (currentLine.Length - GuitarSpeakTuningIdentifier.Length)) == "on")
                            GuitarSpeakWhileTuning = "on";
                        else
                            GuitarSpeakWhileTuning = "off";

                        if (identifierToGrab == GuitarSpeakTuningIdentifier)
                            return GuitarSpeakWhileTuning;
                    }

                    // GUI Settings
                    if (currentLine.Contains(CustomGUIThemeIdentifier))
                    {
                        if (currentLine.Substring(CustomGUIThemeIdentifier.Length, (currentLine.Length - CustomGUIThemeIdentifier.Length)) == "on")
                            CustomGUITheme = "on";
                        else
                            CustomGUITheme = "off";

                        if (identifierToGrab == CustomGUIThemeIdentifier)
                            return CustomGUITheme;
                    }
                    if (currentLine.Contains(CustomGUIBackgroundColorIdentifier))
                    {
                        CustomGUIBackgroundColor = currentLine.Substring(CustomGUIBackgroundColorIdentifier.Length, (currentLine.Length - CustomGUIBackgroundColorIdentifier.Length));

                        if (identifierToGrab == CustomGUIBackgroundColorIdentifier)
                            return CustomGUIBackgroundColor;
                    }
                    if (currentLine.Contains(CustomGUITextColorIdentifier))
                    {
                        CustomGUITextColor = currentLine.Substring(CustomGUITextColorIdentifier.Length, (currentLine.Length - CustomGUITextColorIdentifier.Length));

                        if (identifierToGrab == CustomGUITextColorIdentifier)
                            return CustomGUITextColor;
                    }
                }
                return ""; // Yeah, we don't know what you're looking for...
            }
        }
    }
}
