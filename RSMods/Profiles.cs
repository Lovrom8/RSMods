using System;
using System.Collections.Generic;
using System.IO;
using System.Security.Cryptography;
using MiscUtil.Conversion;
using MiscUtil.IO;
using zlib;
using Newtonsoft.Json.Linq;
using System.Text;
using System.Runtime.CompilerServices;
using Newtonsoft.Json;
using Microsoft.CSharp.RuntimeBinder;
using Microsoft.Win32;
using System.Windows.Forms;
using RSMods.Util;
using System.Linq;

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

        public static string GetSaveDirectory()
        {
            string fullProfileFolder = String.Empty;

            try
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
            catch (NullReferenceException) // If for whatever reason the key doesn't exist, let's not crash the whole application
            {
            }

            return fullProfileFolder;
        }

        #endregion
        #region Backup Profile

        public static void SaveProfile()
        {
            string profileFolder = GetSaveDirectory();

            if (profileFolder == String.Empty)
                return;

            string profileBackupsFolder = Path.Combine(RSMods.Data.Constants.RSFolder, "Profile_Backups");
            DateTime now = DateTime.Now;
            string timedBackupFolder = Path.Combine(profileBackupsFolder, now.ToString("MM-dd-yyyy_HH-mm-ss"));
            string howToRestoreBackupTxt = Path.Combine(profileBackupsFolder, "howto.txt");

            Directory.CreateDirectory(profileBackupsFolder);
            Directory.CreateDirectory(timedBackupFolder);

            using (StreamWriter sw = File.CreateText(howToRestoreBackupTxt))
            {
                sw.WriteLine("If your save gets corrupted, take all the files in one of these folders and put them in this folder: " + profileFolder);
            }

            foreach (string file in Directory.GetFiles(profileFolder))
            {
                File.Copy(file, Path.Combine(timedBackupFolder, Path.GetFileName(file)), true); 
            }
        }

        #endregion

        #region Decrypt Profile

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
                br.ReadBytes(16);
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

        private static byte[] SaveHeader = new byte[16]
        {
            0x45, 0x56, 0x41, 0x53, 0x01, 0x00, 0x00, 0x00,
            0x5D, 0x8F, 0xE7, 0x03, 0x00, 0x00, 0x10, 0x01,
        };


        //public static void TestEncryption()
        //{
        //    string profile = string.Empty;
            
        //    using (StreamReader decryptedFile = new StreamReader("decryptedprofile.json"))
        //    using (Stream decryptedProfileStream = new MemoryStream())
        //    {
        //        profile = decryptedFile.ReadToEnd();

        //        decryptedFile.BaseStream.Seek(0, SeekOrigin.Begin);

        //        decryptedFile.BaseStream.CopyTo(decryptedProfileStream);

        //        decryptedProfileStream.Seek(0, SeekOrigin.End);
        //        decryptedProfileStream.Write(new byte[1] { 0 }, 0, 1);
        //        decryptedProfileStream.Seek(0, SeekOrigin.Begin);

        //        decryptedProfileStream.Position = 0;

        //        using (var encrypted = new MemoryStream())
        //        using (var compressed = new MemoryStream())
        //        using (var brEnc = new EndianBinaryWriter(EndianBitConverter.Little, encrypted))
        //        {
        //            brEnc.Write(SaveHeader, 0, 16);
        //            brEnc.Write((uint)decryptedProfileStream.Length);

        //            Zip(decryptedProfileStream, compressed, decryptedProfileStream.Length);
        //            compressed.Seek(2, SeekOrigin.Current);
        //            EncryptFile(compressed, brEnc.BaseStream, PCSaveKey);

        //            brEnc.BaseStream.Seek(0, SeekOrigin.Begin);

        //            using (StreamWriter sw = new StreamWriter("profile_PRFLDB"))
        //            {
        //                brEnc.BaseStream.CopyTo(sw.BaseStream);
        //            }
        //        }

        //        using (StreamReader RawZlibStream = new StreamReader("compressedzlib.zlib"))
        //        using (Stream ZlibStream = new MemoryStream())
        //        {
        //            RawZlibStream.BaseStream.CopyTo(ZlibStream);

        //            ZlibStream.Seek(0, SeekOrigin.Begin);

        //            using (var encrypted = new MemoryStream())
        //            using (var brEnc = new EndianBinaryWriter(EndianBitConverter.Little, encrypted))
        //            {
        //                brEnc.Write(SaveHeader, 0, 16);
        //                brEnc.Write((uint)decryptedProfileStream.Length);

        //                EncryptFile(ZlibStream, brEnc.BaseStream, PCSaveKey);

        //                brEnc.BaseStream.Seek(0, SeekOrigin.Begin);

        //                using (StreamWriter sw = new StreamWriter("profile_PRFLDB"))
        //                {
        //                    //brEnc.BaseStream.SetLength(brEnc.BaseStream.Length - 0x180);
        //                    brEnc.BaseStream.CopyTo(sw.BaseStream);
        //                }
        //            }
        //        }
        //    }
        //    MessageBox.Show("Done!");
        //} 

        private static void EncryptFile(Stream input, Stream output, byte[] key)
        {
            using (var rij = new RijndaelManaged())
            {
                InitRijndael(rij, key, CipherMode.ECB);
                Crypto(input, output, rij.CreateEncryptor(), input.Length);
            }
        }

        // Profiles.EncryptProfiles<ProfileClasses.PROFILECLASS>(Profiles.JSON_ToObject<ProfileClasses.PROFILECLASS>("PATH_TO_PRFLDB", "PROFILECLASS"), "PROFILECLASS", Profiles.DecryptProfiles("PATH_TO_PRFLDB"), "PATH_TO_PRFLDB");

        public static void EncryptProfiles<T>(T SectionToWrite, string NameOfSection, string profileJSON, string pathToOutput)
        {
            using (MemoryStream memoryStream = new MemoryStream())
            using (StreamReader json = new StreamReader(memoryStream))
            {
                string serializedSection = JsonConvert.SerializeObject(SectionToWrite);
                string jsonHandling = JSON_Formatting(profileJSON, NameOfSection, serializedSection);
                string jsonToUTF8 = FixJsonFormatting(jsonHandling);
                byte[] utf8Encoding = Encoding.UTF8.GetBytes(jsonToUTF8);
                MemoryStream utfMemStream = new MemoryStream(utf8Encoding);
                using (FileStream profile = new FileStream(pathToOutput, FileMode.Create))
                {
                    EncryptProfile(utfMemStream, profile);
                }
            }
        }

        private static void EncryptProfile(Stream stream0, Stream stream1)
        {
            LittleEndianBitConverter endianBitConverter = EndianBitConverter.Little;
            stream0.Position = 0;

            using (MemoryStream memoryStream = new MemoryStream())
            using (MemoryStream memoryStream2 = new MemoryStream())
            using (EndianBinaryWriter endianBinaryWriter = new EndianBinaryWriter(endianBitConverter, stream1))
            {
                byte[] array = null;
                using (MemoryStream memoryStream3 = new MemoryStream())
                {
                    stream0.CopyTo(memoryStream3);
                    memoryStream3.Seek(-1, SeekOrigin.End);

                    if (memoryStream3.ReadByte() != 0)
                    {
                        memoryStream3.Write(new byte[1], 0, 1);
                    }
                    array = memoryStream3.ToArray();
                }
                Zip(array, memoryStream, (long)array.Length);
                EncryptFile(memoryStream, memoryStream2, PCSaveKey);
                endianBinaryWriter.Write(SaveHeader);
                endianBinaryWriter.Write(array.Length);
                endianBinaryWriter.Write(memoryStream2.ToArray());
                endianBinaryWriter.Flush();
            }
        }
        #endregion
        #region Utils
        private static string FixJsonFormatting(string json)
        {
            json = json.Replace("\u0020\u0020", String.Empty); // Get rid of indents
            json = json.Replace("\u0022\u003A", "\u0022\u0020\u003A"); // `":` -> `" :`
            json = json.Replace("\u007B\u007D\u002C", "\u007B\u000A\u007D\u002C"); // "{}," -> "{\n},"
            json = json.Replace("\u0022\u0020\u003a\u0020\u005B", "\u0022\u0020\u003a\u0020\u000A\u005B"); // '" : [' -> '" : \n['
            json = json.Replace("\u007B\u007D\u000A", "\u007B\u000A\u007D"); // "{}\n" -> "{\n}"
            json = json.Replace("\u005B\u005D\u002C", "\u005B\u000A\u005D\u002C"); // "[]," -> "[\n],"
            json = json.Replace("\u005B\u000A\u005B", "\u005B\u000A\u000A\u005B"); // "[\n[" -> "[\n\n["
            json = json.Replace("\u005D\u002C\u000A\u005B", "\u005D\u002C\u000A\u000A\u005B"); // "],\n[" -> "],\n\n["
            json = json.Replace("\u005B\u005D", "\u005B\u000A\u005D"); // "[]" -> "[\n]"
            json = json.Replace("\u007B\u007D", "\u007B\u000A\u007D"); // "{}" -> "{\n}"
            return json;
        }

        public static T JSON_ToObject<T>(string profilePath, string objectToLookFor)
        {
            T result = (default(T) == null) ? Activator.CreateInstance<T>() : default(T);

            using (FileStream fileStream = File.OpenRead(profilePath))
            using (MemoryStream memoryStream = new MemoryStream())
            using (StreamReader streamReader = new StreamReader(memoryStream))
            {
                DecryptProfile(fileStream, memoryStream);
                string json = streamReader.ReadToEnd();
                JToken parsedJson = JObject.Parse(json);
                JToken wantedSection = parsedJson.SelectToken(objectToLookFor);

                if (wantedSection == null)
                    return default(T);

                result = wantedSection.ToObject<T>();
            }
            return result;
        }

        public static T JSON_ToObject<T>(string profilePath, Profile_Sections.Sections section) 
        {
            string sectionName = String.Empty;

            switch (section)
            {
                case Profile_Sections.Sections.Missions:
                    sectionName = "Mission";
                    break;
                case Profile_Sections.Sections.PlayNexts:
                    sectionName = "Playnexts";
                    break;
                case Profile_Sections.Sections.RecentlyPlayedVenues:
                    sectionName = "RecentlyPlayedVenues";
                    break;
                case Profile_Sections.Sections.Chords:
                    sectionName = "Chords";
                    break;
                case Profile_Sections.Sections.Lessons:
                    sectionName = "GE";
                    break;
                case Profile_Sections.Sections.NSP_PlayListRoot2:
                    sectionName = "NSP_PlayListRoot2";
                    break;
                case Profile_Sections.Sections.Options:
                    sectionName = "Options";
                    break;
                case Profile_Sections.Sections.Guitarcade:
                    sectionName = "Guitarcade";
                    break;
                case Profile_Sections.Sections.SongLists:
                    sectionName = "SongListsRoot";
                    break;
                case Profile_Sections.Sections.FavoritesList:
                    sectionName = "FavoritesListRoot";
                    break;
                case Profile_Sections.Sections.DD:
                    sectionName = "DynamicDifficulty";
                    break;
                case Profile_Sections.Sections.Prizes:
                    sectionName = "Prizes";
                    break;
                case Profile_Sections.Sections.LearnASong:
                    sectionName = "Songs";
                    break;
                case Profile_Sections.Sections.SessionMode:
                    sectionName = "SessionMode";
                    break;
                case Profile_Sections.Sections.Achievements:
                    sectionName = "Achievements";
                    break;
                case Profile_Sections.Sections.Stats:
                    sectionName = "Stats";
                    break;
                case Profile_Sections.Sections.ScoreAttack:
                    sectionName = "SongsSA";
                    break;
            }

            if (sectionName == String.Empty)
                return default(T);

            return JSON_ToObject<T>(profilePath, sectionName);
        }

        private static string JSON_Formatting(string jsonProfile, string nameOfSection, string serializedSection)
        {
            object argument = JsonConvert.DeserializeObject(serializedSection);
            object profile = JsonConvert.DeserializeObject(jsonProfile);

            if (Tools.CSharpArgumentInfo.callSite_0 == null)
            {
                Tools.CSharpArgumentInfo.callSite_0 = CallSite<Func<CallSite, object, string, object, object>>.Create(BinderSetIndex(CSharpBinderFlags.None, Type.GetTypeFromHandle(typeof(Profiles).TypeHandle), new Microsoft.CSharp.RuntimeBinder.CSharpArgumentInfo[]
                {
                    CreateCSharpArgumentInfo(CSharpArgumentInfoFlags.None),
                    CreateCSharpArgumentInfo(CSharpArgumentInfoFlags.UseCompileTimeType),
                    CreateCSharpArgumentInfo(CSharpArgumentInfoFlags.None),
                }));
            }
            Tools.CSharpArgumentInfo.callSite_0.Target(Tools.CSharpArgumentInfo.callSite_0, profile, nameOfSection, argument);

            JsonSerializerSettings serializerSettings = new JsonSerializerSettings()
            {
                NullValueHandling = NullValueHandling.Ignore,
                StringEscapeHandling = StringEscapeHandling.EscapeNonAscii,
                Formatting = Formatting.Indented,
                Converters = new List<JsonConverter> { new Tools.DecimalFormatJsonConverter(6) }
            };

            if (Tools.CSharpArgumentInfo.callSite_1 == null)
                Tools.CSharpArgumentInfo.callSite_1 = CallSite<Func<CallSite, object, string>>.Create(BinderConvert(CSharpBinderFlags.None, Type.GetTypeFromHandle(typeof(string).TypeHandle), Type.GetTypeFromHandle(typeof(Profiles).TypeHandle)));

            Func<CallSite, object, string> target = Tools.CSharpArgumentInfo.callSite_1.Target;
            CallSite callsite_3_temp = Tools.CSharpArgumentInfo.callSite_1;

            if (Tools.CSharpArgumentInfo.callSite_2 == null)
            {
                Tools.CSharpArgumentInfo.callSite_2 = CallSite<Func<CallSite, Type, object, JsonSerializerSettings, object>>.Create(BinderInvokeMember(CSharpBinderFlags.None, "SerializeObject", Type.GetTypeFromHandle(typeof(Profiles).TypeHandle), new Microsoft.CSharp.RuntimeBinder.CSharpArgumentInfo[] {
                    CreateCSharpArgumentInfo(CSharpArgumentInfoFlags.UseCompileTimeType | CSharpArgumentInfoFlags.IsStaticType),
                    CreateCSharpArgumentInfo(CSharpArgumentInfoFlags.None),
                    CreateCSharpArgumentInfo(CSharpArgumentInfoFlags.UseCompileTimeType)
                }));
            }
            return target(callsite_3_temp, Tools.CSharpArgumentInfo.callSite_2.Target(Tools.CSharpArgumentInfo.callSite_2, Type.GetTypeFromHandle(typeof(JsonConvert).TypeHandle), profile, serializerSettings));
        }

        private static CallSiteBinder BinderSetIndex(CSharpBinderFlags flags, Type userProfileType, IEnumerable<Microsoft.CSharp.RuntimeBinder.CSharpArgumentInfo> cSharpArgumentInfo) => Binder.SetIndex(flags, userProfileType, cSharpArgumentInfo);
        private static Microsoft.CSharp.RuntimeBinder.CSharpArgumentInfo CreateCSharpArgumentInfo(CSharpArgumentInfoFlags flags) => Microsoft.CSharp.RuntimeBinder.CSharpArgumentInfo.Create(flags, null);
        private static CallSiteBinder BinderConvert(CSharpBinderFlags flags, Type to, Type from) => Binder.Convert(flags, to, from);
        private static CallSiteBinder BinderInvokeMember(CSharpBinderFlags flags, string name, Type context, IEnumerable<Microsoft.CSharp.RuntimeBinder.CSharpArgumentInfo> argumentInfo) => Binder.InvokeMember(flags, name, null, context, argumentInfo);

        #endregion
    }

    #region Tools
    class Tools
    {
        public static class CSharpArgumentInfo
        {
            public static CallSite<Func<CallSite, object, string, object, object>> callSite_0;
            public static CallSite<Func<CallSite, object, string>> callSite_1;
            public static CallSite<Func<CallSite, Type, object, JsonSerializerSettings, object>> callSite_2;
        }

        public class DecimalFormatJsonConverter : JsonConverter // Taken from stack overflow: https://stackoverflow.com/a/24052157 | Still isn't perfect (removes excessive 0's)
        {
            private readonly int _numberOfDecimals;

            public DecimalFormatJsonConverter(int numberOfDecimals)
            {
                _numberOfDecimals = numberOfDecimals;
            }

            public override void WriteJson(JsonWriter writer, object value, JsonSerializer serializer)
            {
                var d = (decimal)value;
                var rounded = Math.Round(d, _numberOfDecimals);
                writer.WriteValue((decimal)rounded);
            }

            public override object ReadJson(JsonReader reader, Type objectType, object existingValue, JsonSerializer serializer)
            {
                throw new NotImplementedException("Unnecessary because CanRead is false. The type will skip the converter.");
            }

            public override bool CanRead
            {
                get { return false; }
            }

            public override bool CanConvert(Type objectType)
            {
                return objectType == typeof(decimal);
            }
        }
    }
    #endregion
    #region Profile Section
    public class Profile_Sections
    {
        public enum Sections
        {
            Missions,
            PlayNexts,
            RecentlyPlayedVenues,
            Chords,
            Lessons,
            NSP_PlayListRoot2,
            Options,
            Guitarcade,
            SongLists,
            FavoritesList,
            DD,
            Prizes,
            LearnASong,
            SessionMode,
            Achievements,
            Stats,
            ScoreAttack
        }

        public static Mission Loaded_Missions;
        public static Playnexts Loaded_Playnexts;
        public static RecentlyPlayedVenues Loaded_RecentlyPlayedVenues;
        public static Chords Loaded_Chords;
        public static NSP_PlayListRoot2 Loaded_NSPPlaylistRoot2;
        public static Options Loaded_Options;
        public static Guitarcade Loaded_Guitarcade;
        public static SongListsRoot Loaded_Songlists;
        public static FavoritesListRoot Loaded_FavoritesList;
        public static DynamicDifficulty Loaded_DynamicDifficulty;
        public static Prizes Loaded_Prizes;
        public static Songs Loaded_LearnASong;
        public static SessionMode Loaded_SessionMode;
        public static Achievements Loaded_Achievements;
        public static Stats Loaded_Stats;
        public static SongsSA Loaded_ScoreAttack;


        public static void LoadProfileSections(string profilePath, ProgressBar progressBar = null)
        {
            bool progressBarActive = progressBar != null;

            if (progressBarActive)
            {
                progressBar.Visible = true;
                progressBar.Value = progressBar.Minimum;
                progressBar.Step = ((progressBar.Maximum - progressBar.Minimum) / 16);
            }

            Loaded_Missions = Profiles.JSON_ToObject<Mission>(profilePath, Sections.Missions);
            if (progressBarActive)
                progressBar.PerformStep();
            Loaded_Playnexts = Profiles.JSON_ToObject<Playnexts>(profilePath, Sections.PlayNexts);
            if (progressBarActive)
                progressBar.PerformStep();
            Loaded_RecentlyPlayedVenues = Profiles.JSON_ToObject<RecentlyPlayedVenues>(profilePath, Sections.RecentlyPlayedVenues);
            if (progressBarActive)
                progressBar.PerformStep();
            Loaded_Chords = Profiles.JSON_ToObject<Chords>(profilePath, Sections.Chords);
            if (progressBarActive)
                progressBar.PerformStep();
            Loaded_NSPPlaylistRoot2 = Profiles.JSON_ToObject<NSP_PlayListRoot2>(profilePath, Sections.NSP_PlayListRoot2);
            if (progressBarActive)
                progressBar.PerformStep();
            Loaded_Options = Profiles.JSON_ToObject<Options>(profilePath, Sections.Options);
            if (progressBarActive)
                progressBar.PerformStep();
            Loaded_Guitarcade = Profiles.JSON_ToObject<Guitarcade>(profilePath, Sections.Guitarcade);
            if (progressBarActive)
                progressBar.PerformStep();
            Loaded_Songlists = Profiles.JSON_ToObject<SongListsRoot>(profilePath, Sections.SongLists);
            if (progressBarActive)
                progressBar.PerformStep();
            Loaded_FavoritesList = Profiles.JSON_ToObject<FavoritesListRoot>(profilePath, Sections.FavoritesList);
            if (progressBarActive)
                progressBar.PerformStep();
            Loaded_DynamicDifficulty = Profiles.JSON_ToObject<DynamicDifficulty>(profilePath, Sections.DD);
            if (progressBarActive)
                progressBar.PerformStep();
            Loaded_Prizes = Profiles.JSON_ToObject<Prizes>(profilePath, Sections.Prizes);
            if (progressBarActive)
                progressBar.PerformStep();
            Loaded_LearnASong = Profiles.JSON_ToObject<Songs>(profilePath, Sections.LearnASong);
            if (progressBarActive)
                progressBar.PerformStep();
            Loaded_SessionMode = Profiles.JSON_ToObject<SessionMode>(profilePath, Sections.SessionMode);
            if (progressBarActive)
                progressBar.PerformStep();
            Loaded_Achievements = Profiles.JSON_ToObject<Achievements>(profilePath, Sections.Achievements);
            if (progressBarActive)
                progressBar.PerformStep();
            Loaded_Stats = Profiles.JSON_ToObject<Stats>(profilePath, Sections.Stats);
            if (progressBarActive)
                progressBar.PerformStep();
            Loaded_ScoreAttack = Profiles.JSON_ToObject<SongsSA>(profilePath, Sections.ScoreAttack);
            if (progressBarActive)
                progressBar.Visible = false;
        }

        /// <summary>
        /// UNKNOWN STATUS = Need more data to see what is in these.
        /// COMPLETE = All work that is needed, has been done.
        /// NOT COMPLETE = Needs a lot more data
        /// needs organization = Sort By Number
        /// </summary>


        #region Missions | COMPLETE?

        // This section is ever-growing.
        public class Mission
        {
            public bool SongMissionIntroShown_15 { get; set; }
            public bool SongMissionIntroShown_14 { get; set; }
            public bool SongMissionIntroShown_12 { get; set; }
            public bool SongMissionIntroShown_11 { get; set; }
            public bool SongMissionIntroShown_18 { get; set; }
            public bool SongMissionIntroShown_21 { get; set; }
            public double Slot2GuitarCade { get; set; }
            public double ActiveMissType_2 { get; set; }
            public double ActiveMissType_0 { get; set; }
            public double ActiveMissType_1 { get; set; }
            public double Slot0Lesson { get; set; }
            public bool LessonMissionOutroShown_0 { get; set; }
            public bool LessonMissionOutroShown_1 { get; set; }
            public bool LessonMissionOutroShown_3 { get; set; }
            public bool LessonMissionOutroShown_4 { get; set; }
            public bool LessonMissionOutroShown_5 { get; set; }
            public bool LessonMissionOutroShown_6 { get; set; }
            public bool LessonMissionOutroShown_8 { get; set; }
            public bool LessonMissionOutroShown_9 { get; set; }
            public double Slot2Lesson { get; set; }
            public double Slot1GuitarCade { get; set; }
            public double ActiveMissSubType_2 { get; set; }
            public double ActiveMissSubType_0 { get; set; }
            public double ActiveMissSubType_1 { get; set; }
            public bool SongMissionOutroShown_9 { get; set; }
            public bool SongMissionOutroShown_8 { get; set; }
            public bool SongMissionOutroShown_1 { get; set; }
            public bool SongMissionOutroShown_0 { get; set; }
            public bool SongMissionOutroShown_3 { get; set; }
            public bool SongMissionOutroShown_5 { get; set; }
            public bool SongMissionOutroShown_4 { get; set; }
            public bool SongMissionOutroShown_7 { get; set; }
            public bool SongMissionOutroShown_6 { get; set; }
            public double Slot1Session { get; set; }
            public double Slot0GuitarCade { get; set; }
            public double Slot1Lesson { get; set; }
            public double NumCompToneStickMiss_7 { get; set; }
            public double NumCompToneStickMiss_4 { get; set; }
            public double NumCompToneStickMiss_3 { get; set; }
            public double NumCompToneStickMiss_2 { get; set; }
            public double NumCompToneStickMiss_1 { get; set; }
            public double NumCompToneStickMiss_0 { get; set; }
            public bool SessionMissionIntroShown_3 { get; set; }
            public bool SessionMissionIntroShown_2 { get; set; }
            public bool SessionMissionIntroShown_1 { get; set; }
            public double NumCompLessonMiss_9 { get; set; }
            public double NumCompLessonMiss_8 { get; set; }
            public double NumCompLessonMiss_3 { get; set; }
            public double NumCompLessonMiss_1 { get; set; }
            public double NumCompLessonMiss_0 { get; set; }
            public double NumCompLessonMiss_6 { get; set; }
            public double NumCompLessonMiss_5 { get; set; }
            public double NumCompLessonMiss_4 { get; set; }
            public double Slot0Current { get; set; }
            public bool LessonMissionOutroShown_13 { get; set; }
            public bool LessonMissionOutroShown_12 { get; set; }
            public bool LessonMissionOutroShown_11 { get; set; }
            public bool LessonMissionOutroShown_10 { get; set; }
            public bool LessonMissionOutroShown_16 { get; set; }
            public bool LessonMissionOutroShown_14 { get; set; }
            public bool LessonMissionOutroShown_19 { get; set; }
            public double Slot0Session { get; set; }
            public double Slot0Level { get; set; }
            public bool SongMissionIntroShown_9 { get; set; }
            public bool SongMissionIntroShown_4 { get; set; }
            public bool SongMissionIntroShown_5 { get; set; }
            public bool SongMissionIntroShown_6 { get; set; }
            public bool SongMissionIntroShown_7 { get; set; }
            public bool SongMissionIntroShown_2 { get; set; }
            public bool SongMissionIntroShown_3 { get; set; }
            public double Slot2Song { get; set; }
            public bool TSMissionIntroShown_4 { get; set; }
            public bool TSMissionIntroShown_5 { get; set; }
            public bool TSMissionIntroShown_1 { get; set; }
            public bool TSMissionIntroShown_2 { get; set; }
            public double Slot2ToneStick { get; set; }
            public double Slot0Song { get; set; }
            public double NumCompLessonMiss_19 { get; set; }
            public double NumCompLessonMiss_16 { get; set; }
            public double NumCompLessonMiss_14 { get; set; }
            public double NumCompLessonMiss_12 { get; set; }
            public double NumCompLessonMiss_13 { get; set; }
            public double NumCompLessonMiss_10 { get; set; }
            public double NumCompLessonMiss_11 { get; set; }
            public double Slot1Song { get; set; }
            public double NumCompSongMiss_0 { get; set; }
            public double NumCompSongMiss_1 { get; set; }
            public double NumCompSongMiss_3 { get; set; }
            public double NumCompSongMiss_4 { get; set; }
            public double NumCompSongMiss_5 { get; set; }
            public double NumCompSongMiss_6 { get; set; }
            public double NumCompSongMiss_7 { get; set; }
            public double NumCompSongMiss_8 { get; set; }
            public double NumCompSongMiss_9 { get; set; }
            public double Slot1Current { get; set; }
            public double NumCompSongMiss_13 { get; set; }
            public double NumCompSongMiss_12 { get; set; }
            public double NumCompSongMiss_11 { get; set; }
            public double NumCompSongMiss_10 { get; set; }
            public double NumCompSongMiss_17 { get; set; }
            public double NumCompSongMiss_16 { get; set; }
            public double NumCompSongMiss_15 { get; set; }
            public double NumCompSongMiss_14 { get; set; }
            public double NumCompSongMiss_19 { get; set; }
            public double NumCompSongMiss_18 { get; set; }
            public double NumCompSongMiss_26 { get; set; }
            public double NumCompSongMiss_27 { get; set; }
            public double NumCompSongMiss_24 { get; set; }
            public double NumCompSongMiss_25 { get; set; }
            public double NumCompSongMiss_22 { get; set; }
            public double NumCompSongMiss_20 { get; set; }
            public double NumCompSongMiss_28 { get; set; }
            public double Slot1Level { get; set; }
            public bool LessonMissionIntroShown_11 { get; set; }
            public bool LessonMissionIntroShown_17 { get; set; }
            public bool SongMissionOutroShown_18 { get; set; }
            public bool SongMissionOutroShown_19 { get; set; }
            public bool SongMissionOutroShown_10 { get; set; }
            public bool SongMissionOutroShown_11 { get; set; }
            public bool SongMissionOutroShown_12 { get; set; }
            public bool SongMissionOutroShown_13 { get; set; }
            public bool SongMissionOutroShown_14 { get; set; }
            public bool SongMissionOutroShown_15 { get; set; }
            public bool SongMissionOutroShown_16 { get; set; }
            public bool SongMissionOutroShown_17 { get; set; }
            public bool SongMissionOutroShown_20 { get; set; }
            public bool SongMissionOutroShown_22 { get; set; }
            public bool SongMissionOutroShown_25 { get; set; }
            public bool SongMissionOutroShown_24 { get; set; }
            public bool SongMissionOutroShown_27 { get; set; }
            public bool SongMissionOutroShown_26 { get; set; }
            public bool SongMissionOutroShown_28 { get; set; }
            public double Slot0ToneStick { get; set; }
            public double NumCompSessionMiss_2 { get; set; }
            public double NumCompSessionMiss_1 { get; set; }
            public double NumCompSessionMiss_0 { get; set; }
            public bool SessionMissionOutroShown_2 { get; set; }
            public bool SessionMissionOutroShown_0 { get; set; }
            public bool SessionMissionOutroShown_1 { get; set; }
            public double Slot2Session { get; set; }
            public double Slot2Current { get; set; }
            public double Slot1ToneStick { get; set; }
            public double Slot2Level { get; set; }
            public bool TSMissionOutroShown_1 { get; set; }
            public bool TSMissionOutroShown_0 { get; set; }
            public bool TSMissionOutroShown_3 { get; set; }
            public bool TSMissionOutroShown_2 { get; set; }
            public bool TSMissionOutroShown_4 { get; set; }
            public bool TSMissionOutroShown_7 { get; set; }
        }
        #endregion
        #region Play Nexts | UNKNOWN STATUS
        public class Playnexts
        {
            public Histories Histories { get; set; }
            public Dictionary<string, PlayNexts_Song> Songs { get; set; }
            public double Version { get; set; }
        }

        public class Histories
        {
            public List<double> SongChallenges { get; set; }
            public List<double> Lessons { get; set; }
            public List<double> RiffRepeaters { get; set; }
            public List<double> Guitarcades { get; set; }
            public List<double> ScoreAttacks { get; set; }
        }

        public class Slot
        {
            public double BuildKey { get; set; }
            public double Record { get; set; }
            public double Data { get; set; }
            public double Status { get; set; }
        }

        public class PlayNexts_Song
        {
            public double TimeStamp { get; set; }

            public Slot Slot3 { get; set; }

            public Slot Slot2 { get; set; }

            public Slot Slot1 { get; set; }
        }
        #endregion
        #region Recently Played Venues | COMPLETE
        public class RecentlyPlayedVenues
        {
            public List<RecentlyPlayedVenues_Entry> Entries { get; set; }
        }

        public class RecentlyPlayedVenues_Entry
        {
            public string VenueBlockName { get; set; }
        }

        #endregion
        public class UI { }
        public class PlayListRoot2 { }

        #region Chords | COMPLETE

        // Yeah no, this section grows with new chords so let's not touch this.
        public class Chords : Dictionary<string, object> { }
        #endregion
        #region Lessons | UNKNOWN STATUS (needs organization)
        public class GE
        {
            public double GE_LTapping_Lsn37_Started { get; set; }
            public double GE_LPinchHarmonics_Lsn30_Stopped { get; set; }
            public double GE_LShifting1_Lsn31_Started { get; set; }
            public double GE_BPicking_Lsn61_Started { get; set; }
            public double GE_RChords2_Lsn8_Started { get; set; }
            public double GE_RChords1_Lsn7_Stopped { get; set; }
            public double GE_LPicking_Lsn29_Completed { get; set; }
            public double GE_LSustain_Lsn35_Started { get; set; }
            public double GE_BPicking_Lsn61_Completed { get; set; }
            public double GE_RChords6_Lsn12_Stopped { get; set; }
            public double GE_LTuningTips_Lsn48_Started { get; set; }
            public double GE_RChords6_Lsn12_Started { get; set; }
            public double GE_LSlides1_Lsn33_Completed { get; set; }
            public double GE_LCapo_Lsn49_Completed { get; set; }
            public double GE_LSustain_Lsn35_Stopped { get; set; }
            public double GE_LCapo_Lsn49_Started { get; set; }
            public double GE_BPicking_Lsn61_Stopped { get; set; }
            public double GE_LTuningTips_Lsn48_Stopped { get; set; }
            public double GE_LShifting1_Lsn31_Stopped { get; set; }
            public bool DoneFirstExperience { get; set; }
            public double GE_LCapo_Lsn49_Stopped { get; set; }
            public double GE_BShifting1_Lsn64_Stopped { get; set; }
            public double GE_LBends1_Lsn3_Started { get; set; }
            public double GE_LBends1_Lsn3_Stopped { get; set; }
            public double GE_RChords6_Lsn12_Completed { get; set; }
            public double GE_LPicking_Lsn29_Started { get; set; }
            public double GE_LSlides1_Lsn33_Started { get; set; }
            public double GE_RChords1_Lsn7_Started { get; set; }
            public double GE_LHopo1_Lsn23_Started { get; set; }
            public double GE_BBasics1_Lsn53_Completed { get; set; }
            public double GE_BShifting1_Lsn64_Completed { get; set; }
            public double GE_LSlides1_Lsn33_Stopped { get; set; }
            public double GE_LPinchHarmonics_Lsn30_Started { get; set; }
            public double GE_FE_beginner_bass_Lsn0_Completed { get; set; }
            public double GE_BBasics1_Lsn53_Started { get; set; }
            public double GE_BBasics1_Lsn53_Stopped { get; set; }
            public double GE_RChords2_Lsn8_Stopped { get; set; }
            public double GE_LDoubleStops2_Lsn19_Stopped { get; set; }
            public double GE_LHarmonics_Lsn22_Started { get; set; }
            public double GE_LDoubleStops2_Lsn19_Started { get; set; }
            public double GE_FE_beginner_bass_Lsn0_Stopped { get; set; }
            public double GE_LHarmonics_Lsn22_Stopped { get; set; }
            public double GE_LHopo1_Lsn23_Stopped { get; set; }
            public double GE_FE_beginner_bass_Lsn0_Started { get; set; }
            public double GE_LShifting1_Lsn31_Completed { get; set; }
            public double GE_LPinchHarmonics_Lsn30_Completed { get; set; }
            public double GE_BShifting1_Lsn64_Started { get; set; }
            public double GE_LTapping_Lsn37_Stopped { get; set; }
            public double GE_LPicking_Lsn29_Stopped { get; set; }
        }
        #endregion
        #region Nonstop Play | UNKNOWN STATUS
        public class NSP_PlayListRoot2
        {
            public double NSP_PlayListArrangements { get; set; }
            public double NSP_PlayListTuning { get; set; }
            public double NSP_PlayListSongList { get; set; }
            public double NSP_PlayListPlayOrder { get; set; }
            public double Duration { get; set; }
        }
        #endregion
        #region Options | UNKNOWN STATUS
        public class Tuning
        {
            public double CapoFret { get; set; }
            public double CentOffset { get; set; }
            public double string2 { get; set; }
            public double string3 { get; set; }
            public double string0 { get; set; }
            public double string1 { get; set; }
            public double string4 { get; set; }
            public double string5 { get; set; }

        }

        public class Options
        {
            public double NSPSongList { get; set; }
            public bool YoYouWantFriesWithThat { get; set; }
            public bool VoiceCommands { get; set; }
            public double MicrophoneInput { get; set; }
            public Tuning TuningBassEmulation { get; set; }
            public double GuitarInput { get; set; }
            public double SoundEffectsVolume { get; set; }
            public bool DisableBrowser { get; set; }
            public bool SubtitleEnabled { get; set; }
            public double ToleranceLevel { get; set; }
            public double DDLevelDownSpeed { get; set; }
            public bool AlternateTuningSets { get; set; }
            public double SASongList { get; set; }
            public bool MicrophoneEnabled { get; set; }
            public double DDSightreadLevel { get; set; }
            public string ScreenResolution { get; set; }
            public string VisualQuality { get; set; }
            public bool EnableNarnia { get; set; }
            public double BassVolume { get; set; }
            public bool EnableHighResScope { get; set; }
            public string DirectConnectDefaultInput { get; set; }
            public bool MasterMode { get; set; }
            public string ColorBlindMode { get; set; }
            public bool FinishedFE { get; set; }
            public bool HasAlreadySeenDisconnectedSetup { get; set; }
            public string SortSongList { get; set; }
            public bool FlipVideos { get; set; }
            public bool UseDefaultAuthenthicTone { get; set; }
            public string GuitarBassKey { get; set; }
            public bool HasDisplayedMPInputModeDialog { get; set; }
            public bool EnableDepthOfField { get; set; }
            public bool FingerNumbering { get; set; }
            public string GuitarKey { get; set; }
            public string GuitarTypeMP { get; set; }
            public double AcceleratorRepetition { get; set; }
            public double MasterModeStyle { get; set; }
            public bool EnablePostEffects { get; set; }
            public Tuning TuningBass { get; set; }
            public double AudioLatency { get; set; }
            public bool ShouldDisplayCalibrationGreeting { get; set; }
            public double ReverbVolume { get; set; }
            public double MusicVolume { get; set; }
            public bool GuitarSimulated { get; set; }
            public double InputVolumeCalibrationBass { get; set; }
            public double ExperienceLevel { get; set; }
            public double LevellerRepetition { get; set; }
            public double InputMode { get; set; }
            public bool NormalizedInputMicGain_BassOverride { get; set; }
            public bool RewindAnimationEnabled { get; set; }
            public string ScreenMode { get; set; }
            public double GuitarVolumePlayer2 { get; set; }
            public bool InvertGuitarStrings { get; set; }
            public double VisualLagCalibration { get; set; }
            public double InputVolumeCalibration { get; set; }
            public double ArrangementSet { get; set; }
            public double NormalizedInputMicGain { get; set; }
            public string SortLessonList { get; set; }
            public double VoiceoverVolume { get; set; }
            public bool PlayedRS1 { get; set; }
            public bool UnpluggedAllowed { get; set; }
            public bool AutoContinue { get; set; }
            public bool Representative { get; set; }
            public bool AutoSaveEnabled { get; set; }
            public double Brightness { get; set; }
            public bool LeftHandGuitar { get; set; }
            public Tuning TuningStandard { get; set; }
            public double GuitarVolume { get; set; }
            public bool MsaaSetting { get; set; }
            public double ScreenPositionX { get; set; }
            public double ScreenPositionY { get; set; }
            public bool IsExperienced { get; set; }
            public double NormalizedInputMicGain_Bass { get; set; }
            public bool HasAlreadyUsedMicrophoneMode { get; set; }
            public bool DDOverrideMax { get; set; }
            public double AcceleratorSpeedIncrement { get; set; }
            public double NoiseFloor { get; set; }
            public bool EnableGuitarNavigation { get; set; }
            public double Route { get; set; }
            public bool NoiseCancellingEnabled { get; set; }
            public double BassVolumePlayer2 { get; set; }
            public string FULLSCREEN { get; set; }
            public double DDLevelUpSpeed { get; set; }
            public bool AcceleratorSpeedUp { get; set; }
            public bool BonusArr { get; set; }
            public bool AudioExclusivity { get; set; }
            public double TIMEOFLAST_WEBSITEVISIT1 { get; set; }
            public double TIMEOFLAST_WEBSITEVISIT2 { get; set; }
            public double AuthenticToneSlot { get; set; }
            public double MasterVolume { get; set; }
            public bool DDEnabled { get; set; }
            public double MicrophoneVolume { get; set; }
            public double NoiseFloorBass { get; set; }
            public string Language { get; set; }
            public bool HasAlreadyUsedDirectConnectMode { get; set; }
            public double Version { get; set; }
            public double FilterSongList { get; set; }
            public string BassStyle { get; set; }
            public bool HasAlreadyUsedRealToneMode { get; set; }
            public bool ViewMistakes { get; set; }
            public string GuitarClass { get; set; }
            public bool LevellerLevelUp { get; set; }
        }
        #endregion
        #region Guitarcade | UNKNOWN STATUS

        public class GC_Instrument
        {
            public double LastTimePlayed { get; set; }
            public double HighScore { get; set; }
            public GC_ActiveChallengesCumulative ActiveChallengesCumulative { get; set; }
            public Dictionary<string, double> ActiveChallenges { get; set; }
            public Dictionary<string, double> ActiveChallengesRandomIndex { get; set; }
            public Dictionary<string, double> CompletedChallenges { get; set; }
            public double PlayCount { get; set; }
            public double SkillLevel { get; set; }
        }

        public class GC_ActiveChallengesCumulative
        {
            [JsonProperty("2")]
            public object _2 { get; set; }

            [JsonProperty("1")]
            public object _1 { get; set; }

            [JsonProperty("0")]
            public object _0 { get; set; }
        }

        public class GC_LevelBasedGames
        {
            public double CurrentLevel { get; set; }
            public double LevelsUnlockedChapterScreen { get; set; }
            public double LevelsUnlocked { get; set; }
        }

        public class GC_TrackAndField
        {
            public GC_Instrument Guitar { get; set; }
            public GC_Instrument Bass { get; set; }
        }

        public class GC_NinjaSlides
        {
            public GC_Instrument Guitar { get; set; }
            public GC_Instrument Bass { get; set; }
        }

        public class GC_Chordead
        {
            public GC_LevelBasedGames Guitar { get; set; }
            public GC_LevelBasedGames Bass { get; set; }
        }

        public class GC_ScaleWarriors
        {
            public GC_LevelBasedGames Guitar { get; set; }
            public GC_LevelBasedGames Bass { get; set; }
        }

        public class GC_DucksPlus
        {
            public GC_Instrument Guitar { get; set; }
            public GC_Instrument Bass { get; set; }
        }

        public class GC_TempleOfBends
        {
            public GC_Instrument Guitar { get; set; }
            public GC_Instrument Bass { get; set; }
        }

        public class GC_ScaleRacer
        {
            public GC_Instrument Guitar { get; set; }
            public GC_Instrument Bass { get; set; }
        }

        public class GC_StarChords
        {
            public GC_Instrument Guitar { get; set; }
            public GC_Instrument Bass { get; set; }
        }

        public class GC_HarmonicHeist
        {
            public GC_Instrument Guitar { get; set; }
            public GC_Instrument Bass { get; set; }
        }

        public class GC_WhaleRider
        {
            public GC_Instrument Guitar { get; set; }
            public GC_Instrument Bass { get; set; }
        }

        public class GC_StringSkipSaloon
        {
            public GC_Instrument Guitar { get; set; }
            public GC_Instrument Bass { get; set; }
        }

        public class GuitarcadeGames
        {
            public GC_TrackAndField GC_TrackAndField { get; set; }
            public GC_NinjaSlides GC_NinjaSlides { get; set; }
            [JsonProperty("$[32925]")]
            public GC_Chordead _32925 { get; set; }
            [JsonProperty("$[32926]")]
            public GC_ScaleWarriors _32926 { get; set; }
            public GC_DucksPlus GC_DucksPlus { get; set; }
            public GC_TempleOfBends GC_TempleOfBends { get; set; }
            public GC_ScaleRacer GC_ScaleRacer { get; set; }
            public GC_StarChords GC_StarChords { get; set; }
            public GC_HarmonicHeist GC_HarmonicHeist { get; set; }
            public GC_WhaleRider GC_WhaleRider { get; set; }
            public GC_StringSkipSaloon GC_StringSkipSaloon { get; set; }
        }

        public class Guitarcade
        {
            public GuitarcadeGames GuitarcadeGames { get; set; }
            public List<object> Stats { get; set; }
            public double Version { get; set; }
        }

        #endregion
        public class PlayedDLCPreviewSongIDs { }

        public class SongListsRoot
        {
            public List<List<string>> SongLists { get; set; }
        }

        public class FavoritesListRoot
        {
            public List<string> FavoritesList { get; set; }
        }

        public class EarnedRocksmithPointsKeys { };

        #region Dynamic Difficulty | UNKNOWN STATUS
        public class DynamicDifficulty
        {
            public DD_Path Combo { get; set; }
            public DD_Path Lead { get; set; }
            public DD_Path Rhythm { get; set; }
            public double SightReadingLevel { get; set; }
            public double SightReadingExperience { get; set; }
            public DD_Path Bass { get; set; }
        }

        public class DD_Path
        {
            public List<double> LevelUpThreshold { get; set; }
        }

        #endregion

        public class Venues { }

        #region Prizes | COMPLETE
        public class Prizes
        {
            // Points
            public double UnawardedPrizePoints { get; set; }
            public double NumPrizePoints { get; set; }

            // Uplay

            public UPlayRewards UPlayRewards { get; set; }

            public bool UplayAction1 { get; set; }
            public bool UplayAction2 { get; set; }
            public bool UplayAction3 { get; set; }
            public bool UplayAction4 { get; set; }
            public bool UplayDlg1 { get; set; }
            public bool UplayDlg2 { get; set; }
            public bool UplayDlg3 { get; set; }
            public bool UplayDlg4 { get; set; }


            // Have we awarded the prize (only exist if achieved).

            public bool IsPrizeAwarded_1 { get; set; }
            public bool IsPrizeAwarded_2 { get; set; }
            public bool IsPrizeAwarded_3 { get; set; }
            public bool IsPrizeAwarded_4 { get; set; }
            public bool IsPrizeAwarded_5 { get; set; }
            public bool IsPrizeAwarded_6 { get; set; }
            // Prize Gap
            public bool IsPrizeAwarded_8 { get; set; }
            public bool IsPrizeAwarded_9 { get; set; }
            public bool IsPrizeAwarded_10 { get; set; }
            public bool IsPrizeAwarded_11 { get; set; }
            public bool IsPrizeAwarded_12 { get; set; }
            public bool IsPrizeAwarded_13 { get; set; }
            // Prize Gap
            public bool IsPrizeAwarded_15 { get; set; }
            public bool IsPrizeAwarded_16 { get; set; }
            public bool IsPrizeAwarded_17 { get; set; }
            public bool IsPrizeAwarded_18 { get; set; }
            public bool IsPrizeAwarded_19 { get; set; }
            public bool IsPrizeAwarded_20 { get; set; }
            public bool IsPrizeAwarded_21 { get; set; }
            // Prize Gap
            public bool IsPrizeAwarded_23 { get; set; }
            public bool IsPrizeAwarded_24 { get; set; }
            public bool IsPrizeAwarded_25 { get; set; }
            public bool IsPrizeAwarded_26 { get; set; }
            public bool IsPrizeAwarded_27 { get; set; }
            public bool IsPrizeAwarded_28 { get; set; }
            // Prize Gap
            public bool IsPrizeAwarded_30 { get; set; }
            public bool IsPrizeAwarded_31 { get; set; }
            public bool IsPrizeAwarded_32 { get; set; }
            public bool IsPrizeAwarded_33 { get; set; }
            public bool IsPrizeAwarded_34 { get; set; }
            public bool IsPrizeAwarded_35 { get; set; }
            // Prize Gap
            public bool IsPrizeAwarded_37 { get; set; }
            public bool IsPrizeAwarded_38 { get; set; }
            public bool IsPrizeAwarded_39 { get; set; }
            public bool IsPrizeAwarded_40 { get; set; }
            public bool IsPrizeAwarded_41 { get; set; }
            public bool IsPrizeAwarded_42 { get; set; }
            public bool IsPrizeAwarded_43 { get; set; }
            public bool IsPrizeAwarded_44 { get; set; }
            // Prize Gap
            public bool IsPrizeAwarded_46 { get; set; }
            public bool IsPrizeAwarded_47 { get; set; }
            public bool IsPrizeAwarded_48 { get; set; }
            public bool IsPrizeAwarded_49 { get; set; }
            public bool IsPrizeAwarded_50 { get; set; }
            // Prize Gap
            public bool IsPrizeAwarded_52 { get; set; }
            public bool IsPrizeAwarded_53 { get; set; }
            public bool IsPrizeAwarded_54 { get; set; }
            public bool IsPrizeAwarded_55 { get; set; }
            public bool IsPrizeAwarded_56 { get; set; }
            public bool IsPrizeAwarded_57 { get; set; }
            public bool IsPrizeAwarded_58 { get; set; }
            public bool IsPrizeAwarded_59 { get; set; }
            // Prize Gap
            public bool IsPrizeAwarded_61 { get; set; }
            public bool IsPrizeAwarded_62 { get; set; }
            public bool IsPrizeAwarded_63 { get; set; }
            public bool IsPrizeAwarded_64 { get; set; }
            public bool IsPrizeAwarded_65 { get; set; }
            public bool IsPrizeAwarded_66 { get; set; }
            public bool IsPrizeAwarded_67 { get; set; }
            // Prize Gap
            public bool IsPrizeAwarded_69 { get; set; }
            public bool IsPrizeAwarded_70 { get; set; }
            public bool IsPrizeAwarded_71 { get; set; }
            public bool IsPrizeAwarded_72 { get; set; }
            public bool IsPrizeAwarded_73 { get; set; }
            public bool IsPrizeAwarded_74 { get; set; }
            public bool IsPrizeAwarded_75 { get; set; }
            public bool IsPrizeAwarded_76 { get; set; }
            // Prize Gap
            public bool IsPrizeAwarded_78 { get; set; }
            public bool IsPrizeAwarded_79 { get; set; }
            public bool IsPrizeAwarded_80 { get; set; }
            public bool IsPrizeAwarded_81 { get; set; }
            public bool IsPrizeAwarded_82 { get; set; }
            // Prize Gap
            public bool IsPrizeAwarded_84 { get; set; }
            public bool IsPrizeAwarded_85 { get; set; }
            public bool IsPrizeAwarded_86 { get; set; }
            public bool IsPrizeAwarded_87 { get; set; }
            public bool IsPrizeAwarded_88 { get; set; }
            public bool IsPrizeAwarded_89 { get; set; }
            public bool IsPrizeAwarded_90 { get; set; }
            public bool IsPrizeAwarded_91 { get; set; }
            public bool IsPrizeAwarded_92 { get; set; }
            // Prize Gap
            public bool IsPrizeAwarded_94 { get; set; }
            public bool IsPrizeAwarded_95 { get; set; }
            public bool IsPrizeAwarded_96 { get; set; }
            public bool IsPrizeAwarded_97 { get; set; }
            public bool IsPrizeAwarded_98 { get; set; }
            public bool IsPrizeAwarded_99 { get; set; }
            // Prize Gap
            public bool IsPrizeAwarded_101 { get; set; }
            public bool IsPrizeAwarded_102 { get; set; }
            public bool IsPrizeAwarded_103 { get; set; }
            public bool IsPrizeAwarded_104 { get; set; }
            public bool IsPrizeAwarded_105 { get; set; }
            public bool IsPrizeAwarded_106 { get; set; }
            public bool IsPrizeAwarded_107 { get; set; }
            public bool IsPrizeAwarded_108 { get; set; }
            // Prize Gap
            public bool IsPrizeAwarded_110 { get; set; }
            public bool IsPrizeAwarded_111 { get; set; }
            public bool IsPrizeAwarded_112 { get; set; }
            public bool IsPrizeAwarded_113 { get; set; }
            public bool IsPrizeAwarded_114 { get; set; }
            public bool IsPrizeAwarded_115 { get; set; }
            public bool IsPrizeAwarded_116 { get; set; }
            // Prize Gap
            public bool IsPrizeAwarded_118 { get; set; }
            public bool IsPrizeAwarded_119 { get; set; }
            public bool IsPrizeAwarded_120 { get; set; }
            public bool IsPrizeAwarded_121 { get; set; }
            public bool IsPrizeAwarded_122 { get; set; }
            public bool IsPrizeAwarded_123 { get; set; }
            public bool IsPrizeAwarded_124 { get; set; }
            // Prize Gap
            public bool IsPrizeAwarded_126 { get; set; }
            public bool IsPrizeAwarded_127 { get; set; }
            public bool IsPrizeAwarded_128 { get; set; }
            public bool IsPrizeAwarded_129 { get; set; }
            public bool IsPrizeAwarded_130 { get; set; }
            // Prize Gap
            public bool IsPrizeAwarded_132 { get; set; }
            public bool IsPrizeAwarded_133 { get; set; }
            public bool IsPrizeAwarded_134 { get; set; }
            public bool IsPrizeAwarded_135 { get; set; }
            public bool IsPrizeAwarded_136 { get; set; }
            public bool IsPrizeAwarded_137 { get; set; }
            public bool IsPrizeAwarded_138 { get; set; }
            public bool IsPrizeAwarded_139 { get; set; }
            public bool IsPrizeAwarded_140 { get; set; }
            // Prize Gap
            public bool IsPrizeAwarded_142 { get; set; }
            public bool IsPrizeAwarded_143 { get; set; }
            public bool IsPrizeAwarded_144 { get; set; }
            public bool IsPrizeAwarded_145 { get; set; }
            public bool IsPrizeAwarded_146 { get; set; }
            public bool IsPrizeAwarded_147 { get; set; }
            // Prize Gap
            public bool IsPrizeAwarded_149 { get; set; }
            public bool IsPrizeAwarded_150 { get; set; }
            public bool IsPrizeAwarded_151 { get; set; }
            public bool IsPrizeAwarded_152 { get; set; }
            public bool IsPrizeAwarded_153 { get; set; }
            // Prize Gap
            // Prize Gap
            public bool IsPrizeAwarded_156 { get; set; }
            public bool IsPrizeAwarded_157 { get; set; }
            public bool IsPrizeAwarded_158 { get; set; }
            public bool IsPrizeAwarded_159 { get; set; }
            public bool IsPrizeAwarded_160 { get; set; }
            public bool IsPrizeAwarded_161 { get; set; }
            public bool IsPrizeAwarded_162 { get; set; }
            public bool IsPrizeAwarded_163 { get; set; }
            // Prize Gap
            public bool IsPrizeAwarded_165 { get; set; }
            public bool IsPrizeAwarded_166 { get; set; }
            public bool IsPrizeAwarded_167 { get; set; }
            public bool IsPrizeAwarded_168 { get; set; }
            // Prize Gap
            public bool IsPrizeAwarded_170 { get; set; }
            public bool IsPrizeAwarded_171 { get; set; }
            public bool IsPrizeAwarded_172 { get; set; }
            public bool IsPrizeAwarded_173 { get; set; }
            public bool IsPrizeAwarded_174 { get; set; }
            public bool IsPrizeAwarded_175 { get; set; }
            public bool IsPrizeAwarded_176 { get; set; }
            public bool IsPrizeAwarded_177 { get; set; }
            public bool IsPrizeAwarded_178 { get; set; }
            public bool IsPrizeAwarded_179 { get; set; }
            public bool IsPrizeAwarded_180 { get; set; }
            public bool IsPrizeAwarded_181 { get; set; }
            public bool IsPrizeAwarded_182 { get; set; }
            public bool IsPrizeAwarded_183 { get; set; }
            // Prize Gap
            public bool IsPrizeAwarded_185 { get; set; }
            public bool IsPrizeAwarded_186 { get; set; }
            

            // Have we shown the user the prize they got (only exist if achieved).

            public bool HasPrizeDialogShown_1 { get; set; }
            public bool HasPrizeDialogShown_2 { get; set; }
            public bool HasPrizeDialogShown_3 { get; set; }
            public bool HasPrizeDialogShown_4 { get; set; }
            public bool HasPrizeDialogShown_5 { get; set; }
            public bool HasPrizeDialogShown_6 { get; set; }
            // Prize Gap
            public bool HasPrizeDialogShown_8 { get; set; }
            public bool HasPrizeDialogShown_9 { get; set; }
            public bool HasPrizeDialogShown_10 { get; set; }
            public bool HasPrizeDialogShown_11 { get; set; }
            public bool HasPrizeDialogShown_12 { get; set; }
            public bool HasPrizeDialogShown_13 { get; set; }
            // Prize Gap
            public bool HasPrizeDialogShown_15 { get; set; }
            public bool HasPrizeDialogShown_16 { get; set; }
            public bool HasPrizeDialogShown_17 { get; set; }
            public bool HasPrizeDialogShown_18 { get; set; }
            public bool HasPrizeDialogShown_19 { get; set; }
            public bool HasPrizeDialogShown_20 { get; set; }
            public bool HasPrizeDialogShown_21 { get; set; }
            // Prize Gap
            public bool HasPrizeDialogShown_23 { get; set; }
            public bool HasPrizeDialogShown_24 { get; set; }
            public bool HasPrizeDialogShown_25 { get; set; }
            public bool HasPrizeDialogShown_26 { get; set; }
            public bool HasPrizeDialogShown_27 { get; set; }
            public bool HasPrizeDialogShown_28 { get; set; }
            // Prize Gap
            public bool HasPrizeDialogShown_30 { get; set; }
            public bool HasPrizeDialogShown_31 { get; set; }
            public bool HasPrizeDialogShown_32 { get; set; }
            public bool HasPrizeDialogShown_33 { get; set; }
            public bool HasPrizeDialogShown_34 { get; set; }
            public bool HasPrizeDialogShown_35 { get; set; }
            // Prize Gap
            public bool HasPrizeDialogShown_37 { get; set; }
            public bool HasPrizeDialogShown_38 { get; set; }
            public bool HasPrizeDialogShown_39 { get; set; }
            public bool HasPrizeDialogShown_40 { get; set; }
            public bool HasPrizeDialogShown_41 { get; set; }
            public bool HasPrizeDialogShown_42 { get; set; }
            public bool HasPrizeDialogShown_43 { get; set; }
            public bool HasPrizeDialogShown_44 { get; set; }
            // Prize Gap
            public bool HasPrizeDialogShown_46 { get; set; }
            public bool HasPrizeDialogShown_47 { get; set; }
            public bool HasPrizeDialogShown_48 { get; set; }
            public bool HasPrizeDialogShown_49 { get; set; }
            public bool HasPrizeDialogShown_50 { get; set; }
            // Prize Gap
            public bool HasPrizeDialogShown_52 { get; set; }
            public bool HasPrizeDialogShown_53 { get; set; }
            public bool HasPrizeDialogShown_54 { get; set; }
            public bool HasPrizeDialogShown_55 { get; set; }
            public bool HasPrizeDialogShown_56 { get; set; }
            public bool HasPrizeDialogShown_57 { get; set; }
            public bool HasPrizeDialogShown_58 { get; set; }
            public bool HasPrizeDialogShown_59 { get; set; }
            // Prize Gap
            public bool HasPrizeDialogShown_61 { get; set; }
            public bool HasPrizeDialogShown_62 { get; set; }
            public bool HasPrizeDialogShown_63 { get; set; }
            public bool HasPrizeDialogShown_64 { get; set; }
            public bool HasPrizeDialogShown_65 { get; set; }
            public bool HasPrizeDialogShown_66 { get; set; }
            public bool HasPrizeDialogShown_67 { get; set; }
            // Prize Gap
            public bool HasPrizeDialogShown_69 { get; set; }
            public bool HasPrizeDialogShown_70 { get; set; }
            public bool HasPrizeDialogShown_71 { get; set; }
            public bool HasPrizeDialogShown_72 { get; set; }
            public bool HasPrizeDialogShown_73 { get; set; }
            public bool HasPrizeDialogShown_74 { get; set; }
            public bool HasPrizeDialogShown_75 { get; set; }
            public bool HasPrizeDialogShown_76 { get; set; }
            // Prize Gap
            public bool HasPrizeDialogShown_78 { get; set; }
            public bool HasPrizeDialogShown_79 { get; set; }
            public bool HasPrizeDialogShown_80 { get; set; }
            public bool HasPrizeDialogShown_81 { get; set; }
            public bool HasPrizeDialogShown_82 { get; set; }
            // Prize Gap
            public bool HasPrizeDialogShown_84 { get; set; }
            public bool HasPrizeDialogShown_85 { get; set; }
            public bool HasPrizeDialogShown_86 { get; set; }
            public bool HasPrizeDialogShown_87 { get; set; }
            public bool HasPrizeDialogShown_88 { get; set; }
            public bool HasPrizeDialogShown_89 { get; set; }
            public bool HasPrizeDialogShown_90 { get; set; }
            public bool HasPrizeDialogShown_91 { get; set; }
            public bool HasPrizeDialogShown_92 { get; set; }
            // Prize Gap
            public bool HasPrizeDialogShown_94 { get; set; }
            public bool HasPrizeDialogShown_95 { get; set; }
            public bool HasPrizeDialogShown_96 { get; set; }
            public bool HasPrizeDialogShown_97 { get; set; }
            public bool HasPrizeDialogShown_98 { get; set; }
            public bool HasPrizeDialogShown_99 { get; set; }
            // Prize Gap
            public bool HasPrizeDialogShown_101 { get; set; }
            public bool HasPrizeDialogShown_102 { get; set; }
            public bool HasPrizeDialogShown_103 { get; set; }
            public bool HasPrizeDialogShown_104 { get; set; }
            public bool HasPrizeDialogShown_105 { get; set; }
            public bool HasPrizeDialogShown_106 { get; set; }
            public bool HasPrizeDialogShown_107 { get; set; }
            public bool HasPrizeDialogShown_108 { get; set; }
            // Prize Gap
            public bool HasPrizeDialogShown_110 { get; set; }
            public bool HasPrizeDialogShown_111 { get; set; }
            public bool HasPrizeDialogShown_112 { get; set; }
            public bool HasPrizeDialogShown_113 { get; set; }
            public bool HasPrizeDialogShown_114 { get; set; }
            public bool HasPrizeDialogShown_115 { get; set; }
            public bool HasPrizeDialogShown_116 { get; set; }
            // Prize Gap
            public bool HasPrizeDialogShown_118 { get; set; }
            public bool HasPrizeDialogShown_119 { get; set; }
            public bool HasPrizeDialogShown_120 { get; set; }
            public bool HasPrizeDialogShown_121 { get; set; }
            public bool HasPrizeDialogShown_122 { get; set; }
            public bool HasPrizeDialogShown_123 { get; set; }
            public bool HasPrizeDialogShown_124 { get; set; }
            // Prize Gap
            public bool HasPrizeDialogShown_126 { get; set; }
            public bool HasPrizeDialogShown_127 { get; set; }
            public bool HasPrizeDialogShown_128 { get; set; }
            public bool HasPrizeDialogShown_129 { get; set; }
            public bool HasPrizeDialogShown_130 { get; set; }
            // Prize Gap
            public bool HasPrizeDialogShown_132 { get; set; }
            public bool HasPrizeDialogShown_133 { get; set; }
            public bool HasPrizeDialogShown_134 { get; set; }
            public bool HasPrizeDialogShown_135 { get; set; }
            public bool HasPrizeDialogShown_136 { get; set; }
            public bool HasPrizeDialogShown_137 { get; set; }
            public bool HasPrizeDialogShown_138 { get; set; }
            public bool HasPrizeDialogShown_139 { get; set; }
            public bool HasPrizeDialogShown_140 { get; set; }
            // Prize Gap
            public bool HasPrizeDialogShown_142 { get; set; }
            public bool HasPrizeDialogShown_143 { get; set; }
            public bool HasPrizeDialogShown_144 { get; set; }
            public bool HasPrizeDialogShown_145 { get; set; }
            public bool HasPrizeDialogShown_146 { get; set; }
            public bool HasPrizeDialogShown_147 { get; set; }
            // Prize Gap
            public bool HasPrizeDialogShown_149 { get; set; }
            public bool HasPrizeDialogShown_150 { get; set; }
            public bool HasPrizeDialogShown_151 { get; set; }
            public bool HasPrizeDialogShown_152 { get; set; }
            public bool HasPrizeDialogShown_153 { get; set; }
            // Prize Gap
            // Prize Gap
            public bool HasPrizeDialogShown_156 { get; set; }
            public bool HasPrizeDialogShown_157 { get; set; }
            public bool HasPrizeDialogShown_158 { get; set; }
            public bool HasPrizeDialogShown_159 { get; set; }
            public bool HasPrizeDialogShown_160 { get; set; }
            public bool HasPrizeDialogShown_161 { get; set; }
            public bool HasPrizeDialogShown_162 { get; set; }
            public bool HasPrizeDialogShown_163 { get; set; }
            // Prize Gap
            public bool HasPrizeDialogShown_165 { get; set; }
            public bool HasPrizeDialogShown_166 { get; set; }
            public bool HasPrizeDialogShown_167 { get; set; }
            public bool HasPrizeDialogShown_168 { get; set; }
            // Prize Gap
            public bool HasPrizeDialogShown_170 { get; set; }
            public bool HasPrizeDialogShown_171 { get; set; }
            public bool HasPrizeDialogShown_172 { get; set; }
            public bool HasPrizeDialogShown_173 { get; set; }
            public bool HasPrizeDialogShown_174 { get; set; }
            public bool HasPrizeDialogShown_175 { get; set; }
            public bool HasPrizeDialogShown_176 { get; set; }
            public bool HasPrizeDialogShown_177 { get; set; }
            public bool HasPrizeDialogShown_178 { get; set; }
            public bool HasPrizeDialogShown_179 { get; set; }
            public bool HasPrizeDialogShown_180 { get; set; }
            public bool HasPrizeDialogShown_181 { get; set; }
            public bool HasPrizeDialogShown_182 { get; set; }
            public bool HasPrizeDialogShown_183 { get; set; }
            // Prize Gap
            public bool HasPrizeDialogShown_185 { get; set; }
            public bool HasPrizeDialogShown_186 { get; set; }
        }

        public class UPlayRewards
        {
            public RS2Reward RS2REWARD04 { get; set; }
            public RS2Reward RS2REWARD03 { get; set; }
            public RS2Reward RS2REWARD02 { get; set; }
        }

        public class RS2Reward
        {
            public double value { get; set; }
            public bool purchased { get; set; }
            public string name { get; set; }
            public string description { get; set; }
        }

        #endregion
        #region Songs | UNKNOWN STATUS
        public class Songs : Dictionary<string, Song_Details> { }

        public class Song_Details
        {
            public double TimeStamp { get; set; }
            public Song_DD DynamicDifficulty { get; set; }
            public Song_PlaynextStats PlaynextStats { get; set; }
        }

        public class Song_DD
        {
            public double Avg { get; set; }
            public Dictionary<string, Song_DD_Phrases> Phrases { get; set; }
            public List<double> LevelUp { get; set; }
        }

        public class Song_DD_Phrases
        {
            public double Inc { get; set; }
            public double Avg { get; set; }
            public double Diff { get; set; }
        }

        public class Song_PlaynextStats
        {
            public double TimeStamp { get; set; }
            public Song_PlaynextStats_PhraseIterations PhraseIterations { get; set; }
            public Dictionary<string, Song_PlaynextStats_Chords> Chords { get; set; }
            public Dictionary<string, Song_PlaynextStats_Articulations> Articulations { get; set; }
        }

        public class Song_PlaynextStats_PhraseIterations
        {
            public Dictionary<string, Song_PlaynextStats_PhraseIteration> PhaseIteration { get; set; }
        }

        public class Song_PlaynextStats_PhraseIteration
        {
            public double ChordsMissed { get; set; }
            public double ArticulationsMissed { get; set; }
            public double NotesMissed { get; set; }
            public double ChordsHit { get; set; }
            public double NotesHit { get; set; }
            public double ArticulationsHit { get; set; }
        }

        public class Song_PlaynextStats_Chords
        {
            public double Hit { get; set; }
            public double Missed { get; set; }
        }

        public class Song_PlaynextStats_Articulations
        {
            public double Hit { get; set; }
            public double Missed { get; set; }
        }
        #endregion
        #region Session Mode | UNKNOWN STATUS
        public class SessionMode
        {
            public SessionMode_CustomBands CustomBands { get; set; }
            public SessionMode_Missions Missions { get; set; }
            public SessionMode_Settings Settings { get; set; }
        }

        public class SessionMode_CustomBands { }

        public class SessionMode_Mission
        {
            [JsonProperty(DefaultValueHandling = DefaultValueHandling.Ignore)]
            public double TimeStamp { get; set; }

            public double PlayCount { get; set; }
        }

        public class SessionMode_Tempos
        {
            [JsonProperty("8")]
            public SessionMode_Mission _8 { get; set; }
            [JsonProperty("3")]
            public SessionMode_Mission _3 { get; set; }
            [JsonProperty("2")]
            public SessionMode_Mission _2 { get; set; }
        }

        public class SessionMode_Notes
        {
            [JsonProperty("9")]
            public SessionMode_Mission _9 { get; set; }
            [JsonProperty("6")]
            public SessionMode_Mission _6 { get; set; }
            [JsonProperty("2")]
            public SessionMode_Mission _2 { get; set; }
        }


        public class SessionMode_Scales
        {
            [JsonProperty("7")]
            public SessionMode_Mission _7 { get; set; }
            [JsonProperty("1")]
            public SessionMode_Mission _1 { get; set; }
            [JsonProperty("14")]
            public SessionMode_Mission _14 { get; set; }
            [JsonProperty("15")]
            public SessionMode_Mission _15 { get; set; }
        }

        public class SessionMode_Rooms
        {
            [JsonProperty("4")]
            public SessionMode_Mission _4 { get; set; }
            [JsonProperty("1")]
            public SessionMode_Mission _1 { get; set; }
            [JsonProperty("0")]
            public SessionMode_Mission _0 { get; set; }
        }

        public class SessionMode_Complexities
        {
            [JsonProperty("1")]
            public SessionMode_Mission _1 { get; set; }
            [JsonProperty("0")]
            public SessionMode_Mission _0 { get; set; }
        }

        public class SessionMode_Grooves
        {
            [JsonProperty("2")]
            public SessionMode_Mission _2 { get; set; }
            [JsonProperty("1")]
            public SessionMode_Mission _1 { get; set; }
        }

        public class SessionMode_Missions
        {
            [JsonProperty("BF96EB56102435A29C9A3D97C649165B")]
            public SessionMode_Mission BF96EB56102435A29C9A3D97C649165B { get; set; }

            [JsonProperty("1219C8F1A6A735E8A7CE398322D96688")]
            public SessionMode_Mission _1219C8F1A6A735E8A7CE398322D96688 { get; set; }

            [JsonProperty("1F76B8CF98CD343282F458D29903800F")]
            public SessionMode_Mission _1F76B8CF98CD343282F458D29903800F { get; set; }

            [JsonProperty("010FE6D6292F3A8EB8258A802C57B230")]
            public SessionMode_Mission _010FE6D6292F3A8EB8258A802C57B230 { get; set; }

            [JsonProperty("DB67160C7BD43582BC8E785D60A447BE")]
            public SessionMode_Mission DB67160C7BD43582BC8E785D60A447BE { get; set; }

            [JsonProperty("411261DDF4FE3BD88A27712AE3986F9C")]
            public SessionMode_Mission _411261DDF4FE3BD88A27712AE3986F9C { get; set; }

            [JsonProperty("F7B3D501A72134058FA09E5C49A22E75")]
            public SessionMode_Mission F7B3D501A72134058FA09E5C49A22E75 { get; set; }

            [JsonProperty("C3C878CF9BA23CDD837E96495AA7A702")]
            public SessionMode_Mission C3C878CF9BA23CDD837E96495AA7A702 { get; set; }

            [JsonProperty("9C0427E4DC4B370BB9F64A88A9CEFC56")]
            public SessionMode_Mission _9C0427E4DC4B370BB9F64A88A9CEFC56 { get; set; }

            [JsonProperty("1A236BD1D8FA3FC8934283ADC5F0F773")]
            public SessionMode_Mission _1A236BD1D8FA3FC8934283ADC5F0F773 { get; set; }

            [JsonProperty("E5501BDA8A593CAC8BA50B3EC78235EF")]
            public SessionMode_Mission E5501BDA8A593CAC8BA50B3EC78235EF { get; set; }

            [JsonProperty("9F6A7AA159483AC1879EC0BFCAC2944B")]
            public SessionMode_Mission _9F6A7AA159483AC1879EC0BFCAC2944B { get; set; }

            [JsonProperty("03B13729D4F63F8FBA0C63767C9262B8")]
            public SessionMode_Mission _03B13729D4F63F8FBA0C63767C9262B8 { get; set; }

            [JsonProperty("03ADA6BA199E3222AB681CA78B477111")]
            public SessionMode_Mission _03ADA6BA199E3222AB681CA78B477111 { get; set; }

            [JsonProperty("AA495F0A950535A18372E82159A3C972")]
            public SessionMode_Mission AA495F0A950535A18372E82159A3C972 { get; set; }

            [JsonProperty("DCFEFFB614AC3005BD72474099CE1C4E")]
            public SessionMode_Mission DCFEFFB614AC3005BD72474099CE1C4E { get; set; }

            [JsonProperty("CC899076BE6134DABB467DEC8F569C4D")]
            public SessionMode_Mission CC899076BE6134DABB467DEC8F569C4D { get; set; }

            [JsonProperty("DF2B1032F33132358DB8363654FFA000")]
            public SessionMode_Mission DF2B1032F33132358DB8363654FFA000 { get; set; }

            [JsonProperty("1A4795A54EAB3D21B8863164F98E0811")]
            public SessionMode_Mission _1A4795A54EAB3D21B8863164F98E0811 { get; set; }

            [JsonProperty("48FB1981378B30FF8BECC9CA7A70D825")]
            public SessionMode_Mission _48FB1981378B30FF8BECC9CA7A70D825 { get; set; }

            [JsonProperty("A0A96416DC3633609BCBD01E07C6025D")]
            public SessionMode_Mission A0A96416DC3633609BCBD01E07C6025D { get; set; }

            [JsonProperty("2DE905F2730A349A9DCE8C4A9B8B3901")]
            public SessionMode_Mission _2DE905F2730A349A9DCE8C4A9B8B3901 { get; set; }

            [JsonProperty("DF17AF729F06377B9BAFA16B2CCA7753")]
            public SessionMode_Mission DF17AF729F06377B9BAFA16B2CCA7753 { get; set; }

            public SessionMode_Tempos Tempos { get; set; }

            [JsonProperty("C3372A367862331BB1726140FA3D0A2B")]
            public SessionMode_Mission C3372A367862331BB1726140FA3D0A2B { get; set; }

            [JsonProperty("06340C8536A73735BCCF07A5A6A19AE7")]
            public SessionMode_Mission _06340C8536A73735BCCF07A5A6A19AE7 { get; set; }

            [JsonProperty("0B4926A17B1E392E99C08E78D97280B6")]
            public SessionMode_Mission _0B4926A17B1E392E99C08E78D97280B6 { get; set; }

            [JsonProperty("B966FC32977730D2AE23164972804684")]
            public SessionMode_Mission B966FC32977730D2AE23164972804684 { get; set; }

            [JsonProperty("847363FB782D31498EE6C2A4C0259660")]
            public SessionMode_Mission _847363FB782D31498EE6C2A4C0259660 { get; set; }

            [JsonProperty("EAB905E9D9C637AA8613C81B194D0789")]
            public SessionMode_Mission EAB905E9D9C637AA8613C81B194D0789 { get; set; }

            [JsonProperty("4E6B94366C0F34148757470B0889041D")]
            public SessionMode_Mission _4E6B94366C0F34148757470B0889041D { get; set; }

            [JsonProperty("4A344400452C39AC926066541F198209")]
            public SessionMode_Mission _4A344400452C39AC926066541F198209 { get; set; }

            [JsonProperty("65998043CFA033798F419FBC0FB8E806")]
            public SessionMode_Mission _65998043CFA033798F419FBC0FB8E806 { get; set; }

            [JsonProperty("B95856593B4F34879D8959FE2BAFF438")]
            public SessionMode_Mission B95856593B4F34879D8959FE2BAFF438 { get; set; }

            [JsonProperty("FDB67B32BA4832098E553148B23B7623")]
            public SessionMode_Mission FDB67B32BA4832098E553148B23B7623 { get; set; }

            [JsonProperty("93BD5C3358B93387AB2598BB396DBFAB")]
            public SessionMode_Mission _93BD5C3358B93387AB2598BB396DBFAB { get; set; }

            [JsonProperty("48FD0185AB4A362AB7CF8B5DD041CA50")]
            public SessionMode_Mission _48FD0185AB4A362AB7CF8B5DD041CA50 { get; set; }

            [JsonProperty("013212529E333EFFB20116387AB01447")]
            public SessionMode_Mission _013212529E333EFFB20116387AB01447 { get; set; }

            [JsonProperty("4161F23998743F90A385C33ABA304C0A")]
            public SessionMode_Mission _4161F23998743F90A385C33ABA304C0A { get; set; }

            [JsonProperty("AD3C0F12ED7A3D229A893FB871CACB85")]
            public SessionMode_Mission AD3C0F12ED7A3D229A893FB871CACB85 { get; set; }

            [JsonProperty("C8CC52ED91CF3BAC90E2AA3CBA5D8692")]
            public SessionMode_Mission C8CC52ED91CF3BAC90E2AA3CBA5D8692 { get; set; }

            [JsonProperty("6B357F63837E3B4599EA21E4189CFF22")]
            public SessionMode_Mission _6B357F63837E3B4599EA21E4189CFF22 { get; set; }

            [JsonProperty("65DFDC5C5D8435F59D19D06D71AED4B7")]
            public SessionMode_Mission _65DFDC5C5D8435F59D19D06D71AED4B7 { get; set; }

            [JsonProperty("2558B65D503236F5892E7CF055F94317")]
            public SessionMode_Mission _2558B65D503236F5892E7CF055F94317 { get; set; }

            [JsonProperty("AA7902F6BD7C368C8AE8C3ACE00E6354")]
            public SessionMode_Mission AA7902F6BD7C368C8AE8C3ACE00E6354 { get; set; }

            [JsonProperty("B0E91AE437673DEA85B2EEE9173B36AB")]
            public SessionMode_Mission B0E91AE437673DEA85B2EEE9173B36AB { get; set; }

            public SessionMode_Notes Notes { get; set; }

            [JsonProperty("F8174557419332FD9814EAD805C5E72F")]
            public SessionMode_Mission F8174557419332FD9814EAD805C5E72F { get; set; }

            [JsonProperty("9BA7702A70313CEAA03C6950FAACE821")]
            public SessionMode_Mission _9BA7702A70313CEAA03C6950FAACE821 { get; set; }

            [JsonProperty("E9B88B6CC74A31FBB6B610EA21F49E37")]
            public SessionMode_Mission E9B88B6CC74A31FBB6B610EA21F49E37 { get; set; }

            [JsonProperty("09A5C099B5853C5DB44F9D7A9C8FC404")]
            public SessionMode_Mission _09A5C099B5853C5DB44F9D7A9C8FC404 { get; set; }

            [JsonProperty("539ECA429D4E333FA2B0D97E9A826EFA")]
            public SessionMode_Mission _539ECA429D4E333FA2B0D97E9A826EFA { get; set; }

            [JsonProperty("6CBECBF7C5B0340DB001255E880359EC")]
            public SessionMode_Mission _6CBECBF7C5B0340DB001255E880359EC { get; set; }

            [JsonProperty("7E24D00F50653AB0891470C10FE543D0")]
            public SessionMode_Mission _7E24D00F50653AB0891470C10FE543D0 { get; set; }

            [JsonProperty("AA07ABD2545D398CBD3561D47961AACC")]
            public SessionMode_Mission AA07ABD2545D398CBD3561D47961AACC { get; set; }

            [JsonProperty("739C196158ED38DD9B58E534C7F9B17B")]
            public SessionMode_Mission _739C196158ED38DD9B58E534C7F9B17B { get; set; }

            [JsonProperty("FF55C9680E3C3355AA8EBE683F278C05")]
            public SessionMode_Mission FF55C9680E3C3355AA8EBE683F278C05 { get; set; }

            [JsonProperty("BE4DFDBA138B3C6282B5FC1914F6E6EC")]
            public SessionMode_Mission BE4DFDBA138B3C6282B5FC1914F6E6EC { get; set; }

            [JsonProperty("9E852B48AA5437D88323F99811862416")]
            public SessionMode_Mission _9E852B48AA5437D88323F99811862416 { get; set; }

            [JsonProperty("EFCD8A9BE2CC3896B50C62A1885D8578")]
            public SessionMode_Mission EFCD8A9BE2CC3896B50C62A1885D8578 { get; set; }

            [JsonProperty("F7F0118DC5763B4784FF84E9CEE28CE9")]
            public SessionMode_Mission F7F0118DC5763B4784FF84E9CEE28CE9 { get; set; }

            [JsonProperty("87C5D0B1DC0E326E90EC212D43F77DA5")]
            public SessionMode_Mission _87C5D0B1DC0E326E90EC212D43F77DA5 { get; set; }

            [JsonProperty("DFC1F34FCFF23DF69B535DABE5AAEAD6")]
            public SessionMode_Mission DFC1F34FCFF23DF69B535DABE5AAEAD6 { get; set; }

            public SessionMode_Scales Scales { get; set; }

            [JsonProperty("1A6B23287C703658ACF30D63D8EC0A27")]
            public SessionMode_Mission _1A6B23287C703658ACF30D63D8EC0A27 { get; set; }

            [JsonProperty("286D3F2E908330CC9FB04C734DCD3586")]
            public SessionMode_Mission _286D3F2E908330CC9FB04C734DCD3586 { get; set; }

            [JsonProperty("2CCC26EF66E332C690E159661E1E2116")]
            public SessionMode_Mission _2CCC26EF66E332C690E159661E1E2116 { get; set; }

            [JsonProperty("53F60DC4BE133E4998389C0247AC3FA6")]
            public SessionMode_Mission _53F60DC4BE133E4998389C0247AC3FA6 { get; set; }

            [JsonProperty("64497E174E4933A58346AE5D325B3EE9")]
            public SessionMode_Mission _64497E174E4933A58346AE5D325B3EE9 { get; set; }

            [JsonProperty("5BAD0314D6A931C088E3F0CA2B57F59C")]
            public SessionMode_Mission _5BAD0314D6A931C088E3F0CA2B57F59C { get; set; }

            [JsonProperty("679B3EE913423F3EB20511AB43420BEA")]
            public SessionMode_Mission _679B3EE913423F3EB20511AB43420BEA { get; set; }

            public SessionMode_Rooms Rooms { get; set; }

            [JsonProperty("281E3623FB6B34A3B5DB99B69ED65B8F")]
            public SessionMode_Mission _281E3623FB6B34A3B5DB99B69ED65B8F { get; set; }

            [JsonProperty("D71194C59C163842B0D06A15EBBAEB77")]
            public SessionMode_Mission D71194C59C163842B0D06A15EBBAEB77 { get; set; }

            [JsonProperty("2EE5059D6B213C188EA4BBF758A99004")]
            public SessionMode_Mission _2EE5059D6B213C188EA4BBF758A99004 { get; set; }

            [JsonProperty("781CA376FACC385097ACEF8B83017227")]
            public SessionMode_Mission _781CA376FACC385097ACEF8B83017227 { get; set; }

            [JsonProperty("E57F5A48AE0235BEAC4BA3FA26C81DE7")]
            public SessionMode_Mission E57F5A48AE0235BEAC4BA3FA26C81DE7 { get; set; }

            [JsonProperty("AB9B725327713916888DDBF80878946E")]
            public SessionMode_Mission AB9B725327713916888DDBF80878946E { get; set; }

            public SessionMode_Complexities Complexities { get; set; }

            public SessionMode_Grooves Grooves { get; set; }

            [JsonProperty("AF5B84E33F153E4E9E0D2CFFE0AFA614")]
            public SessionMode_Mission AF5B84E33F153E4E9E0D2CFFE0AFA614 { get; set; }

            [JsonProperty("05E7C2A820D83A67A64ABBF612CC4287")]
            public SessionMode_Mission _05E7C2A820D83A67A64ABBF612CC4287 { get; set; }

            [JsonProperty("8A1C59EA18EB33FF92B4933FC88396C1")]
            public SessionMode_Mission _8A1C59EA18EB33FF92B4933FC88396C1 { get; set; }

            [JsonProperty("195EF76D59853C69A8C556C0FFEADFB7")]
            public SessionMode_Mission _195EF76D59853C69A8C556C0FFEADFB7 { get; set; }
        }

        public class SessionMode_Settings
        {
            public string Instrument2 { get; set; }
            public string Instrument3 { get; set; }
            public string Instrument0 { get; set; }
            public string Instrument1 { get; set; }
            public double Complexity { get; set; }
            public bool HasValidSessionData { get; set; }
            public double Groove { get; set; }
            public double Volume1 { get; set; }
            public double Volume0 { get; set; }
            public double Volume3 { get; set; }
            public double Volume2 { get; set; }
            public double TempoSlew { get; set; }
            public double Tempo { get; set; }
            public string Band { get; set; }
            public double Pan2 { get; set; }
            public double Pan3 { get; set; }
            public double Pan0 { get; set; }
            public double Pan1 { get; set; }
            public double Room { get; set; }
            public double Root { get; set; }
            public double Scale { get; set; }
        }
        #endregion
        #region Achievements | UNKNOWN STATUS (needs organization)
        public class Achievements
        {
            public string SongStreakID { get; set; }
            public bool HitChord_70 { get; set; }
            public bool HitChord_71 { get; set; }
            public bool HitChord_72 { get; set; }
            public bool HitChord_73 { get; set; }
            public bool HitChord_74 { get; set; }
            public bool HitChord_75 { get; set; }
            public bool HitChord_76 { get; set; }
            public bool HitChord_77 { get; set; }
            public bool HitChord_63 { get; set; }
            public bool HitChord_62 { get; set; }
            public bool HitChord_61 { get; set; }
            public bool HitChord_60 { get; set; }
            public bool HitChord_67 { get; set; }
            public bool HitChord_66 { get; set; }
            public bool HitChord_65 { get; set; }
            public bool HitChord_64 { get; set; }
            public bool HitChord_69 { get; set; }
            public bool HitChord_68 { get; set; }
            public bool HitChord_56 { get; set; }
            public bool HitChord_57 { get; set; }
            public bool HitChord_54 { get; set; }
            public bool HitChord_55 { get; set; }
            public bool HitChord_52 { get; set; }
            public bool HitChord_53 { get; set; }
            public bool HitChord_50 { get; set; }
            public bool HitChord_51 { get; set; }
            public bool HitChord_58 { get; set; }
            public bool HitChord_59 { get; set; }
            public bool HitChord_49 { get; set; }
            public bool HitChord_48 { get; set; }
            public bool HitChord_41 { get; set; }
            public bool HitChord_40 { get; set; }
            public bool HitChord_43 { get; set; }
            public bool HitChord_42 { get; set; }
            public bool HitChord_45 { get; set; }
            public bool HitChord_44 { get; set; }
            public bool HitChord_47 { get; set; }
            public bool HitChord_46 { get; set; }
            public bool HitChord_38 { get; set; }
            public bool HitChord_39 { get; set; }
            public bool HitChord_34 { get; set; }
            public bool HitChord_35 { get; set; }
            public bool HitChord_36 { get; set; }
            public bool HitChord_37 { get; set; }
            public bool HitChord_30 { get; set; }
            public bool HitChord_31 { get; set; }
            public bool HitChord_32 { get; set; }
            public bool HitChord_33 { get; set; }
            public bool HitChord_29 { get; set; }
            public bool HitChord_28 { get; set; }
            public bool HitChord_27 { get; set; }
            public bool HitChord_26 { get; set; }
            public bool HitChord_25 { get; set; }
            public bool HitChord_24 { get; set; }
            public bool HitChord_23 { get; set; }
            public bool HitChord_22 { get; set; }
            public bool HitChord_21 { get; set; }
            public bool HitChord_20 { get; set; }
            public bool HitChord_12 { get; set; }
            public bool HitChord_13 { get; set; }
            public bool HitChord_10 { get; set; }
            public bool HitChord_11 { get; set; }
            public bool HitChord_16 { get; set; }
            public bool HitChord_17 { get; set; }
            public bool HitChord_14 { get; set; }
            public bool HitChord_15 { get; set; }
            public bool HitChord_18 { get; set; }
            public bool HitChord_19 { get; set; }
            public bool HitChord_7 { get; set; }
            public bool HitChord_6 { get; set; }
            public bool HitChord_5 { get; set; }
            public bool HitChord_4 { get; set; }
            public bool HitChord_3 { get; set; }
            public bool HitChord_2 { get; set; }
            public bool HitChord_1 { get; set; }
            public bool HitChord_0 { get; set; }
            public bool HitChord_9 { get; set; }
            public bool HitChord_8 { get; set; }
        }
        #endregion
        #region Stats | UNKNOWN STATUS
        public class Stats
        {
            public double GuitarcadeTempleBendsHS { get; set; }
            public double RhythmSkillRating { get; set; }
            public double StatVersionCurrent { get; set; }
            public double SessionCnt { get; set; }
            public double SANumSongsClearedEasy { get; set; }
            public double FingerprintMastery_Rhythm { get; set; }
            public double GuitarcadeNinjaSlideNHS { get; set; }
            public double GuitarcadeHarmonicHeistHS { get; set; }
            public double SongsBassPlayedCount { get; set; }
            public double SoloRating { get; set; }
            public double GuitarcadePlayedTotal { get; set; }
            public double BassSkillRating { get; set; }
            public double FingerprintTechniques_Rhythm { get; set; }
            public string SongMissCompleted { get; set; }
            public double FingerprintRhythm_ChordsHit { get; set; }
            public double SongsPlayedCount { get; set; }
            public double SessionTime { get; set; }
            public double NSP_First { get; set; }
            public string SMMissCompleted { get; set; }
            public Dictionary<string, bool> DLCTag { get; set; }
            public double TotalNumMissionCompletions { get; set; }
            public double SASongsPlayedMediumAndMore { get; set; }
            public double NumSongsMastered { get; set; }
            public double DLCPlayedCount { get; set; }
            public double FingerprintMastery_Bass { get; set; }
            public double SongsLeadPlayedCount { get; set; }
            public double SessionMissionTime { get; set; }
            public double TimePlayedAndLesson { get; set; }
            public string HitTechniques { get; set; }
            public List<V_List> GuitarcadePlayedTime { get; set; }
            public double DiversityRating { get; set; }
            public List<V_List> LessonPlayedCount { get; set; }
            public double GuitarcadeStringSkipSaloonHS { get; set; }
            public double SANumSongsClearedHard { get; set; }
            public double FingerprintTechniques_Lead { get; set; }
            public double SASongsPlayedMaster { get; set; }
            public double ConsecutiveWeeks { get; set; }
            public double MaxConsecutiveWeeks { get; set; }
            public double FingerprintAccuracy_Rhythm { get; set; }
            public double TechniqueRating { get; set; }
            public double MasteryPeakPercent { get; set; }
            public double ChordShapeRating { get; set; }
            public double HighestSoloAccuracy { get; set; }
            public double SASongsPlayedHardAndMore { get; set; }
            public double GuitarcadeHurtlinHurdlesHS { get; set; }
            public string LessonsCompletedMsk { get; set; }
            public double FingerprintTechniques_Bass { get; set; }
            public double SessionInstrumentsUsed { get; set; }
            public double Streak { get; set; }
            public double RS2014SongsPlayed { get; set; }
            public double SongsStreakCummulative { get; set; }
            public string LESMissCompleted { get; set; }
            public double LessonsCompletedCnt { get; set; }
            public string DateWeek { get; set; }
            public double SongsRhythmPlayedCount { get; set; }
            public double GuitarcadeDucksReduxHS { get; set; }
            public double StatsOnlineVersion { get; set; }
            public double MissCompletedCnt { get; set; }
            public Dictionary<string, Stats_Song> Songs { get; set; }
            public double LongestSession { get; set; }
            public double FingerprintAccuracy_Bass { get; set; }
            public string Achievements { get; set; }
            public double AccuracyRating { get; set; }
            public List<V_List> ArticulationAccuracy { get; set; }
            public double ScoreAttackFailStreak { get; set; }
            public Stats_History History { get; set; }
            public double ConsecutiveNotes { get; set; }
            public double FingerprintRhythm_ArpeggiosHit { get; set; }
            public double FingerprintAccuracy_Lead { get; set; }
            public double SessionMissionsCompleted { get; set; }
            public double NumRSRecommendsCompleted { get; set; }
            public double MaxConsecutiveDays { get; set; }
            public string GuitarcadePlayedMsk { get; set; }
            public string Date { get; set; }
            public double SANumSongsClearedMaster { get; set; }
            public double GuitarcadeGoneWailinHS { get; set; }
            public double UniqueChordsPlayed { get; set; }
            public double Learned { get; set; }
            public List<V_List> GuitarcadePlayedCnt { get; set; }
            public double FingerprintMastery_Lead { get; set; }
            public double SASongsPlayedEasyAndMore { get; set; }
            public double LeadSkillRating { get; set; }
            public double GuitarcadeScaleRacerHS { get; set; }
            public double TimePlayed { get; set; }
            public double TimeShop { get; set; }
            public double ConsecutiveDays { get; set; }
            public string TSMissCompleted { get; set; }
        }

        public class Stats_Song
        {
            public double MasteryLast { get; set; }
            public double PlayedCount { get; set; }
            public double AccuracyGlobal { get; set; }
            public string DateLAS { get; set; }
            public double AccuracyChords { get; set; }
            public double MasteryPrevious { get; set; }
            public double MasteryPeak { get; set; }
            public double MasteryPreviousPeak { get; set; }
            public double Streak { get; set; }
            public List<V_List> ArticulationAccuracy { get; set; }
            public List<V_List> ChordsAccuracies { get; set; }
        }

        public class Stats_History
        {
            public string LastUpdate { get; set; }
        }

        #endregion
        #region Score Attack | UNKNOWN STATUS
        public class SongsSA : Dictionary<string, SA_SongDetails> { }

        public class SA_SongDetails
        {
            public double TimeStamp { get; set; }
            public SA_SongDetails_Badges Badges { get; set; }
            public double Difficulty { get; set; }
            public double PlayCount { get; set; }
            public SA_SongDetails_HighScores HighScores { get; set; }
        }

        public class SA_SongDetails_Badges
        {
            public double Hard { get; set; }
            public double Medium { get; set; }
            public double Easy { get; set; }
            public double Master { get; set; }
        }

        public class SA_SongDetails_HighScores
        {
            public double Hard { get; set; }
            public double Medium { get; set; }
            public double Easy { get; set; }
            public double Master { get; set; }
        }
        #endregion

        public class V_List // Very general class
        {
            public double V { get; set; }
        }


        public static void LockAndUnlockRewards(bool unlock = true)
        {
            // I want to start off by saying I am so sorry for whoever is looking at this code.
            // This code is gonna be super long, and there's probably a better way of doing this.
            // But this is how it's gonna be for right now.

            Profile_Sections.Prizes prizes = Loaded_Prizes;

            // Number of Points

            prizes.NumPrizePoints = unlock ? 1300.0 : 0.0;
            prizes.UnawardedPrizePoints = 0.0;

            // Uplay Stuff

            prizes.UplayAction1 = unlock;
            prizes.UplayAction2 = unlock;
            prizes.UplayAction3 = unlock;
            prizes.UplayAction4 = unlock;
            prizes.UplayDlg1 = unlock;
            prizes.UplayDlg2 = unlock;
            prizes.UplayDlg3 = unlock;
            prizes.UplayDlg4 = unlock;

            // Prizes Awarded

            prizes.IsPrizeAwarded_1 = unlock;
            prizes.IsPrizeAwarded_2 = unlock;
            prizes.IsPrizeAwarded_3 = unlock;
            prizes.IsPrizeAwarded_4 = unlock;
            prizes.IsPrizeAwarded_5 = unlock;
            prizes.IsPrizeAwarded_6 = unlock;
            // Prize Gap
            prizes.IsPrizeAwarded_8 = unlock;
            prizes.IsPrizeAwarded_9 = unlock;
            prizes.IsPrizeAwarded_10 = unlock;
            prizes.IsPrizeAwarded_11 = unlock;
            prizes.IsPrizeAwarded_12 = unlock;
            prizes.IsPrizeAwarded_13 = unlock;
            // Prize Gap
            prizes.IsPrizeAwarded_15 = unlock;
            prizes.IsPrizeAwarded_16 = unlock;
            prizes.IsPrizeAwarded_17 = unlock;
            prizes.IsPrizeAwarded_18 = unlock;
            prizes.IsPrizeAwarded_19 = unlock;
            prizes.IsPrizeAwarded_20 = unlock;
            prizes.IsPrizeAwarded_21 = unlock;
            // Prize Gap
            prizes.IsPrizeAwarded_23 = unlock;
            prizes.IsPrizeAwarded_24 = unlock;
            prizes.IsPrizeAwarded_25 = unlock;
            prizes.IsPrizeAwarded_26 = unlock;
            prizes.IsPrizeAwarded_27 = unlock;
            prizes.IsPrizeAwarded_28 = unlock;
            // Prize Gap
            prizes.IsPrizeAwarded_30 = unlock;
            prizes.IsPrizeAwarded_31 = unlock;
            prizes.IsPrizeAwarded_32 = unlock;
            prizes.IsPrizeAwarded_33 = unlock;
            prizes.IsPrizeAwarded_34 = unlock;
            prizes.IsPrizeAwarded_35 = unlock;
            // Prize Gap
            prizes.IsPrizeAwarded_37 = unlock;
            prizes.IsPrizeAwarded_38 = unlock;
            prizes.IsPrizeAwarded_39 = unlock;
            prizes.IsPrizeAwarded_40 = unlock;
            prizes.IsPrizeAwarded_41 = unlock;
            prizes.IsPrizeAwarded_42 = unlock;
            prizes.IsPrizeAwarded_43 = unlock;
            prizes.IsPrizeAwarded_44 = unlock;
            // Prize Gap
            prizes.IsPrizeAwarded_46 = unlock;
            prizes.IsPrizeAwarded_47 = unlock;
            prizes.IsPrizeAwarded_48 = unlock;
            prizes.IsPrizeAwarded_49 = unlock;
            prizes.IsPrizeAwarded_50 = unlock;
            // Prize Gap
            prizes.IsPrizeAwarded_52 = unlock;
            prizes.IsPrizeAwarded_53 = unlock;
            prizes.IsPrizeAwarded_54 = unlock;
            prizes.IsPrizeAwarded_55 = unlock;
            prizes.IsPrizeAwarded_56 = unlock;
            prizes.IsPrizeAwarded_57 = unlock;
            prizes.IsPrizeAwarded_58 = unlock;
            prizes.IsPrizeAwarded_59 = unlock;
            // Prize Gap
            prizes.IsPrizeAwarded_61 = unlock;
            prizes.IsPrizeAwarded_62 = unlock;
            prizes.IsPrizeAwarded_63 = unlock;
            prizes.IsPrizeAwarded_64 = unlock;
            prizes.IsPrizeAwarded_65 = unlock;
            prizes.IsPrizeAwarded_66 = unlock;
            prizes.IsPrizeAwarded_67 = unlock;
            // Prize Gap
            prizes.IsPrizeAwarded_69 = unlock;
            prizes.IsPrizeAwarded_70 = unlock;
            prizes.IsPrizeAwarded_71 = unlock;
            prizes.IsPrizeAwarded_72 = unlock;
            prizes.IsPrizeAwarded_73 = unlock;
            prizes.IsPrizeAwarded_74 = unlock;
            prizes.IsPrizeAwarded_75 = unlock;
            prizes.IsPrizeAwarded_76 = unlock;
            // Prize Gap
            prizes.IsPrizeAwarded_78 = unlock;
            prizes.IsPrizeAwarded_79 = unlock;
            prizes.IsPrizeAwarded_80 = unlock;
            prizes.IsPrizeAwarded_81 = unlock;
            prizes.IsPrizeAwarded_82 = unlock;
            // Prize Gap
            prizes.IsPrizeAwarded_84 = unlock;
            prizes.IsPrizeAwarded_85 = unlock;
            prizes.IsPrizeAwarded_86 = unlock;
            prizes.IsPrizeAwarded_87 = unlock;
            prizes.IsPrizeAwarded_88 = unlock;
            prizes.IsPrizeAwarded_89 = unlock;
            prizes.IsPrizeAwarded_90 = unlock;
            prizes.IsPrizeAwarded_91 = unlock;
            prizes.IsPrizeAwarded_92 = unlock;
            // Prize Gap
            prizes.IsPrizeAwarded_94 = unlock;
            prizes.IsPrizeAwarded_95 = unlock;
            prizes.IsPrizeAwarded_96 = unlock;
            prizes.IsPrizeAwarded_97 = unlock;
            prizes.IsPrizeAwarded_98 = unlock;
            prizes.IsPrizeAwarded_99 = unlock;
            // Prize Gap
            prizes.IsPrizeAwarded_101 = unlock;
            prizes.IsPrizeAwarded_102 = unlock;
            prizes.IsPrizeAwarded_103 = unlock;
            prizes.IsPrizeAwarded_104 = unlock;
            prizes.IsPrizeAwarded_105 = unlock;
            prizes.IsPrizeAwarded_106 = unlock;
            prizes.IsPrizeAwarded_107 = unlock;
            prizes.IsPrizeAwarded_108 = unlock;
            // Prize Gap
            prizes.IsPrizeAwarded_110 = unlock;
            prizes.IsPrizeAwarded_111 = unlock;
            prizes.IsPrizeAwarded_112 = unlock;
            prizes.IsPrizeAwarded_113 = unlock;
            prizes.IsPrizeAwarded_114 = unlock;
            prizes.IsPrizeAwarded_115 = unlock;
            prizes.IsPrizeAwarded_116 = unlock;
            // Prize Gap
            prizes.IsPrizeAwarded_118 = unlock;
            prizes.IsPrizeAwarded_119 = unlock;
            prizes.IsPrizeAwarded_120 = unlock;
            prizes.IsPrizeAwarded_121 = unlock;
            prizes.IsPrizeAwarded_122 = unlock;
            prizes.IsPrizeAwarded_123 = unlock;
            prizes.IsPrizeAwarded_124 = unlock;
            // Prize Gap
            prizes.IsPrizeAwarded_126 = unlock;
            prizes.IsPrizeAwarded_127 = unlock;
            prizes.IsPrizeAwarded_128 = unlock;
            prizes.IsPrizeAwarded_129 = unlock;
            prizes.IsPrizeAwarded_130 = unlock;
            // Prize Gap
            prizes.IsPrizeAwarded_132 = unlock;
            prizes.IsPrizeAwarded_133 = unlock;
            prizes.IsPrizeAwarded_134 = unlock;
            prizes.IsPrizeAwarded_135 = unlock;
            prizes.IsPrizeAwarded_136 = unlock;
            prizes.IsPrizeAwarded_137 = unlock;
            prizes.IsPrizeAwarded_138 = unlock;
            prizes.IsPrizeAwarded_139 = unlock;
            prizes.IsPrizeAwarded_140 = unlock;
            // Prize Gap
            prizes.IsPrizeAwarded_142 = unlock;
            prizes.IsPrizeAwarded_143 = unlock;
            prizes.IsPrizeAwarded_144 = unlock;
            prizes.IsPrizeAwarded_145 = unlock;
            prizes.IsPrizeAwarded_146 = unlock;
            prizes.IsPrizeAwarded_147 = unlock;
            // Prize Gap
            prizes.IsPrizeAwarded_149 = unlock;
            prizes.IsPrizeAwarded_150 = unlock;
            prizes.IsPrizeAwarded_151 = unlock;
            prizes.IsPrizeAwarded_152 = unlock;
            prizes.IsPrizeAwarded_153 = unlock;
            // Prize Gap
            // Prize Gap
            prizes.IsPrizeAwarded_156 = unlock;
            prizes.IsPrizeAwarded_157 = unlock;
            prizes.IsPrizeAwarded_158 = unlock;
            prizes.IsPrizeAwarded_159 = unlock;
            prizes.IsPrizeAwarded_160 = unlock;
            prizes.IsPrizeAwarded_161 = unlock;
            prizes.IsPrizeAwarded_162 = unlock;
            prizes.IsPrizeAwarded_163 = unlock;
            // Prize Gap
            prizes.IsPrizeAwarded_165 = unlock;
            prizes.IsPrizeAwarded_166 = unlock;
            prizes.IsPrizeAwarded_167 = unlock;
            prizes.IsPrizeAwarded_168 = unlock;
            // Prize Gap
            prizes.IsPrizeAwarded_170 = unlock;
            prizes.IsPrizeAwarded_171 = unlock;
            prizes.IsPrizeAwarded_172 = unlock;
            prizes.IsPrizeAwarded_173 = unlock;
            prizes.IsPrizeAwarded_174 = unlock;
            prizes.IsPrizeAwarded_175 = unlock;
            prizes.IsPrizeAwarded_176 = unlock;
            prizes.IsPrizeAwarded_177 = unlock;
            prizes.IsPrizeAwarded_178 = unlock;
            prizes.IsPrizeAwarded_179 = unlock;
            prizes.IsPrizeAwarded_180 = unlock;
            prizes.IsPrizeAwarded_181 = unlock;
            prizes.IsPrizeAwarded_182 = unlock;
            prizes.IsPrizeAwarded_183 = unlock;
            // Prize Gap
            prizes.IsPrizeAwarded_185 = unlock;
            prizes.IsPrizeAwarded_186 = unlock;

            // Prize Awarded Dialog Seen
            prizes.HasPrizeDialogShown_1 = unlock;
            prizes.HasPrizeDialogShown_2 = unlock;
            prizes.HasPrizeDialogShown_3 = unlock;
            prizes.HasPrizeDialogShown_4 = unlock;
            prizes.HasPrizeDialogShown_5 = unlock;
            prizes.HasPrizeDialogShown_6 = unlock;
            // Prize Gap
            prizes.HasPrizeDialogShown_8 = unlock;
            prizes.HasPrizeDialogShown_9 = unlock;
            prizes.HasPrizeDialogShown_10 = unlock;
            prizes.HasPrizeDialogShown_11 = unlock;
            prizes.HasPrizeDialogShown_12 = unlock;
            prizes.HasPrizeDialogShown_13 = unlock;
            // Prize Gap
            prizes.HasPrizeDialogShown_15 = unlock;
            prizes.HasPrizeDialogShown_16 = unlock;
            prizes.HasPrizeDialogShown_17 = unlock;
            prizes.HasPrizeDialogShown_18 = unlock;
            prizes.HasPrizeDialogShown_19 = unlock;
            prizes.HasPrizeDialogShown_20 = unlock;
            prizes.HasPrizeDialogShown_21 = unlock;
            // Prize Gap
            prizes.HasPrizeDialogShown_23 = unlock;
            prizes.HasPrizeDialogShown_24 = unlock;
            prizes.HasPrizeDialogShown_25 = unlock;
            prizes.HasPrizeDialogShown_26 = unlock;
            prizes.HasPrizeDialogShown_27 = unlock;
            prizes.HasPrizeDialogShown_28 = unlock;
            // Prize Gap
            prizes.HasPrizeDialogShown_30 = unlock;
            prizes.HasPrizeDialogShown_31 = unlock;
            prizes.HasPrizeDialogShown_32 = unlock;
            prizes.HasPrizeDialogShown_33 = unlock;
            prizes.HasPrizeDialogShown_34 = unlock;
            prizes.HasPrizeDialogShown_35 = unlock;
            // Prize Gap
            prizes.HasPrizeDialogShown_37 = unlock;
            prizes.HasPrizeDialogShown_38 = unlock;
            prizes.HasPrizeDialogShown_39 = unlock;
            prizes.HasPrizeDialogShown_40 = unlock;
            prizes.HasPrizeDialogShown_41 = unlock;
            prizes.HasPrizeDialogShown_42 = unlock;
            prizes.HasPrizeDialogShown_43 = unlock;
            prizes.HasPrizeDialogShown_44 = unlock;
            // Prize Gap
            prizes.HasPrizeDialogShown_46 = unlock;
            prizes.HasPrizeDialogShown_47 = unlock;
            prizes.HasPrizeDialogShown_48 = unlock;
            prizes.HasPrizeDialogShown_49 = unlock;
            prizes.HasPrizeDialogShown_50 = unlock;
            // Prize Gap
            prizes.HasPrizeDialogShown_52 = unlock;
            prizes.HasPrizeDialogShown_53 = unlock;
            prizes.HasPrizeDialogShown_54 = unlock;
            prizes.HasPrizeDialogShown_55 = unlock;
            prizes.HasPrizeDialogShown_56 = unlock;
            prizes.HasPrizeDialogShown_57 = unlock;
            prizes.HasPrizeDialogShown_58 = unlock;
            prizes.HasPrizeDialogShown_59 = unlock;
            // Prize Gap
            prizes.HasPrizeDialogShown_61 = unlock;
            prizes.HasPrizeDialogShown_62 = unlock;
            prizes.HasPrizeDialogShown_63 = unlock;
            prizes.HasPrizeDialogShown_64 = unlock;
            prizes.HasPrizeDialogShown_65 = unlock;
            prizes.HasPrizeDialogShown_66 = unlock;
            prizes.HasPrizeDialogShown_67 = unlock;
            // Prize Gap
            prizes.HasPrizeDialogShown_69 = unlock;
            prizes.HasPrizeDialogShown_70 = unlock;
            prizes.HasPrizeDialogShown_71 = unlock;
            prizes.HasPrizeDialogShown_72 = unlock;
            prizes.HasPrizeDialogShown_73 = unlock;
            prizes.HasPrizeDialogShown_74 = unlock;
            prizes.HasPrizeDialogShown_75 = unlock;
            prizes.HasPrizeDialogShown_76 = unlock;
            // Prize Gap
            prizes.HasPrizeDialogShown_78 = unlock;
            prizes.HasPrizeDialogShown_79 = unlock;
            prizes.HasPrizeDialogShown_80 = unlock;
            prizes.HasPrizeDialogShown_81 = unlock;
            prizes.HasPrizeDialogShown_82 = unlock;
            // Prize Gap
            prizes.HasPrizeDialogShown_84 = unlock;
            prizes.HasPrizeDialogShown_85 = unlock;
            prizes.HasPrizeDialogShown_86 = unlock;
            prizes.HasPrizeDialogShown_87 = unlock;
            prizes.HasPrizeDialogShown_88 = unlock;
            prizes.HasPrizeDialogShown_89 = unlock;
            prizes.HasPrizeDialogShown_90 = unlock;
            prizes.HasPrizeDialogShown_91 = unlock;
            prizes.HasPrizeDialogShown_92 = unlock;
            // Prize Gap
            prizes.HasPrizeDialogShown_94 = unlock;
            prizes.HasPrizeDialogShown_95 = unlock;
            prizes.HasPrizeDialogShown_96 = unlock;
            prizes.HasPrizeDialogShown_97 = unlock;
            prizes.HasPrizeDialogShown_98 = unlock;
            prizes.HasPrizeDialogShown_99 = unlock;
            // Prize Gap
            prizes.HasPrizeDialogShown_101 = unlock;
            prizes.HasPrizeDialogShown_102 = unlock;
            prizes.HasPrizeDialogShown_103 = unlock;
            prizes.HasPrizeDialogShown_104 = unlock;
            prizes.HasPrizeDialogShown_105 = unlock;
            prizes.HasPrizeDialogShown_106 = unlock;
            prizes.HasPrizeDialogShown_107 = unlock;
            prizes.HasPrizeDialogShown_108 = unlock;
            // Prize Gap
            prizes.HasPrizeDialogShown_110 = unlock;
            prizes.HasPrizeDialogShown_111 = unlock;
            prizes.HasPrizeDialogShown_112 = unlock;
            prizes.HasPrizeDialogShown_113 = unlock;
            prizes.HasPrizeDialogShown_114 = unlock;
            prizes.HasPrizeDialogShown_115 = unlock;
            prizes.HasPrizeDialogShown_116 = unlock;
            // Prize Gap
            prizes.HasPrizeDialogShown_118 = unlock;
            prizes.HasPrizeDialogShown_119 = unlock;
            prizes.HasPrizeDialogShown_120 = unlock;
            prizes.HasPrizeDialogShown_121 = unlock;
            prizes.HasPrizeDialogShown_122 = unlock;
            prizes.HasPrizeDialogShown_123 = unlock;
            prizes.HasPrizeDialogShown_124 = unlock;
            // Prize Gap
            prizes.HasPrizeDialogShown_126 = unlock;
            prizes.HasPrizeDialogShown_127 = unlock;
            prizes.HasPrizeDialogShown_128 = unlock;
            prizes.HasPrizeDialogShown_129 = unlock;
            prizes.HasPrizeDialogShown_130 = unlock;
            // Prize Gap
            prizes.HasPrizeDialogShown_132 = unlock;
            prizes.HasPrizeDialogShown_133 = unlock;
            prizes.HasPrizeDialogShown_134 = unlock;
            prizes.HasPrizeDialogShown_135 = unlock;
            prizes.HasPrizeDialogShown_136 = unlock;
            prizes.HasPrizeDialogShown_137 = unlock;
            prizes.HasPrizeDialogShown_138 = unlock;
            prizes.HasPrizeDialogShown_139 = unlock;
            prizes.HasPrizeDialogShown_140 = unlock;
            // Prize Gap
            prizes.HasPrizeDialogShown_142 = unlock;
            prizes.HasPrizeDialogShown_143 = unlock;
            prizes.HasPrizeDialogShown_144 = unlock;
            prizes.HasPrizeDialogShown_145 = unlock;
            prizes.HasPrizeDialogShown_146 = unlock;
            prizes.HasPrizeDialogShown_147 = unlock;
            // Prize Gap
            prizes.HasPrizeDialogShown_149 = unlock;
            prizes.HasPrizeDialogShown_150 = unlock;
            prizes.HasPrizeDialogShown_151 = unlock;
            prizes.HasPrizeDialogShown_152 = unlock;
            prizes.HasPrizeDialogShown_153 = unlock;
            // Prize Gap
            // Prize Gap
            prizes.HasPrizeDialogShown_156 = unlock;
            prizes.HasPrizeDialogShown_157 = unlock;
            prizes.HasPrizeDialogShown_158 = unlock;
            prizes.HasPrizeDialogShown_159 = unlock;
            prizes.HasPrizeDialogShown_160 = unlock;
            prizes.HasPrizeDialogShown_161 = unlock;
            prizes.HasPrizeDialogShown_162 = unlock;
            prizes.HasPrizeDialogShown_163 = unlock;
            // Prize Gap
            prizes.HasPrizeDialogShown_165 = unlock;
            prizes.HasPrizeDialogShown_166 = unlock;
            prizes.HasPrizeDialogShown_167 = unlock;
            prizes.HasPrizeDialogShown_168 = unlock;
            // Prize Gap
            prizes.HasPrizeDialogShown_170 = unlock;
            prizes.HasPrizeDialogShown_171 = unlock;
            prizes.HasPrizeDialogShown_172 = unlock;
            prizes.HasPrizeDialogShown_173 = unlock;
            prizes.HasPrizeDialogShown_174 = unlock;
            prizes.HasPrizeDialogShown_175 = unlock;
            prizes.HasPrizeDialogShown_176 = unlock;
            prizes.HasPrizeDialogShown_177 = unlock;
            prizes.HasPrizeDialogShown_178 = unlock;
            prizes.HasPrizeDialogShown_179 = unlock;
            prizes.HasPrizeDialogShown_180 = unlock;
            prizes.HasPrizeDialogShown_181 = unlock;
            prizes.HasPrizeDialogShown_182 = unlock;
            prizes.HasPrizeDialogShown_183 = unlock;
            // Prize Gap
            prizes.HasPrizeDialogShown_185 = unlock;
            prizes.HasPrizeDialogShown_186 = unlock;

            Loaded_Prizes = prizes;
        }
    }

}
#endregion