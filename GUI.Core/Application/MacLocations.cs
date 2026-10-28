using System;
using System.IO;
using System.Linq;

namespace RSMods
{
    /// <summary>
    /// Where RS Re-ARMed (the native Apple Silicon port of Rocksmith 2014) keeps the game and the saves on macOS.
    /// RSMods on macOS works with RS Re-ARMed: the Steam copy of the Mac game runs under Rosetta and loads no mods.
    /// </summary>
    public static class MacLocations
    {
        /// <summary>RS Re-ARMed's folder; RS_NATIVE_HOME overrides it, as for RS Re-ARMed's own scripts.</summary>
        public static string Home
        {
            get
            {
                string fromEnv = Environment.GetEnvironmentVariable("RS_NATIVE_HOME");
                if (!string.IsNullOrEmpty(fromEnv))
                    return fromEnv;
                return Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.UserProfile),
                    "Library", "Application Support", "RS Re-ARMed");
            }
        }

        /// <summary>The game folder: Rocksmith2014.app, the game's files (linked from the Steam install) and RSMods.ini.</summary>
        public static string GameFolder => Path.Combine(Home, "game");

        /// <summary>The mods folder RS Re-ARMed 1.2.0 and later load libraries from at startup.</summary>
        public static string ModsFolder => Path.Combine(Home, "mods");

        /// <summary>The first save folder (steam/&lt;account&gt;/221680/remote) that has profiles, or an empty string.</summary>
        public static string SaveFolder()
        {
            string steam = Path.Combine(Home, "steam");
            if (!Directory.Exists(steam))
                return string.Empty;
            try
            {
                return Directory.GetDirectories(steam)
                    .Select(account => Path.Combine(account, "221680", "remote"))
                    .FirstOrDefault(remote => File.Exists(Path.Combine(remote, "LocalProfiles.json"))) ?? string.Empty;
            }
            catch (IOException)
            {
                return string.Empty;
            }
            catch (UnauthorizedAccessException)
            {
                return string.Empty;
            }
        }
    }
}
