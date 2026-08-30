using System;
using System.IO;

namespace RSMods.ASIO
{
    public enum WasapiOutputMode
    {
        Prompt = -1,
        Off = 0,
        On = 1
    }

    /// <summary>
    /// Reads and writes one RS_ASIO.ini file. The host supplies the path so this settings model has no
    /// dependency on install-location discovery or a particular UI framework.
    /// </summary>
    public sealed class AsioSettings
    {
        public const string DefaultFileName = "RS_ASIO.ini";

        private readonly IniManager _ini;

        public AsioSettings(string filePath)
        {
            if (string.IsNullOrWhiteSpace(filePath))
                throw new ArgumentException("An RS_ASIO.ini path is required.", nameof(filePath));

            FilePath = filePath;
            _ini = new IniManager(filePath);
            _ini.Load();
            _ini.SetSectionComments("[Asio]",
            [
                "; available buffer size modes:",
                ";    driver - respect buffer size setting set in the driver",
                ";    host   - use a buffer size as close as possible to that requested by the host application",
                ";    custom - use the buffer size specified in CustomBufferSize field"
            ]);

            Config = new ConfigSettings(_ini);
            AsioSection = new AsioSectionSettings(_ini);
            Output = new OutputSettings(_ini);
            Input0 = new InputSettings(_ini, "[Asio.Input.0]", 0);
            Input1 = new Input1Settings(_ini);
            InputMic = new InputSettings(_ini, "[Asio.Input.Mic]", 1);
        }

        public string FilePath { get; }
        public bool SettingsExist => File.Exists(FilePath);
        public ConfigSettings Config { get; }
        public AsioSectionSettings AsioSection { get; }
        public OutputSettings Output { get; }
        public InputSettings Input0 { get; }
        public Input1Settings Input1 { get; }
        public InputSettings InputMic { get; }

        public event Action SettingChanged
        {
            add => _ini.SettingChanged += value;
            remove => _ini.SettingChanged -= value;
        }

        public event Action<IniValidationWarning> ValidationWarning
        {
            add => _ini.ValidationWarning += value;
            remove => _ini.ValidationWarning -= value;
        }

        public void Save() => _ini.Save();

        public sealed class ConfigSettings
        {
            private readonly IniSection _section;

            internal ConfigSettings(IniManager ini) => _section = new IniSection(ini, "[Config]");

            public WasapiOutputMode WasapiOutputs { get => _section.GetEnumInt(WasapiOutputMode.Off); set { _section.SetEnumInt(value); _section.Save(); } }
            public bool EnableWasapiInputs { get => _section.GetBool(); set { _section.SetBool(value); _section.Save(); } }
            public bool EnableAsio { get => _section.GetBool(true); set { _section.SetBool(value); _section.Save(); } }
        }

        public sealed class AsioSectionSettings
        {
            private readonly IniSection _section;

            internal AsioSectionSettings(IniManager ini) => _section = new IniSection(ini, "[Asio]");

            public string BufferSizeMode { get => _section.GetString("driver"); set { _section.SetString(value); _section.Save(); } }
            public int CustomBufferSize { get => _section.GetInt(48); set { _section.SetInt(value); _section.Save(); } }
        }

        public sealed class OutputSettings
        {
            private readonly IniSection _section;

            internal OutputSettings(IniManager ini) => _section = new IniSection(ini, "[Asio.Output]");

            // Driver= blank when Disabled; not commented.
            public bool Disabled { get => Driver == ""; set { Driver = value ? "" : Driver; } }
            public string Driver { get => _section.GetString(); set { _section.SetString(value); _section.Save(); } }
            public int BaseChannel { get => _section.GetInt(0); set { _section.SetInt(value); _section.Save(); } }
            public int AltBaseChannel { get => _section.GetInt(0); set { _section.SetInt(value); _section.Save(); } }
            public bool EnableSoftwareEndpointVolumeControl { get => _section.GetBool(true); set { _section.SetBool(value); _section.Save(); } }
            public bool EnableSoftwareMasterVolumeControl { get => _section.GetBool(true); set { _section.SetBool(value); _section.Save(); } }
            public int SoftwareMasterVolumePercent { get => _section.GetInt(100); set { _section.SetInt(value); _section.Save(); } }
            public bool EnableRefCountHack { get => _section.GetBool(); set { _section.SetBool(value); _section.Save(); } }
        }

        public sealed class InputSettings
        {
            private readonly IniSection _section;
            private readonly int _channelDefault;

            internal InputSettings(IniManager ini, string sectionName, int channelDefault)
            {
                _section = new IniSection(ini, sectionName);
                _channelDefault = channelDefault;
            }

            public bool Disabled { get => Driver == ""; set { Driver = value ? "" : Driver; } }
            public string Driver { get => _section.GetString(); set { _section.SetString(value); _section.Save(); } }
            public int Channel { get => _section.GetInt(_channelDefault); set { _section.SetInt(value); _section.Save(); } }
            public bool EnableSoftwareEndpointVolumeControl { get => _section.GetBool(true); set { _section.SetBool(value); _section.Save(); } }
            public bool EnableSoftwareMasterVolumeControl { get => _section.GetBool(true); set { _section.SetBool(value); _section.Save(); } }
            public int SoftwareMasterVolumePercent { get => _section.GetInt(100); set { _section.SetInt(value); _section.Save(); } }
            public bool EnableRefCountHack { get => _section.GetBool(); set { _section.SetBool(value); _section.Save(); } }
        }

        public sealed class Input1Settings
        {
            private const string SectionName = "[Asio.Input.1]";
            private readonly IniManager _ini;
            private readonly IniSection _section;

            internal Input1Settings(IniManager ini)
            {
                _ini = ini;
                _section = new IniSection(ini, SectionName);
            }

            // Input1 disabled = ;Driver=value (commented out, value preserved).
            public bool Disabled
            {
                get => _ini.IsCommented(SectionName, "Driver");
                set
                {
                    string currentDriver = value
                        ? _ini.GetCommentedString(SectionName, "Driver", _ini.GetString(SectionName, "Driver"))
                        : _ini.GetCommentedString(SectionName, "Driver", "");
                    _ini.SetCommentedString(SectionName, "Driver", currentDriver, value);
                    _ini.Save();
                }
            }

            public string Driver
            {
                get => _ini.IsCommented(SectionName, "Driver")
                    ? _ini.GetCommentedString(SectionName, "Driver")
                    : _section.GetString();
                set { _section.SetString(value); _section.Save(); }
            }

            public int Channel { get => _section.GetInt(1); set { _section.SetInt(value); _section.Save(); } }
            public bool EnableSoftwareEndpointVolumeControl { get => _section.GetBool(true); set { _section.SetBool(value); _section.Save(); } }
            public bool EnableSoftwareMasterVolumeControl { get => _section.GetBool(true); set { _section.SetBool(value); _section.Save(); } }
            public int SoftwareMasterVolumePercent { get => _section.GetInt(100); set { _section.SetInt(value); _section.Save(); } }
            public bool EnableRefCountHack { get => _section.GetBool(); set { _section.SetBool(value); _section.Save(); } }
        }
    }
}
