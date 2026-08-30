using RSMods.Util;
using System;
using System.IO;
using System.Reflection;
using System.Runtime.CompilerServices;

namespace RSMods
{
    public static class RsModsSettings
    {
        private static Action _settingChangedHandler;
        public static event Action SettingChanged
        {
            add { _settingChangedHandler += value; if (_ini != null) _ini.SettingChanged += value; }
            remove { _settingChangedHandler -= value; if (_ini != null) _ini.SettingChanged -= value; }
        }

        private static IniManager _ini;
        private static IniSection _songListTitles;
        private static IniSection _keybinds;
        private static IniSection _audioKeybindings;
        private static IniSection _toggleSwitches;
        private static IniSection _stringColors;
        private static IniSection _modSettings;
        private static IniSection _guitarSpeak;
        private static IniSection _highwayColors;
        private static IniSection _guiSettings;

        public static void LoadSettingsFromINI()
        {
            string path = Path.Combine(GenUtil.GetRSDirectory(), "RSMods.ini");
            _ini = new IniManager(path);
            if (_settingChangedHandler != null)
                _ini.SettingChanged += _settingChangedHandler;

            _ini.Load();

            _songListTitles = new IniSection(_ini, "[SongListTitles]");
            _keybinds = new IniSection(_ini, "[Keybinds]");
            _audioKeybindings = new IniSection(_ini, "[Audio Keybindings]");
            _toggleSwitches = new IniSection(_ini, "[Toggle Switches]");
            _stringColors = new IniSection(_ini, "[String Colors]");
            _modSettings = new IniSection(_ini, "[Mod Settings]");
            _guitarSpeak = new IniSection(_ini, "[Guitar Speak]");
            _highwayColors = new IniSection(_ini, "[Highway Colors]");
            _guiSettings = new IniSection(_ini, "[GUI Settings]");

            SeedDefaultsAndSave();
        }

        public static void Save()
        {
            _ini.Save();
        }

        public static string GetSongListTitle(int index)
        {
            return _songListTitles.GetString($"SongListTitle_{index}", $"Define Song List {index} Here");
        }

        public static void SetSongListTitle(int index, string value)
        {
            _songListTitles.SetString($"SongListTitle_{index}", value);
        }

        private static void SeedDefaultsAndSave()
        {
            for (int i = 1; i <= 20; i++) GetSongListTitle(i);

            foreach (Type nestedClass in typeof(RsModsSettings).GetNestedTypes(BindingFlags.Public | BindingFlags.Static))
            {
                foreach (PropertyInfo prop in nestedClass.GetProperties(BindingFlags.Public | BindingFlags.Static))
                {
                    prop.GetValue(null); // Triggers the 'get' and saves the default
                }
            }

            _ini.Save();
        }

        public static class Keybinds
        {
            private static string GetConvertedKey([CallerMemberName] string key = "")
            {
                string val = _keybinds.GetString("", key);
                string converted = KeyConversion.VirtualKey(val);
                return !string.IsNullOrEmpty(converted) ? converted : val;
            }

            public static string ToggleLoftKey { get => GetConvertedKey(); set => _keybinds.SetString(value); }
            public static string ShowSongTimerKey { get => GetConvertedKey(); set => _keybinds.SetString(value); }
            public static string ForceReEnumerationKey { get => GetConvertedKey(); set => _keybinds.SetString(value); }
            public static string RainbowStringsKey { get => GetConvertedKey(); set => _keybinds.SetString(value); }
            public static string RainbowNotesKey { get => GetConvertedKey(); set => _keybinds.SetString(value); }
            public static string RemoveLyricsKey { get => GetConvertedKey(); set => _keybinds.SetString(value); }
            public static string RRSpeedKey { get => GetConvertedKey(); set => _keybinds.SetString(value); }
            public static string TuningOffsetKey { get => GetConvertedKey(); set => _keybinds.SetString(value); }
            public static string ToggleExtendedRangeKey { get => GetConvertedKey(); set => _keybinds.SetString(value); }
            public static string LoopStartKey { get => GetConvertedKey(); set => _keybinds.SetString(value); }
            public static string LoopEndKey { get => GetConvertedKey(); set => _keybinds.SetString(value); }
            public static string RewindKey { get => GetConvertedKey(); set => _keybinds.SetString(value); }
        }

        public static class AudioKeybindings
        {
            private static string GetConvertedKey([CallerMemberName] string key = "")
            {
                string val = _audioKeybindings.GetString("", key);
                string converted = KeyConversion.VirtualKey(val);
                return !string.IsNullOrEmpty(converted) ? converted : val;
            }

            public static string MasterVolumeKey { get => GetConvertedKey(); set => _audioKeybindings.SetString(value); }
            public static string SongVolumeKey { get => GetConvertedKey(); set => _audioKeybindings.SetString(value); }
            public static string Player1VolumeKey { get => GetConvertedKey(); set => _audioKeybindings.SetString(value); }
            public static string Player2VolumeKey { get => GetConvertedKey(); set => _audioKeybindings.SetString(value); }
            public static string MicrophoneVolumeKey { get => GetConvertedKey(); set => _audioKeybindings.SetString(value); }
            public static string VoiceOverVolumeKey { get => GetConvertedKey(); set => _audioKeybindings.SetString(value); }
            public static string SFXVolumeKey { get => GetConvertedKey(); set => _audioKeybindings.SetString(value); }
            public static string DisplayMixerKey { get => GetConvertedKey(); set => _audioKeybindings.SetString(value); }
            public static string MutePlayer1Key { get => GetConvertedKey(); set => _audioKeybindings.SetString(value); }
            public static string MutePlayer2Key { get => GetConvertedKey(); set => _audioKeybindings.SetString(value); }
        }

        public static class Toggles
        {
            public static bool ToggleLoft { get => _toggleSwitches.GetBool(); set => _toggleSwitches.SetBool(value); }
            public static bool VolumeControl { get => _toggleSwitches.GetBool(); set => _toggleSwitches.SetBool(value); }
            public static bool ShowSongTimer { get => _toggleSwitches.GetBool(); set => _toggleSwitches.SetBool(value); }
            public static EnumerationMode ForceReEnumeration { get => _toggleSwitches.GetEnum(EnumerationMode.Off); set => _toggleSwitches.SetEnum(value); }
            public static bool RainbowStrings { get => _toggleSwitches.GetBool(); set => _toggleSwitches.SetBool(value); }
            public static bool RainbowNotes { get => _toggleSwitches.GetBool(); set => _toggleSwitches.SetBool(value); }
            public static bool ExtendedRange { get => _toggleSwitches.GetBool(); set => _toggleSwitches.SetBool(value); }
            public static bool ExtendedRangeDropTuning { get => _toggleSwitches.GetBool(); set => _toggleSwitches.SetBool(value); }
            public static bool ExtendedRangeFixBassTuning { get => _toggleSwitches.GetBool(); set => _toggleSwitches.SetBool(value); }
            public static CustomStringColorMode CustomStringColors { get => _toggleSwitches.GetEnumInt(CustomStringColorMode.Off); set => _toggleSwitches.SetEnumInt(value); }
            public static OnOffMode SeparateNoteColors { get => _toggleSwitches.GetEnum(OnOffMode.Off); set => _toggleSwitches.SetEnum(value); }
            public static bool Headstock { get => _toggleSwitches.GetBool(); set => _toggleSwitches.SetBool(value); }
            public static bool Skyline { get => _toggleSwitches.GetBool(); set => _toggleSwitches.SetBool(value); }
            public static bool GreenScreenWall { get => _toggleSwitches.GetBool(); set => _toggleSwitches.SetBool(value); }
            public static bool ForceProfileLoad { get => _toggleSwitches.GetBool(); set => _toggleSwitches.SetBool(value); }
            public static bool Fretless { get => _toggleSwitches.GetBool(); set => _toggleSwitches.SetBool(value); }
            public static bool Inlays { get => _toggleSwitches.GetBool(); set => _toggleSwitches.SetBool(value); }
            public static LoftMode ToggleLoftWhen { get => _toggleSwitches.GetEnum(LoftMode.Manual); set => _toggleSwitches.SetEnum(value); }
            public static bool LaneMarkers { get => _toggleSwitches.GetBool(); set => _toggleSwitches.SetBool(value); }
            public static SkylineMode ToggleSkylineWhen { get => _toggleSwitches.GetEnum(SkylineMode.Song); set => _toggleSwitches.SetEnum(value); }
            public static bool Lyrics { get => _toggleSwitches.GetBool(); set => _toggleSwitches.SetBool(value); }
            public static LyricsMode RemoveLyricsWhen { get => _toggleSwitches.GetEnum(LyricsMode.Manual); set => _toggleSwitches.SetEnum(value); }
            public static bool GuitarSpeak { get => _toggleSwitches.GetBool(); set => _toggleSwitches.SetBool(value); }
            public static HeadstockMode RemoveHeadstockWhen { get => _toggleSwitches.GetEnum(HeadstockMode.Song); set => _toggleSwitches.SetEnum(value); }
            public static bool ScreenShotScores { get => _toggleSwitches.GetBool(); set => _toggleSwitches.SetBool(value); }
            public static bool RRSpeedAboveOneHundred { get => _toggleSwitches.GetBool(); set => _toggleSwitches.SetBool(value); }
            public static bool AutoTuneForSong { get => _toggleSwitches.GetBool(); set => _toggleSwitches.SetBool(value); }
            public static string AutoTuneForSongDevice { get => _toggleSwitches.GetString(""); set => _toggleSwitches.SetString(value); }
            public static string MidiInDevice { get => _toggleSwitches.GetString(""); set => _toggleSwitches.SetString(value); }
            public static AutoTuneWhen AutoTuneForSongWhen { get => _toggleSwitches.GetEnum(AutoTuneWhen.Manual); set => _toggleSwitches.SetEnum(value); }
            public static string AutoTuneForSoftwareSemitoneSettings { get => _toggleSwitches.GetString(""); set => _toggleSwitches.SetString(value); }
            public static string AutoTuneForSoftwareSemitoneTriggers { get => _toggleSwitches.GetString(""); set => _toggleSwitches.SetString(value); }
            public static string AutoTuneForSoftwareTrueTuningSettings { get => _toggleSwitches.GetString(""); set => _toggleSwitches.SetString(value); }
            public static string AutoTuneForSoftwareTrueTuningTriggers { get => _toggleSwitches.GetString(""); set => _toggleSwitches.SetString(value); }
            public static bool ChordsMode { get => _toggleSwitches.GetBool(); set => _toggleSwitches.SetBool(value); }
            public static bool ShowCurrentNoteOnScreen { get => _toggleSwitches.GetBool(); set => _toggleSwitches.SetBool(value); }
            public static string OnScreenFont { get => _toggleSwitches.GetString("Arial"); set => _toggleSwitches.SetString(value); }
            public static int OnScreenFontSize { get => _toggleSwitches.GetInt(24); set => _toggleSwitches.SetInt(value); }
            public static string ProfileToLoad { get => _toggleSwitches.GetString(""); set => _toggleSwitches.SetString(value); }
            public static SongTimerWhen ShowSongTimerWhen { get => _toggleSwitches.GetEnum(SongTimerWhen.Manual); set => _toggleSwitches.SetEnum(value); }
            public static string ShowSelectedVolumeWhen { get => _toggleSwitches.GetString("manual"); set => _toggleSwitches.SetString(value); }
            public static OnOffMode SecondaryMonitor { get => _toggleSwitches.GetEnum(OnOffMode.Off); set => _toggleSwitches.SetEnum(value); }
            public static bool SongPreviews { get => _toggleSwitches.GetBool(); set => _toggleSwitches.SetBool(value); }
            public static bool OverrideInputVolumeEnabled { get => _toggleSwitches.GetBool(); set => _toggleSwitches.SetBool(value); }
            public static string OverrideInputVolumeDevice { get => _toggleSwitches.GetString(""); set => _toggleSwitches.SetString(value); }
            public static bool AllowAudioInBackground { get => _toggleSwitches.GetBool(); set => _toggleSwitches.SetBool(value); }
            public static bool BypassTwoRTCMessageBox { get => _toggleSwitches.GetBool(); set => _toggleSwitches.SetBool(value); }
            public static bool LinearRiffRepeater { get => _toggleSwitches.GetBool(); set => _toggleSwitches.SetBool(value); }
            public static bool AltOutputSampleRate { get => _toggleSwitches.GetBool(); set => _toggleSwitches.SetBool(value); }
            public static bool AllowLooping { get => _toggleSwitches.GetBool(); set => _toggleSwitches.SetBool(value); }
            public static bool AllowRewind { get => _toggleSwitches.GetBool(); set => _toggleSwitches.SetBool(value); }
            public static bool FixOculusCrash { get => _toggleSwitches.GetBool(); set => _toggleSwitches.SetBool(value); }
            public static bool FixBrokenTones { get => _toggleSwitches.GetBool(); set => _toggleSwitches.SetBool(value); }
            public static bool UseCustomNSPTimer { get => _toggleSwitches.GetBool(); set => _toggleSwitches.SetBool(value); }
            public static bool DisplayCurrentAccuracy { get => _toggleSwitches.GetBool(); set => _toggleSwitches.SetBool(value); }
            public static bool PreventMidSongPause { get => _toggleSwitches.GetBool(); set => _toggleSwitches.SetBool(value); }
            public static bool RemoveFingerprints { get => _toggleSwitches.GetBool(); set => _toggleSwitches.SetBool(value); }
        }

        public static class StringColors
        {
            private static readonly string[] _normalDefaults = ["ff4f5a", "e2c102", "1dacf9", "ff9216", "3fcc0c", "c825ed"];
            private static readonly string[] _colorblindDefaults = ["00c68e", "ff4f5a", "e2c102", "1dacf9", "ff9216", "3fcc0c"];

            private static string Key(string type, int index, bool normal) => $"{type}{index}_{(normal ? "N" : "CB")}";
            private static string Default(int index, bool normal) => normal ? _normalDefaults[index] : _colorblindDefaults[index];

            public static string GetStringColor(int index, bool normal) => _stringColors.GetString(Default(index, normal), Key("string", index, normal));
            public static void SetStringColor(int index, bool normal, string color) => _stringColors.SetString(color, Key("string", index, normal));

            public static string GetNoteColor(int index, bool normal) => _stringColors.GetString(Default(index, normal), Key("note", index, normal));
            public static void SetNoteColor(int index, bool normal, string color) => _stringColors.SetString(color, Key("note", index, normal));
        }

        public static class ModSettings
        {
            public static int ExtendedRangeModeAt { get => _modSettings.GetInt(-5); set => _modSettings.SetInt(value); }
            public static int CheckForNewSongsInterval { get => _modSettings.GetInt(5000); set => _modSettings.SetInt(value); }
            public static decimal RRSpeedInterval { get => _modSettings.GetDecimal(2); set => _modSettings.SetDecimal(value); }
            public static TuningPedalDevice TuningPedal { get => _modSettings.GetEnumInt(TuningPedalDevice.None); set => _modSettings.SetEnumInt(value); }
            public static int TuningOffset { get => _modSettings.GetInt(0); set => _modSettings.SetInt(value); }
            public static int VolumeControlInterval { get => _modSettings.GetInt(5); set => _modSettings.SetInt(value); }
            public static int SecondaryMonitorXPosition { get => _modSettings.GetInt(0); set => _modSettings.SetInt(value); }
            public static int SecondaryMonitorYPosition { get => _modSettings.GetInt(0); set => _modSettings.SetInt(value); }
            public static NoteColorMode SeparateNoteColorsMode { get => _modSettings.GetEnumInt(NoteColorMode.Off); set => _modSettings.SetEnumInt(value); }
            public static int OverrideInputVolume { get => _modSettings.GetInt(17); set => _modSettings.SetInt(value); }
            public static int AlternativeOutputSampleRate { get => _modSettings.GetInt(48000); set => _modSettings.SetInt(value); }
            public static int LoopingLeadUp { get => _modSettings.GetInt(0); set => _modSettings.SetInt(value); }
            public static int RewindBy { get => _modSettings.GetInt(5000); set => _modSettings.SetInt(value); }
            public static int RewindLeadup { get => _modSettings.GetInt(2000); set => _modSettings.SetInt(value); }
            public static int CustomNSPTimeLimit { get => _modSettings.GetInt(10000); set => _modSettings.SetInt(value); }
        }

        public static class GuitarSpeak
        {
            public static string GuitarSpeakDeleteWhen { get => _guitarSpeak.GetString(""); set => _guitarSpeak.SetString(value); }
            public static string GuitarSpeakSpaceWhen { get => _guitarSpeak.GetString(""); set => _guitarSpeak.SetString(value); }
            public static string GuitarSpeakEnterWhen { get => _guitarSpeak.GetString(""); set => _guitarSpeak.SetString(value); }
            public static string GuitarSpeakTabWhen { get => _guitarSpeak.GetString(""); set => _guitarSpeak.SetString(value); }
            public static string GuitarSpeakPGUPWhen { get => _guitarSpeak.GetString(""); set => _guitarSpeak.SetString(value); }
            public static string GuitarSpeakPGDNWhen { get => _guitarSpeak.GetString(""); set => _guitarSpeak.SetString(value); }
            public static string GuitarSpeakUPWhen { get => _guitarSpeak.GetString(""); set => _guitarSpeak.SetString(value); }
            public static string GuitarSpeakDNWhen { get => _guitarSpeak.GetString(""); set => _guitarSpeak.SetString(value); }
            public static string GuitarSpeakESCWhen { get => _guitarSpeak.GetString(""); set => _guitarSpeak.SetString(value); }
            public static string GuitarSpeakCloseWhen { get => _guitarSpeak.GetString(""); set => _guitarSpeak.SetString(value); }
            public static string GuitarSpeakOBracketWhen { get => _guitarSpeak.GetString(""); set => _guitarSpeak.SetString(value); }
            public static string GuitarSpeakCBracketWhen { get => _guitarSpeak.GetString(""); set => _guitarSpeak.SetString(value); }
            public static string GuitarSpeakTildeaWhen { get => _guitarSpeak.GetString(""); set => _guitarSpeak.SetString(value); }
            public static string GuitarSpeakForSlashWhen { get => _guitarSpeak.GetString(""); set => _guitarSpeak.SetString(value); }
            public static string GuitarSpeakAltWhen { get => _guitarSpeak.GetString(""); set => _guitarSpeak.SetString(value); }
            public static bool GuitarSpeakWhileTuning { get => _guitarSpeak.GetBool(); set => _guitarSpeak.SetBool(value); }
        }

        public static class HighwayColors
        {
            public static bool CustomHighwayColors { get => _highwayColors.GetBool(); set => _highwayColors.SetBool(value); }
            public static string CustomHighwayNumbered { get => _highwayColors.GetString(""); set => _highwayColors.SetString(value); }
            public static string CustomHighwayUnNumbered { get => _highwayColors.GetString(""); set => _highwayColors.SetString(value); }
            public static string CustomHighwayGutter { get => _highwayColors.GetString(""); set => _highwayColors.SetString(value); }
            public static string CustomFretNubmers { get => _highwayColors.GetString(""); set => _highwayColors.SetString(value); } // Keeping the original typo from old code to preserve user settings!
        }

        public static class GUISettings
        {
            public static bool CustomTheme { get => _guiSettings.GetBool(); set => _guiSettings.SetBool(value); }
            public static string ThemeBackgroundColor { get => _guiSettings.GetString("F0FFFF"); set => _guiSettings.SetString(value); }
            public static string ThemeTextColor { get => _guiSettings.GetString("000000"); set => _guiSettings.SetString(value); }
            public static string ThemeButtonColor { get => _guiSettings.GetString("E3E3E3"); set => _guiSettings.SetString(value); }
            public static bool BackupProfile { get => _guiSettings.GetBool(true); set => _guiSettings.SetBool(value); } // Default is "on"
            public static int NumberOfBackups { get => _guiSettings.GetInt(50); set => _guiSettings.SetInt(value); }
        }
    }
}
