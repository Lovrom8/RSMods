using System;
using System.IO;

namespace RSMods.Rocksmith
{
    /// <summary>
    /// Reads and writes one Rocksmith.ini file. The host supplies the path so this settings model has no
    /// dependency on install-location discovery or a particular UI framework.
    /// </summary>
    public sealed class RocksmithSettings
    {
        public const string DefaultFileName = "Rocksmith.ini";

        private readonly IniManager _ini;

        public RocksmithSettings(string filePath)
        {
            if (string.IsNullOrWhiteSpace(filePath))
                throw new ArgumentException("A Rocksmith.ini path is required.", nameof(filePath));

            FilePath = filePath;
            _ini = new IniManager(filePath);
            _ini.Load();

            Audio = new AudioSettings(_ini);
            RendererWin32 = new RendererSettings(_ini);
            Net = new NetSettings(_ini);
        }

        public string FilePath { get; }
        public bool SettingsExist => File.Exists(FilePath);
        public AudioSettings Audio { get; }
        public RendererSettings RendererWin32 { get; }
        public NetSettings Net { get; }

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

        public sealed class AudioSettings
        {
            public const int MinLatencyBuffer = 0;
            public const int DefaultLatencyBuffer = 4;
            public const int MaxLatencyBuffer = 16;

            private readonly IniSection _section;

            internal AudioSettings(IniManager ini) => _section = new IniSection(ini, "[Audio]", numericBools: true);

            public bool EnableMicrophone { get => _section.GetBool(true); set { _section.SetBool(value); _section.Save(); } }
            public bool ExclusiveMode { get => _section.GetBool(true); set { _section.SetBool(value); _section.Save(); } }
            public int LatencyBuffer { get => _section.GetInt(DefaultLatencyBuffer); set { _section.SetInt(value); _section.Save(); } }
            public string ForceDefaultPlaybackDevice { get => _section.GetString(); set { _section.SetString(value); _section.Save(); } }
            public bool ForceWDM { get => _section.GetBool(); set { _section.SetBool(value); _section.Save(); } }
            public bool ForceDirectXSink { get => _section.GetBool(); set { _section.SetBool(value); _section.Save(); } }
            public bool DumpAudioLog { get => _section.GetBool(); set { _section.SetBool(value); _section.Save(); } }
            public int MaxOutputBufferSize { get => _section.GetInt(0); set { _section.SetInt(value); _section.Save(); } }
            public bool RealToneCableOnly { get => _section.GetBool(); set { _section.SetBool(value); _section.Save(); } }
            public bool Win32UltraLowLatencyMode { get => _section.GetBool(true); set { _section.SetBool(value); _section.Save(); } }
        }

        public sealed class RendererSettings
        {
            private readonly IniSection _section;

            internal RendererSettings(IniManager ini) => _section = new IniSection(ini, "[Renderer.Win32]", numericBools: true);

            public bool ShowGamepadUI { get => _section.GetBool(); set { _section.SetBool(value); _section.Save(); } }
            public int ScreenWidth { get => _section.GetInt(0); set { _section.SetInt(value); _section.Save(); } }
            public int ScreenHeight { get => _section.GetInt(0); set { _section.SetInt(value); _section.Save(); } }
            public FullscreenMode Fullscreen { get => _section.GetEnumInt(FullscreenMode.Exclusive); set { _section.SetEnumInt(value); _section.Save(); } }
            public VisualQualityMode VisualQuality { get => _section.GetEnumInt(VisualQualityMode.Medium); set { _section.SetEnumInt(value); _section.Save(); } }
            public int RenderingWidth { get => _section.GetInt(0); set { _section.SetInt(value); _section.Save(); } }
            public int RenderingHeight { get => _section.GetInt(0); set { _section.SetInt(value); _section.Save(); } }
            public bool EnablePostEffects { get => _section.GetBool(true); set { _section.SetBool(value); _section.Save(); } }
            public bool EnableShadows { get => _section.GetBool(true); set { _section.SetBool(value); _section.Save(); } }
            public bool EnableHighResScope { get => _section.GetBool(true); set { _section.SetBool(value); _section.Save(); } }
            public bool EnableDepthOfField { get => _section.GetBool(true); set { _section.SetBool(value); _section.Save(); } }
            public bool EnablePerPixelLighting { get => _section.GetBool(true); set { _section.SetBool(value); _section.Save(); } }
            public MsaaMode MsaaSamples { get => _section.GetEnumInt(MsaaMode.X4); set { _section.SetEnumInt(value); _section.Save(); } }
            public bool DisableBrowser { get => _section.GetBool(); set { _section.SetBool(value); _section.Save(); } }
        }

        public sealed class NetSettings
        {
            private readonly IniSection _section;

            internal NetSettings(IniManager ini) => _section = new IniSection(ini, "[Net]", numericBools: true);

            public bool UseProxy { get => _section.GetBool(true); set { _section.SetBool(value); _section.Save(); } }
        }
    }
}
