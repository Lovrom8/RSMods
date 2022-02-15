using System.IO;
using RSMods.Util;

#pragma warning disable IDE0052

namespace RSMods
{
    class ReadSettings
    {
        #region Setup Variables
        private static string
                             // Song Lists (ones that come without modifying the profile).
                             Songlist1Name, Songlist2Name, Songlist3Name, Songlist4Name, Songlist5Name, Songlist6Name,
                             // Song Lists (ones that have to be added manually by modifying the user's profile).
                             Songlist7Name, Songlist8Name, Songlist9Name, Songlist10Name, Songlist11Name, Songlist12Name,
                             Songlist13Name, Songlist14Name, Songlist15Name, Songlist16Name, Songlist17Name, Songlist18Name,
                             Songlist19Name, Songlist20Name,

                             // Mod Key Bindings
                             ToggleLoftKey, ShowSongTimerKey, ForceReEnumerationKey, RainbowStringsKey, RainbowNotesKey,
                             RemoveLyricsKey, RRSpeedKey, TuningOffsetKey, ToggleExtendedRangeKey, LoopStartKey, LoopEndKey,
                             RewindKey,

                             // Audio Key Bindings
                             MasterVolumeKey, SongVolumeKey, Player1VolumeKey, Player2VolumeKey, MicrophoneVolumeKey, VoiceOverVolumeKey, SFXVolumeKey, ChangeSelectedVolumeKey,

                             // Mod On / Off
                             ToggleLoftEnabled, VolumeControlEnabled, ShowSongTimerEnabled, ForceReEnumerationEnabled, RainbowStringsEnabled, RainbowNotesEnabled,
                             ExtendedRangeEnabled, ExtendedRangeDropTuning, ExtendedRangeFixBassTuning, CustomStringColorsNumber, SeparateNoteColors,
                             DiscoModeEnabled, RemoveHeadstockEnabled, RemoveSkylineEnabled, GreenscreenWallEnabled, ForceProfileEnabled, FretlessEnabled, RemoveInlaysEnabled, ToggleLoftWhen,
                             ToggleSkylineWhen, RemoveLaneMarkersEnabled, RemoveLyricsEnabled, RemoveLyricsWhen, GuitarSpeakEnabled, RemoveHeadstockWhen, ScreenShotScores,
                             MidiAutoTuning, MidiAutoTuningDevice, MidiInDevice, MidiAutoTuningWhen, MidiSoftwareSemitoneTriggers, MidiSoftwareSemitoneSettings, MidiSoftwareTrueTuningTriggers, MidiSoftwareTrueTuningSettings, ChordsMode,
                             RiffRepeaterAboveHundred, ShowCurrentNoteOnScreen, OnScreenFont, ProfileToLoad, ShowSongTimerWhen, ShowSelectedVolumeWhen, SecondaryMonitor, RemoveSongPreviews, OverrideInputVolumeEnabled, OverrideInputVolumeDevice,
                             AllowAudioInBackground, BypassTwoRTCMessageBox, LinearRiffRepeater, UseAlternativeOutputSampleRate, AllowLooping, AllowRewind, FixOculusCrash, FixBrokenTones, UseCustomNSPTimer,


                             // String Colors
                             String0Color_N, String1Color_N, String2Color_N, String3Color_N, String4Color_N, String5Color_N,
                             String0Color_CB, String1Color_CB, String2Color_CB, String3Color_CB, String4Color_CB, String5Color_CB,
                             SeparateNoteColorsMode,
                             Note0Color_N, Note1Color_N, Note2Color_N, Note3Color_N, Note4Color_N, Note5Color_N,
                             Note0Color_CB, Note1Color_CB, Note2Color_CB, Note3Color_CB, Note4Color_CB, Note5Color_CB,


                             // Mod Settings
                             ExtendedRangeTuning, CheckForNewSongInterval, RiffRepeaterSpeedInterval, TuningPedal, MidiTuningOffset,
                             VolumeControlInterval, SecondaryMonitorXPosition, SecondaryMonitorYPosition, OverrideInputVolume, AlternativeOutputSampleRate, LoopingLeadUp,
                             RewindBy, CustomNSPTimeLimit,

                             // Guitar Speak
                             GuitarSpeakDelete, GuitarSpeakSpace, GuitarSpeakEnter, GuitarSpeakTab, GuitarSpeakPGUP, GuitarSpeakPGDN, GuitarSpeakUP, GuitarSpeakDN, GuitarSpeakESC,
                             GuitarSpeakClose, GuitarSpeakOBracket, GuitarSpeakCBracket, GuitarSpeakTildea, GuitarSpeakForSlash, GuitarSpeakAlt, GuitarSpeakWhileTuning,

                             // Highway Colors
                             CustomHighwayColors, CustomHighwayNumbered, CustomHighwayUnNumbered, CustomHighwayGutter, CustomFretNubmers,

                             // GUI Settings
                             CustomGUITheme, CustomGUIBackgroundColor, CustomGUITextColor, CustomGUIButtonColor, BackupProfile, NumberOfBackups, SpeedUpLoadup;

        public static string
            // Song List Identifiers (the ones that come with the game)
            Songlist1Identifier = "SongListTitle_1 = ",
            Songlist2Identifier = "SongListTitle_2 = ",
            Songlist3Identifier = "SongListTitle_3 = ",
            Songlist4Identifier = "SongListTitle_4 = ",
            Songlist5Identifier = "SongListTitle_5 = ",
            Songlist6Identifier = "SongListTitle_6 = ",

            // Song List Identifiers (the ones that require profile modification)
            Songlist7Identifier  = "SongListTitle_7 = ",
            Songlist8Identifier  = "SongListTitle_8 = ",
            Songlist9Identifier  = "SongListTitle_9 = ",
            Songlist10Identifier = "SongListTitle_10 = ",
            Songlist11Identifier = "SongListTitle_11 = ",
            Songlist12Identifier = "SongListTitle_12 = ",
            Songlist13Identifier = "SongListTitle_13 = ",
            Songlist14Identifier = "SongListTitle_14 = ",
            Songlist15Identifier = "SongListTitle_15 = ",
            Songlist16Identifier = "SongListTitle_16 = ",
            Songlist17Identifier = "SongListTitle_17 = ",
            Songlist18Identifier = "SongListTitle_18 = ",
            Songlist19Identifier = "SongListTitle_19 = ",
            Songlist20Identifier = "SongListTitle_20 = ",

            // Keybindings
            ToggleLoftIdentifier                = "ToggleLoftKey = ",
            ShowSongTimerIdentifier             = "ShowSongTimerKey = ",
            ForceReEnumerationIdentifier        = "ForceReEnumerationKey = ",
            RainbowStringsIdentifier            = "RainbowStringsKey = ",
            RainbowNotesIdentifier              = "RainbowNotesKey = ",
            RemoveLyricsKeyIdentifier           = "RemoveLyricsKey = ",
            RRSpeedKeyIdentifier                = "RRSpeedKey = ",
            TuningOffsetKeyIdentifier           = "TuningOffsetKey = ",
            ToggleExtendedRangeKeyIdentifier    = "ToggleExtendedRangeKey = ",
            LoopStartKeyIdentifier              = "LoopStartKey = ",
            LoopEndKeyIdentifier                = "LoopEndKey = ",
            RewindKeyIdentifier                 = "RewindKey = ",

            // Audio Keybindings
            MasterVolumeKeyIdentifier           = "MasterVolumeKey = ",
            SongVolumeKeyIdentifier             = "SongVolumeKey = ",
            Player1VolumeKeyIdentifier          = "Player1VolumeKey = ",
            Player2VolumeKeyIdentifier          = "Player2VolumeKey = ",
            MicrophoneVolumeKeyIdentifier       = "MicrophoneVolumeKey = ",
            VoiceOverVolumeKeyIdentifier        = "VoiceOverVolumeKey = ",
            SFXVolumeKeyIdentifier              = "SFXVolumeKey = ",
            ChangeSelectedVolumeKeyIdentifier   = "ChangedSelectedVolumeKey = ",

            // Toggle Effects
            ToggleLoftEnabledIdentifier                 = "ToggleLoft = ",
            VolumeControlEnabledIdentifier              = "VolumeControl = ",
            ShowSongTimerEnabledIdentifier              = "ShowSongTimer = ",
            ForceReEnumerationEnabledIdentifier         = "ForceReEnumeration = ",
            RainbowStringsEnabledIdentifier             = "RainbowStrings = ",
            RainbowNotesEnabledIdentifier               = "RainbowNotes = ",
            ExtendedRangeEnabledIdentifier              = "ExtendedRange = ",
            ExtendedRangeDropTuningIdentifier           = "ExtendedRangeDropTuning = ",
            ExtendedRangeFixBassTuningIdentifier        = "ExtendedRangeFixBassTuning = ",
            CustomStringColorNumberIndetifier           = "CustomStringColors = ",
            SeparateNoteColorsIdentifier                = "SeparateNoteColors = ",
            DiscoModeIdentifier                         = "DiscoMode = ",
            RemoveHeadstockIdentifier                   = "Headstock = ",
            RemoveSkylineIdentifier                     = "Skyline = ",
            GreenScreenWallIdentifier                   = "GreenScreenWall = ",
            ForceProfileEnabledIdentifier               = "ForceProfileLoad = ",
            FretlessModeEnabledIdentifier               = "Fretless = ",
            RemoveInlaysIdentifier                      = "Inlays = ",
            ToggleLoftWhenIdentifier                    = "ToggleLoftWhen = ",
            ToggleSkylineWhenIdentifier                 = "ToggleSkylineWhen = ",
            RemoveLaneMarkersIdentifier                 = "LaneMarkers = ",
            RemoveLyricsIdentifier                      = "Lyrics = ",
            RemoveLyricsWhenIdentifier                  = "RemoveLyricsWhen = ",
            GuitarSpeakIdentifier                       = "GuitarSpeak = ",
            RemoveHeadstockWhenIdentifier               = "RemoveHeadstockWhen = ",
            ScreenShotScoresIdentifier                  = "ScreenShotScores = ",
            RiffRepeaterAboveHundredIdentifier          = "RRSpeedAboveOneHundred = ",
            MidiAutoTuningIdentifier                    = "AutoTuneForSong = ",
            MidiAutoTuningDeviceIdentifier              = "AutoTuneForSongDevice = ",
            MidiInDeviceIdentifier                      = "MidiInDevice = ",
            MidiAutoTuningWhenIdentifier                = "AutoTuneForSongWhen = ",
            MidiSoftwareSemitoneTriggersIdentifier      = "AutoTuneForSoftwareSemitoneTriggers = ",
            MidiSoftwareSemitoneSettingsIdentifier      = "AutoTuneForSoftwareSemitoneSettings = ",
            MidiSoftwareTrueTuningTriggersIdentifier    = "AutoTuneForSoftwareTrueTuningTriggers = ",
            MidiSoftwareTrueTuningSettingsIdentifier    = "AutoTuneForSoftwareTrueTuningSettings = ",
            ChordsModeIdentifier                        = "ChordsMode = ",
            ShowCurrentNoteOnScreenIdentifier           = "ShowCurrentNoteOnScreen = ",
            OnScreenFontIdentifier                      = "OnScreenFont = ",
            ProfileToLoadIdentifier                     = "ProfileToLoad = ",
            ShowSongTimerWhenIdentifier                 = "ShowSongTimerWhen = ",
            ShowSelectedVolumeWhenIdentifier            = "ShowSelectedVolumeWhen = ",
            SecondaryMonitorIdentifier                  = "SecondaryMonitor = ",
            RemoveSongPreviewsIdentifier                = "SongPreviews = ",
            OverrideInputVolumeEnabledIdentifier        = "OverrideInputVolumeEnabled = ",
            OverrideInputVolumeDeviceIdentifier         = "OverrideInputVolumeDevice = ",
            AllowAudioInBackgroundIdentifier            = "AllowAudioInBackground = ",
            BypassTwoRTCMessageBoxIdentifier            = "BypassTwoRTCMessageBox = ",
            LinearRiffRepeaterIdentifier                = "LinearRiffRepeater = ",
            UseAlternativeOutputSampleRateIdentifier    = "AltOutputSampleRate = ",
            AllowLoopingIdentifier                      = "AllowLooping = ",
            AllowRewindIdentifier                       = "AllowRewind = ",
            FixOculusCrashIdentifier                    = "FixOculusCrash = ",
            FixBrokenTonesIdentifier                    = "FixBrokenTones = ",
            UseCustomNSPTimerIdentifier                 = "UseCustomNSPTimer = ",


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

                // Note Colors (Normal {N} & Colorblind {CB})
                // Normal Note Colors
                Note0Color_N_Identifier = "note0_N = ",
                Note1Color_N_Identifier = "note1_N = ",
                Note2Color_N_Identifier = "note2_N = ",
                Note3Color_N_Identifier = "note3_N = ",
                Note4Color_N_Identifier = "note4_N = ",
                Note5Color_N_Identifier = "note5_N = ",
                // Colorblind Note Colors
                Note0Color_CB_Identifier = "note0_CB = ",
                Note1Color_CB_Identifier = "note1_CB = ",
                Note2Color_CB_Identifier = "note2_CB = ",
                Note3Color_CB_Identifier = "note3_CB = ",
                Note4Color_CB_Identifier = "note4_CB = ",
                Note5Color_CB_Identifier = "note5_CB = ",


            // Mod Settings
            ExtendedRangeTuningIdentifier           = "ExtendedRangeModeAt = ",
            CheckForNewSongIntervalIdentifier       = "CheckForNewSongsInterval = ",
            RiffRepeaterSpeedIntervalIdentifier     = "RRSpeedInterval = ",
            TuningPedalIdentifier                   = "TuningPedal = ",
            MidiTuningOffsetIdentifier              = "TuningOffset = ",
            VolumeControlIntervalIdentifier         = "VolumeControlInterval = ",
            SecondaryMonitorXPositionIdentifier     = "SecondaryMonitorXPosition = ",
            SecondaryMonitorYPositionIdentifier     = "SecondaryMonitorYPosition = ",
            SeparateNoteColorsModeIdentifier        = "SeparateNoteColorsMode = ",
            OverrideInputVolumeIdentifier           = "OverrideInputVolume = ",
            AlternativeOutputSampleRateIdentifier   = "AlternativeOutputSampleRate = ",
            LoopingLeadUpIdentifier                 = "LoopingLeadUp = ",
            RewindByIdentifier                      = "RewindBy = ",
            CustomNSPTimeLimitIdentifier            = "CustomNSPTimeLimit = ",

            // Guitar Speak
            GuitarSpeakDeleteIdentifier     = "GuitarSpeakDeleteWhen = ",
            GuitarSpeakSpaceIdentifier      = "GuitarSpeakSpaceWhen = ",
            GuitarSpeakEnterIdentifier      = "GuitarSpeakEnterWhen = ",
            GuitarSpeakTabIdentifier        = "GuitarSpeakTabWhen = ",
            GuitarSpeakPGUPIdentifier       = "GuitarSpeakPGUPWhen = ",
            GuitarSpeakPGDNIdentifier       = "GuitarSpeakPGDNWhen = ",
            GuitarSpeakUPIdentifier         = "GuitarSpeakUPWhen = ",
            GuitarSpeakDNIdentifier         = "GuitarSpeakDNWhen = ",
            GuitarSpeakESCIdentifier        = "GuitarSpeakESCWhen = ",
            GuitarSpeakCloseIdentifier      = "GuitarSpeakCloseWhen = ",
            GuitarSpeakOBracketIdentifier   = "GuitarSpeakOBracketWhen = ",
            GuitarSpeakCBracketIdentifier   = "GuitarSpeakCBracketWhen = ",
            GuitarSpeakTildeaIdentifier     = "GuitarSpeakTildeaWhen = ",
            GuitarSpeakForSlashIdentifier   = "GuitarSpeakForSlashWhen = ",
            GuitarSpeakAltIdentifier        = "GuitarSpeakAltWhen = ",
            GuitarSpeakTuningIdentifier     = "GuitarSpeakWhileTuning = ",

            // Highway Colors
            CustomHighwayColorsIdentifier       = "CustomHighwayColors = ",
            CustomHighwayNumberedIdentifier     = "CustomHighwayNumbered = ",
            CustomHighwayUnNumberedIdentifier   = "CustomHighwayUnNumbered = ",
            CustomHighwayGutterIdentifier       = "CustomHighwayGutter = ",
            CustomFretNubmersIdentifier         = "CustomFretNubmers = ",

            // GUI Settings
            CustomGUIThemeIdentifier            = "CustomTheme = ",
            CustomGUIBackgroundColorIdentifier  = "ThemeBackgroundColor = ",
            CustomGUITextColorIdentifier        = "ThemeTextColor = ",
            CustomGUIButtonColorIdentifier      = "ThemeButtonColor = ",
            BackupProfileIdentifier             = "BackupProfile = ",
            NumberOfBackupsIdentifier           = "NumberOfBackups = ",
            SpeedUpLoadupIdentifier             = "SpeedUpLoadup = ";

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
            if (!DoesSettingsINIExist())
                WriteSettings.WriteINI(WriteSettings.saveSettingsOrDefaults); // Creates Settings File
        }

        public static bool DoesSettingsINIExist() => File.Exists(Path.Combine(GenUtil.GetRSDirectory(), "RSMods.ini"));
        #endregion
        public static string ProcessSettings(string identifierToGrab)
        {
            VerifySettingsINI();
            foreach (string currentLine in File.ReadLines(Path.Combine(GenUtil.GetRSDirectory(), "RSMods.ini")))
            {
                #region Song Lists
                // Song Lists (the ones that come with the game by default).

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

                // Song Lists (the ones that have to manually be added).
                if (IdentifierIsFound(currentLine, Songlist7Identifier, identifierToGrab))
                    return FillSettingVariable(Songlist7Identifier, SettingType.STRING, currentLine, out Songlist7Name);
                if (IdentifierIsFound(currentLine, Songlist8Identifier, identifierToGrab))
                    return FillSettingVariable(Songlist8Identifier, SettingType.STRING, currentLine, out Songlist8Name);
                if (IdentifierIsFound(currentLine, Songlist9Identifier, identifierToGrab))
                    return FillSettingVariable(Songlist9Identifier, SettingType.STRING, currentLine, out Songlist9Name);
                if (IdentifierIsFound(currentLine, Songlist10Identifier, identifierToGrab))
                    return FillSettingVariable(Songlist10Identifier, SettingType.STRING, currentLine, out Songlist10Name);
                if (IdentifierIsFound(currentLine, Songlist11Identifier, identifierToGrab))
                    return FillSettingVariable(Songlist11Identifier, SettingType.STRING, currentLine, out Songlist11Name);
                if (IdentifierIsFound(currentLine, Songlist12Identifier, identifierToGrab))
                    return FillSettingVariable(Songlist12Identifier, SettingType.STRING, currentLine, out Songlist12Name);
                if (IdentifierIsFound(currentLine, Songlist13Identifier, identifierToGrab))
                    return FillSettingVariable(Songlist13Identifier, SettingType.STRING, currentLine, out Songlist13Name);
                if (IdentifierIsFound(currentLine, Songlist14Identifier, identifierToGrab))
                    return FillSettingVariable(Songlist14Identifier, SettingType.STRING, currentLine, out Songlist14Name);
                if (IdentifierIsFound(currentLine, Songlist15Identifier, identifierToGrab))
                    return FillSettingVariable(Songlist15Identifier, SettingType.STRING, currentLine, out Songlist15Name);
                if (IdentifierIsFound(currentLine, Songlist16Identifier, identifierToGrab))
                    return FillSettingVariable(Songlist16Identifier, SettingType.STRING, currentLine, out Songlist16Name);
                if (IdentifierIsFound(currentLine, Songlist17Identifier, identifierToGrab))
                    return FillSettingVariable(Songlist17Identifier, SettingType.STRING, currentLine, out Songlist17Name);
                if (IdentifierIsFound(currentLine, Songlist18Identifier, identifierToGrab))
                    return FillSettingVariable(Songlist18Identifier, SettingType.STRING, currentLine, out Songlist18Name);
                if (IdentifierIsFound(currentLine, Songlist19Identifier, identifierToGrab))
                    return FillSettingVariable(Songlist19Identifier, SettingType.STRING, currentLine, out Songlist19Name);
                if (IdentifierIsFound(currentLine, Songlist20Identifier, identifierToGrab))
                    return FillSettingVariable(Songlist20Identifier, SettingType.STRING, currentLine, out Songlist20Name);
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
                if (IdentifierIsFound(currentLine, TuningOffsetKeyIdentifier, identifierToGrab))
                    return FillSettingVariable(TuningOffsetKeyIdentifier, SettingType.VKEY, currentLine, out TuningOffsetKey);
                if (IdentifierIsFound(currentLine, ToggleExtendedRangeKeyIdentifier, identifierToGrab))
                    return FillSettingVariable(ToggleExtendedRangeKeyIdentifier, SettingType.VKEY, currentLine, out ToggleExtendedRangeKey);
                if (IdentifierIsFound(currentLine, LoopStartKeyIdentifier, identifierToGrab))
                    return FillSettingVariable(LoopStartKeyIdentifier, SettingType.VKEY, currentLine, out LoopStartKey);
                if (IdentifierIsFound(currentLine, LoopEndKeyIdentifier, identifierToGrab))
                    return FillSettingVariable(LoopEndKeyIdentifier, SettingType.VKEY, currentLine, out LoopEndKey);
                if (IdentifierIsFound(currentLine, RewindKeyIdentifier, identifierToGrab))
                    return FillSettingVariable(RewindKeyIdentifier, SettingType.VKEY, currentLine, out RewindKey);
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
                if (IdentifierIsFound(currentLine, ExtendedRangeFixBassTuningIdentifier, identifierToGrab))
                    return FillSettingVariable(ExtendedRangeFixBassTuningIdentifier, SettingType.ON_OFF, currentLine, out ExtendedRangeFixBassTuning);
                if (IdentifierIsFound(currentLine, CustomStringColorNumberIndetifier, identifierToGrab))
                    return FillSettingVariable(CustomStringColorNumberIndetifier, SettingType.STRING, currentLine, out CustomStringColorsNumber);
                if (IdentifierIsFound(currentLine, SeparateNoteColorsIdentifier, identifierToGrab))
                    return FillSettingVariable(SeparateNoteColorsIdentifier, SettingType.STRING, currentLine, out SeparateNoteColors);
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
                if (IdentifierIsFound(currentLine, MidiInDeviceIdentifier, identifierToGrab))
                    return FillSettingVariable(MidiInDeviceIdentifier, SettingType.STRING, currentLine, out MidiInDevice);
                if (IdentifierIsFound(currentLine, MidiAutoTuningWhenIdentifier, identifierToGrab))
                    return FillSettingVariable(MidiAutoTuningWhenIdentifier, SettingType.STRING, currentLine, out MidiAutoTuningWhen);
                if (IdentifierIsFound(currentLine, MidiSoftwareSemitoneSettingsIdentifier, identifierToGrab))
                    return FillSettingVariable(MidiSoftwareSemitoneSettingsIdentifier, SettingType.STRING, currentLine, out MidiSoftwareSemitoneSettings);
                if (IdentifierIsFound(currentLine, MidiSoftwareSemitoneTriggersIdentifier, identifierToGrab))
                    return FillSettingVariable(MidiSoftwareSemitoneTriggersIdentifier, SettingType.STRING, currentLine, out MidiSoftwareSemitoneTriggers);
                if (IdentifierIsFound(currentLine, MidiSoftwareTrueTuningSettingsIdentifier, identifierToGrab))
                    return FillSettingVariable(MidiSoftwareTrueTuningSettingsIdentifier, SettingType.STRING, currentLine, out MidiSoftwareTrueTuningSettings);
                if (IdentifierIsFound(currentLine, MidiSoftwareTrueTuningTriggersIdentifier, identifierToGrab))
                    return FillSettingVariable(MidiSoftwareTrueTuningTriggersIdentifier, SettingType.STRING, currentLine, out MidiSoftwareTrueTuningTriggers);
                if (IdentifierIsFound(currentLine, ChordsModeIdentifier, identifierToGrab))
                    return FillSettingVariable(ChordsModeIdentifier, SettingType.ON_OFF, currentLine, out ChordsMode);
                if (IdentifierIsFound(currentLine, ShowCurrentNoteOnScreenIdentifier, identifierToGrab))
                    return FillSettingVariable(ShowCurrentNoteOnScreenIdentifier, SettingType.ON_OFF, currentLine, out ShowCurrentNoteOnScreen);
                if (IdentifierIsFound(currentLine, OnScreenFontIdentifier, identifierToGrab))
                    return FillSettingVariable(OnScreenFontIdentifier, SettingType.STRING, currentLine, out OnScreenFont);
                if (IdentifierIsFound(currentLine, ProfileToLoadIdentifier, identifierToGrab))
                    return FillSettingVariable(ProfileToLoadIdentifier, SettingType.STRING, currentLine, out ProfileToLoad);
                if (IdentifierIsFound(currentLine, ShowSongTimerWhenIdentifier, identifierToGrab))
                    return FillSettingVariable(ShowSongTimerWhenIdentifier, SettingType.STRING, currentLine, out ShowSongTimerWhen);
                if (IdentifierIsFound(currentLine, ShowSelectedVolumeWhenIdentifier, identifierToGrab))
                    return FillSettingVariable(ShowSelectedVolumeWhenIdentifier, SettingType.STRING, currentLine, out ShowSelectedVolumeWhen);
                if (IdentifierIsFound(currentLine, SecondaryMonitorIdentifier, identifierToGrab))
                    return FillSettingVariable(SecondaryMonitorIdentifier, SettingType.STRING, currentLine, out SecondaryMonitor);
                if (IdentifierIsFound(currentLine, RemoveSongPreviewsIdentifier, identifierToGrab))
                    return FillSettingVariable(RemoveSongPreviewsIdentifier, SettingType.ON_OFF, currentLine, out RemoveSongPreviews);
                if (IdentifierIsFound(currentLine, OverrideInputVolumeEnabledIdentifier, identifierToGrab))
                    return FillSettingVariable(OverrideInputVolumeEnabledIdentifier, SettingType.ON_OFF, currentLine, out OverrideInputVolumeEnabled);
                if (IdentifierIsFound(currentLine, OverrideInputVolumeDeviceIdentifier, identifierToGrab))
                    return FillSettingVariable(OverrideInputVolumeDeviceIdentifier, SettingType.STRING, currentLine, out OverrideInputVolumeDevice);
                if (IdentifierIsFound(currentLine, AllowAudioInBackgroundIdentifier, identifierToGrab))
                    return FillSettingVariable(AllowAudioInBackgroundIdentifier, SettingType.ON_OFF, currentLine, out AllowAudioInBackground);
                if (IdentifierIsFound(currentLine, BypassTwoRTCMessageBoxIdentifier, identifierToGrab))
                    return FillSettingVariable(BypassTwoRTCMessageBoxIdentifier, SettingType.ON_OFF, currentLine, out BypassTwoRTCMessageBox);
                if (IdentifierIsFound(currentLine, LinearRiffRepeaterIdentifier, identifierToGrab))
                    return FillSettingVariable(LinearRiffRepeaterIdentifier, SettingType.ON_OFF, currentLine, out LinearRiffRepeater);
                if (IdentifierIsFound(currentLine, UseAlternativeOutputSampleRateIdentifier, identifierToGrab))
                    return FillSettingVariable(UseAlternativeOutputSampleRateIdentifier, SettingType.ON_OFF, currentLine, out UseAlternativeOutputSampleRate);
                if (IdentifierIsFound(currentLine, AllowLoopingIdentifier, identifierToGrab))
                    return FillSettingVariable(AllowLoopingIdentifier, SettingType.ON_OFF, currentLine, out AllowLooping);
                if (IdentifierIsFound(currentLine, AllowRewindIdentifier, identifierToGrab))
                    return FillSettingVariable(AllowRewindIdentifier, SettingType.ON_OFF, currentLine, out AllowRewind);
                if (IdentifierIsFound(currentLine, FixOculusCrashIdentifier, identifierToGrab))
                    return FillSettingVariable(FixOculusCrashIdentifier, SettingType.ON_OFF, currentLine, out FixOculusCrash);
                if (IdentifierIsFound(currentLine, FixBrokenTonesIdentifier, identifierToGrab))
                    return FillSettingVariable(FixBrokenTonesIdentifier, SettingType.ON_OFF, currentLine, out FixBrokenTones);
                if (IdentifierIsFound(currentLine, UseCustomNSPTimerIdentifier, identifierToGrab))
                    return FillSettingVariable(UseCustomNSPTimerIdentifier, SettingType.ON_OFF, currentLine, out UseCustomNSPTimer);
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

                // Note Colors (Normal {N} & Colorblind {CB})

                // Normal Colors
                if (IdentifierIsFound(currentLine, Note0Color_N_Identifier, identifierToGrab))
                    return FillSettingVariable(Note0Color_N_Identifier, SettingType.STRING, currentLine, out Note0Color_N);
                if (IdentifierIsFound(currentLine, Note1Color_N_Identifier, identifierToGrab))
                    return FillSettingVariable(Note1Color_N_Identifier, SettingType.STRING, currentLine, out Note1Color_N);
                if (IdentifierIsFound(currentLine, Note2Color_N_Identifier, identifierToGrab))
                    return FillSettingVariable(Note2Color_N_Identifier, SettingType.STRING, currentLine, out Note2Color_N);
                if (IdentifierIsFound(currentLine, Note3Color_N_Identifier, identifierToGrab))
                    return FillSettingVariable(Note3Color_N_Identifier, SettingType.STRING, currentLine, out Note3Color_N);
                if (IdentifierIsFound(currentLine, Note4Color_N_Identifier, identifierToGrab))
                    return FillSettingVariable(Note4Color_N_Identifier, SettingType.STRING, currentLine, out Note4Color_N);
                if (IdentifierIsFound(currentLine, Note5Color_N_Identifier, identifierToGrab))
                    return FillSettingVariable(Note5Color_N_Identifier, SettingType.STRING, currentLine, out Note5Color_N);

                // Color Blind Colors
                if (IdentifierIsFound(currentLine, Note0Color_CB_Identifier, identifierToGrab))
                    return FillSettingVariable(Note0Color_CB_Identifier, SettingType.STRING, currentLine, out Note0Color_CB);
                if (IdentifierIsFound(currentLine, Note1Color_CB_Identifier, identifierToGrab))
                    return FillSettingVariable(Note1Color_CB_Identifier, SettingType.STRING, currentLine, out Note1Color_CB);
                if (IdentifierIsFound(currentLine, Note2Color_CB_Identifier, identifierToGrab))
                    return FillSettingVariable(Note2Color_CB_Identifier, SettingType.STRING, currentLine, out Note2Color_CB);
                if (IdentifierIsFound(currentLine, Note3Color_CB_Identifier, identifierToGrab))
                    return FillSettingVariable(Note3Color_CB_Identifier, SettingType.STRING, currentLine, out Note3Color_CB);
                if (IdentifierIsFound(currentLine, Note4Color_CB_Identifier, identifierToGrab))
                    return FillSettingVariable(Note4Color_CB_Identifier, SettingType.STRING, currentLine, out Note4Color_CB);
                if (IdentifierIsFound(currentLine, Note5Color_CB_Identifier, identifierToGrab))
                    return FillSettingVariable(Note5Color_CB_Identifier, SettingType.STRING, currentLine, out Note5Color_CB);
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
                if (IdentifierIsFound(currentLine, MidiTuningOffsetIdentifier, identifierToGrab))
                    return FillSettingVariable(MidiTuningOffsetIdentifier, SettingType.STRING, currentLine, out MidiTuningOffset);
                if (IdentifierIsFound(currentLine, VolumeControlIntervalIdentifier, identifierToGrab))
                    return FillSettingVariable(VolumeControlIntervalIdentifier, SettingType.STRING, currentLine, out VolumeControlInterval);
                if (IdentifierIsFound(currentLine, SecondaryMonitorXPositionIdentifier, identifierToGrab))
                    return FillSettingVariable(SecondaryMonitorXPositionIdentifier, SettingType.STRING, currentLine, out SecondaryMonitorXPosition);
                if (IdentifierIsFound(currentLine, SecondaryMonitorYPositionIdentifier, identifierToGrab))
                    return FillSettingVariable(SecondaryMonitorYPositionIdentifier, SettingType.STRING, currentLine, out SecondaryMonitorYPosition);
                if (IdentifierIsFound(currentLine, SeparateNoteColorsModeIdentifier, identifierToGrab))
                    return FillSettingVariable(SeparateNoteColorsModeIdentifier, SettingType.STRING, currentLine, out SeparateNoteColorsMode);
                if (IdentifierIsFound(currentLine, OverrideInputVolumeIdentifier, identifierToGrab))
                    return FillSettingVariable(OverrideInputVolumeIdentifier, SettingType.STRING, currentLine, out OverrideInputVolume);
                if (IdentifierIsFound(currentLine, AlternativeOutputSampleRateIdentifier, identifierToGrab))
                    return FillSettingVariable(AlternativeOutputSampleRateIdentifier, SettingType.STRING, currentLine, out AlternativeOutputSampleRate);
                if (IdentifierIsFound(currentLine, LoopingLeadUpIdentifier, identifierToGrab))
                    return FillSettingVariable(LoopingLeadUpIdentifier, SettingType.STRING, currentLine, out LoopingLeadUp);
                if (IdentifierIsFound(currentLine, RewindByIdentifier, identifierToGrab))
                    return FillSettingVariable(RewindByIdentifier, SettingType.STRING, currentLine, out RewindBy);
                if (IdentifierIsFound(currentLine, CustomNSPTimeLimitIdentifier, identifierToGrab))
                    return FillSettingVariable(CustomNSPTimeLimitIdentifier, SettingType.STRING, currentLine, out CustomNSPTimeLimit);
                
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
                if (IdentifierIsFound(currentLine, GuitarSpeakAltIdentifier, identifierToGrab))
                    return FillSettingVariable(GuitarSpeakAltIdentifier, SettingType.STRING, currentLine, out GuitarSpeakAlt);
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
                if (IdentifierIsFound(currentLine, CustomGUIButtonColorIdentifier, identifierToGrab))
                    return FillSettingVariable(CustomGUIButtonColorIdentifier, SettingType.STRING, currentLine, out CustomGUIButtonColor);
                if (IdentifierIsFound(currentLine, BackupProfileIdentifier, identifierToGrab))
                    return FillSettingVariable(BackupProfileIdentifier, SettingType.ON_OFF, currentLine, out BackupProfile);
                if (IdentifierIsFound(currentLine, NumberOfBackupsIdentifier, identifierToGrab))
                    return FillSettingVariable(NumberOfBackupsIdentifier, SettingType.STRING, currentLine, out NumberOfBackups);
                if (IdentifierIsFound(currentLine, SpeedUpLoadupIdentifier, identifierToGrab))
                    return FillSettingVariable(SpeedUpLoadupIdentifier, SettingType.STRING, currentLine, out SpeedUpLoadup);

                
                #endregion
            }
            return string.Empty; // Yeah, we don't know what you're looking for...
        }
    }
}
