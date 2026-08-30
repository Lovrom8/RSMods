using System;
using System.Collections.Generic;
using System.Globalization;
using System.IO;
using System.Security.Cryptography;
using MiscUtil.Conversion;
using MiscUtil.IO;
using zlib;
using Newtonsoft.Json;
using Newtonsoft.Json.Linq;
using RocksmithToolkitLib.DLCPackage.Manifest2014.Tone;
using System.Text;
using Microsoft.Win32;
using RSMods.Util;
using System.Linq;
using RSMods.Data;
using System.Web.Profile;

namespace RSMods
{
    static class Profiles
    {
        #region General

        public static string CurrentUnpackedProfileName { get; set; } = string.Empty;

        private static readonly byte[] PCSaveKey =
        [
            0x72, 0x8B, 0x36, 0x9E, 0x24, 0xED, 0x01, 0x34,
            0x76, 0x85, 0x11, 0x02, 0x18, 0x12, 0xAF, 0xC0,
            0xA3, 0xC2, 0x5D, 0x02, 0x06, 0x5F, 0x16, 0x6B,
            0x4B, 0xCC, 0x58, 0xCD, 0x26, 0x44, 0xF2, 0x9E
        ];

        private static void InitRijndael(Rijndael rij, byte[] key, CipherMode cipher)
        {
            rij.Padding = PaddingMode.None;
            rij.Mode = cipher;
            rij.BlockSize = 128;
            rij.IV = new byte[16];
            rij.Key = key;
        }

        private static void Crypto(Stream input, Stream output, ICryptoTransform transform, long len)
        {
            var buffer = new byte[512];
            int pad = buffer.Length - (int)(len % buffer.Length);

            var coder = new CryptoStream(output, transform, CryptoStreamMode.Write);

            long totalRead = 0;

            while (totalRead < len)
            {
                int size = (int)Math.Min(len - totalRead, buffer.Length);

                int bytesRead = input.Read(buffer, 0, size);

                if (bytesRead == 0)
                {
                    break;
                }

                coder.Write(buffer, 0, bytesRead);
                totalRead += bytesRead;
            }

            if (pad > 0 && pad < buffer.Length)
            {
                coder.Write(new byte[pad], 0, pad);
            }

            coder.FlushFinalBlock();
            output.Flush();

            if (output.CanSeek)
            {
                output.Seek(0, SeekOrigin.Begin);
            }
        }

        private static void Unzip(Stream str, Stream outStream, bool rewind = true)
        {
            int len;
            var buffer = new byte[65536];
            var zInputStream = new ZInputStream(str);

            while ((len = zInputStream.read(buffer, 0, buffer.Length)) > 0)
                outStream.Write(buffer, 0, len);

            zInputStream.Close();
            buffer = null;

            if (rewind)
            {
                outStream.Position = 0;
                outStream.Flush();
            }
        }

        private static long Zip(Stream str, Stream outStream, long plainLen, bool rewind = true)
        {
            var buffer = new byte[65536];
            var zOutputStream = new ZOutputStream(outStream, 9);

            long totalRead = 0;

            while (totalRead < plainLen)
            {
                var size = (int)Math.Min(plainLen - totalRead, buffer.Length);

                int bytesRead = str.Read(buffer, 0, size);

                if (bytesRead == 0)
                {
                    break;
                }

                zOutputStream.Write(buffer, 0, bytesRead);

                totalRead += bytesRead;
            }

            zOutputStream.finish();

            if (rewind)
            {
                outStream.Flush();
                if (outStream.CanSeek)
                {
                    outStream.Position = 0;
                }
            }

            return zOutputStream.TotalOut;
        }

        private static long Zip(byte[] array, Stream outStream, long plainLen, bool rewind = true) => Zip(new MemoryStream(array), outStream, plainLen, rewind);

        public static Dictionary<string, string> AvailableProfiles()
        {
            Dictionary<string, string> profiles = [];

            if (string.IsNullOrEmpty(Profiles.GetSaveDirectory()))
                return [];

            DirectoryInfo directory = new(Profiles.GetSaveDirectory());
            try
            {
                JToken token = JObject.Parse(DecryptProfiles(Path.Combine(directory.FullName, "LocalProfiles.json")));
                foreach (var profile in token.SelectToken("Profiles"))
                {
                    profiles.Add(profile.SelectToken("PlayerName").ToString(), profile.SelectToken("UniqueID").ToString());
                }
            }
            catch
            { }

            return profiles;
        }

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
                    return GenUtil.GetSteamProfilesFolderManual();

                string steamPath = steamKey.GetValue("SteamPath") as string;
                if (string.IsNullOrEmpty(steamPath))
                    return GenUtil.GetSteamProfilesFolderManual();

                using var usersKey = steamKey.OpenSubKey("Users");
                if (usersKey == null)
                    return GenUtil.GetSteamProfilesFolderManual();

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

            return GenUtil.GetSteamProfilesFolderManual();
        }

        #endregion

        public static void SaveProfile()
        {
            string profileFolder = GetSaveDirectory();
            if (string.IsNullOrEmpty(profileFolder)) return;

            string profileBackupsFolder = Path.Combine(Constants.RSFolder, "Profile_Backups");
            string timestamp = DateTime.Now.ToString("MM-dd-yyyy_HH-mm-ss");
            string timedBackupFolder = Path.Combine(profileBackupsFolder, timestamp);

            Directory.CreateDirectory(timedBackupFolder);

            File.WriteAllText(Path.Combine(profileBackupsFolder, "howto.txt"), $"If your save gets corrupted, take all the files in one of these folders and put them here: {profileFolder}");

            foreach (string file in Directory.EnumerateFiles(profileFolder))
            {
                File.Copy(file, Path.Combine(timedBackupFolder, Path.GetFileName(file)), true);
            }
        }

        #region Decrypt Profile

        private static JObject decryptedProfile = null;
        public static JObject DecryptedProfile { get => decryptedProfile; }

        private static void DecryptFile(Stream input, Stream output, byte[] key)
        {
            using (var rij = new RijndaelManaged())
            {
                InitRijndael(rij, key, CipherMode.ECB);
                Crypto(input, output, rij.CreateDecryptor(), input.Length);
            }
        }

        private static void DecryptProfile(Stream str, Stream outStream)
        {
            var source = EndianBitConverter.Little;
            var dec = EndianBitConverter.Big;

            str.Position = 0;
            using (var decrypted = new MemoryStream())
            using (var br = new EndianBinaryReader(source, str))
            using (var brDec = new EndianBinaryReader(dec, decrypted))
            {
                //EVAS + header
                br.ReadBytes(8);
                UserId = br.ReadBytes(4);
                br.ReadBytes(4);
                uint zLen = br.ReadUInt32();
                DecryptFile(br.BaseStream, decrypted, PCSaveKey);

                //unZip
                ushort xU = brDec.ReadUInt16();
                brDec.BaseStream.Position -= sizeof(ushort);
                if (xU == 30938)//LE 55928 //BE 30938
                {
                    Unzip(brDec.BaseStream, outStream);
                }//endless loop if not
            }
        }

        public static string DecryptProfiles(string path, bool dumpToFile = false, string dumpFile = "profileDump.json")
        {
            try
            {
                using var input = File.OpenRead(path);
                using var outMS = new MemoryStream();
                using var br = new StreamReader(outMS);

                DecryptProfile(input, outMS);

                if (dumpToFile)
                {
                    using StreamWriter sw = File.CreateText(dumpFile);
                    sw.WriteLine(br.ReadToEnd());
                }

                return br.ReadToEnd();
            }
            catch
            {
                return "";
            }
        }

        #endregion
        #region Encrypt Profile

        private readonly static byte[] SaveHeader =
        [
            0x45, 0x56, 0x41, 0x53,
            0x01, 0x00, 0x00, 0x00
        ];

        private static byte[] UserId = new byte[4];
        private readonly static byte[] EndOfSaveHeader =
        [
            0x00, 0x00, 0x10, 0x01
        ];

        public static void EncryptCurrentProfile()
        {
            EncryptProfile(DecryptedProfile.ToString(Newtonsoft.Json.Formatting.None), GetProfilePathFromName(CurrentUnpackedProfileName));
        }

        private static void EncryptProfile(string ProfileJson, string FileName)
        {
            using MemoryStream decryptedProfileStream = new();

            if (ProfileJson.EndsWith("\r\n"))
            {
                ProfileJson = ProfileJson.Remove(ProfileJson.LastIndexOf("\r\n"));
            }

            byte[] decryptedProfileArray = Encoding.UTF8.GetBytes(ProfileJson.ToArray());

            using (var encrypted = new MemoryStream())
            using (var compressed = new MemoryStream())
            using (var brEnc = new EndianBinaryWriter(EndianBitConverter.Little, encrypted))
            {
                brEnc.Write(SaveHeader, 0, 8);
                brEnc.Write(UserId, 0, 4);
                brEnc.Write(EndOfSaveHeader, 0, 4);
                brEnc.Write((uint)decryptedProfileArray.Length);

                Zip(decryptedProfileArray, compressed, decryptedProfileArray.Length);
                EncryptFile(compressed, encrypted, PCSaveKey);
                brEnc.Write(encrypted.ToArray());
                brEnc.Flush();

                brEnc.BaseStream.Seek(0, SeekOrigin.Begin);

                using (StreamWriter sw = new StreamWriter(FileName))
                {
                    brEnc.BaseStream.CopyTo(sw.BaseStream);
                }
            }
        }

        public static void SaveTonesToProfile(List<object> newGuitarTones, List<object> newBassTones)
        {
            var existingGuitarTones = DecryptedProfile["CustomTones"]?.ToObject<List<object>>() ?? [];
            var existingBassTones = DecryptedProfile["BassTones"]?.ToObject<List<object>>() ?? [];

            existingGuitarTones.AddRange(newGuitarTones);
            existingBassTones.AddRange(newBassTones);

            DecryptedProfile["CustomTones"] = JToken.FromObject(existingGuitarTones);
            DecryptedProfile["BassTones"] = JToken.FromObject(existingBassTones);

            EncryptCurrentProfile();
        }

        private static void EncryptFile(Stream input, Stream output, byte[] key)
        {
            using (var rij = new RijndaelManaged())
            {
                InitRijndael(rij, key, CipherMode.ECB);
                Crypto(input, output, rij.CreateEncryptor(), input.Length);
            }
        }
        #endregion
        #region Helpers
        public static void ChangeRewardStatus(bool unlock)
        {
            JToken prizes = DecryptedProfile["Prizes"];

            // Set number of points
            prizes["NumPrizePoints"] = unlock ? 1300.0 : 0.0;
            prizes["UnawardedPrizePoints"] = 0.0;

            // Uplay Rewards. Since late 2020, all Uplay rewards are given out by default. Make sure these are always set to true.
            prizes["UplayAction1"] = true;
            prizes["UplayAction2"] = true;
            prizes["UplayAction3"] = true;
            prizes["UplayAction4"] = true;
            prizes["UplayDlg1"] = true;
            prizes["UplayDlg2"] = true;
            prizes["UplayDlg3"] = true;
            prizes["UplayDlg4"] = true;

            // Rewards go all the way up to 186, but we have to skip a few entries
            for (int i = 1; i < 187; i++)
            {
                if (i == 7 || i == 14 || i == 22 || i == 29 || i == 36 || i == 45 ||
                    i == 60 || i == 68 || i == 77 || i == 83 || i == 93 || i == 100 ||
                    i == 109 || i == 117 || i == 125 || i == 131 || i == 141 ||
                    i == 148 || i == 154 || i == 155 || i == 164 || i == 169 || i == 184)
                {
                    continue;
                }

                prizes[$"IsPrizeAwarded_{i}"] = unlock;
                prizes[$"HasPrizeDialogShown_{i}"] = unlock;
            }

            DecryptedProfile["Prizes"] = prizes;
        }

        private static JObject ParseProfile(string profileName)
        {
            string profilePath = GetProfilePathFromName(profileName);
            string decryptedData = DecryptProfiles(profilePath);

            return JObject.Parse(decryptedData);
        }

        public static int GetSongListCount(string profileName)
        {
            try
            {
                JObject parsedJson = ParseProfile(profileName);
                JArray songLists = (JArray)parsedJson["SongListsRoot"]["SongLists"];

                return songLists.Count;
            }
            catch
            {
                return 0;
            }
        }

        public static string GetProfilePathFromName(string profileName) => Path.Combine(GetSaveDirectory(), AvailableProfiles()[profileName] + "_PRFLDB");

        public static void SetProfileAsActive(string selectedProfile)
        {
            decryptedProfile = ParseProfile(selectedProfile);
        }
        #endregion

        public static bool ShouldIncludeSong(SongData song, HashSet<string> ownedDLC)
        {
            if (!song.Shipping || string.IsNullOrEmpty(song.Artist) || string.IsNullOrEmpty(song.Title))
                return false;

            if (song.RS1AppID != 0 && !ownedDLC.Contains(song.RS1AppID.ToString()))
                return false;

            return true;
        }

        public static HashSet<string> GetOwnedRS1DLC()
        {
            return new HashSet<string>(
                DecryptedProfile["Stats"]["DLCTag"]
                    .Children<JProperty>()
                    .Select(p => p.Name)
            );
        }

        public static int SongListCount => DecryptedProfile != null ? GetProfileSongLists().Count : 6;

        public static List<List<string>> GetProfileSongLists() => DecryptedProfile["SongListsRoot"]["SongLists"].ToObject<List<List<string>>>();
        public static List<string> GetProfileFavoriteSongs() => DecryptedProfile["FavoritesListRoot"]["FavoritesList"].ToObject<List<string>>();

        public static List<List<string>> GetProfileSongListsWithFavorites()
        {
            var customLists = GetProfileSongLists();
            var favorites = GetProfileFavoriteSongs();

            return [favorites, .. customLists];
        }

        public static void SetSongInList(string dlcKey, int listIndex, bool add)
        {
            List<string> list = DecryptedProfile["SongListsRoot"]["SongLists"][listIndex].ToObject<List<string>>();

            if (add && !list.Contains(dlcKey)) list.Add(dlcKey);
            else if (!add && list.Contains(dlcKey)) list.Remove(dlcKey);

            DecryptedProfile["SongListsRoot"]["SongLists"][listIndex] = JToken.FromObject(list);
        }

        public static void SetSongInFavorites(string dlcKey, bool add)
        {
            List<string> favorites = GetProfileFavoriteSongs();

            if (add && !favorites.Contains(dlcKey)) favorites.Add(dlcKey);
            else if (!add && favorites.Contains(dlcKey)) favorites.Remove(dlcKey);

            DecryptedProfile["FavoritesListRoot"]["FavoritesList"] = JToken.FromObject(favorites);
        }

        public static bool AddSongList()
        {
            var lists = GetProfileSongLists();
            if (lists.Count >= 20) return false;

            lists.Add([]);
            DecryptedProfile["SongListsRoot"]["SongLists"] = JToken.FromObject(lists);
            EncryptCurrentProfile();
            return true;
        }

        public static bool RemoveSongList()
        {
            var lists = GetProfileSongLists();
            if (lists.Count <= 6) return false;

            lists.RemoveAt(lists.Count - 1);
            DecryptedProfile["SongListsRoot"]["SongLists"] = JToken.FromObject(lists);
            EncryptCurrentProfile();
            return true;
        }

        #region Backup Management

        private const string BackupSourceFormat = "MM-dd-yyyy_HH-mm-ss";
        private const string BackupDisplayFormat = "MMM dd, yyyy @ HH:mm:ss";

        private static string FormatBackupFolderName(string folderName)
        {
            if (DateTime.TryParseExact(folderName, BackupSourceFormat, null, DateTimeStyles.None, out var dt))
                return dt.ToString(BackupDisplayFormat);
            return null;
        }

        public static IEnumerable<string> GetFormattedBackupNames()
        {
            string backupPath = Path.Combine(GenUtil.GetRSDirectory(), "Profile_Backups");
            if (!Directory.Exists(backupPath))
                return Enumerable.Empty<string>();

            return Directory.EnumerateDirectories(backupPath)
                .Select(Path.GetFileName)
                .Select(FormatBackupFolderName)
                .Where(formatted => formatted != null)
                .Reverse();
        }

        /// <summary>
        /// Returns the full source directory path for a display-formatted backup name,
        /// or null if the name cannot be parsed.
        /// </summary>
        public static string GetBackupSourceDir(string displayName)
        {
            if (!DateTime.TryParseExact(displayName, BackupDisplayFormat, CultureInfo.CurrentCulture, DateTimeStyles.None, out DateTime dt))
                return null;

            return Path.Combine(GenUtil.GetRSDirectory(), "Profile_Backups", dt.ToString(BackupSourceFormat));
        }

        public static void DeleteOldBackups(int maxAmountOfBackups)
        {
            if (maxAmountOfBackups == 0) // User says they want all the backups.
                return;

            string backupFolder = Path.Combine(Constants.RSFolder, "Profile_Backups");
            if (!Directory.Exists(backupFolder))
                return;

            DirectoryInfo[] backups = [.. new DirectoryInfo(backupFolder).GetDirectories().OrderBy(f => f.LastWriteTime)];
            int foldersLeftToRemove = backups.Length - maxAmountOfBackups;

            foreach (DirectoryInfo backup in backups)
            {
                if (foldersLeftToRemove == 0)
                    break;

                if (Array.IndexOf(backups, backup.Name) < backups.Length - maxAmountOfBackups)
                {
                    foreach (string file in Directory.GetFiles(backup.FullName))
                        File.Delete(file);
                    Directory.Delete(backup.FullName);
                    foldersLeftToRemove--;
                }
            }
        }

        public static void RestoreBackup(string sourceDir, string targetDir)
        {
            foreach (string file in Directory.EnumerateFiles(sourceDir))
                File.Copy(file, Path.Combine(targetDir, Path.GetFileName(file)), true);
        }

        #endregion
        #region Tone Import

        public static (int ImportedCount, List<string> ErrorMessages) ProcessToneManifests(string[] filenames)
        {
            List<object> allGuitarTones = [];
            List<object> allBassTones = [];
            List<string> errors = [];

            foreach (string filename in filenames)
            {
                try
                {
                    var (guitarTones, bassTones) = ParseSingleManifest(filename);
                    allGuitarTones.AddRange(guitarTones);
                    allBassTones.AddRange(bassTones);
                }
                catch (Exception ex)
                {
                    errors.Add(ex.Message);
                }
            }

            int totalImported = allGuitarTones.Count + allBassTones.Count;

            if (totalImported > 0)
                SaveTonesToProfile(allGuitarTones, allBassTones);

            return (totalImported, errors);
        }

        private static (List<object> GuitarTones, List<object> BassTones) ParseSingleManifest(string filename)
        {
            string name = Path.GetFileName(filename);

            var manifest = JObject.Parse(File.ReadAllText(filename));

            if (manifest["Entries"] is not JObject entries)
                throw new InvalidDataException($"Input Tone Manifest missing valid Entries: {name}");

            if (!entries.HasValues)
                throw new InvalidDataException($"Input Tone Manifest Entries has no children: {name}");

            var firstEntry = entries.Properties().First();

            if (string.IsNullOrEmpty(firstEntry.Name))
                throw new InvalidDataException($"Input Tone Manifest has no ArrangementId: {name}");

            if (firstEntry.Value.Type == JTokenType.Null || firstEntry.Value.Type == JTokenType.Undefined)
                throw new InvalidDataException($"Input Tone Manifest has invalid ArrangementId: {name}");

            if (firstEntry.Value["Attributes"] is not JToken attributes)
                throw new InvalidDataException($"Input Tone Manifest has no arrangement Attributes: {name}");

            string arrangementName = attributes["ArrangementName"]?.ToString();
            if (string.IsNullOrEmpty(arrangementName))
                throw new InvalidDataException($"Input Tone Manifest missing Arrangement Name: {name}");

            if (attributes["Tones"] is not JArray tones)
                throw new InvalidDataException($"Input Tone Manifest missing Tones: {name}");

            bool isBass = arrangementName.IndexOf("Bass", StringComparison.OrdinalIgnoreCase) >= 0;
            var toneList = tones.ToObject<List<object>>();

            return isBass ? ([], toneList) : (toneList, []);
        }

        public static (int ImportedCount, List<string> ErrorMessages) ProcessXmlTones(string[] filenames, Func<Tone2014, bool> isGuitarPrompt)
        {
            List<object> guitarTones = [];
            List<object> bassTones = [];
            List<string> errors = [];

            foreach (string filename in filenames)
            {
                try
                {
                    Tone2014 tone = Tone2014.LoadFromXmlTemplateFile(filename);

                    if (tone == null)
                    {
                        errors.Add($"Failed to load tone from {Path.GetFileName(filename)}");
                        continue;
                    }

                    if (isGuitarPrompt(tone))
                        guitarTones.Add(tone);
                    else
                        bassTones.Add(tone);
                }
                catch (Exception ex)
                {
                    errors.Add($"Error reading {Path.GetFileName(filename)}: {ex.Message}");
                }
            }

            int totalImported = guitarTones.Count + bassTones.Count;

            if (totalImported > 0)
                SaveTonesToProfile(guitarTones, bassTones);

            return (totalImported, errors);
        }

        #endregion
    }
}