using System;
using System.Collections.Generic;
using System.Globalization;
using System.IO;
using System.Linq;

namespace RSMods
{
    /// <summary>Creates, lists, prunes, and restores Rocksmith profile backups.</summary>
    public sealed class ProfileBackupService
    {
        private const string BackupSourceFormat = "MM-dd-yyyy_HH-mm-ss";
        private const string BackupDisplayFormat = "MMM dd, yyyy @ HH:mm:ss";

        public static void CreateBackup(string profileFolder, string rocksmithFolder)
        {
            if (string.IsNullOrEmpty(profileFolder))
                return;

            string backupsFolder = Path.Combine(rocksmithFolder, "Profile_Backups");
            string timestampedFolder = Path.Combine(backupsFolder, DateTime.Now.ToString(BackupSourceFormat));
            Directory.CreateDirectory(timestampedFolder);

            File.WriteAllText(
                Path.Combine(backupsFolder, "howto.txt"),
                $"If your save gets corrupted, take all the files in one of these folders and put them here: {profileFolder}");

            foreach (string file in Directory.EnumerateFiles(profileFolder))
                File.Copy(file, Path.Combine(timestampedFolder, Path.GetFileName(file)), true);
        }

        public static IEnumerable<string> GetFormattedBackupNames(string rocksmithFolder)
        {
            string backupPath = Path.Combine(rocksmithFolder, "Profile_Backups");
            if (!Directory.Exists(backupPath))
                return [];

            return Directory.EnumerateDirectories(backupPath)
                .Select(Path.GetFileName)
                .Select(FormatBackupFolderName)
                .Where(formatted => formatted != null)
                .Reverse();
        }

        public static string GetBackupSourceDirectory(string rocksmithFolder, string displayName)
        {
            if (!DateTime.TryParseExact(displayName, BackupDisplayFormat, CultureInfo.CurrentCulture, DateTimeStyles.None, out DateTime timestamp))
            {
                return null;
            }

            return Path.Combine(rocksmithFolder, "Profile_Backups", timestamp.ToString(BackupSourceFormat));
        }

        public static void DeleteOldBackups(string rocksmithFolder, int maximumBackups)
        {
            if (maximumBackups == 0) return;

            string backupFolder = Path.Combine(rocksmithFolder, "Profile_Backups");
            if (!Directory.Exists(backupFolder)) return;

            DirectoryInfo[] backups = [.. new DirectoryInfo(backupFolder)
                .GetDirectories()
                .OrderBy(folder => folder.LastWriteTime)];

            foreach (DirectoryInfo backup in backups.Take(Math.Max(0, backups.Length - maximumBackups)))
            {
                foreach (FileInfo file in backup.GetFiles())
                {
                    file.Delete();
                }

                backup.Delete();
            }
        }

        public static void RestoreBackup(string sourceDirectory, string targetDirectory)
        {
            foreach (string file in Directory.EnumerateFiles(sourceDirectory))
                File.Copy(file, Path.Combine(targetDirectory, Path.GetFileName(file)), true);
        }

        private static string FormatBackupFolderName(string folderName)
        {
            if (DateTime.TryParseExact(folderName, BackupSourceFormat, CultureInfo.InvariantCulture, DateTimeStyles.None, out DateTime timestamp))
            {
                return timestamp.ToString(BackupDisplayFormat, CultureInfo.CurrentCulture);
            }

            return null;
        }
    }
}
