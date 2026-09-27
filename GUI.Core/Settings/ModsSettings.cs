using RSMods.Core.Settings;
using RSMods.Util;
using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Reflection;

namespace RSMods
{
    public static class RsModsSettings
    {
        private static Action _settingChangedHandler;
        private static Action<IniValidationWarning> _validationWarningHandler;
        public static event Action SettingChanged
        {
            add { _settingChangedHandler += value; if (_ini != null) _ini.SettingChanged += value; }
            remove { _settingChangedHandler -= value; if (_ini != null) _ini.SettingChanged -= value; }
        }

        public static event Action<IniValidationWarning> ValidationWarning
        {
            add { _validationWarningHandler += value; if (_ini != null) _ini.ValidationWarning += value; }
            remove { _validationWarningHandler -= value; if (_ini != null) _ini.ValidationWarning -= value; }
        }

        private static IniManager _ini;
        public static IniManager Ini => _ini;
        private static IniSection _songListTitles;
        private static IniSection _toggleSwitches;
        private static IniSection _stringColors;
        private static IniSection _modSettings;
        private static IniSection _guitarSpeak;
        private static IniSection _highwayColors;
        private static IniSection _guiSettings;

        public static void LoadSettingsFromINI(IManifestService manifest)
        {
            string path = Path.Combine(GenUtil.GetRSDirectory(), "RSMods.ini");
            _ini = new IniManager(path);
            if (_settingChangedHandler != null)
                _ini.SettingChanged += _settingChangedHandler;
            if (_validationWarningHandler != null)
                _ini.ValidationWarning += _validationWarningHandler;

            _ini.Load();

            _songListTitles = new IniSection(_ini, "[SongListTitles]");
            _toggleSwitches = new IniSection(_ini, "[Toggle Switches]");
            _stringColors = new IniSection(_ini, "[String Colors]");
            _modSettings = new IniSection(_ini, "[Mod Settings]");
            _guitarSpeak = new IniSection(_ini, "[Guitar Speak]");
            _highwayColors = new IniSection(_ini, "[Highway Colors]");
            _guiSettings = new IniSection(_ini, "[GUI Settings]");

            SeedDefaultsAndSave(manifest);
        }

        /// <inheritdoc cref="IniManager.Save"/>
        public static bool Save() => _ini.Save();

        public static List<string> SongListTitles { get; } = [];

        public static List<string> RefreshSongListTitles(int songListCount)
        {
            SongListTitles.Clear();

            for (int i = 1; i <= songListCount; i++)
                SongListTitles.Add(GetSongListTitle(i));

            return SongListTitles;
        }

        public static string GetSongListTitle(int index)
        {
            return _songListTitles.GetString(defaultValue: $"Define Song List {index} Here", key: $"SongListTitle_{index}");
        }

        public static void SetSongListTitle(int index, string value)
        {
            _songListTitles.SetString(value, key: $"SongListTitle_{index}");
        }

        private static void SeedDefaultsAndSave(IManifestService manifest)
        {
            for (int i = 1; i <= 20; i++) GetSongListTitle(i);

            foreach (Type nestedClass in typeof(RsModsSettings).GetNestedTypes(BindingFlags.Public | BindingFlags.Static))
            {
                foreach (PropertyInfo prop in nestedClass.GetProperties(BindingFlags.Public | BindingFlags.Static))
                {
                    prop.GetValue(null); // Triggers the 'get' and saves the default
                }
            }

            SeedManifestDefaults(_ini, manifest);

            // Best effort, as before: a locked file mustn't stop the app starting; the next save retries.
            try
            {
                _ini.Save();
            }
            catch (IOException ex)
            {
                Debug.WriteLine($"Failed to save INI file: {ex.Message}");
            }
        }

        /// <summary>
        /// Writes each manifest setting's declared default where the INI has no value, as the store's own
        /// properties do. Key binds and custom-editor launchers were never seeded, so they still aren't.
        /// </summary>
        public static void SeedManifestDefaults(IniManager ini, IManifestService manifest)
        {
            foreach (SettingDescriptor setting in manifest.AllSettings)
            {
                if (setting.Type == SettingType.Key || !string.IsNullOrEmpty(setting.Editor))
                    continue;

                ini.GetString(SettingFieldViewModel.NormalizeSection(setting.Ini.Section), setting.Ini.Name, setting.Default);
            }
        }

        public static class Toggles
        {
            public static bool ExtendedRange { get => _toggleSwitches.GetBool(); set => _toggleSwitches.SetBool(value); }
            public static CustomStringColorMode CustomStringColors { get => _toggleSwitches.GetEnumInt(CustomStringColorMode.Off); set => _toggleSwitches.SetEnumInt(value); }
            public static OnOffMode SeparateNoteColors { get => _toggleSwitches.GetEnum(OnOffMode.Off); set => _toggleSwitches.SetEnum(value); }
            public static bool FixOculusCrash { get => _toggleSwitches.GetBool(); set => _toggleSwitches.SetBool(value); }
            public static bool FixBrokenTones { get => _toggleSwitches.GetBool(); set => _toggleSwitches.SetBool(value); }
            public static bool PreventMidSongPause { get => _toggleSwitches.GetBool(); set => _toggleSwitches.SetBool(value); }
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
            public static NoteColorMode SeparateNoteColorsMode { get => _modSettings.GetEnumInt(NoteColorMode.Off); set => _modSettings.SetEnumInt(value); }
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
            // Variant is "System" | "Light" | "Dark"; accent is a 6-digit hex
            // (empty means FluentTheme's default accent).
            public static string AppThemeVariant { get => _guiSettings.GetString("System"); set => _guiSettings.SetString(value); }
            public static string AppAccentColor { get => _guiSettings.GetString(""); set => _guiSettings.SetString(value); }
            public static bool BackupProfile { get => _guiSettings.GetBool(true); set => _guiSettings.SetBool(value); } // Default is "on"
            public static int NumberOfBackups { get => _guiSettings.GetInt(50); set => _guiSettings.SetInt(value); }
        }
    }
}
