using RSMods.Util;
using System.IO;

namespace RSMods.Rocksmith
{
    public static class RocksmithSettings
    {
        private static readonly IniManager _ini = new(Path.Combine(GenUtil.GetRSDirectory(), "Rocksmith.ini"));

        static RocksmithSettings() => _ini.Load();

        public static class Audio
        {
            private static readonly IniSection _s = new(_ini, "[Audio]", numericBools: true);
            public static bool EnableMicrophone { get => _s.GetBool(true); set { _s.SetBool(value); _s.Save(); } }
            public static bool ExclusiveMode { get => _s.GetBool(true); set { _s.SetBool(value); _s.Save(); } }
            public static int LatencyBuffer { get => _s.GetInt(4); set { _s.SetInt(value); _s.Save(); } }
            public static string ForceDefaultPlaybackDevice { get => _s.GetString(); set { _s.SetString(value); _s.Save(); } }
            public static bool ForceWDM { get => _s.GetBool(); set { _s.SetBool(value); _s.Save(); } }
            public static bool ForceDirectXSink { get => _s.GetBool(); set { _s.SetBool(value); _s.Save(); } }
            public static bool DumpAudioLog { get => _s.GetBool(); set { _s.SetBool(value); _s.Save(); } }
            public static int MaxOutputBufferSize { get => _s.GetInt(0); set { _s.SetInt(value); _s.Save(); } }
            public static bool RealToneCableOnly { get => _s.GetBool(); set { _s.SetBool(value); _s.Save(); } }
            public static bool Win32UltraLowLatencyMode { get => _s.GetBool(true); set { _s.SetBool(value); _s.Save(); } }

            public const int MinLatencyBuffer = 0;
            public const int DefaultLatencyBuffer = 4;
            public const int MaxLatencyBuffer = 16;
        }

        public static class RendererWin32
        {
            private static readonly IniSection _s = new(_ini, "[Renderer.Win32]", numericBools: true);
            public static bool ShowGamepadUI { get => _s.GetBool(); set { _s.SetBool(value); _s.Save(); } }
            public static int ScreenWidth { get => _s.GetInt(0); set { _s.SetInt(value); _s.Save(); } }
            public static int ScreenHeight { get => _s.GetInt(0); set { _s.SetInt(value); _s.Save(); } }
            public static FullscreenMode Fullscreen { get => _s.GetEnumInt(FullscreenMode.Exclusive); set { _s.SetEnumInt(value); _s.Save(); } }
            public static VisualQualityMode VisualQuality { get => _s.GetEnumInt(VisualQualityMode.Medium); set { _s.SetEnumInt(value); _s.Save(); } }
            public static int RenderingWidth { get => _s.GetInt(0); set { _s.SetInt(value); _s.Save(); } }
            public static int RenderingHeight { get => _s.GetInt(0); set { _s.SetInt(value); _s.Save(); } }
            public static bool EnablePostEffects { get => _s.GetBool(true); set { _s.SetBool(value); _s.Save(); } }
            public static bool EnableShadows { get => _s.GetBool(true); set { _s.SetBool(value); _s.Save(); } }
            public static bool EnableHighResScope { get => _s.GetBool(true); set { _s.SetBool(value); _s.Save(); } }
            public static bool EnableDepthOfField { get => _s.GetBool(true); set { _s.SetBool(value); _s.Save(); } }
            public static bool EnablePerPixelLighting { get => _s.GetBool(true); set { _s.SetBool(value); _s.Save(); } }
            public static MsaaMode MsaaSamples { get => _s.GetEnumInt(MsaaMode.X4); set { _s.SetEnumInt(value); _s.Save(); } }
            public static bool DisableBrowser { get => _s.GetBool(); set { _s.SetBool(value); _s.Save(); } }
        }

        public static class Net
        {
            private static readonly IniSection _s = new(_ini, "[Net]", numericBools: true);
            public static bool UseProxy { get => _s.GetBool(true); set { _s.SetBool(value); _s.Save(); } }
        }
    }
}
