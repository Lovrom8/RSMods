using System.IO;

namespace RS2014_Mod_Installer.Core
{
    // The single source of truth for where the configurator is installed and launched from. Keeping this
    // in one place preserves the installed-path/shortcut contract: <Rocksmith>/RSMods/RSMods.exe.
    public static class ConfiguratorPaths
    {
        public static string RsModsFolder(string rocksmithLocation) => Path.Combine(rocksmithLocation, "RSMods");

        public static string ExecutablePath(string rocksmithLocation) => Path.Combine(RsModsFolder(rocksmithLocation), "RSMods.exe");
    }
}
