using System;
using System.Collections.Generic;
using System.Globalization;
using System.IO;
using System.Linq;

namespace RSMods
{
    /// <summary>A restorable profile backup. A backup the DLL hard-linked into several tiers is one entry with each tier in <see cref="Sources"/>.</summary>
    public sealed class ProfileBackup
    {
        public required string Folder { get; init; }
        public required DateTime Utc { get; init; }
        public List<string> Sources { get; } = [];

        public override string ToString() =>
            Utc.ToLocalTime().ToString("MMM d yyyy @ HH:mm:ss", CultureInfo.CurrentCulture) + " (" + string.Join(", ", Sources) + ")";
    }

    /// <summary>
    /// Creates, lists, prunes, and restores Rocksmith profile backups.
    /// Backups live in "Profile_AutoBackups\&lt;Steam account&gt;\&lt;folder&gt;\&lt;UTC time&gt;", shared with the DLL: the DLL fills the
    /// tier folders while the game runs, and the GUI backs up into "Before GUI" when it opens.
    /// </summary>
    public static class ProfileBackupService
    {
        public const string BeforeGuiBackups = "Before GUI";
        private static readonly string[] BackupFolders = [BeforeGuiBackups, "Every 10 Minutes", "Hourly", "Quarter Daily", "Daily"];
        private const string BackupNameFormat = "yyyy-MM-dd_HH-mm-ss"; // UTC; the DLL names its backups the same way.
        private const string PartialBackupPrefix = ".partial_";

        // Where the GUI used to put its backups. Nothing is written here anymore, but they can still be restored.
        private const string OldBackupsFolder = "Profile_Backups";
        private const string OldBackupNameFormat = "MM-dd-yyyy_HH-mm-ss"; // Local time.
        private const string OldBackupSource = "Old GUI Backup";

        /// <summary>The backups folder for the Steam account the save folder belongs to ("&lt;Steam&gt;\userdata\&lt;account&gt;\221680\remote"), or empty.</summary>
        public static string AccountBackupsFolder(string rocksmithFolder, string saveFolder)
        {
            if (string.IsNullOrEmpty(rocksmithFolder) || string.IsNullOrEmpty(saveFolder))
                return string.Empty;

            string account = new DirectoryInfo(saveFolder.TrimEnd('\\', '/')).Parent?.Parent?.Name;
            if (account == null || !uint.TryParse(account, out _))
                return string.Empty;

            return Path.Combine(rocksmithFolder, "Profile_AutoBackups", account);
        }

        /// <summary>
        /// Copies the save folder into "Before GUI". The copy goes to a ".partial_" folder that's renamed when it's complete,
        /// so a copy cut short never looks like a backup.
        /// </summary>
        public static void CreateBackup(string saveFolder, string rocksmithFolder)
        {
            string backupsFolder = AccountBackupsFolder(rocksmithFolder, saveFolder);
            if (backupsFolder.Length == 0 || !Directory.Exists(saveFolder))
                return;

            string beforeGui = Path.Combine(backupsFolder, BeforeGuiBackups);
            string name = DateTime.UtcNow.ToString(BackupNameFormat, CultureInfo.InvariantCulture);
            string made = Path.Combine(beforeGui, name);
            string partial = Path.Combine(beforeGui, PartialBackupPrefix + name);

            if (Directory.Exists(made)) // Already backed up this second.
                return;

            try
            {
                Directory.CreateDirectory(partial);
                foreach (string file in Directory.EnumerateFiles(saveFolder))
                    File.Copy(file, Path.Combine(partial, Path.GetFileName(file)), true);
                Directory.Move(partial, made);
            }
            catch
            {
                try { Directory.Delete(partial, true); } catch { }
                throw;
            }
        }

        /// <summary>
        /// Deletes the oldest "Before GUI" backups past <paramref name="maximumBackups"/> (0 keeps them all), and copies a previous run didn't finish.
        /// The DLL's folders prune themselves, and anything that isn't a backup we named is left alone.
        /// </summary>
        public static void DeleteOldBackups(string rocksmithFolder, string saveFolder, int maximumBackups)
        {
            string backupsFolder = AccountBackupsFolder(rocksmithFolder, saveFolder);
            string beforeGui = backupsFolder.Length == 0 ? string.Empty : Path.Combine(backupsFolder, BeforeGuiBackups);
            if (beforeGui.Length == 0 || !Directory.Exists(beforeGui))
                return;

            var backups = new List<string>();
            foreach (string folder in Directory.GetDirectories(beforeGui))
            {
                string name = Path.GetFileName(folder);
                if (name.StartsWith(PartialBackupPrefix, StringComparison.Ordinal))
                    Directory.Delete(folder, true);
                else if (DateTime.TryParseExact(name, BackupNameFormat, CultureInfo.InvariantCulture, DateTimeStyles.None, out _))
                    backups.Add(folder);
            }

            if (maximumBackups == 0) // User says they want all the backups.
                return;

            backups.Sort(StringComparer.Ordinal); // The names sort by time.
            for (int i = 0; i < backups.Count - maximumBackups; i++)
                Directory.Delete(backups[i], true);
        }

        /// <summary>Every backup that can be restored, newest first, including the old GUI's "Profile_Backups".</summary>
        public static List<ProfileBackup> ListBackups(string rocksmithFolder, string saveFolder)
        {
            var backups = new Dictionary<string, ProfileBackup>();

            string backupsFolder = AccountBackupsFolder(rocksmithFolder, saveFolder);
            if (backupsFolder.Length != 0)
            {
                foreach (string source in BackupFolders)
                {
                    string sourceFolder = Path.Combine(backupsFolder, source);
                    if (!Directory.Exists(sourceFolder))
                        continue;

                    foreach (string folder in Directory.GetDirectories(sourceFolder))
                    {
                        string name = Path.GetFileName(folder);
                        if (!DateTime.TryParseExact(name, BackupNameFormat, CultureInfo.InvariantCulture,
                                DateTimeStyles.AssumeUniversal | DateTimeStyles.AdjustToUniversal, out DateTime utc))
                            continue;

                        string key = source == BeforeGuiBackups ? source + name : name; // The GUI's backups are never linked into the DLL's folders.
                        if (!backups.TryGetValue(key, out ProfileBackup backup))
                            backups[key] = backup = new ProfileBackup { Folder = folder, Utc = utc };
                        backup.Sources.Add(source);
                    }
                }
            }

            string oldFolder = string.IsNullOrEmpty(rocksmithFolder) ? string.Empty : Path.Combine(rocksmithFolder, OldBackupsFolder);
            if (oldFolder.Length != 0 && Directory.Exists(oldFolder))
            {
                foreach (string folder in Directory.GetDirectories(oldFolder))
                {
                    if (!DateTime.TryParseExact(Path.GetFileName(folder), OldBackupNameFormat, CultureInfo.InvariantCulture,
                            DateTimeStyles.AssumeLocal | DateTimeStyles.AdjustToUniversal, out DateTime utc))
                        continue;

                    var backup = new ProfileBackup { Folder = folder, Utc = utc };
                    backup.Sources.Add(OldBackupSource);
                    backups["old" + folder] = backup;
                }
            }

            return [.. backups.Values.OrderByDescending(b => b.Utc)];
        }

        public static void RestoreBackup(string sourceDirectory, string targetDirectory)
        {
            foreach (string file in Directory.EnumerateFiles(sourceDirectory))
                File.Copy(file, Path.Combine(targetDirectory, Path.GetFileName(file)), true);
        }
    }
}
