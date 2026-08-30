using System.IO;
using RSMods.Util;

namespace RSMods.ASIO
{
    public static class AsioSettings
    {
        private static readonly IniManager _ini = new(Path.Combine(GenUtil.GetRSDirectory(), "RS_ASIO.ini"));

        static AsioSettings() => _ini.Load();

        public static bool SettingsExist => File.Exists(Path.Combine(GenUtil.GetRSDirectory(), "RS_ASIO.ini"));

        public static void Save() => _ini.Save();

        public static class Config
        {
            private static readonly IniSection _s = new(_ini, "[Config]");

            public static bool EnableWasapiOutputs { get => _s.GetBool(); set { _s.SetBool(value); _s.Save(); } }
            public static bool EnableWasapiInputs { get => _s.GetBool(); set { _s.SetBool(value); _s.Save(); } }
            public static bool EnableAsio { get => _s.GetBool(); set { _s.SetBool(value); _s.Save(); } }
        }

        public static class AsioSection
        {
            private static readonly IniSection _s = new(_ini, "[Asio]");

            public static string BufferSizeMode { get => _s.GetString("driver"); set { _s.SetString(value); _s.Save(); } }
            public static int CustomBufferSize { get => _s.GetInt(48); set { _s.SetInt(value); _s.Save(); } }
        }

        public static class Output
        {
            private static readonly IniSection _s = new(_ini, "[Asio.Output]");

            // Driver= blank when Disabled; not commented
            public static bool Disabled { get => Driver == ""; set { Driver = value ? "" : Driver; } }
            public static string Driver { get => _s.GetString(); set { _s.SetString(value); _s.Save(); } }
            public static int BaseChannel { get => _s.GetInt(0); set { _s.SetInt(value); _s.Save(); } }
            public static int AltBaseChannel { get => _s.GetInt(0); set { _s.SetInt(value); _s.Save(); } }
            public static bool EnableSoftwareEndpointVolumeControl { get => _s.GetBool(); set { _s.SetBool(value); _s.Save(); } }
            public static bool EnableSoftwareMasterVolumeControl { get => _s.GetBool(); set { _s.SetBool(value); _s.Save(); } }
            public static int SoftwareMasterVolumePercent { get => _s.GetInt(0); set { _s.SetInt(value); _s.Save(); } }
            public static bool EnableRefCountHack { get => _s.GetBool(); set { _s.SetBool(value); _s.Save(); } }
        }

        public static class Input0
        {
            private static readonly IniSection _s = new(_ini, "[Asio.Input.0]");

            public static bool Disabled { get => Driver == ""; set { Driver = value ? "" : Driver; } }
            public static string Driver { get => _s.GetString(); set { _s.SetString(value); _s.Save(); } }
            public static int Channel { get => _s.GetInt(0); set { _s.SetInt(value); _s.Save(); } }
            public static bool EnableSoftwareEndpointVolumeControl { get => _s.GetBool(); set { _s.SetBool(value); _s.Save(); } }
            public static bool EnableSoftwareMasterVolumeControl { get => _s.GetBool(); set { _s.SetBool(value); _s.Save(); } }
            public static int SoftwareMasterVolumePercent { get => _s.GetInt(0); set { _s.SetInt(value); _s.Save(); } }
            public static bool EnableRefCountHack { get => _s.GetBool(); set { _s.SetBool(value); _s.Save(); } }
        }

        public static class Input1
        {
            private const string SectionName = "[Asio.Input.1]";
            private static readonly IniSection _s = new(_ini, SectionName);

            // Input1 disabled = ;Driver=value (commented out, value preserved)
            public static bool Disabled
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
            public static string Driver { get => _ini.IsCommented(SectionName, "Driver") ? _ini.GetCommentedString(SectionName, "Driver") : _s.GetString(); set { _s.SetString(value); _s.Save(); } }
            public static int Channel { get => _s.GetInt(0); set { _s.SetInt(value); _s.Save(); } }
            public static bool EnableSoftwareEndpointVolumeControl { get => _s.GetBool(); set { _s.SetBool(value); _s.Save(); } }
            public static bool EnableSoftwareMasterVolumeControl { get => _s.GetBool(); set { _s.SetBool(value); _s.Save(); } }
            public static int SoftwareMasterVolumePercent { get => _s.GetInt(0); set { _s.SetInt(value); _s.Save(); } }
            public static bool EnableRefCountHack { get => _s.GetBool(); set { _s.SetBool(value); _s.Save(); } }
        }

        public static class InputMic
        {
            private static readonly IniSection _s = new(_ini, "[Asio.Input.Mic]");

            public static bool Disabled { get => Driver == ""; set { Driver = value ? "" : Driver; } }
            public static string Driver { get => _s.GetString(); set { _s.SetString(value); _s.Save(); } }
            public static int Channel { get => _s.GetInt(0); set { _s.SetInt(value); _s.Save(); } }
            public static bool EnableSoftwareEndpointVolumeControl { get => _s.GetBool(); set { _s.SetBool(value); _s.Save(); } }
            public static bool EnableSoftwareMasterVolumeControl { get => _s.GetBool(); set { _s.SetBool(value); _s.Save(); } }
            public static int SoftwareMasterVolumePercent { get => _s.GetInt(0); set { _s.SetInt(value); _s.Save(); } }
            public static bool EnableRefCountHack { get => _s.GetBool(); set { _s.SetBool(value); _s.Save(); } }
        }
    }
}