using System.IO;
using RSMods.Util;
using System.Windows.Forms;

#pragma warning disable IDE0052

namespace RSMods
{
    class ReadSettings
    {
        #region Setup Variables
        private static string Songlist1Name, Songlist2Name, Songlist3Name, Songlist4Name, Songlist5Name, Songlist6Name,
                             ToggleLoftKey, ShowSongTimerKey, ForceReEnumerationKey, RainbowStringsKey, RainbowNotesKey, RemoveLyricsKey, RRSpeedKey,
                             MasterVolumeKey, SongVolumeKey, Player1VolumeKey, Player2VolumeKey, MicrophoneVolumeKey, VoiceOverVolumeKey, SFXVolumeKey, ChangeSelectedVolumeKey,
                             ToggleLoftEnabled, VolumeControlEnabled, ShowSongTimerEnabled, ForceReEnumerationEnabled, RainbowStringsEnabled, RainbowNotesEnabled, ExtendedRangeEnabled, ExtendedRangeDropTuning, CustomStringColorsNumber,
                             DiscoModeEnabled, RemoveHeadstockEnabled, RemoveSkylineEnabled, GreenscreenWallEnabled, ForceProfileEnabled, FretlessEnabled, RemoveInlaysEnabled, ToggleLoftWhen,
                             ToggleSkylineWhen, RemoveLaneMarkersEnabled, RemoveLyricsEnabled, RemoveLyricsWhen, GuitarSpeakEnabled, RemoveHeadstockWhen, ScreenShotScores,
                                                RiffRepeaterAboveHundred, MidiAutoTuning, MidiAutoTuningDevice, TuningPedal, ChordsMode, ShowCurrentNoteOnScreen, OnScreenFont, ProfileToLoad,
                             String0Color_N, String1Color_N, String2Color_N, String3Color_N, String4Color_N, String5Color_N, String0Color_CB, String1Color_CB, String2Color_CB, String3Color_CB, String4Color_CB, String5Color_CB,
                             ExtendedRangeTuning, CheckForNewSongInterval, RiffRepeaterSpeedInterval, VolumeControlInterval,
                             GuitarSpeakDelete, GuitarSpeakSpace, GuitarSpeakEnter, GuitarSpeakTab, GuitarSpeakPGUP, GuitarSpeakPGDN, GuitarSpeakUP, GuitarSpeakDN, GuitarSpeakESC,
                                                GuitarSpeakClose, GuitarSpeakOBracket, GuitarSpeakCBracket, GuitarSpeakTildea, GuitarSpeakForSlash, GuitarSpeakWhileTuning,
                             CustomGUITheme, CustomGUIBackgroundColor, CustomGUITextColor, BackupProfile, NumberOfBackups,
                             CustomHighwayColors, CustomHighwayNumbered, CustomHighwayUnNumbered, CustomHighwayGutter, CustomFretNubmers;
        public static string
            // Song List Identifiers
            Songlist1Identifier = "SongListTitle_1 = ",
            Songlist2Identifier = "SongListTitle_2 = ",
            Songlist3Identifier = "SongListTitle_3 = ",
            Songlist4Identifier = "SongListTitle_4 = ",
            Songlist5Identifier = "SongListTitle_5 = ",
            Songlist6Identifier = "SongListTitle_6 = ",

            // Keybindings
            ToggleLoftIdentifier = "ToggleLoftKey = ",
            ShowSongTimerIdentifier = "ShowSongTimerKey = ",
            ForceReEnumerationIdentifier = "ForceReEnumerationKey = ",
            RainbowStringsIdentifier = "RainbowStringsKey = ",
            RainbowNotesIdentifier = "RainbowNotesKey = ",
            RemoveLyricsKeyIdentifier = "RemoveLyricsKey = ",
            RRSpeedKeyIdentifier = "RRSpeedKey = ",

            // Audio Keybindings
            MasterVolumeKeyIdentifier = "MasterVolumeKey = ",
            SongVolumeKeyIdentifier = "SongVolumeKey = ",
            Player1VolumeKeyIdentifier = "Player1VolumeKey = ",
            Player2VolumeKeyIdentifier = "Player2VolumeKey = ",
            MicrophoneVolumeKeyIdentifier = "MicrophoneVolumeKey = ",
            VoiceOverVolumeKeyIdentifier = "VoiceOverVolumeKey = ",
            SFXVolumeKeyIdentifier = "SFXVolumeKey = ",
            ChangeSelectedVolumeKeyIdentifier = "ChangedSelectedVolumeKey = ",

            // Toggle Effects
            ToggleLoftEnabledIdentifier = "ToggleLoft = ",
            VolumeControlEnabledIdentifier = "VolumeControl = ",
            ShowSongTimerEnabledIdentifier = "ShowSongTimer = ",
            ForceReEnumerationEnabledIdentifier = "ForceReEnumeration = ",
            RainbowStringsEnabledIdentifier = "RainbowStrings = ",
            RainbowNotesEnabledIdentifier = "RainbowNotes = ",
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
            ProfileToLoadIdentifier = "ProfileToLoad = ",

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
            VolumeControlIntervalIdentifier = "VolumeControlInterval = ",

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

            // Highway Colors
            CustomHighwayColorsIdentifier = "CustomHighwayColors = ",
            CustomHighwayNumberedIdentifier = "CustomHighwayNumbered = ",
            CustomHighwayUnNumberedIdentifier = "CustomHighwayUnNumbered = ",
            CustomHighwayGutterIdentifier = "CustomHighwayGutter = ",
            CustomFretNubmersIdentifier = "CustomFretNubmers = ",

            // GUI Settings
            CustomGUIThemeIdentifier = "CustomTheme = ",
            CustomGUIBackgroundColorIdentifier = "ThemeBackgroundColor = ",
            CustomGUITextColorIdentifier = "ThemeTextColor = ",
            BackupProfileIdentifier = "BackupProfile = ",
            NumberOfBackupsIdentifier = "NumberOfBackups = ";

        #endregion
        #region Functions to make things look nicer
        enum SettingType
        {
            ON_OFF = 0,
            STRING = 1,
            VKEY = 2
        };

        private static string FillSettingVariable(string settingIdentifier, SettingType settingType, string currentLine, out string setting)
        {
            setting = "";
            if (currentLine.Contains(settingIdentifier))
            {
                if(settingType == SettingType.ON_OFF)
                {
                    if (currentLine.Substring(settingIdentifier.Length, (currentLine.Length - settingIdentifier.Length)) == "on")
                        setting = "on";
                    else
                        setting = "off";
                }

                else if (settingType == SettingType.STRING || settingType == SettingType.VKEY)
                    setting = currentLine.Substring(settingIdentifier.Length, (currentLine.Length - settingIdentifier.Length));

                if (settingType == SettingType.VKEY && KeyConversion.VirtualKey(setting) != "")
                    setting = KeyConversion.VirtualKey(setting);
            }
            return setting;
        }

        private static bool IdentifierIsFound(string currentLine, string settingToFind, string identifierToGrab) => currentLine.Contains(settingToFind) && settingToFind == identifierToGrab;

        private static void VerifySettingsINI()
        {
            if (!File.Exists(Path.Combine(GenUtil.GetRSDirectory(), "RSMods.ini")))
                WriteSettings.WriteINI(WriteSettings.Settings); // Creates Settings File
        }
        #endregion
        public static string ProcessSettings(string identifierToGrab)
        {
            VerifySettingsINI();
            foreach (string currentLine in File.ReadLines(Path.Combine(GenUtil.GetRSDirectory(), "RSMods.ini")))
            {
                #region Song Lists
                // Song Lists

                if (IdentifierIsFound(currentLine, Songlist1Identifier, identifierToGrab))
                    return FillSettingVariable(Songlist1Identifier, SettingType.STRING, currentLine, out Songlist1Name);
                if (IdentifierIsFound(currentLine, Songlist2Identifier, identifierToGrab))
                    return FillSettingVariable(Songlist2Identifier, SettingType.STRING, currentLine, out Songlist2Name);
                if (IdentifierIsFound(currentLine, Songlist3Identifier, identifierToGrab))
                    return FillSettingVariable(Songlist3Identifier, SettingType.STRING, currentLine, out Songlist3Name);
                if (IdentifierIsFound(currentLine, Songlist4Identifier, identifierToGrab))
                    return FillSettingVariable(Songlist4Identifier, SettingType.STRING, currentLine, out Songlist4Name);
                if (IdentifierIsFound(currentLine, Songlist5Identifier, identifierToGrab))
                    return FillSettingVariable(Songlist5Identifier, SettingType.STRING, currentLine, out Songlist5Name);
                if (IdentifierIsFound(currentLine, Songlist6Identifier, identifierToGrab))
                    return FillSettingVariable(Songlist6Identifier, SettingType.STRING, currentLine, out Songlist6Name);
                #endregion
                #region Keybindings
                // Keybindings

                if (IdentifierIsFound(currentLine, ToggleLoftIdentifier, identifierToGrab))
                    return FillSettingVariable(ToggleLoftIdentifier, SettingType.VKEY, currentLine, out ToggleLoftKey);
                if (IdentifierIsFound(currentLine, ShowSongTimerIdentifier, identifierToGrab))
                    return FillSettingVariable(ShowSongTimerIdentifier, SettingType.VKEY, currentLine, out ShowSongTimerKey);
                if (IdentifierIsFound(currentLine, ForceReEnumerationIdentifier, identifierToGrab))
                    return FillSettingVariable(ForceReEnumerationIdentifier, SettingType.VKEY, currentLine, out ForceReEnumerationKey);
                if (IdentifierIsFound(currentLine, RainbowStringsIdentifier, identifierToGrab))
                    return FillSettingVariable(RainbowStringsIdentifier, SettingType.VKEY, currentLine, out RainbowStringsKey);
                if (IdentifierIsFound(currentLine, RainbowNotesIdentifier, identifierToGrab))
                    return FillSettingVariable(RainbowNotesIdentifier, SettingType.VKEY, currentLine, out RainbowNotesKey);
                if (IdentifierIsFound(currentLine, RemoveLyricsKeyIdentifier, identifierToGrab))
                    return FillSettingVariable(RemoveLyricsKeyIdentifier, SettingType.VKEY, currentLine, out RemoveLyricsKey);
                if (IdentifierIsFound(currentLine, RRSpeedKeyIdentifier, identifierToGrab))
                    return FillSettingVariable(RRSpeedKeyIdentifier, SettingType.VKEY, currentLine, out RRSpeedKey);

                #endregion
                #region Audio Keybindings
                // Audio Keybindings

                if (IdentifierIsFound(currentLine, MasterVolumeKeyIdentifier, identifierToGrab))
                    return FillSettingVariable(MasterVolumeKeyIdentifier, SettingType.VKEY, currentLine, out MasterVolumeKey);
                if (IdentifierIsFound(currentLine, SongVolumeKeyIdentifier, identifierToGrab))
                    return FillSettingVariable(SongVolumeKeyIdentifier, SettingType.VKEY, currentLine, out SongVolumeKey);
                if (IdentifierIsFound(currentLine, Player1VolumeKeyIdentifier, identifierToGrab))
                    return FillSettingVariable(Player1VolumeKeyIdentifier, SettingType.VKEY, currentLine, out Player1VolumeKey);
                if (IdentifierIsFound(currentLine, Player2VolumeKeyIdentifier, identifierToGrab))
                    return FillSettingVariable(Player2VolumeKeyIdentifier, SettingType.VKEY, currentLine, out Player2VolumeKey);
                if (IdentifierIsFound(currentLine, MicrophoneVolumeKeyIdentifier, identifierToGrab))
                    return FillSettingVariable(MicrophoneVolumeKeyIdentifier, SettingType.VKEY, currentLine, out MicrophoneVolumeKey);
                if (IdentifierIsFound(currentLine, VoiceOverVolumeKeyIdentifier, identifierToGrab))
                    return FillSettingVariable(VoiceOverVolumeKeyIdentifier, SettingType.VKEY, currentLine, out VoiceOverVolumeKey);
                if (IdentifierIsFound(currentLine, SFXVolumeKeyIdentifier, identifierToGrab))
                    return FillSettingVariable(SFXVolumeKeyIdentifier, SettingType.VKEY, currentLine, out SFXVolumeKey);
                if (IdentifierIsFound(currentLine, ChangeSelectedVolumeKeyIdentifier, identifierToGrab))
                    return FillSettingVariable(ChangeSelectedVolumeKeyIdentifier, SettingType.VKEY, currentLine, out ChangeSelectedVolumeKey);

                #endregion
                #region Toggle Switches
                // Toggle Switches

                if (IdentifierIsFound(currentLine, ToggleLoftEnabledIdentifier, identifierToGrab))
                    return FillSettingVariable(ToggleLoftEnabledIdentifier, SettingType.ON_OFF, currentLine, out ToggleLoftEnabled);
                if (IdentifierIsFound(currentLine, VolumeControlEnabledIdentifier, identifierToGrab))
                    return FillSettingVariable(VolumeControlEnabledIdentifier, SettingType.ON_OFF, currentLine, out VolumeControlEnabled);
                if (IdentifierIsFound(currentLine, ShowSongTimerEnabledIdentifier, identifierToGrab))
                    return FillSettingVariable(ShowSongTimerEnabledIdentifier, SettingType.ON_OFF, currentLine, out ShowSongTimerEnabled);
                if (IdentifierIsFound(currentLine, ForceReEnumerationEnabledIdentifier, identifierToGrab))
                    return FillSettingVariable(ForceReEnumerationEnabledIdentifier, SettingType.STRING, currentLine, out ForceReEnumerationEnabled);
                if (IdentifierIsFound(currentLine, RainbowStringsEnabledIdentifier, identifierToGrab))
                    return FillSettingVariable(RainbowStringsEnabledIdentifier, SettingType.ON_OFF, currentLine, out RainbowStringsEnabled);
                if (IdentifierIsFound(currentLine, RainbowNotesEnabledIdentifier, identifierToGrab))
                    return FillSettingVariable(RainbowNotesEnabledIdentifier, SettingType.ON_OFF, currentLine, out RainbowNotesEnabled);
                if (IdentifierIsFound(currentLine, ExtendedRangeEnabledIdentifier, identifierToGrab))
                    return FillSettingVariable(ExtendedRangeEnabledIdentifier, SettingType.ON_OFF, currentLine, out ExtendedRangeEnabled);
                if (IdentifierIsFound(currentLine, ExtendedRangeDropTuningIdentifier, identifierToGrab))
                    return FillSettingVariable(ExtendedRangeDropTuningIdentifier, SettingType.ON_OFF, currentLine, out ExtendedRangeDropTuning);
                if (IdentifierIsFound(currentLine, CustomStringColorNumberIndetifier, identifierToGrab))
                    return FillSettingVariable(CustomStringColorNumberIndetifier, SettingType.STRING, currentLine, out CustomStringColorsNumber);
                if (IdentifierIsFound(currentLine, DiscoModeIdentifier, identifierToGrab))
                    return FillSettingVariable(DiscoModeIdentifier, SettingType.ON_OFF, currentLine, out DiscoModeEnabled);
                if (IdentifierIsFound(currentLine, RemoveHeadstockIdentifier, identifierToGrab))
                    return FillSettingVariable(RemoveHeadstockIdentifier, SettingType.ON_OFF, currentLine, out RemoveHeadstockEnabled);
                if (IdentifierIsFound(currentLine, RemoveSkylineIdentifier, identifierToGrab))
                    return FillSettingVariable(RemoveSkylineIdentifier, SettingType.ON_OFF, currentLine, out RemoveSkylineEnabled);
                if (IdentifierIsFound(currentLine, GreenScreenWallIdentifier, identifierToGrab))
                    return FillSettingVariable(GreenScreenWallIdentifier, SettingType.ON_OFF, currentLine, out GreenscreenWallEnabled);
                if (IdentifierIsFound(currentLine, ForceProfileEnabledIdentifier, identifierToGrab))
                    return FillSettingVariable(ForceProfileEnabledIdentifier, SettingType.ON_OFF, currentLine, out ForceProfileEnabled);
                if (IdentifierIsFound(currentLine, FretlessModeEnabledIdentifier, identifierToGrab))
                    return FillSettingVariable(FretlessModeEnabledIdentifier, SettingType.ON_OFF, currentLine, out FretlessEnabled);
                if (IdentifierIsFound(currentLine, RemoveInlaysIdentifier, identifierToGrab))
                    return FillSettingVariable(RemoveInlaysIdentifier, SettingType.ON_OFF, currentLine, out RemoveInlaysEnabled);
                if (IdentifierIsFound(currentLine, ToggleLoftWhenIdentifier, identifierToGrab))
                    return FillSettingVariable(ToggleLoftWhenIdentifier, SettingType.STRING, currentLine, out ToggleLoftWhen);
                if (IdentifierIsFound(currentLine, RemoveLaneMarkersIdentifier, identifierToGrab))
                    return FillSettingVariable(RemoveLaneMarkersIdentifier, SettingType.ON_OFF, currentLine, out RemoveLaneMarkersEnabled);
                if (IdentifierIsFound(currentLine, ToggleSkylineWhenIdentifier, identifierToGrab))
                    return FillSettingVariable(ToggleSkylineWhenIdentifier, SettingType.STRING, currentLine, out ToggleSkylineWhen);
                if (IdentifierIsFound(currentLine, RemoveLyricsIdentifier, identifierToGrab))
                    return FillSettingVariable(RemoveLyricsIdentifier, SettingType.ON_OFF, currentLine, out RemoveLyricsEnabled);
                if (IdentifierIsFound(currentLine, RemoveLyricsWhenIdentifier, identifierToGrab))
                    return FillSettingVariable(RemoveLyricsWhenIdentifier, SettingType.STRING, currentLine, out RemoveLyricsWhen);
                if (IdentifierIsFound(currentLine, GuitarSpeakIdentifier, identifierToGrab))
                    return FillSettingVariable(GuitarSpeakIdentifier, SettingType.ON_OFF, currentLine, out GuitarSpeakEnabled);
                if (IdentifierIsFound(currentLine, RemoveHeadstockWhenIdentifier, identifierToGrab))
                    return FillSettingVariable(RemoveHeadstockWhenIdentifier, SettingType.STRING, currentLine, out RemoveHeadstockWhen);
                if (IdentifierIsFound(currentLine, ScreenShotScoresIdentifier, identifierToGrab))
                    return FillSettingVariable(ScreenShotScoresIdentifier, SettingType.ON_OFF, currentLine, out ScreenShotScores);
                if (IdentifierIsFound(currentLine, RiffRepeaterAboveHundredIdentifier, identifierToGrab))
                    return FillSettingVariable(RiffRepeaterAboveHundredIdentifier, SettingType.ON_OFF, currentLine, out RiffRepeaterAboveHundred);
                if (IdentifierIsFound(currentLine, MidiAutoTuningIdentifier, identifierToGrab))
                    return FillSettingVariable(MidiAutoTuningIdentifier, SettingType.ON_OFF, currentLine, out MidiAutoTuning);
                if (IdentifierIsFound(currentLine, MidiAutoTuningDeviceIdentifier, identifierToGrab))
                    return FillSettingVariable(MidiAutoTuningDeviceIdentifier, SettingType.STRING, currentLine, out MidiAutoTuningDevice);
                if (IdentifierIsFound(currentLine, ChordsModeIdentifier, identifierToGrab))
                    return FillSettingVariable(ChordsModeIdentifier, SettingType.ON_OFF, currentLine, out ChordsMode);
                if (IdentifierIsFound(currentLine, ShowCurrentNoteOnScreenIdentifier, identifierToGrab))
                    return FillSettingVariable(ShowCurrentNoteOnScreenIdentifier, SettingType.ON_OFF, currentLine, out ShowCurrentNoteOnScreen);
                if (IdentifierIsFound(currentLine, OnScreenFontIdentifier, identifierToGrab))
                    return FillSettingVariable(OnScreenFontIdentifier, SettingType.STRING, currentLine, out OnScreenFont);
                if (IdentifierIsFound(currentLine, ProfileToLoadIdentifier, identifierToGrab))
                    return FillSettingVariable(ProfileToLoadIdentifier, SettingType.STRING, currentLine, out ProfileToLoad);

                #endregion
                #region String Colors
                // String Colors (Normal {N} & Colorblind {CB})

                // Normal Colors

                if (IdentifierIsFound(currentLine, String0Color_N_Identifier, identifierToGrab))
                    return FillSettingVariable(String0Color_N_Identifier, SettingType.STRING, currentLine, out String0Color_N);
                if (IdentifierIsFound(currentLine, String1Color_N_Identifier, identifierToGrab))
                    return FillSettingVariable(String1Color_N_Identifier, SettingType.STRING, currentLine, out String1Color_N);
                if (IdentifierIsFound(currentLine, String2Color_N_Identifier, identifierToGrab))
                    return FillSettingVariable(String2Color_N_Identifier, SettingType.STRING, currentLine, out String2Color_N);
                if (IdentifierIsFound(currentLine, String3Color_N_Identifier, identifierToGrab))
                    return FillSettingVariable(String3Color_N_Identifier, SettingType.STRING, currentLine, out String3Color_N);
                if (IdentifierIsFound(currentLine, String4Color_N_Identifier, identifierToGrab))
                    return FillSettingVariable(String4Color_N_Identifier, SettingType.STRING, currentLine, out String4Color_N);
                if (IdentifierIsFound(currentLine, String5Color_N_Identifier, identifierToGrab))
                    return FillSettingVariable(String5Color_N_Identifier, SettingType.STRING, currentLine, out String5Color_N);

                // Color Blind Colors
                if (IdentifierIsFound(currentLine, String0Color_CB_Identifier, identifierToGrab))
                    return FillSettingVariable(String0Color_CB_Identifier, SettingType.STRING, currentLine, out String0Color_CB);
                if (IdentifierIsFound(currentLine, String1Color_CB_Identifier, identifierToGrab))
                    return FillSettingVariable(String1Color_CB_Identifier, SettingType.STRING, currentLine, out String1Color_CB);
                if (IdentifierIsFound(currentLine, String2Color_CB_Identifier, identifierToGrab))
                    return FillSettingVariable(String2Color_CB_Identifier, SettingType.STRING, currentLine, out String2Color_CB);
                if (IdentifierIsFound(currentLine, String3Color_CB_Identifier, identifierToGrab))
                    return FillSettingVariable(String3Color_CB_Identifier, SettingType.STRING, currentLine, out String3Color_CB);
                if (IdentifierIsFound(currentLine, String4Color_CB_Identifier, identifierToGrab))
                    return FillSettingVariable(String4Color_CB_Identifier, SettingType.STRING, currentLine, out String4Color_CB);
                if (IdentifierIsFound(currentLine, String5Color_CB_Identifier, identifierToGrab))
                    return FillSettingVariable(String5Color_CB_Identifier, SettingType.STRING, currentLine, out String5Color_CB);
                #endregion
                #region Mod Settings
                // Mod Settings

                if (IdentifierIsFound(currentLine, ExtendedRangeTuningIdentifier, identifierToGrab))
                    return FillSettingVariable(ExtendedRangeTuningIdentifier, SettingType.STRING, currentLine, out ExtendedRangeTuning);
                if (IdentifierIsFound(currentLine, CheckForNewSongIntervalIdentifier, identifierToGrab))
                    return FillSettingVariable(CheckForNewSongIntervalIdentifier, SettingType.STRING, currentLine, out CheckForNewSongInterval);
                if (IdentifierIsFound(currentLine, RiffRepeaterSpeedIntervalIdentifier, identifierToGrab))
                    return FillSettingVariable(RiffRepeaterSpeedIntervalIdentifier, SettingType.STRING, currentLine, out RiffRepeaterSpeedInterval);
                if (IdentifierIsFound(currentLine, TuningPedalIdentifier, identifierToGrab))
                    return FillSettingVariable(TuningPedalIdentifier, SettingType.STRING, currentLine, out TuningPedal);
                if (IdentifierIsFound(currentLine, VolumeControlIntervalIdentifier, identifierToGrab))
                    return FillSettingVariable(VolumeControlIntervalIdentifier, SettingType.STRING, currentLine, out VolumeControlInterval);
                #endregion
                #region Guitar Speak

                // Guitar Speak

                if (IdentifierIsFound(currentLine, GuitarSpeakDeleteIdentifier, identifierToGrab))
                    return FillSettingVariable(GuitarSpeakDeleteIdentifier, SettingType.STRING, currentLine, out GuitarSpeakDelete);
                if (IdentifierIsFound(currentLine, GuitarSpeakSpaceIdentifier, identifierToGrab))
                    return FillSettingVariable(GuitarSpeakSpaceIdentifier, SettingType.STRING, currentLine, out GuitarSpeakSpace);
                if (IdentifierIsFound(currentLine, GuitarSpeakEnterIdentifier, identifierToGrab))
                    return FillSettingVariable(GuitarSpeakEnterIdentifier, SettingType.STRING, currentLine, out GuitarSpeakEnter);
                if (IdentifierIsFound(currentLine, GuitarSpeakTabIdentifier, identifierToGrab))
                    return FillSettingVariable(GuitarSpeakTabIdentifier, SettingType.STRING, currentLine, out GuitarSpeakTab);
                if (IdentifierIsFound(currentLine, GuitarSpeakPGUPIdentifier, identifierToGrab))
                    return FillSettingVariable(GuitarSpeakPGUPIdentifier, SettingType.STRING, currentLine, out GuitarSpeakPGUP);
                if (IdentifierIsFound(currentLine, GuitarSpeakPGDNIdentifier, identifierToGrab))
                    return FillSettingVariable(GuitarSpeakPGDNIdentifier, SettingType.STRING, currentLine, out GuitarSpeakPGDN);
                if (IdentifierIsFound(currentLine, GuitarSpeakUPIdentifier, identifierToGrab))
                    return FillSettingVariable(GuitarSpeakUPIdentifier, SettingType.STRING, currentLine, out GuitarSpeakUP);
                if (IdentifierIsFound(currentLine, GuitarSpeakDNIdentifier, identifierToGrab))
                    return FillSettingVariable(GuitarSpeakDNIdentifier, SettingType.STRING, currentLine, out GuitarSpeakDN);
                if (IdentifierIsFound(currentLine, GuitarSpeakESCIdentifier, identifierToGrab))
                    return FillSettingVariable(GuitarSpeakESCIdentifier, SettingType.STRING, currentLine, out GuitarSpeakESC);
                if (IdentifierIsFound(currentLine, GuitarSpeakCloseIdentifier, identifierToGrab))
                    return FillSettingVariable(GuitarSpeakCloseIdentifier, SettingType.STRING, currentLine, out GuitarSpeakClose);
                if (IdentifierIsFound(currentLine, GuitarSpeakOBracketIdentifier, identifierToGrab))
                    return FillSettingVariable(GuitarSpeakOBracketIdentifier, SettingType.STRING, currentLine, out GuitarSpeakOBracket);
                if (IdentifierIsFound(currentLine, GuitarSpeakCBracketIdentifier, identifierToGrab))
                    return FillSettingVariable(GuitarSpeakCBracketIdentifier, SettingType.STRING, currentLine, out GuitarSpeakCBracket);
                if (IdentifierIsFound(currentLine, GuitarSpeakTildeaIdentifier, identifierToGrab))
                    return FillSettingVariable(GuitarSpeakTildeaIdentifier, SettingType.STRING, currentLine, out GuitarSpeakTildea);
                if (IdentifierIsFound(currentLine, GuitarSpeakForSlashIdentifier, identifierToGrab))
                    return FillSettingVariable(GuitarSpeakForSlashIdentifier, SettingType.STRING, currentLine, out GuitarSpeakForSlash);
                if (IdentifierIsFound(currentLine, GuitarSpeakTuningIdentifier, identifierToGrab))
                    return FillSettingVariable(GuitarSpeakTuningIdentifier, SettingType.ON_OFF, currentLine, out GuitarSpeakWhileTuning);
                #endregion
                #region Highway Colors
                // Highway Colors

                if (IdentifierIsFound(currentLine, CustomHighwayColorsIdentifier, identifierToGrab))
                    return FillSettingVariable(CustomHighwayColorsIdentifier, SettingType.STRING, currentLine, out CustomHighwayColors);
                if (IdentifierIsFound(currentLine, CustomHighwayNumberedIdentifier, identifierToGrab))
                    return FillSettingVariable(CustomHighwayNumberedIdentifier, SettingType.STRING, currentLine, out CustomHighwayNumbered);
                if (IdentifierIsFound(currentLine, CustomHighwayUnNumberedIdentifier, identifierToGrab))
                    return FillSettingVariable(CustomHighwayUnNumberedIdentifier, SettingType.STRING, currentLine, out CustomHighwayUnNumbered);
                if (IdentifierIsFound(currentLine, CustomHighwayGutterIdentifier, identifierToGrab))
                    return FillSettingVariable(CustomHighwayGutterIdentifier, SettingType.STRING, currentLine, out CustomHighwayGutter);
                if (IdentifierIsFound(currentLine, CustomFretNubmersIdentifier, identifierToGrab))
                    return FillSettingVariable(CustomFretNubmersIdentifier, SettingType.STRING, currentLine, out CustomFretNubmers);
                #endregion
                #region GUI Settings
                // GUI Settings

                if (IdentifierIsFound(currentLine, CustomGUIThemeIdentifier, identifierToGrab))
                    return FillSettingVariable(CustomGUIThemeIdentifier, SettingType.ON_OFF, currentLine, out CustomGUITheme);
                if (IdentifierIsFound(currentLine, CustomGUIBackgroundColorIdentifier, identifierToGrab))
                    return FillSettingVariable(CustomGUIBackgroundColorIdentifier, SettingType.STRING, currentLine, out CustomGUIBackgroundColor);
                if (IdentifierIsFound(currentLine, CustomGUITextColorIdentifier, identifierToGrab))
                    return FillSettingVariable(CustomGUITextColorIdentifier, SettingType.STRING, currentLine, out CustomGUITextColor);
                if (IdentifierIsFound(currentLine, BackupProfileIdentifier, identifierToGrab))
                    return FillSettingVariable(BackupProfileIdentifier, SettingType.ON_OFF, currentLine, out BackupProfile);
                if (IdentifierIsFound(currentLine, NumberOfBackupsIdentifier, identifierToGrab))
                    return FillSettingVariable(NumberOfBackupsIdentifier, SettingType.STRING, currentLine, out NumberOfBackups);
                #endregion
            }
            return ""; // Yeah, we don't know what you're looking for...
        }
    }
}
