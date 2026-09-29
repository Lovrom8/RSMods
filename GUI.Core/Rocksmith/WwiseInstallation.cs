#nullable enable
using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text.RegularExpressions;

namespace RSMods.Core
{
    /// <summary>What <see cref="WwiseInstallation.Check"/> found.</summary>
    public enum WwiseStatus
    {
        /// <summary>A usable Wwise is installed and the converter will find it.</summary>
        Ready,

        /// <summary>No Wwise install was found.</summary>
        NotInstalled,

        /// <summary>Wwise is installed, but no version the converter can drive.</summary>
        Incompatible,
    }

    /// <summary>One Wwise install folder and what it offers.</summary>
    public sealed record WwiseInstall(string Folder, string Version, int Year, bool HasCli)
    {
        /// <summary>
        /// The toolkit converter runs WwiseCLI.exe against the Rocksmith project templates it ships, which cover
        /// Wwise 2013 through 2017; later releases dropped WwiseCLI.exe.
        /// </summary>
        public bool IsCompatible => HasCli && Year is >= WwiseInstallation.MinYear and <= WwiseInstallation.MaxYear;
    }

    public sealed record WwiseCheckResult(WwiseStatus Status, WwiseInstall? Selected, IReadOnlyList<WwiseInstall> Installs);

    /// <summary>
    /// Finds Wwise installs for SoundPacks, which converts WAV / OGG / MP3 lines to the game's format with Wwise.
    /// The converter finds Wwise through the WWISEROOT environment variable, which the Wwise installer sets to the
    /// last version installed; when that one can't be used but a compatible install exists, this points the
    /// variable (for this process only) at it.
    /// </summary>
    public static class WwiseInstallation
    {
        public const string RootVariable = "WWISEROOT";
        public const int MinYear = 2013;
        public const int MaxYear = 2017;

        // Wwise's default install folders: "Wwise v2013.2.10 build 4884" (older) and "Wwise_2023.1.19.8928" (newer).
        private static readonly Regex VersionPattern = new(@"(20\d{2})\.(\d+)(?:\.(\d+))?", RegexOptions.Compiled);

        public static WwiseCheckResult Check() => Check(Environment.GetEnvironmentVariable(RootVariable), DefaultSearchFolders(), useCompatible: true);

        /// <summary>
        /// Looks at <paramref name="rootVariable"/> and every Wwise folder under <paramref name="searchFolders"/>.
        /// With <paramref name="useCompatible"/>, a compatible install becomes this process's WWISEROOT when the
        /// variable's own install can't be used.
        /// </summary>
        public static WwiseCheckResult Check(string? rootVariable, IEnumerable<string> searchFolders, bool useCompatible)
        {
            var installs = new List<WwiseInstall>();

            WwiseInstall? fromVariable = string.IsNullOrWhiteSpace(rootVariable) ? null : Inspect(rootVariable.Trim());
            if (fromVariable is not null)
                installs.Add(fromVariable);

            foreach (string searchFolder in searchFolders)
            {
                if (!Directory.Exists(searchFolder))
                    continue;

                IEnumerable<string> folders;
                try
                {
                    folders = Directory.EnumerateDirectories(searchFolder, "Wwise*").ToList();
                }
                catch (Exception ex) when (ex is IOException or UnauthorizedAccessException)
                {
                    continue;
                }

                foreach (string folder in folders)
                {
                    if (installs.Any(i => SameFolder(i.Folder, folder)))
                        continue;
                    if (Inspect(folder) is { } install)
                        installs.Add(install);
                }
            }

            if (fromVariable is { IsCompatible: true })
                return new WwiseCheckResult(WwiseStatus.Ready, fromVariable, installs);

            // Newest compatible first: its template is the closest match to what CustomsForge distributes.
            WwiseInstall? compatible = installs.Where(i => i.IsCompatible).OrderByDescending(i => i.Year).FirstOrDefault();
            if (compatible is not null)
            {
                if (useCompatible)
                    Environment.SetEnvironmentVariable(RootVariable, compatible.Folder);
                return new WwiseCheckResult(WwiseStatus.Ready, compatible, installs);
            }

            return new WwiseCheckResult(installs.Count == 0 ? WwiseStatus.NotInstalled : WwiseStatus.Incompatible, null, installs);
        }

        /// <summary>Reads one install folder, or null when it isn't one (missing, or no Authoring folder).</summary>
        public static WwiseInstall? Inspect(string folder)
        {
            string authoring = Path.Combine(folder, "Authoring");
            if (!Directory.Exists(authoring))
                return null;

            Match version = VersionPattern.Match(Path.GetFileName(folder.TrimEnd(Path.DirectorySeparatorChar, Path.AltDirectorySeparatorChar)));
            int year = version.Success ? int.Parse(version.Groups[1].Value) : 0;

            bool hasCli = new[] { "Win32", "x64" }
                .Any(platform => File.Exists(Path.Combine(authoring, platform, "Release", "bin", "WwiseCLI.exe")));

            return new WwiseInstall(folder, version.Success ? version.Value : "unknown version", year, hasCli);
        }

        private static IEnumerable<string> DefaultSearchFolders()
        {
            foreach (Environment.SpecialFolder programFiles in new[] { Environment.SpecialFolder.ProgramFilesX86, Environment.SpecialFolder.ProgramFiles })
            {
                string root = Environment.GetFolderPath(programFiles);
                if (!string.IsNullOrEmpty(root))
                    yield return Path.Combine(root, "Audiokinetic");
            }

            // Newer Wwise Launcher installs default to <system drive>\Audiokinetic.
            string? systemDrive = Path.GetPathRoot(Environment.GetFolderPath(Environment.SpecialFolder.Windows));
            if (!string.IsNullOrEmpty(systemDrive))
                yield return Path.Combine(systemDrive, "Audiokinetic");
        }

        private static bool SameFolder(string a, string b) =>
            string.Equals(
                Path.GetFullPath(a).TrimEnd(Path.DirectorySeparatorChar),
                Path.GetFullPath(b).TrimEnd(Path.DirectorySeparatorChar),
                StringComparison.OrdinalIgnoreCase);
    }
}
