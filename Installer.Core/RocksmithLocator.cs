using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text.RegularExpressions;
using Microsoft.Win32;

namespace RS2014_Mod_Installer.Core
{
    public enum RocksmithLocationStatus
    {
        Found,
        CachePsarcMissing,
        NotFound
    }

    public sealed class RocksmithLocationResult(RocksmithLocationStatus status, string path)
    {
        public RocksmithLocationStatus Status { get; } = status;

        // Best-effort discovered path. Non-empty for Found and CachePsarcMissing; empty for NotFound.
        public string Path { get; } = path ?? string.Empty;
    }

    // Locates the Rocksmith 2014 install folder from Steam and the registry. When automatic discovery
    // fails it reports NotFound/CachePsarcMissing and lets the UI layer prompt the user.
    public static class RocksmithLocator
    {
        public static RocksmithLocationResult Locate()
        {
            string steamRoot = GetSteamDirectory();
            if (string.IsNullOrEmpty(steamRoot))
                return new RocksmithLocationResult(RocksmithLocationStatus.NotFound, string.Empty);

            string defaultPath = Path.Combine(steamRoot, "steamapps\\common\\Rocksmith2014");
            if (Directory.Exists(defaultPath))
            {
                return File.Exists(Path.Combine(defaultPath, "cache.psarc"))
                    ? new RocksmithLocationResult(RocksmithLocationStatus.Found, defaultPath)
                    : new RocksmithLocationResult(RocksmithLocationStatus.CachePsarcMissing, defaultPath);
            }

            // Default Steam path is absent: try each known registry install location, then custom Steam
            // library folders from libraryfolders.vdf. Any candidate must be a real RS folder to count.
            string candidate = ParseRSFolderCandidates(steamRoot);

            return !string.IsNullOrEmpty(candidate) && IsRocksmithFolder(candidate)
                ? new RocksmithLocationResult(RocksmithLocationStatus.Found, candidate)
                : new RocksmithLocationResult(RocksmithLocationStatus.NotFound, string.Empty);
        }

        private static string ParseRSFolderCandidates(string steamRoot)
        {
            string candidate = string.Empty;

            foreach (Tuple<string, string> installRegKey in InstallRegKeys)
            {
                string path = GetStringValueFromRegistry(installRegKey.Item1, installRegKey.Item2);
                if (!string.IsNullOrEmpty(path) && Directory.Exists(path))
                {
                    candidate = path;
                    break;
                }
            }

            if (string.IsNullOrEmpty(candidate))
            {
                candidate = GetCustomRSFolder(steamRoot);
            }

            return candidate;
        }

        // A folder qualifies as a Rocksmith 2014 install if it has a non-empty dlc/ directory and a
        // cache.psarc. Used both by discovery and to validate a folder the user selects manually.
        public static bool IsRocksmithFolder(string folderPath)
        {
            if (string.IsNullOrEmpty(folderPath) || !Directory.Exists(folderPath)) return false;

            string dlcFolderPath = System.IO.Path.Combine(folderPath, "dlc");
            string cachePsarcPath = System.IO.Path.Combine(folderPath, "cache.psarc");

            return Directory.Exists(dlcFolderPath) && !IsDirectoryEmpty(dlcFolderPath) && File.Exists(cachePsarcPath);
        }

        public static string GetSteamDirectory()
        {
            const string steamRegPath = @"HKEY_CURRENT_USER\SOFTWARE\Valve\Steam";

            return GetStringValueFromRegistry(steamRegPath, "SteamPath").Replace('/', '\\');
        }

        public static List<Tuple<string, string>> InstallRegKeys { get; set; } =
        [
            new(@"HKEY_LOCAL_MACHINE\SOFTWARE\Wow6432Node\Ubisoft\Rocksmith2014", "installdir"),
            new(@"HKEY_LOCAL_MACHINE\SOFTWARE\Wow6432Node\Microsoft\Windows\CurrentVersion\Uninstall\Steam App 221680", "InstallLocation"),
            new(@"HKEY_LOCAL_MACHINE\SOFTWARE\Ubisoft\Rocksmith2014", "InstallLocation"),
            new(@"HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\Steam App 221680", "InstallLocation")
        ];

        private static bool IsDirectoryEmpty(string path) => !Directory.EnumerateFileSystemEntries(path).Any();

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

        private static string GetCustomRSFolder(string mainSteamPath)
        {
            var customSteamappsFolders = GetCustomSteamappsFolders(mainSteamPath);
            if (customSteamappsFolders == null) return string.Empty;

            foreach (var folder in customSteamappsFolders)
            {
                string dirPath = Path.Combine(folder, "steamapps", "appmanifest_221680.acf");
                if (!File.Exists(dirPath))
                    continue;

                string finalPath = Path.GetDirectoryName(dirPath);
                if (string.IsNullOrEmpty(finalPath))
                    continue;

                string rsFolderPath = Path.Combine(finalPath, "common", "Rocksmith2014");
                if (IsRocksmithFolder(rsFolderPath))
                    return rsFolderPath;
            }

            return string.Empty;
        }

        private static List<string> GetCustomSteamappsFolders(string mainSteamPath)
        {
            const string libRegex = "(^\\t\"[1-9]\").*(\".*\")";
            var libDirs = new List<string>();

            string steamappsFolder = Path.Combine(mainSteamPath, "steamapps");
            string libVdf = Path.Combine(steamappsFolder, "libraryfolders.vdf");

            if (!File.Exists(libVdf))
                return libDirs;

            foreach (string l in File.ReadAllLines(libVdf))
            {
                Match reg = Regex.Match(l, libRegex);
                if (reg.Success)
                {
                    string dir = reg.Groups[2].Value;
                    if (dir != string.Empty)
                        libDirs.Add(dir.Trim('\"'));
                }
            }

            return libDirs;
        }
    }
}
