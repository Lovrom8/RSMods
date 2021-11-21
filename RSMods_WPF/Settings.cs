using System.IO;

namespace RSMods_WPF
{
    public class Settings
    {
        public static string SettingsFile { get { return "RSMods.ini"; } }

        public static bool HasValidSettingsFile() => File.Exists(SettingsFile) && File.ReadAllText(SettingsFile).Length > 0;
    }
}
