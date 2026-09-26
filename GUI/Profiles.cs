using System;
using System.Collections.Generic;
using System.IO;
using System.Security.Cryptography;
using MiscUtil.Conversion;
using MiscUtil.IO;
using zlib;
using Newtonsoft.Json.Linq;
using System.Text;
using Microsoft.Win32;
using RSMods.Util;
using System.Linq;
using System.Globalization;
using RSMods.Data;

namespace RSMods
{
    class Profiles
    {

        #region General

        private static byte[] PCSaveKey = new byte[32]
        {
            0x72, 0x8B, 0x36, 0x9E, 0x24, 0xED, 0x01, 0x34,
            0x76, 0x85, 0x11, 0x02, 0x18, 0x12, 0xAF, 0xC0,
            0xA3, 0xC2, 0x5D, 0x02, 0x06, 0x5F, 0x16, 0x6B,
            0x4B, 0xCC, 0x58, 0xCD, 0x26, 0x44, 0xF2, 0x9E
        };

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
            while (input.Position < len)
            {
                int size = (int)Math.Min(len - input.Position, buffer.Length);
                input.Read(buffer, 0, size);
                coder.Write(buffer, 0, size);
            }

            if (pad > 0)
                coder.Write(new byte[pad], 0, pad);

            coder.Flush();
            output.Seek(0, SeekOrigin.Begin);
            output.Flush();
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

            while (str.Position < plainLen)
            {
                var size = (int)Math.Min(plainLen - str.Position, buffer.Length);
                str.Read(buffer, 0, size);
                zOutputStream.Write(buffer, 0, size);
            }

            zOutputStream.finish(); buffer = null;
            if (rewind)
            {
                outStream.Position = 0;
                outStream.Flush();
            }

            return zOutputStream.TotalOut;
        }

        private static long Zip(byte[] array, Stream outStream, long plainLen, bool rewind = true) => Zip(new MemoryStream(array), outStream, plainLen, rewind);

        public static Dictionary<string, string> AvailableProfiles()
        {
            Dictionary<string, string> profiles = new Dictionary<string, string>();

            if (Profiles.GetSaveDirectory() == String.Empty)
                return new Dictionary<string, string>();

            DirectoryInfo directory = new DirectoryInfo(Profiles.GetSaveDirectory());
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
            string fullProfileFolder = forceRegistry ? string.Empty : Constants.SavePath;

            try
            {
                if (fullProfileFolder == string.Empty)
                {
                    RegistryKey availableUser = Registry.CurrentUser.OpenSubKey("SOFTWARE").OpenSubKey("Valve").OpenSubKey("Steam").OpenSubKey("Users");

                    if (availableUser == null) // If the key doesn't exist for whatever reason, try to manually find the path by searching for files with 
                        return GenUtil.GetSteamProfilesFolderManual();

                    string steamFolder = Registry.CurrentUser.OpenSubKey("SOFTWARE").OpenSubKey("Valve").OpenSubKey("Steam").GetValue("SteamPath").ToString(), profileSubFolders = "/221680/remote", userDataFolder = "/userdata/";

                    foreach (string user in availableUser.GetSubKeyNames())
                    {
                        try
                        {
                            if (Directory.Exists(steamFolder + userDataFolder + user + profileSubFolders))
                            {
                                fullProfileFolder = steamFolder + userDataFolder + user + profileSubFolders;
                                break;
                            }
                            else
                                continue;
                        }
                        catch (Exception) // Directory doesn't exist and shoots an error.
                        {
                            continue;
                        }
                    }
                }
                else
                {
                    if (!fullProfileFolder.IsSavePath())
                    {
                        Constants.SavePath = string.Empty;
                        return GetSaveDirectory();
                    }
                }
            }
            catch (NullReferenceException) // If for whatever reason the key doesn't exist, let's not crash the whole application
            {
            }

            return fullProfileFolder;
        }

        #endregion
        #region Backup Profile

        // Backups live in "Profile_AutoBackups\<Steam account>\<folder>\<UTC time>", shared with the DLL.
        // The DLL fills the tier folders while the game runs; the GUI backs up into "Before GUI" when it opens,
        // so profile edits made here while the game is closed always have a backup too.
        public const string BeforeGuiBackups = "Before GUI";
        private static readonly string[] BackupFolders = { BeforeGuiBackups, "Every 10 Minutes", "Hourly", "Quarter Daily", "Daily" };
        private const string BackupNameFormat = "yyyy-MM-dd_HH-mm-ss"; // UTC. The DLL names its backups the same way.
        private const string PartialBackupPrefix = ".partial_";

        // Where the GUI used to put its backups. Nothing is written here anymore, but they can still be restored.
        private const string OldBackupsFolder = "Profile_Backups";
        private const string OldBackupNameFormat = "MM-dd-yyyy_HH-mm-ss"; // Local time.

        public class ProfileBackup
        {
            public string Folder;
            public DateTime Utc;
            public List<string> Sources = new List<string>();

            public override string ToString() => Utc.ToLocalTime().ToString("MMM d yyyy @ HH:mm:ss", CultureInfo.CurrentCulture) + " (" + string.Join(", ", Sources) + ")";
        }

        /// <summary>
        /// The backups folder for the Steam account the save folder belongs to: the save folder is "<Steam>\userdata\<account>\221680\remote".
        /// </summary>
        public static string AccountBackupsFolder()
        {
            string saveFolder = GetSaveDirectory();
            if (saveFolder == String.Empty)
                return String.Empty;

            string account = new DirectoryInfo(saveFolder.TrimEnd('\\', '/')).Parent?.Parent?.Name;
            if (account == null || !uint.TryParse(account, out _))
                return String.Empty;

            return Path.Combine(Constants.RSFolder, "Profile_AutoBackups", account);
        }

        public static void SaveProfile()
        {
            string profileFolder = GetSaveDirectory();
            string backupsFolder = AccountBackupsFolder();

            if (profileFolder == String.Empty || backupsFolder == String.Empty)
                return;

            string beforeGui = Path.Combine(backupsFolder, BeforeGuiBackups);
            string name = DateTime.UtcNow.ToString(BackupNameFormat, CultureInfo.InvariantCulture);
            string made = Path.Combine(beforeGui, name);
            string partial = Path.Combine(beforeGui, PartialBackupPrefix + name);

            if (Directory.Exists(made)) // Already backed up this second.
                return;

            // Copy into a ".partial_" folder and rename it when it's complete, so a copy cut short never looks like a backup.
            try
            {
                Directory.CreateDirectory(partial);
                foreach (string file in Directory.GetFiles(profileFolder))
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
        /// Deletes the oldest "Before GUI" backups past maxAmountOfBackups (0 keeps them all), and copies a previous run didn't finish.
        /// The DLL's folders prune themselves, and anything that isn't a backup we named is left alone.
        /// </summary>
        public static void DeleteOldBackups(int maxAmountOfBackups)
        {
            string backupsFolder = AccountBackupsFolder();
            string beforeGui = backupsFolder == String.Empty ? String.Empty : Path.Combine(backupsFolder, BeforeGuiBackups);
            if (beforeGui == String.Empty || !Directory.Exists(beforeGui))
                return;

            List<string> backups = new List<string>();
            foreach (string folder in Directory.GetDirectories(beforeGui))
            {
                string name = Path.GetFileName(folder);
                if (name.StartsWith(PartialBackupPrefix))
                    Directory.Delete(folder, true);
                else if (DateTime.TryParseExact(name, BackupNameFormat, CultureInfo.InvariantCulture, DateTimeStyles.None, out _))
                    backups.Add(folder);
            }

            if (maxAmountOfBackups == 0) // User says they want all the backups.
                return;

            backups.Sort(StringComparer.Ordinal); // The names sort by time.
            for (int i = 0; i < backups.Count - maxAmountOfBackups; i++)
                Directory.Delete(backups[i], true);
        }

        /// <summary>
        /// Every backup that can be restored, newest first. A backup the DLL put in several folders is listed once, with each folder it's in.
        /// </summary>
        public static List<ProfileBackup> ListBackups()
        {
            Dictionary<string, ProfileBackup> backups = new Dictionary<string, ProfileBackup>();

            string backupsFolder = AccountBackupsFolder();
            if (backupsFolder != String.Empty)
            {
                foreach (string source in BackupFolders)
                {
                    string sourceFolder = Path.Combine(backupsFolder, source);
                    if (!Directory.Exists(sourceFolder))
                        continue;

                    foreach (string folder in Directory.GetDirectories(sourceFolder))
                    {
                        string name = Path.GetFileName(folder);
                        if (!DateTime.TryParseExact(name, BackupNameFormat, CultureInfo.InvariantCulture, DateTimeStyles.AssumeUniversal | DateTimeStyles.AdjustToUniversal, out DateTime utc))
                            continue;

                        string key = source == BeforeGuiBackups ? source + name : name; // The GUI's backups are never linked into the DLL's folders.
                        if (!backups.TryGetValue(key, out ProfileBackup backup))
                            backups[key] = backup = new ProfileBackup { Folder = folder, Utc = utc };
                        backup.Sources.Add(source);
                    }
                }
            }

            string oldFolder = Path.Combine(Constants.RSFolder, OldBackupsFolder);
            if (Directory.Exists(oldFolder))
            {
                foreach (string folder in Directory.GetDirectories(oldFolder))
                {
                    if (DateTime.TryParseExact(Path.GetFileName(folder), OldBackupNameFormat, CultureInfo.InvariantCulture, DateTimeStyles.AssumeLocal | DateTimeStyles.AdjustToUniversal, out DateTime utc))
                        backups["old" + folder] = new ProfileBackup { Folder = folder, Utc = utc, Sources = { "Old GUI Backup" } };
                }
            }

            return backups.Values.OrderByDescending(b => b.Utc).ToList();
        }

        #endregion

        #region Decrypt Profile

        public static JObject DecryptedProfile = null;
        
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
                using (var input = File.OpenRead(path))
                using (var outMS = new MemoryStream())
                using (var br = new StreamReader(outMS))
                {
                    DecryptProfile(input, outMS);

                    if (dumpToFile)
                    {
                        using (StreamWriter sw = File.CreateText(dumpFile))
                        {
                            sw.WriteLine(br.ReadToEnd());
                        }
                    }

                    return br.ReadToEnd();
                }
            }
            catch
            {
                return "";
            }
        }

        #endregion
        #region Encrypt Profile

        private static byte[] SaveHeader = new byte[8]
        {
            0x45, 0x56, 0x41, 0x53,
            0x01, 0x00, 0x00, 0x00
        };

        private static byte[] UserId = new byte[4];
        private static byte[] EndOfSaveHeader = new byte[4]
        {
            0x00, 0x00, 0x10, 0x01
        };


        public static void EncryptProfile(string ProfileJson, string FileName)
        {
            using (MemoryStream decryptedProfileStream = new MemoryStream())
            {
                if(ProfileJson.EndsWith("\r\n"))
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
        #endregion
    }


}