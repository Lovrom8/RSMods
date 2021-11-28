using System.IO;

namespace RSMods_WPF
{
    public class Settings
    {
        public static string SettingsFile { get { return "RSMods.ini"; } }

        public static string AsioSettingsFile { get { return "RS_ASIO.ini"; } }

        public static bool HasValidSettingsFile() => File.Exists(SettingsFile) && File.ReadAllText(SettingsFile).Length > 0;
        
        public static bool HasValidAsioSettingsFile() => File.Exists(AsioSettingsFile) && File.ReadAllText(AsioSettingsFile).Length > 0;
    }
}
