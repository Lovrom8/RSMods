using Newtonsoft.Json.Linq;
using RocksmithToolkitLib.DLCPackage.Manifest2014.Tone;
using RSMods.Data;
using RSMods.Util;
using System;
using System.Collections.Generic;

namespace RSMods
{
    /// <summary>
    /// Compatibility facade for the existing WinForms call sites. New UI code can inject and use
    /// ProfileService, ProfileBackupService, and ProfileToneImportService directly.
    /// </summary>
    public static class Profiles
    {
        private static readonly ProfileService Service = new ProfileService();
        private static readonly ProfileBackupService Backups = new ProfileBackupService();
        private static readonly ProfileToneImportService ToneImports = new ProfileToneImportService(Service);

        public static string CurrentUnpackedProfileName
        {
            get => Service.CurrentProfileName;
            set => Service.CurrentProfileName = value;
        }

        public static JObject DecryptedProfile => Service.ActiveProfile;
        public static int SongListCount => Service.SongListCount;

        public static Dictionary<string, string> AvailableProfiles() => Service.GetAvailableProfiles();

        public static void SaveProfile()
        {
            Backups.CreateBackup(GenUtil.GetSaveDirectory(), Constants.RSFolder);
        }

        public static string DecryptProfiles(
            string path,
            bool dumpToFile = false,
            string dumpFile = "profileDump.json")
        {
            return Service.DecodeProfileJson(path, dumpToFile, dumpFile);
        }

        public static void EncryptCurrentProfile() => Service.SaveActiveProfile();

        public static void SaveTonesToProfile(List<object> newGuitarTones, List<object> newBassTones)
        {
            Service.SaveTones(newGuitarTones, newBassTones);
        }

        public static void ChangeRewardStatus(bool unlock) => Service.ChangeRewardStatus(unlock);

        public static int GetSongListCount(string profileName) => Service.GetSongListCount(profileName);

        public static string GetProfilePathFromName(string profileName) => Service.GetProfilePath(profileName);

        public static void SetProfileAsActive(string selectedProfile) => Service.SelectProfile(selectedProfile);

        public static bool ShouldIncludeSong(SongData song, HashSet<string> ownedDlc)
        {
            return Service.ShouldIncludeSong(song, ownedDlc);
        }

        public static HashSet<string> GetOwnedRS1DLC() => Service.GetOwnedRs1Dlc();

        public static List<List<string>> GetProfileSongLists() => Service.GetProfileSongLists();

        public static List<string> GetProfileFavoriteSongs() => Service.GetProfileFavoriteSongs();

        public static List<List<string>> GetProfileSongListsWithFavorites()
        {
            return Service.GetProfileSongListsWithFavorites();
        }

        public static void SetSongInList(string dlcKey, int listIndex, bool add)
        {
            Service.SetSongInList(dlcKey, listIndex, add);
        }

        public static void SetSongInFavorites(string dlcKey, bool add)
        {
            Service.SetSongInFavorites(dlcKey, add);
        }

        public static bool AddSongList() => Service.AddSongList();

        public static bool RemoveSongList() => Service.RemoveSongList();

        public static IEnumerable<string> GetFormattedBackupNames()
        {
            return Backups.GetFormattedBackupNames(GenUtil.GetRSDirectory());
        }

        public static string GetBackupSourceDir(string displayName)
        {
            return Backups.GetBackupSourceDirectory(GenUtil.GetRSDirectory(), displayName);
        }

        public static void DeleteOldBackups(int maxAmountOfBackups)
        {
            Backups.DeleteOldBackups(Constants.RSFolder, maxAmountOfBackups);
        }

        public static void RestoreBackup(string sourceDir, string targetDir)
        {
            Backups.RestoreBackup(sourceDir, targetDir);
        }

        public static (int ImportedCount, List<string> ErrorMessages) ProcessToneManifests(string[] fileNames)
        {
            return ToneImports.ImportManifests(fileNames);
        }

        public static (int ImportedCount, List<string> ErrorMessages) ProcessXmlTones(
            string[] fileNames,
            Func<Tone2014, bool> isGuitarPrompt)
        {
            return ToneImports.ImportXmlTones(fileNames, isGuitarPrompt);
        }
    }
}
