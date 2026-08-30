using Microsoft.Win32;
using RSMods.Core;
using RSMods.Data;
using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Reflection;
using System.Text.RegularExpressions;

namespace RSMods.Util
{
    public static class GenUtil
    {
        public static T Clamp<T>(T value, T min, T max) where T : IComparable<T>
            => value.CompareTo(min) < 0 ? min : value.CompareTo(max) > 0 ? max : value;

        public static bool IsDirectoryEmpty(string path) => !Directory.EnumerateFileSystemEntries(path).Any();

        public static void ExtractEmbeddedResource(string outputDir, Assembly resourceAssembly, string resourceLocation, string[] files)
        {
            if (!Directory.Exists(outputDir))
                Directory.CreateDirectory(outputDir);

            foreach (string file in files)
            {
                string resourcePath = Path.Combine(outputDir, file);

                Stream stream = resourceAssembly.GetManifestResourceStream(String.Format("{0}.{1}", resourceLocation, file));

                if (stream == null)
                    return;

                using FileStream fileStream = new FileStream(resourcePath, FileMode.Create);
                stream.CopyTo(fileStream);
            }
        }

        public static string GetDefaultBrowser(string url)
        {
            const string ieFallback = "iexplore.exe";
            const string subKeyPath = @"Software\Microsoft\Windows\Shell\Associations\UrlAssociations\http\UserChoice";

            using (RegistryKey userChoiceKey = Registry.CurrentUser.OpenSubKey(subKeyPath))
            {
                string progId = userChoiceKey?.GetValue("Progid")?.ToString();

                if (string.IsNullOrEmpty(progId))
                {
                    return ieFallback;
                }

                if (progId.Contains("edge", StringComparison.OrdinalIgnoreCase))
                {
                    return $"microsoft-edge:{url}";
                }
            }

            return url;
        }

        public static T Map<T, TU>(this T target, TU source) // Copy properties of base class to its derived class 
        {
            var tprops = target.GetType().GetProperties();

            tprops.Where(x => x.CanWrite).ToList().ForEach(prop =>
            {
                var sp = source.GetType().GetProperty(prop.Name);
                if (sp != null)
                {
                    var value = sp.GetValue(source, null);
                    target.GetType().GetProperty(prop.Name).SetValue(target, value, null);
                }
            });

            return target;
        }

        public static bool IsRSFolder(this string folderPath)
        {
            if (!Directory.Exists(folderPath))
                return false;

            string cachePsarcPath = Path.Combine(folderPath, "cache.psarc");

            return File.Exists(cachePsarcPath);
        }

        public static bool IsSavePath(this string savePath)
        {
            if (!Directory.Exists(savePath))
                return false;

            string localProfiles = Path.Combine(savePath, "LocalProfiles.json");

            return File.Exists(localProfiles);
        }

        private static string GetStringValueFromRegistry(string keyName, string valueName)
        {
            try
            {
                var retValue = (string)Registry.GetValue(keyName, valueName, "");
                return retValue ?? string.Empty;
            }
            catch (Exception)
            {
                return string.Empty;
            }
        }

        private static List<string> GetCustomSteamappsFolders(string mainSteamPath)
        {
            const string libRegex = "(^\\t\"[1-9]\").*(\".*\")";
            var libDirs = new List<string>();

            string steamappsFolder = Path.Combine(mainSteamPath, "steamapps");
            string libVdf = Path.Combine(steamappsFolder, "libraryfolders.vdf");

            if (!File.Exists(libVdf))
                return [];

            foreach (string l in File.ReadAllLines(libVdf))
            {
                var reg = Regex.Match(l, libRegex);

                if (reg.Success)
                {
                    string dir = reg.Groups[2].Value;

                    if (dir != string.Empty)
                    {
                        string ndir = dir.Trim('\"');
                        libDirs.Add(ndir);
                    }
                }
            }

            if (libDirs.Count == 0)
                return [];

            return libDirs;
        }

        private static string GetCustomRSFolder(string mainSteamPath)
        {
            var customSteamappsFolders = GetCustomSteamappsFolders(mainSteamPath);
            string rsFolderPath = string.Empty;

            if (customSteamappsFolders == null || customSteamappsFolders.Count == 0)
                return string.Empty;

            foreach (var customSteamappsFolder in customSteamappsFolders)
            {
                string dirPath = Path.Combine(customSteamappsFolder, "steamapps", "appmanifest_221680.acf");

                if (!File.Exists(dirPath))
                    continue;

                string finalPath = Path.GetDirectoryName(dirPath);
                if (string.IsNullOrEmpty(finalPath))
                    continue;

                rsFolderPath = Path.Combine(finalPath, "common", "Rocksmith2014");

                if (rsFolderPath.IsRSFolder())
                    return rsFolderPath;
            }

            return string.Empty;
        }

        public static string GetSteamDirectory()
        {
            const string steamRegPath = @"HKEY_CURRENT_USER\SOFTWARE\Valve\Steam"; //IIRC it isn't the same on X86 machines, but do we really need to support those?

            return GetStringValueFromRegistry(steamRegPath, "SteamPath").Replace('/', '\\');
        }

        public static List<Tuple<string, string>> InstallRegKeys { get; set; } =
        [
            new(@"HKEY_LOCAL_MACHINE\SOFTWARE\Wow6432Node\Ubisoft\Rocksmith2014", "installdir"),
            new(@"HKEY_LOCAL_MACHINE\SOFTWARE\Wow6432Node\Microsoft\Windows\CurrentVersion\Uninstall\Steam App 221680", "InstallLocation"),
            new(@"HKEY_LOCAL_MACHINE\SOFTWARE\Ubisoft\Rocksmith2014", "InstallLocation"),
            new(@"HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\Steam App 221680", "InstallLocation")
        ];

        /// <summary>
        /// Best-effort detection of the Rocksmith save folder. Reads the cached / persisted value and,
        /// as a side effect, refreshes <see cref="Constants.SavePathDeclined"/> from the settings file.
        /// Returns the found path or an empty string; interactive prompting lives in
        /// <see cref="RSLocationResolver"/>.
        /// </summary>
        public static string GetSaveFolder()
        {
            if (!IsSavePath(Constants.SavePath))
            {
                if (File.Exists(Constants.SettingsPath))
                {
                    var settings = new FlatKeyValueSettingsStore(Constants.SettingsPath);
                    Constants.SavePath = settings.GetString("SavePath");
                    Constants.SavePathDeclined = bool.TryParse(settings.GetString("BypassSavePrompt"), out bool declined) && declined;
                    if (Constants.SavePath != string.Empty)
                        return Constants.SavePath;
                }
            }
            else
            {
                return Constants.SavePath;
            }

            try
            {
                string potentialSaveFolder = GetSaveDirectory();

                if (potentialSaveFolder.IsSavePath())
                    return potentialSaveFolder;
            }
            catch
            {
                // Detection is best-effort; the resolver surfaces "not found" to the user.
            }

            return string.Empty;
        }

        public static string GetRsModsPath() => Path.Combine(GetRSDirectory(), "RSMods");

        /// <summary>
        /// Best-effort detection of the Rocksmith 2014 install folder: the cached / persisted value, then
        /// probing (parent of our own folder, Steam common, install registry keys, custom Steam libraries).
        /// Returns a validated folder or an empty string when nothing is found; the "please point us at it"
        /// prompting and shutdown-on-give-up flow live in <see cref="RSLocationResolver"/>.
        /// </summary>
        public static string GetRSDirectory()
        {
            if (!IsRSFolder(Constants.RSFolder))
            {
                if (File.Exists(Constants.SettingsPath))
                {
                    var settings = new FlatKeyValueSettingsStore(Constants.SettingsPath);
                    Constants.RSFolder = settings.GetString("RSPath");
                    if (Constants.RSFolder != string.Empty)
                    {
                        return Constants.RSFolder;
                    }
                }
            }
            else
            {
                return Constants.RSFolder;
            }

            try
            {
                var rs2RootDir = string.Empty;
                var steamRootPath = GetSteamDirectory();

                // Before anything else, check whether we are sitting inside a RS install folder.
                string startupParent = Directory.GetParent(AppContext.BaseDirectory)?.FullName;
                if (!string.IsNullOrEmpty(startupParent) && startupParent.IsRSFolder())
                    return startupParent;

                if (!string.IsNullOrEmpty(steamRootPath))
                {
                    rs2RootDir = Path.Combine(steamRootPath, "steamapps\\common\\Rocksmith2014");

                    if (!Directory.Exists(rs2RootDir)) // RS-Folder doesn't exist
                    {
                        // Go through each possible registry location
                        foreach (var installRegKey in InstallRegKeys)
                        {
                            var path = GetStringValueFromRegistry(installRegKey.Item1, installRegKey.Item2);

                            if (!string.IsNullOrEmpty(path))
                            {
                                rs2RootDir = path;
                                break;
                            }
                        }

                        if (string.IsNullOrEmpty(rs2RootDir))
                        {
                            rs2RootDir = GetCustomRSFolder(steamRootPath); // Grab custom Steam library paths from .vdf file
                        }

                        if (string.IsNullOrEmpty(rs2RootDir) || !rs2RootDir.IsRSFolder()) // Nothing valid detected — the resolver prompts.
                            return string.Empty;
                    }
                    else if (!rs2RootDir.IsRSFolder()) // Folder exists but cache.psarc doesn't (old install / steam left-overs) — the resolver prompts.
                    {
                        return string.Empty;
                    }
                }

                return rs2RootDir;
            }
            catch
            {
                // Detection is best-effort
            }

            return string.Empty;
        }

        /// <summary>
        /// Best-effort probe of the Steam userdata tree for a Rocksmith (221680) profile folder.
        /// Returns an empty string when none is found; callers fall back to the resolver's prompt.
        /// </summary>
        public static string GetSteamProfilesFolderManual()
        {
            string steamUserdataPath = Path.Combine(GetSteamDirectory(), "userdata");
            try
            {
                var subdirs = new DirectoryInfo(steamUserdataPath).GetDirectories(@"221680", SearchOption.AllDirectories).ToArray();
                var userprofileFolder = subdirs.FirstOrDefault(dir => !dir.FullName.Contains("760")); //760 is the ID for Steam's screenshot thingy

                if (userprofileFolder != null && Directory.Exists(userprofileFolder.FullName))
                    return userprofileFolder.FullName;
            }
            catch (IOException)
            {
                // Steam not installed / userdata missing — nothing to detect.
            }

            return string.Empty;
        }

        /// <summary>
        /// Best-effort detection of the Rocksmith save (profile) folder: the cached value, then the Steam
        /// registry (each user's <c>userdata/&lt;id&gt;/221680/remote</c>), falling back to a userdata probe.
        /// Returns the found path or an empty string. Pure path detection — no prompting.
        /// </summary>
        public static string GetSaveDirectory(bool forceRegistry = false)
        {
            if (!forceRegistry && !string.IsNullOrEmpty(Constants.SavePath))
            {
                if (Constants.SavePath.IsSavePath())
                {
                    return Constants.SavePath;
                }

                Constants.SavePath = string.Empty;
            }

            try
            {
                using var steamKey = Registry.CurrentUser.OpenSubKey(@"SOFTWARE\Valve\Steam");
                if (steamKey == null)
                    return GetSteamProfilesFolderManual();

                string steamPath = steamKey.GetValue("SteamPath") as string;
                if (string.IsNullOrEmpty(steamPath))
                    return GetSteamProfilesFolderManual();

                using var usersKey = steamKey.OpenSubKey("Users");
                if (usersKey == null)
                    return GetSteamProfilesFolderManual();

                foreach (string user in usersKey.GetSubKeyNames())
                {
                    string fullProfileFolder = Path.Combine(steamPath, "userdata", user, "221680", "remote");

                    if (Directory.Exists(fullProfileFolder))
                    {
                        return fullProfileFolder;
                    }
                }
            }
            catch { }

            return GetSteamProfilesFolderManual();
        }
    }
}
