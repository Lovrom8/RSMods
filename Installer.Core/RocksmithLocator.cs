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

            // A default folder without cache.psarc may be a leftover; another source can still find the real install.
            string cachePsarcMissing = string.Empty;

            string defaultPath = Path.Combine(steamRoot, "steamapps\\common\\Rocksmith2014");
            if (Directory.Exists(defaultPath))
            {
                if (File.Exists(Path.Combine(defaultPath, "cache.psarc")))
                    return new RocksmithLocationResult(RocksmithLocationStatus.Found, defaultPath);

                cachePsarcMissing = defaultPath;
            }

            // Then each known registry install location, then custom Steam library folders from libraryfolders.vdf.
            // Any candidate must be a real RS folder to count.
            string candidate = FindRSFolderCandidate(steamRoot);
            if (!string.IsNullOrEmpty(candidate))
                return new RocksmithLocationResult(RocksmithLocationStatus.Found, candidate);

            return string.IsNullOrEmpty(cachePsarcMissing)
                ? new RocksmithLocationResult(RocksmithLocationStatus.NotFound, string.Empty)
                : new RocksmithLocationResult(RocksmithLocationStatus.CachePsarcMissing, cachePsarcMissing);
        }

        private static string FindRSFolderCandidate(string steamRoot)
        {
            foreach (Tuple<string, string> installRegKey in InstallRegKeys)
            {
                string path = GetStringValueFromRegistry(installRegKey.Item1, installRegKey.Item2);
                if (IsReadableRocksmithFolder(path))
                    return path;
            }

            return GetCustomRSFolder(steamRoot);
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

        // For discovery: a candidate that can't be read (access denied on dlc/) just isn't the one.
        private static bool IsReadableRocksmithFolder(string folderPath)
        {
            try
            {
                return IsRocksmithFolder(folderPath);
            }
            catch (Exception ex) when (ex is IOException or UnauthorizedAccessException)
            {
                return false;
            }
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
                if (IsReadableRocksmithFolder(rsFolderPath))
                    return rsFolderPath;
            }

            return string.Empty;
        }

        private static List<string> GetCustomSteamappsFolders(string mainSteamPath)
        {
            string libVdf = Path.Combine(mainSteamPath, "steamapps", "libraryfolders.vdf");

            try
            {
                return File.Exists(libVdf) ? ParseLibraryFolders(File.ReadAllLines(libVdf)) : [];
            }
            catch (Exception ex) when (ex is IOException or UnauthorizedAccessException)
            {
                // Like the old GenUtil.GetRSDirectory: unreadable discovery data just means not found, so Browse still works.
                return [];
            }
        }

        // Steam library roots from libraryfolders.vdf. Current Steam nests each library as "<n>" { "path" "<dir>" ... };
        // before 2021 it was a one-line "<n>" "<dir>". Values escape backslashes as \\.
        internal static List<string> ParseLibraryFolders(IEnumerable<string> lines)
        {
            var libDirs = new List<string>();
            foreach (string line in lines)
            {
                Match match = NestedLibraryPath.Match(line);
                if (!match.Success)
                    match = OneLineLibraryPath.Match(line);

                if (match.Success && match.Groups[1].Value.Length > 0)
                    libDirs.Add(Regex.Replace(match.Groups[1].Value, @"\\(.)", "$1"));
            }

            return libDirs;
        }

        private static readonly Regex NestedLibraryPath = new(@"^\s*""path""\s+""((?:[^""\\]|\\.)*)""", RegexOptions.IgnoreCase);

        // Only at the top level (one tab in): deeper numeric keys are the app lists of the current format.
        private static readonly Regex OneLineLibraryPath = new(@"^\t""\d+""\s+""((?:[^""\\]|\\.)*)""");
    }
}
