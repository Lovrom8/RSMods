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
            string fullProfileFolder = string.Empty;

            try
            {
                RegistryKey availableUser = Registry.CurrentUser.OpenSubKey("SOFTWARE").OpenSubKey("Valve").OpenSubKey("Steam").OpenSubKey("Users");

                if (availableUser == null) // If the key doesn't exist for whatever reason, try to manually find the path by searching for files with 
                    return GenUtil.GetSteamProfilesFolderManual();

                string steamFolder = Registry.CurrentUser.OpenSubKey("SOFTWARE").OpenSubKey("Valve").OpenSubKey("Steam").GetValue("SteamPath").ToString(), profileSubFolders = "/221680/remote", userDataFolder = "/userdata/";

                foreach (string user in availableUser.GetSubKeyNames())
                {
                    if (Directory.Exists(steamFolder + userDataFolder + user + profileSubFolders))
                    {
                        fullProfileFolder = steamFolder + userDataFolder + user + profileSubFolders;
                        break;
                    }

                    else
                        continue;
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

            if (profileFolder == "")
            {
                MessageBox.Show("We can't find your profiles to backup");
                return;
            }

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

            foreach (var file in Directory.GetFiles(profileFolder))
            {
                File.Copy(file, Path.Combine(timedBackupFolder, Path.GetFileName(file)));
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


        public static void LoadProfileSections(string profilePath)
        {
            Loaded_Missions = Profiles.JSON_ToObject<Mission>(profilePath, Sections.Missions);
            Loaded_Playnexts = Profiles.JSON_ToObject<Playnexts>(profilePath, Sections.PlayNexts);
            Loaded_RecentlyPlayedVenues = Profiles.JSON_ToObject<RecentlyPlayedVenues>(profilePath, Sections.RecentlyPlayedVenues);
            Loaded_Chords = Profiles.JSON_ToObject<Chords>(profilePath, Sections.Chords);
            Loaded_NSPPlaylistRoot2 = Profiles.JSON_ToObject<NSP_PlayListRoot2>(profilePath, Sections.NSP_PlayListRoot2);
            Loaded_Options = Profiles.JSON_ToObject<Options>(profilePath, Sections.Options);
            Loaded_Guitarcade = Profiles.JSON_ToObject<Guitarcade>(profilePath, Sections.Guitarcade);
            Loaded_Songlists = Profiles.JSON_ToObject<SongListsRoot>(profilePath, Sections.SongLists);
            Loaded_FavoritesList = Profiles.JSON_ToObject<FavoritesListRoot>(profilePath, Sections.FavoritesList);
            Loaded_DynamicDifficulty = Profiles.JSON_ToObject<DynamicDifficulty>(profilePath, Sections.DD);
            Loaded_Prizes = Profiles.JSON_ToObject<Prizes>(profilePath, Sections.Prizes);
            Loaded_LearnASong = Profiles.JSON_ToObject<Songs>(profilePath, Sections.LearnASong);
            Loaded_SessionMode = Profiles.JSON_ToObject<SessionMode>(profilePath, Sections.SessionMode);
            Loaded_Achievements = Profiles.JSON_ToObject<Achievements>(profilePath, Sections.Achievements);
            Loaded_Stats = Profiles.JSON_ToObject<Stats>(profilePath, Sections.Stats);
            Loaded_ScoreAttack = Profiles.JSON_ToObject<SongsSA>(profilePath, Sections.ScoreAttack);
        }

        #region Missions | NOT COMPLETE
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

        #region Play Nexts | NOT COMPLETE
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
        #region Recently Played Venues | NOT COMPLETE
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

        #region Chords | NOT COMPLETE
        public class Chords
        {
            [JsonProperty("6110c00")]
            public bool _6110c00 { get; set; }

            [JsonProperty("a310000")]
            public bool A310000 { get; set; }

            [JsonProperty("a729ca0")]
            public bool A729ca0 { get; set; }

            [JsonProperty("50000")]
            public bool _50000 { get; set; }

            [JsonProperty("8800")]
            public bool _8800 { get; set; }

            [JsonProperty("731000")]
            public bool _731000 { get; set; }

            [JsonProperty("480e0")]
            public bool _480e0 { get; set; }

            [JsonProperty("1400")]
            public bool _1400 { get; set; }

            [JsonProperty("73140")]
            public bool _73140 { get; set; }

            [JsonProperty("848000")]
            public bool _848000 { get; set; }

            [JsonProperty("28060")]
            public bool _28060 { get; set; }

            [JsonProperty("2126")]
            public bool _2126 { get; set; }

            [JsonProperty("1465")]
            public bool _1465 { get; set; }

            [JsonProperty("1460")]
            public bool _1460 { get; set; }

            [JsonProperty("1463")]
            public bool _1463 { get; set; }

            [JsonProperty("9")]
            public bool _9 { get; set; }

            [JsonProperty("7")]
            public bool _7 { get; set; }

            [JsonProperty("4")]
            public bool _4 { get; set; }

            [JsonProperty("3")]
            public bool _3 { get; set; }

            [JsonProperty("2")]
            public bool _2 { get; set; }

            [JsonProperty("0")]
            public bool _0 { get; set; }

            [JsonProperty("c")]
            public bool C { get; set; }

            [JsonProperty("a")]
            public bool A { get; set; }

            [JsonProperty("6528000")]
            public bool _6528000 { get; set; }

            [JsonProperty("ea49ce0")]
            public bool Ea49ce0 { get; set; }

            [JsonProperty("62d40")]
            public bool _62d40 { get; set; }

            [JsonProperty("e73a4e0")]
            public bool E73a4e0 { get; set; }

            [JsonProperty("2106")]
            public bool _2106 { get; set; }

            [JsonProperty("2108")]
            public bool _2108 { get; set; }

            [JsonProperty("b63140")]
            public bool B63140 { get; set; }

            [JsonProperty("781a0")]
            public bool _781a0 { get; set; }

            [JsonProperty("62540")]
            public bool _62540 { get; set; }

            [JsonProperty("10638000")]
            public bool _10638000 { get; set; }

            [JsonProperty("1442")]
            public bool _1442 { get; set; }

            public double TimeStamp { get; set; }

            [JsonProperty("1cd60000")]
            public bool _1cd60000 { get; set; }

            [JsonProperty("73180")]
            public bool _73180 { get; set; }

            [JsonProperty("7318c")]
            public bool _7318c { get; set; }

            [JsonProperty("73980")]
            public bool _73980 { get; set; }

            [JsonProperty("12b58000")]
            public bool _12b58000 { get; set; }

            [JsonProperty("a600000")]
            public bool A600000 { get; set; }

            [JsonProperty("1ef7b000")]
            public bool _1ef7b000 { get; set; }

            [JsonProperty("a49c00")]
            public bool A49c00 { get; set; }

            [JsonProperty("a731c00")]
            public bool A731c00 { get; set; }

            [JsonProperty("201080")]
            public bool _201080 { get; set; }

            [JsonProperty("28000")]
            public bool _28000 { get; set; }

            [JsonProperty("a531ce5")]
            public bool A531ce5 { get; set; }

            [JsonProperty("731084")]
            public bool _731084 { get; set; }

            [JsonProperty("6000860")]
            public bool _6000860 { get; set; }

            [JsonProperty("a8394a5")]
            public bool A8394a5 { get; set; }

            [JsonProperty("d6b000")]
            public bool D6b000 { get; set; }

            [JsonProperty("4314c0")]
            public bool _4314c0 { get; set; }

            [JsonProperty("394a0")]
            public bool _394a0 { get; set; }

            [JsonProperty("394a5")]
            public bool _394a5 { get; set; }

            [JsonProperty("120c00")]
            public bool _120c00 { get; set; }

            [JsonProperty("b53000")]
            public bool B53000 { get; set; }

            [JsonProperty("2988")]
            public bool _2988 { get; set; }

            [JsonProperty("28c20")]
            public bool _28c20 { get; set; }

            [JsonProperty("8040")]
            public bool _8040 { get; set; }

            [JsonProperty("d63140")]
            public bool D63140 { get; set; }

            [JsonProperty("394c0")]
            public bool _394c0 { get; set; }

            [JsonProperty("a53000")]
            public bool A53000 { get; set; }

            [JsonProperty("6318c")]
            public bool _6318c { get; set; }

            [JsonProperty("63180")]
            public bool _63180 { get; set; }

            [JsonProperty("39ce8")]
            public bool _39ce8 { get; set; }

            [JsonProperty("39ce0")]
            public bool _39ce0 { get; set; }

            [JsonProperty("39ce7")]
            public bool _39ce7 { get; set; }

            [JsonProperty("a638000")]
            public bool A638000 { get; set; }

            [JsonProperty("8431880")]
            public bool _8431880 { get; set; }

            [JsonProperty("8431884")]
            public bool _8431884 { get; set; }

            [JsonProperty("839ca5")]
            public bool _839ca5 { get; set; }

            [JsonProperty("c838000")]
            public bool C838000 { get; set; }

            [JsonProperty("28c60")]
            public bool _28c60 { get; set; }

            [JsonProperty("28c61")]
            public bool _28c61 { get; set; }

            [JsonProperty("28c63")]
            public bool _28c63 { get; set; }

            [JsonProperty("8000")]
            public bool _8000 { get; set; }

            [JsonProperty("7bda0")]
            public bool _7bda0 { get; set; }

            [JsonProperty("218c0")]
            public bool _218c0 { get; set; }

            [JsonProperty("58120")]
            public bool _58120 { get; set; }

            [JsonProperty("a529ce5")]
            public bool A529ce5 { get; set; }

            [JsonProperty("28860")]
            public bool _28860 { get; set; }

            [JsonProperty("39ca5")]
            public bool _39ca5 { get; set; }

            [JsonProperty("39ca0")]
            public bool _39ca0 { get; set; }

            [JsonProperty("5280a")]
            public bool _5280a { get; set; }

            [JsonProperty("a018005")]
            public bool A018005 { get; set; }

            [JsonProperty("28842")]
            public bool _28842 { get; set; }

            [JsonProperty("1184000")]
            public bool _1184000 { get; set; }

            [JsonProperty("b52108")]
            public bool B52108 { get; set; }

            [JsonProperty("2908")]
            public bool _2908 { get; set; }

            [JsonProperty("2906")]
            public bool _2906 { get; set; }

            [JsonProperty("2110c00")]
            public bool _2110c00 { get; set; }

            [JsonProperty("a529ca5")]
            public bool A529ca5 { get; set; }

            [JsonProperty("6318ca3")]
            public bool _6318ca3 { get; set; }

            [JsonProperty("4a560")]
            public bool _4a560 { get; set; }

            [JsonProperty("6000000")]
            public bool _6000000 { get; set; }

            [JsonProperty("6000003")]
            public bool _6000003 { get; set; }

            [JsonProperty("7424c0")]
            public bool _7424c0 { get; set; }

            [JsonProperty("739ca0")]
            public bool _739ca0 { get; set; }

            [JsonProperty("2948")]
            public bool _2948 { get; set; }

            [JsonProperty("294a")]
            public bool _294a { get; set; }

            [JsonProperty("2110c41")]
            public bool _2110c41 { get; set; }

            [JsonProperty("20c63")]
            public bool _20c63 { get; set; }

            [JsonProperty("539ca0")]
            public bool _539ca0 { get; set; }

            [JsonProperty("63")]
            public bool _63 { get; set; }

            [JsonProperty("61")]
            public bool _61 { get; set; }

            [JsonProperty("60")]
            public bool _60 { get; set; }

            [JsonProperty("64")]
            public bool _64 { get; set; }

            [JsonProperty("6301400")]
            public bool _6301400 { get; set; }

            [JsonProperty("63140")]
            public bool _63140 { get; set; }

            [JsonProperty("6314a")]
            public bool _6314a { get; set; }

            [JsonProperty("20861")]
            public bool _20861 { get; set; }

            [JsonProperty("20860")]
            public bool _20860 { get; set; }

            [JsonProperty("2110c60")]
            public bool _2110c60 { get; set; }

            [JsonProperty("2110c61")]
            public bool _2110c61 { get; set; }

            [JsonProperty("20c40")]
            public bool _20c40 { get; set; }

            [JsonProperty("40")]
            public bool _40 { get; set; }

            [JsonProperty("43")]
            public bool _43 { get; set; }

            [JsonProperty("4a520")]
            public bool _4a520 { get; set; }

            [JsonProperty("950100")]
            public bool _950100 { get; set; }

            [JsonProperty("6000040")]
            public bool _6000040 { get; set; }

            [JsonProperty("6000043")]
            public bool _6000043 { get; set; }

            [JsonProperty("4014c0")]
            public bool _4014c0 { get; set; }

            [JsonProperty("739ce0")]
            public bool _739ce0 { get; set; }

            [JsonProperty("20840")]
            public bool _20840 { get; set; }

            [JsonProperty("20842")]
            public bool _20842 { get; set; }

            [JsonProperty("c63a106")]
            public bool C63a106 { get; set; }

            [JsonProperty("23")]
            public bool _23 { get; set; }

            [JsonProperty("4014a0")]
            public bool _4014a0 { get; set; }

            [JsonProperty("23198000")]
            public bool _23198000 { get; set; }

            [JsonProperty("29420")]
            public bool _29420 { get; set; }

            [JsonProperty("84a400")]
            public bool _84a400 { get; set; }

            [JsonProperty("8394a0")]
            public bool _8394a0 { get; set; }

            [JsonProperty("8394a5")]
            public bool _8394a5 { get; set; }

            [JsonProperty("639ca0")]
            public bool _639ca0 { get; set; }

            [JsonProperty("329460")]
            public bool _329460 { get; set; }

            [JsonProperty("63214a3")]
            public bool _63214a3 { get; set; }

            [JsonProperty("20800")]
            public bool _20800 { get; set; }

            [JsonProperty("4320000")]
            public bool _4320000 { get; set; }

            [JsonProperty("a639c00")]
            public bool A639c00 { get; set; }

            [JsonProperty("29400")]
            public bool _29400 { get; set; }

            [JsonProperty("219000")]
            public bool _219000 { get; set; }

            [JsonProperty("10a49c00")]
            public bool _10a49c00 { get; set; }

            [JsonProperty("73a400")]
            public bool _73a400 { get; set; }

            [JsonProperty("4221040")]
            public bool _4221040 { get; set; }

            [JsonProperty("4221042")]
            public bool _4221042 { get; set; }

            [JsonProperty("301400")]
            public bool _301400 { get; set; }

            [JsonProperty("1c0")]
            public bool _1c0 { get; set; }

            [JsonProperty("a738000")]
            public bool A738000 { get; set; }

            [JsonProperty("29460")]
            public bool _29460 { get; set; }

            [JsonProperty("29463")]
            public bool _29463 { get; set; }

            [JsonProperty("6300000")]
            public bool _6300000 { get; set; }

            [JsonProperty("6300002")]
            public bool _6300002 { get; set; }

            [JsonProperty("6300003")]
            public bool _6300003 { get; set; }

            [JsonProperty("8521800")]
            public bool _8521800 { get; set; }

            [JsonProperty("1694ad69")]
            public bool _1694ad69 { get; set; }

            [JsonProperty("11000")]
            public bool _11000 { get; set; }

            [JsonProperty("11002")]
            public bool _11002 { get; set; }

            [JsonProperty("7000e0")]
            public bool _7000e0 { get; set; }

            [JsonProperty("801d00")]
            public bool _801d00 { get; set; }

            [JsonProperty("600100")]
            public bool _600100 { get; set; }

            [JsonProperty("a3a400")]
            public bool A3a400 { get; set; }

            [JsonProperty("6398e0")]
            public bool _6398e0 { get; set; }

            [JsonProperty("749ce0")]
            public bool _749ce0 { get; set; }

            [JsonProperty("12a48000")]
            public bool _12a48000 { get; set; }

            [JsonProperty("942900")]
            public bool _942900 { get; set; }

            [JsonProperty("300840")]
            public bool _300840 { get; set; }

            [JsonProperty("6300040")]
            public bool _6300040 { get; set; }

            [JsonProperty("6300043")]
            public bool _6300043 { get; set; }

            [JsonProperty("84a800")]
            public bool _84a800 { get; set; }

            [JsonProperty("e84a400")]
            public bool E84a400 { get; set; }

            [JsonProperty("a4a4e7")]
            public bool A4a4e7 { get; set; }

            [JsonProperty("a4a4e0")]
            public bool A4a4e0 { get; set; }

            [JsonProperty("20880")]
            public bool _20880 { get; set; }

            [JsonProperty("52108")]
            public bool _52108 { get; set; }

            [JsonProperty("52100")]
            public bool _52100 { get; set; }

            [JsonProperty("300860")]
            public bool _300860 { get; set; }

            [JsonProperty("6300060")]
            public bool _6300060 { get; set; }

            [JsonProperty("9024e0")]
            public bool _9024e0 { get; set; }

            [JsonProperty("b5b400")]
            public bool B5b400 { get; set; }

            [JsonProperty("398b")]
            public bool _398b { get; set; }

            [JsonProperty("21400")]
            public bool _21400 { get; set; }

            [JsonProperty("429460")]
            public bool _429460 { get; set; }

            [JsonProperty("8521884")]
            public bool _8521884 { get; set; }

            [JsonProperty("8521880")]
            public bool _8521880 { get; set; }

            [JsonProperty("11082")]
            public bool _11082 { get; set; }

            [JsonProperty("a521000")]
            public bool A521000 { get; set; }

            [JsonProperty("429400")]
            public bool _429400 { get; set; }

            [JsonProperty("20040")]
            public bool _20040 { get; set; }

            [JsonProperty("10842108")]
            public bool _10842108 { get; set; }

            [JsonProperty("510800")]
            public bool _510800 { get; set; }

            [JsonProperty("e530000")]
            public bool E530000 { get; set; }

            [JsonProperty("21440")]
            public bool _21440 { get; set; }

            [JsonProperty("4218c0")]
            public bool _4218c0 { get; set; }

            [JsonProperty("2118000")]
            public bool _2118000 { get; set; }

            [JsonProperty("c0000a6")]
            public bool C0000a6 { get; set; }

            [JsonProperty("e739c00")]
            public bool E739c00 { get; set; }

            [JsonProperty("4418000")]
            public bool _4418000 { get; set; }

            [JsonProperty("118800")]
            public bool _118800 { get; set; }

            [JsonProperty("2218c20")]
            public bool _2218c20 { get; set; }

            [JsonProperty("20000")]
            public bool _20000 { get; set; }

            [JsonProperty("d53000")]
            public bool D53000 { get; set; }

            [JsonProperty("e73a527")]
            public bool E73a527 { get; set; }

            [JsonProperty("4631")]
            public bool _4631 { get; set; }

            [JsonProperty("521460")]
            public bool _521460 { get; set; }

            [JsonProperty("462f")]
            public bool _462f { get; set; }

            [JsonProperty("106000e8")]
            public bool _106000e8 { get; set; }

            [JsonProperty("a528000")]
            public bool A528000 { get; set; }

            [JsonProperty("521800")]
            public bool _521800 { get; set; }

            [JsonProperty("8429800")]
            public bool _8429800 { get; set; }

            [JsonProperty("107b800")]
            public bool _107b800 { get; set; }

            [JsonProperty("10803")]
            public bool _10803 { get; set; }

            [JsonProperty("10800")]
            public bool _10800 { get; set; }

            [JsonProperty("4a0e0")]
            public bool _4a0e0 { get; set; }

            [JsonProperty("18c60000")]
            public bool _18c60000 { get; set; }

            [JsonProperty("12738000")]
            public bool _12738000 { get; set; }

            [JsonProperty("10860")]
            public bool _10860 { get; set; }

            [JsonProperty("10863")]
            public bool _10863 { get; set; }

            [JsonProperty("c730000")]
            public bool C730000 { get; set; }

            [JsonProperty("110c00")]
            public bool _110c00 { get; set; }

            [JsonProperty("110c01")]
            public bool _110c01 { get; set; }

            [JsonProperty("300040")]
            public bool _300040 { get; set; }

            [JsonProperty("300043")]
            public bool _300043 { get; set; }

            [JsonProperty("420e")]
            public bool _420e { get; set; }

            [JsonProperty("10842")]
            public bool _10842 { get; set; }

            [JsonProperty("10840")]
            public bool _10840 { get; set; }

            [JsonProperty("1cc58000")]
            public bool _1cc58000 { get; set; }

            [JsonProperty("10c61")]
            public bool _10c61 { get; set; }

            [JsonProperty("10c60")]
            public bool _10c60 { get; set; }

            [JsonProperty("ca6")]
            public bool Ca6 { get; set; }

            [JsonProperty("ca0")]
            public bool Ca0 { get; set; }

            [JsonProperty("74a4e0")]
            public bool _74a4e0 { get; set; }

            [JsonProperty("e73a400")]
            public bool E73a400 { get; set; }

            [JsonProperty("300060")]
            public bool _300060 { get; set; }

            [JsonProperty("12049ce0")]
            public bool _12049ce0 { get; set; }

            [JsonProperty("20a5")]
            public bool _20a5 { get; set; }

            [JsonProperty("1ed60000")]
            public bool _1ed60000 { get; set; }

            [JsonProperty("10c41")]
            public bool _10c41 { get; set; }

            [JsonProperty("110c40")]
            public bool _110c40 { get; set; }

            [JsonProperty("300000")]
            public bool _300000 { get; set; }

            [JsonProperty("300003")]
            public bool _300003 { get; set; }

            [JsonProperty("8398e0")]
            public bool _8398e0 { get; set; }

            [JsonProperty("20c6")]
            public bool _20c6 { get; set; }

            [JsonProperty("8429884")]
            public bool _8429884 { get; set; }

            [JsonProperty("418c0")]
            public bool _418c0 { get; set; }

            [JsonProperty("418c6")]
            public bool _418c6 { get; set; }

            [JsonProperty("218c20")]
            public bool _218c20 { get; set; }

            [JsonProperty("a529400")]
            public bool A529400 { get; set; }

            [JsonProperty("2208c20")]
            public bool _2208c20 { get; set; }

            [JsonProperty("107b400")]
            public bool _107b400 { get; set; }

            [JsonProperty("601c00")]
            public bool _601c00 { get; set; }

            [JsonProperty("a510000")]
            public bool A510000 { get; set; }

            [JsonProperty("6418000")]
            public bool _6418000 { get; set; }

            [JsonProperty("e848000")]
            public bool E848000 { get; set; }

            [JsonProperty("1ad78000")]
            public bool _1ad78000 { get; set; }

            [JsonProperty("110c60")]
            public bool _110c60 { get; set; }

            [JsonProperty("110c61")]
            public bool _110c61 { get; set; }

            [JsonProperty("4e71")]
            public bool _4e71 { get; set; }

            [JsonProperty("218c00")]
            public bool _218c00 { get; set; }

            [JsonProperty("19460")]
            public bool _19460 { get; set; }

            [JsonProperty("10c01")]
            public bool _10c01 { get; set; }

            [JsonProperty("10c00")]
            public bool _10c00 { get; set; }

            [JsonProperty("10c03")]
            public bool _10c03 { get; set; }

            [JsonProperty("14a58000")]
            public bool _14a58000 { get; set; }

            [JsonProperty("511002")]
            public bool _511002 { get; set; }

            [JsonProperty("7420c0")]
            public bool _7420c0 { get; set; }

            [JsonProperty("10628000")]
            public bool _10628000 { get; set; }

            [JsonProperty("939ce0")]
            public bool _939ce0 { get; set; }

            [JsonProperty("601d00")]
            public bool _601d00 { get; set; }

            [JsonProperty("601d08")]
            public bool _601d08 { get; set; }

            [JsonProperty("400c80")]
            public bool _400c80 { get; set; }

            [JsonProperty("218000")]
            public bool _218000 { get; set; }

            [JsonProperty("c529400")]
            public bool C529400 { get; set; }

            [JsonProperty("e738000")]
            public bool E738000 { get; set; }

            [JsonProperty("4010440")]
            public bool _4010440 { get; set; }

            [JsonProperty("63a000")]
            public bool _63a000 { get; set; }

            [JsonProperty("400000")]
            public bool _400000 { get; set; }

            [JsonProperty("10004")]
            public bool _10004 { get; set; }

            [JsonProperty("10002")]
            public bool _10002 { get; set; }

            [JsonProperty("10003")]
            public bool _10003 { get; set; }

            [JsonProperty("10000")]
            public bool _10000 { get; set; }

            [JsonProperty("4a4e0")]
            public bool _4a4e0 { get; set; }

            [JsonProperty("4a4e7")]
            public bool _4a4e7 { get; set; }

            [JsonProperty("a4a000")]
            public bool A4a000 { get; set; }

            [JsonProperty("431800")]
            public bool _431800 { get; set; }

            [JsonProperty("23188000")]
            public bool _23188000 { get; set; }

            [JsonProperty("8000004")]
            public bool _8000004 { get; set; }

            [JsonProperty("a529c00")]
            public bool A529c00 { get; set; }

            [JsonProperty("c632106")]
            public bool C632106 { get; set; }

            [JsonProperty("18c5a400")]
            public bool _18c5a400 { get; set; }

            [JsonProperty("8530000")]
            public bool _8530000 { get; set; }

            [JsonProperty("100860")]
            public bool _100860 { get; set; }

            [JsonProperty("110002")]
            public bool _110002 { get; set; }

            [JsonProperty("110000")]
            public bool _110000 { get; set; }

            [JsonProperty("6429460")]
            public bool _6429460 { get; set; }

            [JsonProperty("60140")]
            public bool _60140 { get; set; }

            [JsonProperty("320c63")]
            public bool _320c63 { get; set; }

            [JsonProperty("18020")]
            public bool _18020 { get; set; }

            [JsonProperty("100840")]
            public bool _100840 { get; set; }

            [JsonProperty("a4ac00")]
            public bool A4ac00 { get; set; }

            [JsonProperty("e740000")]
            public bool E740000 { get; set; }

            [JsonProperty("f73180")]
            public bool F73180 { get; set; }

            [JsonProperty("f7318c")]
            public bool F7318c { get; set; }

            [JsonProperty("210000")]
            public bool _210000 { get; set; }

            [JsonProperty("18d70000")]
            public bool _18d70000 { get; set; }

            [JsonProperty("a510800")]
            public bool A510800 { get; set; }

            [JsonProperty("520080")]
            public bool _520080 { get; set; }

            [JsonProperty("842000")]
            public bool _842000 { get; set; }

            [JsonProperty("8421000")]
            public bool _8421000 { get; set; }

            [JsonProperty("18005")]
            public bool _18005 { get; set; }

            [JsonProperty("18000")]
            public bool _18000 { get; set; }

            [JsonProperty("431880")]
            public bool _431880 { get; set; }

            [JsonProperty("2318000")]
            public bool _2318000 { get; set; }

            [JsonProperty("42100")]
            public bool _42100 { get; set; }

            [JsonProperty("e720000")]
            public bool E720000 { get; set; }

            [JsonProperty("4194c0")]
            public bool _4194c0 { get; set; }

            [JsonProperty("a4a900")]
            public bool A4a900 { get; set; }

            [JsonProperty("6100800")]
            public bool _6100800 { get; set; }

            [JsonProperty("308840")]
            public bool _308840 { get; set; }

            [JsonProperty("100800")]
            public bool _100800 { get; set; }

            [JsonProperty("110060")]
            public bool _110060 { get; set; }

            [JsonProperty("629c00")]
            public bool _629c00 { get; set; }

            [JsonProperty("320c00")]
            public bool _320c00 { get; set; }

            [JsonProperty("a4a920")]
            public bool A4a920 { get; set; }

            [JsonProperty("6100860")]
            public bool _6100860 { get; set; }

            [JsonProperty("2318c20")]
            public bool _2318c20 { get; set; }

            [JsonProperty("c63800")]
            public bool C63800 { get; set; }

            [JsonProperty("4311000")]
            public bool _4311000 { get; set; }

            [JsonProperty("4211082")]
            public bool _4211082 { get; set; }

            [JsonProperty("4211080")]
            public bool _4211080 { get; set; }

            [JsonProperty("210880")]
            public bool _210880 { get; set; }

            [JsonProperty("42008")]
            public bool _42008 { get; set; }

            [JsonProperty("63000a5")]
            public bool _63000a5 { get; set; }

            [JsonProperty("500ca0")]
            public bool _500ca0 { get; set; }

            [JsonProperty("a49ce0")]
            public bool A49ce0 { get; set; }

            [JsonProperty("a49ce7")]
            public bool A49ce7 { get; set; }

            [JsonProperty("2318c00")]
            public bool _2318c00 { get; set; }

            [JsonProperty("8421084")]
            public bool _8421084 { get; set; }

            [JsonProperty("8421884")]
            public bool _8421884 { get; set; }

            [JsonProperty("c63180")]
            public bool C63180 { get; set; }

            [JsonProperty("29ce0")]
            public bool _29ce0 { get; set; }

            [JsonProperty("29ce5")]
            public bool _29ce5 { get; set; }

            [JsonProperty("842908")]
            public bool _842908 { get; set; }

            [JsonProperty("8531800")]
            public bool _8531800 { get; set; }

            [JsonProperty("100060")]
            public bool _100060 { get; set; }

            [JsonProperty("c7420c0")]
            public bool C7420c0 { get; set; }

            [JsonProperty("f73000")]
            public bool F73000 { get; set; }

            [JsonProperty("e629c00")]
            public bool E629c00 { get; set; }

            [JsonProperty("100c03")]
            public bool _100c03 { get; set; }

            [JsonProperty("6ad6b")]
            public bool _6ad6b { get; set; }

            [JsonProperty("318c20")]
            public bool _318c20 { get; set; }

            [JsonProperty("18820")]
            public bool _18820 { get; set; }

            [JsonProperty("41cc5")]
            public bool _41cc5 { get; set; }

            [JsonProperty("c732120")]
            public bool C732120 { get; set; }

            [JsonProperty("2000c60")]
            public bool _2000c60 { get; set; }

            [JsonProperty("210800")]
            public bool _210800 { get; set; }

            [JsonProperty("c4a400")]
            public bool C4a400 { get; set; }

            [JsonProperty("100c60")]
            public bool _100c60 { get; set; }

            [JsonProperty("8421800")]
            public bool _8421800 { get; set; }

            [JsonProperty("1084a948")]
            public bool _1084a948 { get; set; }

            [JsonProperty("318c00")]
            public bool _318c00 { get; set; }

            [JsonProperty("18800")]
            public bool _18800 { get; set; }

            [JsonProperty("6628000")]
            public bool _6628000 { get; set; }

            [JsonProperty("16b60000")]
            public bool _16b60000 { get; set; }

            [JsonProperty("2000c40")]
            public bool _2000c40 { get; set; }

            [JsonProperty("310800")]
            public bool _310800 { get; set; }

            [JsonProperty("310802")]
            public bool _310802 { get; set; }

            [JsonProperty("a629c00")]
            public bool A629c00 { get; set; }

            [JsonProperty("1084a800")]
            public bool _1084a800 { get; set; }

            [JsonProperty("318c60")]
            public bool _318c60 { get; set; }

            [JsonProperty("6318c63")]
            public bool _6318c63 { get; set; }

            [JsonProperty("100000")]
            public bool _100000 { get; set; }

            [JsonProperty("6318000")]
            public bool _6318000 { get; set; }

            [JsonProperty("200a0")]
            public bool _200a0 { get; set; }

            [JsonProperty("210840")]
            public bool _210840 { get; set; }

            [JsonProperty("c63140")]
            public bool C63140 { get; set; }

            [JsonProperty("ea3a400")]
            public bool Ea3a400 { get; set; }

            [JsonProperty("8531880")]
            public bool _8531880 { get; set; }

            [JsonProperty("8531884")]
            public bool _8531884 { get; set; }

            [JsonProperty("24a5")]
            public bool _24a5 { get; set; }

            [JsonProperty("49c00")]
            public bool _49c00 { get; set; }

            [JsonProperty("c62d80")]
            public bool C62d80 { get; set; }

            [JsonProperty("c63000")]
            public bool C63000 { get; set; }

            [JsonProperty("9420c0")]
            public bool _9420c0 { get; set; }

            [JsonProperty("24c7")]
            public bool _24c7 { get; set; }

            [JsonProperty("a52a0a5")]
            public bool A52a0a5 { get; set; }

            [JsonProperty("4a8e0")]
            public bool _4a8e0 { get; set; }

            [JsonProperty("842")]
            public bool _842 { get; set; }

            [JsonProperty("840")]
            public bool _840 { get; set; }

            [JsonProperty("2ce7")]
            public bool _2ce7 { get; set; }

            [JsonProperty("8400004")]
            public bool _8400004 { get; set; }

            [JsonProperty("24e5")]
            public bool _24e5 { get; set; }

            [JsonProperty("24e6")]
            public bool _24e6 { get; set; }

            [JsonProperty("24e7")]
            public bool _24e7 { get; set; }

            [JsonProperty("24e0")]
            public bool _24e0 { get; set; }

            [JsonProperty("24e9")]
            public bool _24e9 { get; set; }

            [JsonProperty("952c00")]
            public bool _952c00 { get; set; }

            [JsonProperty("28e5")]
            public bool _28e5 { get; set; }

            [JsonProperty("28e7")]
            public bool _28e7 { get; set; }

            [JsonProperty("e948000")]
            public bool E948000 { get; set; }

            [JsonProperty("a52108")]
            public bool A52108 { get; set; }

            [JsonProperty("14a60000")]
            public bool _14a60000 { get; set; }

            [JsonProperty("110380c")]
            public bool _110380c { get; set; }

            [JsonProperty("9018c6")]
            public bool _9018c6 { get; set; }

            [JsonProperty("50100")]
            public bool _50100 { get; set; }

            [JsonProperty("318800")]
            public bool _318800 { get; set; }

            [JsonProperty("128cc00")]
            public bool _128cc00 { get; set; }

            [JsonProperty("860")]
            public bool _860 { get; set; }

            [JsonProperty("863")]
            public bool _863 { get; set; }

            [JsonProperty("862")]
            public bool _862 { get; set; }

            [JsonProperty("865")]
            public bool _865 { get; set; }

            [JsonProperty("107b5ad")]
            public bool _107b5ad { get; set; }

            [JsonProperty("107b5a0")]
            public bool _107b5a0 { get; set; }

            [JsonProperty("94ad60")]
            public bool _94ad60 { get; set; }

            [JsonProperty("298c4")]
            public bool _298c4 { get; set; }

            [JsonProperty("49d00")]
            public bool _49d00 { get; set; }

            [JsonProperty("18c63800")]
            public bool _18c63800 { get; set; }

            [JsonProperty("16c58000")]
            public bool _16c58000 { get; set; }

            [JsonProperty("952900")]
            public bool _952900 { get; set; }

            [JsonProperty("800")]
            public bool _800 { get; set; }

            [JsonProperty("d6294a")]
            public bool D6294a { get; set; }

            [JsonProperty("d62940")]
            public bool D62940 { get; set; }

            [JsonProperty("b6b400")]
            public bool B6b400 { get; set; }

            [JsonProperty("2108000")]
            public bool _2108000 { get; set; }

            [JsonProperty("49d20")]
            public bool _49d20 { get; set; }

            [JsonProperty("539c00")]
            public bool _539c00 { get; set; }

            [JsonProperty("e8")]
            public bool E8 { get; set; }

            [JsonProperty("e700000")]
            public bool E700000 { get; set; }

            [JsonProperty("e700007")]
            public bool E700007 { get; set; }

            [JsonProperty("31c07")]
            public bool _31c07 { get; set; }

            [JsonProperty("31c00")]
            public bool _31c00 { get; set; }

            [JsonProperty("5a529")]
            public bool _5a529 { get; set; }

            [JsonProperty("31806")]
            public bool _31806 { get; set; }

            [JsonProperty("31800")]
            public bool _31800 { get; set; }

            [JsonProperty("31480")]
            public bool _31480 { get; set; }

            [JsonProperty("5ad29")]
            public bool _5ad29 { get; set; }

            [JsonProperty("5ad20")]
            public bool _5ad20 { get; set; }

            [JsonProperty("c0")]
            public bool C0 { get; set; }

            [JsonProperty("94a400")]
            public bool _94a400 { get; set; }

            [JsonProperty("200c00")]
            public bool _200c00 { get; set; }

            [JsonProperty("3a106")]
            public bool _3a106 { get; set; }

            [JsonProperty("3a100")]
            public bool _3a100 { get; set; }

            [JsonProperty("a0")]
            public bool A0 { get; set; }

            [JsonProperty("a6")]
            public bool A6 { get; set; }

            [JsonProperty("a5")]
            public bool A5 { get; set; }

            [JsonProperty("639c00")]
            public bool _639c00 { get; set; }

            [JsonProperty("e83a400")]
            public bool E83a400 { get; set; }

            [JsonProperty("5a569")]
            public bool _5a569 { get; set; }

            [JsonProperty("200800")]
            public bool _200800 { get; set; }

            [JsonProperty("a5b120")]
            public bool A5b120 { get; set; }

            [JsonProperty("631400")]
            public bool _631400 { get; set; }

            [JsonProperty("194c3")]
            public bool _194c3 { get; set; }

            [JsonProperty("742000")]
            public bool _742000 { get; set; }

            [JsonProperty("1294a400")]
            public bool _1294a400 { get; set; }

            [JsonProperty("40100")]
            public bool _40100 { get; set; }

            [JsonProperty("18c58000")]
            public bool _18c58000 { get; set; }

            [JsonProperty("7018e0")]
            public bool _7018e0 { get; set; }

            [JsonProperty("630080")]
            public bool _630080 { get; set; }

            [JsonProperty("d6b56b")]
            public bool D6b56b { get; set; }

            [JsonProperty("d6b560")]
            public bool D6b560 { get; set; }

            [JsonProperty("d6b400")]
            public bool D6b400 { get; set; }

            [JsonProperty("294a5")]
            public bool _294a5 { get; set; }

            [JsonProperty("12a5ad20")]
            public bool _12a5ad20 { get; set; }

            [JsonProperty("12a5ad29")]
            public bool _12a5ad29 { get; set; }

            [JsonProperty("e84a4e0")]
            public bool E84a4e0 { get; set; }

            [JsonProperty("31c80")]
            public bool _31c80 { get; set; }

            [JsonProperty("194a3")]
            public bool _194a3 { get; set; }

            [JsonProperty("e03800")]
            public bool E03800 { get; set; }

            [JsonProperty("5a920")]
            public bool _5a920 { get; set; }

            [JsonProperty("3a520")]
            public bool _3a520 { get; set; }

            [JsonProperty("31884")]
            public bool _31884 { get; set; }

            [JsonProperty("31880")]
            public bool _31880 { get; set; }

            [JsonProperty("c62c00")]
            public bool C62c00 { get; set; }

            [JsonProperty("1cc60000")]
            public bool _1cc60000 { get; set; }

            [JsonProperty("1842")]
            public bool _1842 { get; set; }

            [JsonProperty("1294a529")]
            public bool _1294a529 { get; set; }

            [JsonProperty("9424e0")]
            public bool _9424e0 { get; set; }

            [JsonProperty("200c80")]
            public bool _200c80 { get; set; }

            [JsonProperty("318000")]
            public bool _318000 { get; set; }

            [JsonProperty("25290000")]
            public bool _25290000 { get; set; }

            [JsonProperty("d68160")]
            public bool D68160 { get; set; }

            [JsonProperty("200080")]
            public bool _200080 { get; set; }

            [JsonProperty("22003800")]
            public bool _22003800 { get; set; }

            [JsonProperty("18c63000")]
            public bool _18c63000 { get; set; }

            [JsonProperty("831400")]
            public bool _831400 { get; set; }

            [JsonProperty("838000")]
            public bool _838000 { get; set; }

            [JsonProperty("e83a540")]
            public bool E83a540 { get; set; }

            [JsonProperty("a501080")]
            public bool A501080 { get; set; }

            [JsonProperty("418c00")]
            public bool _418c00 { get; set; }

            [JsonProperty("1c84")]
            public bool _1c84 { get; set; }

            [JsonProperty("421800")]
            public bool _421800 { get; set; }

            [JsonProperty("a628000")]
            public bool A628000 { get; set; }

            [JsonProperty("200040")]
            public bool _200040 { get; set; }

            [JsonProperty("801c0")]
            public bool _801c0 { get; set; }

            [JsonProperty("5b1c0")]
            public bool _5b1c0 { get; set; }

            [JsonProperty("74a008")]
            public bool _74a008 { get; set; }

            [JsonProperty("1d29")]
            public bool _1d29 { get; set; }

            [JsonProperty("1d20")]
            public bool _1d20 { get; set; }

            [JsonProperty("501440")]
            public bool _501440 { get; set; }

            [JsonProperty("8628000")]
            public bool _8628000 { get; set; }

            [JsonProperty("1900")]
            public bool _1900 { get; set; }

            [JsonProperty("1906")]
            public bool _1906 { get; set; }

            [JsonProperty("418400")]
            public bool _418400 { get; set; }

            [JsonProperty("a538000")]
            public bool A538000 { get; set; }

            [JsonProperty("200000")]
            public bool _200000 { get; set; }

            [JsonProperty("730000")]
            public bool _730000 { get; set; }

            [JsonProperty("320ca0")]
            public bool _320ca0 { get; set; }

            [JsonProperty("1d08")]
            public bool _1d08 { get; set; }

            [JsonProperty("1d07")]
            public bool _1d07 { get; set; }

            [JsonProperty("1d05")]
            public bool _1d05 { get; set; }

            [JsonProperty("1d00")]
            public bool _1d00 { get; set; }

            [JsonProperty("1886")]
            public bool _1886 { get; set; }

            [JsonProperty("1884")]
            public bool _1884 { get; set; }

            [JsonProperty("1082")]
            public bool _1082 { get; set; }

            [JsonProperty("1080")]
            public bool _1080 { get; set; }

            [JsonProperty("1084")]
            public bool _1084 { get; set; }

            [JsonProperty("10852900")]
            public bool _10852900 { get; set; }

            [JsonProperty("741c00")]
            public bool _741c00 { get; set; }

            [JsonProperty("418420")]
            public bool _418420 { get; set; }

            [JsonProperty("418421")]
            public bool _418421 { get; set; }

            [JsonProperty("420c0")]
            public bool _420c0 { get; set; }

            [JsonProperty("420c6")]
            public bool _420c6 { get; set; }

            [JsonProperty("602106")]
            public bool _602106 { get; set; }

            [JsonProperty("638000")]
            public bool _638000 { get; set; }

            [JsonProperty("74000")]
            public bool _74000 { get; set; }

            [JsonProperty("1c00")]
            public bool _1c00 { get; set; }

            [JsonProperty("6110800")]
            public bool _6110800 { get; set; }

            [JsonProperty("501400")]
            public bool _501400 { get; set; }

            [JsonProperty("c700000")]
            public bool C700000 { get; set; }

            [JsonProperty("421080")]
            public bool _421080 { get; set; }

            [JsonProperty("53180")]
            public bool _53180 { get; set; }

            [JsonProperty("e739ce7")]
            public bool E739ce7 { get; set; }

            [JsonProperty("e739ce0")]
            public bool E739ce0 { get; set; }

            [JsonProperty("1c63")]
            public bool _1c63 { get; set; }

            [JsonProperty("1d40")]
            public bool _1d40 { get; set; }

            [JsonProperty("a539c00")]
            public bool A539c00 { get; set; }

            [JsonProperty("1044")]
            public bool _1044 { get; set; }

            [JsonProperty("1042")]
            public bool _1042 { get; set; }

            [JsonProperty("1040")]
            public bool _1040 { get; set; }

            [JsonProperty("18e70000")]
            public bool _18e70000 { get; set; }

            [JsonProperty("4a108")]
            public bool _4a108 { get; set; }

            [JsonProperty("30080")]
            public bool _30080 { get; set; }

            [JsonProperty("2110000")]
            public bool _2110000 { get; set; }

            [JsonProperty("254a")]
            public bool _254a { get; set; }

            [JsonProperty("2547")]
            public bool _2547 { get; set; }

            [JsonProperty("2540")]
            public bool _2540 { get; set; }

            [JsonProperty("2549")]
            public bool _2549 { get; set; }

            [JsonProperty("1000")]
            public bool _1000 { get; set; }

            [JsonProperty("531800")]
            public bool _531800 { get; set; }

            [JsonProperty("10a42900")]
            public bool _10a42900 { get; set; }

            [JsonProperty("2529")]
            public bool _2529 { get; set; }

            [JsonProperty("2527")]
            public bool _2527 { get; set; }

            [JsonProperty("a520000")]
            public bool A520000 { get; set; }

            [JsonProperty("421000")]
            public bool _421000 { get; set; }

            [JsonProperty("84218c4")]
            public bool _84218c4 { get; set; }

            [JsonProperty("2500")]
            public bool _2500 { get; set; }

            [JsonProperty("2507")]
            public bool _2507 { get; set; }

            [JsonProperty("a50000")]
            public bool A50000 { get; set; }

            [JsonProperty("10ca0")]
            public bool _10ca0 { get; set; }

            [JsonProperty("14a5294a")]
            public bool _14a5294a { get; set; }

            [JsonProperty("14c58000")]
            public bool _14c58000 { get; set; }

            [JsonProperty("421040")]
            public bool _421040 { get; set; }

            [JsonProperty("421042")]
            public bool _421042 { get; set; }

            [JsonProperty("a629ca0")]
            public bool A629ca0 { get; set; }

            [JsonProperty("e93a4e0")]
            public bool E93a4e0 { get; set; }

            [JsonProperty("52d69")]
            public bool _52d69 { get; set; }

            [JsonProperty("ea4a800")]
            public bool Ea4a800 { get; set; }

            [JsonProperty("41d00")]
            public bool _41d00 { get; set; }

            [JsonProperty("49ca0")]
            public bool _49ca0 { get; set; }

            [JsonProperty("4421040")]
            public bool _4421040 { get; set; }

            [JsonProperty("731c00")]
            public bool _731c00 { get; set; }

            [JsonProperty("2400")]
            public bool _2400 { get; set; }

            [JsonProperty("d73000")]
            public bool D73000 { get; set; }

            [JsonProperty("41d27")]
            public bool _41d27 { get; set; }

            [JsonProperty("c8420c0")]
            public bool C8420c0 { get; set; }

            [JsonProperty("8721884")]
            public bool _8721884 { get; set; }

            [JsonProperty("314a0")]
            public bool _314a0 { get; set; }

            [JsonProperty("73800")]
            public bool _73800 { get; set; }

            [JsonProperty("2d29")]
            public bool _2d29 { get; set; }

            [JsonProperty("498e6")]
            public bool _498e6 { get; set; }

            [JsonProperty("1484a800")]
            public bool _1484a800 { get; set; }

            [JsonProperty("429000")]
            public bool _429000 { get; set; }

            [JsonProperty("52948")]
            public bool _52948 { get; set; }

            [JsonProperty("49ce8")]
            public bool _49ce8 { get; set; }

            [JsonProperty("49ce7")]
            public bool _49ce7 { get; set; }

            [JsonProperty("49ce0")]
            public bool _49ce0 { get; set; }

            [JsonProperty("2d4b")]
            public bool _2d4b { get; set; }

            [JsonProperty("14a5b180")]
            public bool _14a5b180 { get; set; }

            [JsonProperty("14a5b18a")]
            public bool _14a5b18a { get; set; }

            [JsonProperty("6300840")]
            public bool _6300840 { get; set; }

            [JsonProperty("31ce0")]
            public bool _31ce0 { get; set; }

            [JsonProperty("31ce5")]
            public bool _31ce5 { get; set; }

            [JsonProperty("2d6b")]
            public bool _2d6b { get; set; }

            [JsonProperty("2d69")]
            public bool _2d69 { get; set; }

            [JsonProperty("4201080")]
            public bool _4201080 { get; set; }

            [JsonProperty("94a4e0")]
            public bool _94a4e0 { get; set; }

            [JsonProperty("2219000")]
            public bool _2219000 { get; set; }

            [JsonProperty("6519460")]
            public bool _6519460 { get; set; }

            [JsonProperty("52900")]
            public bool _52900 { get; set; }

            [JsonProperty("52908")]
            public bool _52908 { get; set; }

            [JsonProperty("6300860")]
            public bool _6300860 { get; set; }

            [JsonProperty("424c0")]
            public bool _424c0 { get; set; }

            [JsonProperty("4a948")]
            public bool _4a948 { get; set; }

            [JsonProperty("4a940")]
            public bool _4a940 { get; set; }

            [JsonProperty("301000")]
            public bool _301000 { get; set; }

            [JsonProperty("10738000")]
            public bool _10738000 { get; set; }

            [JsonProperty("a730000")]
            public bool A730000 { get; set; }

            [JsonProperty("d00000")]
            public bool D00000 { get; set; }

            [JsonProperty("408c61")]
            public bool _408c61 { get; set; }

            [JsonProperty("7340b")]
            public bool _7340b { get; set; }

            [JsonProperty("2118c00")]
            public bool _2118c00 { get; set; }

            [JsonProperty("4220000")]
            public bool _4220000 { get; set; }

            [JsonProperty("1f188000")]
            public bool _1f188000 { get; set; }

            [JsonProperty("20440")]
            public bool _20440 { get; set; }

            [JsonProperty("23180000")]
            public bool _23180000 { get; set; }

            [JsonProperty("2118c20")]
            public bool _2118c20 { get; set; }

            [JsonProperty("a5a400")]
            public bool A5a400 { get; set; }

            [JsonProperty("2118c41")]
            public bool _2118c41 { get; set; }

            [JsonProperty("a6294a0")]
            public bool A6294a0 { get; set; }

            [JsonProperty("a5ad20")]
            public bool A5ad20 { get; set; }

            [JsonProperty("3194a3")]
            public bool _3194a3 { get; set; }

            [JsonProperty("8021000")]
            public bool _8021000 { get; set; }

            [JsonProperty("29d00")]
            public bool _29d00 { get; set; }

            [JsonProperty("a5010a0")]
            public bool A5010a0 { get; set; }

            [JsonProperty("18c70000")]
            public bool _18c70000 { get; set; }

            [JsonProperty("529400")]
            public bool _529400 { get; set; }

            [JsonProperty("8314e0")]
            public bool _8314e0 { get; set; }

            [JsonProperty("29c00")]
            public bool _29c00 { get; set; }

            [JsonProperty("b52900")]
            public bool B52900 { get; set; }

            [JsonProperty("e548000")]
            public bool E548000 { get; set; }

            [JsonProperty("429800")]
            public bool _429800 { get; set; }

            [JsonProperty("6418400")]
            public bool _6418400 { get; set; }

            [JsonProperty("108c400")]
            public bool _108c400 { get; set; }

            [JsonProperty("380a0")]
            public bool _380a0 { get; set; }

            [JsonProperty("8631880")]
            public bool _8631880 { get; set; }

            [JsonProperty("6418420")]
            public bool _6418420 { get; set; }

            [JsonProperty("531ce0")]
            public bool _531ce0 { get; set; }

            [JsonProperty("531ce5")]
            public bool _531ce5 { get; set; }

            [JsonProperty("529c00")]
            public bool _529c00 { get; set; }

            [JsonProperty("118c400")]
            public bool _118c400 { get; set; }

            [JsonProperty("300c40")]
            public bool _300c40 { get; set; }

            [JsonProperty("529460")]
            public bool _529460 { get; set; }

            [JsonProperty("318a")]
            public bool _318a { get; set; }

            [JsonProperty("318c")]
            public bool _318c { get; set; }

            [JsonProperty("600000")]
            public bool _600000 { get; set; }

            [JsonProperty("e73800")]
            public bool E73800 { get; set; }

            [JsonProperty("1ce70000")]
            public bool _1ce70000 { get; set; }

            [JsonProperty("4110000")]
            public bool _4110000 { get; set; }

            [JsonProperty("e742527")]
            public bool E742527 { get; set; }

            [JsonProperty("12950000")]
            public bool _12950000 { get; set; }

            [JsonProperty("702000")]
            public bool _702000 { get; set; }

            [JsonProperty("c628000")]
            public bool C628000 { get; set; }

            [JsonProperty("314a")]
            public bool _314a { get; set; }

            [JsonProperty("528c00")]
            public bool _528c00 { get; set; }

            [JsonProperty("21042")]
            public bool _21042 { get; set; }

            [JsonProperty("21043")]
            public bool _21043 { get; set; }

            [JsonProperty("21040")]
            public bool _21040 { get; set; }

            [JsonProperty("3a4e0")]
            public bool _3a4e0 { get; set; }

            [JsonProperty("1cc5")]
            public bool _1cc5 { get; set; }

            [JsonProperty("1cc7")]
            public bool _1cc7 { get; set; }

            [JsonProperty("321400")]
            public bool _321400 { get; set; }

            [JsonProperty("2d6b0000")]
            public bool _2d6b0000 { get; set; }

            [JsonProperty("e742400")]
            public bool E742400 { get; set; }

            [JsonProperty("1ca7")]
            public bool _1ca7 { get; set; }

            [JsonProperty("1ca5")]
            public bool _1ca5 { get; set; }

            [JsonProperty("1ca0")]
            public bool _1ca0 { get; set; }

            [JsonProperty("3108")]
            public bool _3108 { get; set; }

            [JsonProperty("21000")]
            public bool _21000 { get; set; }

            [JsonProperty("e73400")]
            public bool E73400 { get; set; }

            [JsonProperty("8010004")]
            public bool _8010004 { get; set; }

            [JsonProperty("a518000")]
            public bool A518000 { get; set; }

            [JsonProperty("8520000")]
            public bool _8520000 { get; set; }

            [JsonProperty("628000")]
            public bool _628000 { get; set; }

            [JsonProperty("1ce0")]
            public bool _1ce0 { get; set; }

            [JsonProperty("1ce7")]
            public bool _1ce7 { get; set; }

            [JsonProperty("1ce5")]
            public bool _1ce5 { get; set; }

            [JsonProperty("628c60")]
            public bool _628c60 { get; set; }

            [JsonProperty("628c63")]
            public bool _628c63 { get; set; }

            [JsonProperty("6310000")]
            public bool _6310000 { get; set; }

            [JsonProperty("14a50000")]
            public bool _14a50000 { get; set; }

            [JsonProperty("e839ce0")]
            public bool E839ce0 { get; set; }

            [JsonProperty("12948000")]
            public bool _12948000 { get; set; }

            [JsonProperty("c63a000")]
            public bool C63a000 { get; set; }

            [JsonProperty("629460")]
            public bool _629460 { get; set; }

            [JsonProperty("839c0")]
            public bool _839c0 { get; set; }

            [JsonProperty("881e0")]
            public bool _881e0 { get; set; }

            [JsonProperty("528060")]
            public bool _528060 { get; set; }

            [JsonProperty("6318c0")]
            public bool _6318c0 { get; set; }

            [JsonProperty("51ce7")]
            public bool _51ce7 { get; set; }

            [JsonProperty("110803")]
            public bool _110803 { get; set; }

            [JsonProperty("110800")]
            public bool _110800 { get; set; }

            [JsonProperty("10a5")]
            public bool _10a5 { get; set; }

            [JsonProperty("10a4")]
            public bool _10a4 { get; set; }

            [JsonProperty("10a2")]
            public bool _10a2 { get; set; }

            [JsonProperty("e730000")]
            public bool E730000 { get; set; }

            [JsonProperty("63194a3")]
            public bool _63194a3 { get; set; }

            [JsonProperty("21082")]
            public bool _21082 { get; set; }

            [JsonProperty("6428000")]
            public bool _6428000 { get; set; }

            [JsonProperty("83a4e0")]
            public bool _83a4e0 { get; set; }

            [JsonProperty("321040")]
            public bool _321040 { get; set; }

            [JsonProperty("e83a4e0")]
            public bool E83a4e0 { get; set; }

            [JsonProperty("628c00")]
            public bool _628c00 { get; set; }

            [JsonProperty("8328000")]
            public bool _8328000 { get; set; }

            [JsonProperty("1ec60000")]
            public bool _1ec60000 { get; set; }

            [JsonProperty("431400")]
            public bool _431400 { get; set; }

            [JsonProperty("318c0")]
            public bool _318c0 { get; set; }

            [JsonProperty("110840")]
            public bool _110840 { get; set; }

            [JsonProperty("c5a40b")]
            public bool C5a40b { get; set; }

            [JsonProperty("c5a400")]
            public bool C5a400 { get; set; }

            [JsonProperty("521040")]
            public bool _521040 { get; set; }

            [JsonProperty("10800000")]
            public bool _10800000 { get; set; }

            [JsonProperty("f74000")]
            public bool F74000 { get; set; }

            [JsonProperty("18e68000")]
            public bool _18e68000 { get; set; }

            [JsonProperty("a739ca0")]
            public bool A739ca0 { get; set; }

            [JsonProperty("5a4e0")]
            public bool _5a4e0 { get; set; }

            [JsonProperty("211080")]
            public bool _211080 { get; set; }

            [JsonProperty("c8320c0")]
            public bool C8320c0 { get; set; }

            [JsonProperty("c602000")]
            public bool C602000 { get; set; }

            [JsonProperty("19080")]
            public bool _19080 { get; set; }

            [JsonProperty("19082")]
            public bool _19082 { get; set; }

            [JsonProperty("70180")]
            public bool _70180 { get; set; }

            [JsonProperty("18e0")]
            public bool _18e0 { get; set; }

            [JsonProperty("18e4")]
            public bool _18e4 { get; set; }

            [JsonProperty("18e7")]
            public bool _18e7 { get; set; }

            [JsonProperty("18e6")]
            public bool _18e6 { get; set; }

            [JsonProperty("c5a529")]
            public bool C5a529 { get; set; }

            [JsonProperty("320800")]
            public bool _320800 { get; set; }

            [JsonProperty("18c6")]
            public bool _18c6 { get; set; }

            [JsonProperty("18c4")]
            public bool _18c4 { get; set; }

            [JsonProperty("3def")]
            public bool _3def { get; set; }

            [JsonProperty("3ded")]
            public bool _3ded { get; set; }

            [JsonProperty("400c0")]
            public bool _400c0 { get; set; }

            [JsonProperty("42527")]
            public bool _42527 { get; set; }

            [JsonProperty("3010a0")]
            public bool _3010a0 { get; set; }

            [JsonProperty("35ad")]
            public bool _35ad { get; set; }

            [JsonProperty("f03000")]
            public bool F03000 { get; set; }

            [JsonProperty("18a4")]
            public bool _18a4 { get; set; }

            [JsonProperty("400e0")]
            public bool _400e0 { get; set; }

            [JsonProperty("949c00")]
            public bool _949c00 { get; set; }

            [JsonProperty("211000")]
            public bool _211000 { get; set; }

            [JsonProperty("8420000")]
            public bool _8420000 { get; set; }

            [JsonProperty("111000")]
            public bool _111000 { get; set; }

            [JsonProperty("8210000")]
            public bool _8210000 { get; set; }

            [JsonProperty("10400")]
            public bool _10400 { get; set; }

            [JsonProperty("c83a000")]
            public bool C83a000 { get; set; }

            [JsonProperty("5018e0")]
            public bool _5018e0 { get; set; }

            [JsonProperty("4310040")]
            public bool _4310040 { get; set; }

            [JsonProperty("48000")]
            public bool _48000 { get; set; }

            [JsonProperty("73a4e0")]
            public bool _73a4e0 { get; set; }

            [JsonProperty("129cc00")]
            public bool _129cc00 { get; set; }

            [JsonProperty("310000")]
            public bool _310000 { get; set; }

            [JsonProperty("310002")]
            public bool _310002 { get; set; }

            [JsonProperty("39ce")]
            public bool _39ce { get; set; }

            [JsonProperty("39cc")]
            public bool _39cc { get; set; }

            [JsonProperty("2000043")]
            public bool _2000043 { get; set; }

            [JsonProperty("a639ca0")]
            public bool A639ca0 { get; set; }

            [JsonProperty("520840")]
            public bool _520840 { get; set; }

            [JsonProperty("520842")]
            public bool _520842 { get; set; }

            [JsonProperty("4310000")]
            public bool _4310000 { get; set; }

            [JsonProperty("5294a0")]
            public bool _5294a0 { get; set; }

            [JsonProperty("14a0")]
            public bool _14a0 { get; set; }

            [JsonProperty("14a3")]
            public bool _14a3 { get; set; }

            [JsonProperty("14a5")]
            public bool _14a5 { get; set; }

            [JsonProperty("14a7")]
            public bool _14a7 { get; set; }

            [JsonProperty("e838000")]
            public bool E838000 { get; set; }

            [JsonProperty("20078000")]
            public bool _20078000 { get; set; }

            [JsonProperty("b4a0e0")]
            public bool B4a0e0 { get; set; }

            [JsonProperty("c6420c0")]
            public bool C6420c0 { get; set; }

            [JsonProperty("c4ac00")]
            public bool C4ac00 { get; set; }

            [JsonProperty("18420")]
            public bool _18420 { get; set; }

            [JsonProperty("18421")]
            public bool _18421 { get; set; }

            [JsonProperty("831ce0")]
            public bool _831ce0 { get; set; }

            [JsonProperty("520800")]
            public bool _520800 { get; set; }

            [JsonProperty("280c6")]
            public bool _280c6 { get; set; }

            [JsonProperty("a4a800")]
            public bool A4a800 { get; set; }

            [JsonProperty("a4a807")]
            public bool A4a807 { get; set; }

            [JsonProperty("a4a808")]
            public bool A4a808 { get; set; }

            [JsonProperty("529ce0")]
            public bool _529ce0 { get; set; }

            [JsonProperty("8320800")]
            public bool _8320800 { get; set; }

            [JsonProperty("a539ca5")]
            public bool A539ca5 { get; set; }

            [JsonProperty("a539ca0")]
            public bool A539ca0 { get; set; }

            [JsonProperty("7b5ad")]
            public bool _7b5ad { get; set; }

            [JsonProperty("7b5a0")]
            public bool _7b5a0 { get; set; }

            [JsonProperty("5294e5")]
            public bool _5294e5 { get; set; }

            [JsonProperty("14e5")]
            public bool _14e5 { get; set; }

            [JsonProperty("a728000")]
            public bool A728000 { get; set; }

            [JsonProperty("4298c0")]
            public bool _4298c0 { get; set; }

            [JsonProperty("4298c4")]
            public bool _4298c4 { get; set; }

            [JsonProperty("10a50000")]
            public bool _10a50000 { get; set; }

            [JsonProperty("e94a4e0")]
            public bool E94a4e0 { get; set; }

            [JsonProperty("4000004")]
            public bool _4000004 { get; set; }

            [JsonProperty("a531c00")]
            public bool A531c00 { get; set; }

            [JsonProperty("6329463")]
            public bool _6329463 { get; set; }

            [JsonProperty("6329460")]
            public bool _6329460 { get; set; }

            [JsonProperty("1084000")]
            public bool _1084000 { get; set; }

            [JsonProperty("3a400")]
            public bool _3a400 { get; set; }

            [JsonProperty("e021000")]
            public bool E021000 { get; set; }

            [JsonProperty("8200c80")]
            public bool _8200c80 { get; set; }

            [JsonProperty("a4a400")]
            public bool A4a400 { get; set; }

            [JsonProperty("63a106")]
            public bool _63a106 { get; set; }

            [JsonProperty("839c00")]
            public bool _839c00 { get; set; }

            [JsonProperty("c000000")]
            public bool C000000 { get; set; }

            [JsonProperty("732000")]
            public bool _732000 { get; set; }

            [JsonProperty("23298000")]
            public bool _23298000 { get; set; }

            [JsonProperty("3214a3")]
            public bool _3214a3 { get; set; }

            [JsonProperty("3214a0")]
            public bool _3214a0 { get; set; }

            [JsonProperty("18c68000")]
            public bool _18c68000 { get; set; }

            [JsonProperty("4000043")]
            public bool _4000043 { get; set; }

            [JsonProperty("4000040")]
            public bool _4000040 { get; set; }

            [JsonProperty("31ca")]
            public bool _31ca { get; set; }

            [JsonProperty("c6b800")]
            public bool C6b800 { get; set; }

            [JsonProperty("8200004")]
            public bool _8200004 { get; set; }

            [JsonProperty("2108421")]
            public bool _2108421 { get; set; }

            [JsonProperty("631880")]
            public bool _631880 { get; set; }

            [JsonProperty("4000064")]
            public bool _4000064 { get; set; }

            [JsonProperty("108400")]
            public bool _108400 { get; set; }

            [JsonProperty("401")]
            public bool _401 { get; set; }

            [JsonProperty("10a58000")]
            public bool _10a58000 { get; set; }

            [JsonProperty("39c05")]
            public bool _39c05 { get; set; }

            [JsonProperty("39c00")]
            public bool _39c00 { get; set; }

            [JsonProperty("93a4e0")]
            public bool _93a4e0 { get; set; }

            [JsonProperty("18c63")]
            public bool _18c63 { get; set; }

            [JsonProperty("18c60")]
            public bool _18c60 { get; set; }

            [JsonProperty("39460")]
            public bool _39460 { get; set; }

            [JsonProperty("12a5ac00")]
            public bool _12a5ac00 { get; set; }

            [JsonProperty("631900")]
            public bool _631900 { get; set; }

            [JsonProperty("84298c0")]
            public bool _84298c0 { get; set; }

            [JsonProperty("84298c4")]
            public bool _84298c4 { get; set; }

            [JsonProperty("421")]
            public bool _421 { get; set; }

            [JsonProperty("5a120")]
            public bool _5a120 { get; set; }

            [JsonProperty("c40")]
            public bool C40 { get; set; }

            [JsonProperty("c41")]
            public bool C41 { get; set; }

            [JsonProperty("c43")]
            public bool C43 { get; set; }

            [JsonProperty("51d20")]
            public bool _51d20 { get; set; }

            [JsonProperty("2108c00")]
            public bool _2108c00 { get; set; }

            [JsonProperty("b5ad20")]
            public bool B5ad20 { get; set; }

            [JsonProperty("b5ad29")]
            public bool B5ad29 { get; set; }

            [JsonProperty("38c63")]
            public bool _38c63 { get; set; }

            [JsonProperty("12952d69")]
            public bool _12952d69 { get; set; }

            [JsonProperty("c63")]
            public bool C63 { get; set; }

            [JsonProperty("c60")]
            public bool C60 { get; set; }

            [JsonProperty("c61")]
            public bool C61 { get; set; }

            [JsonProperty("74a400")]
            public bool _74a400 { get; set; }

            [JsonProperty("18c23")]
            public bool _18c23 { get; set; }

            [JsonProperty("18c21")]
            public bool _18c21 { get; set; }

            [JsonProperty("18c20")]
            public bool _18c20 { get; set; }

            [JsonProperty("51d00")]
            public bool _51d00 { get; set; }

            [JsonProperty("464")]
            public bool _464 { get; set; }

            [JsonProperty("2108c61")]
            public bool _2108c61 { get; set; }

            [JsonProperty("c740000")]
            public bool C740000 { get; set; }

            [JsonProperty("4290c0")]
            public bool _4290c0 { get; set; }

            [JsonProperty("c04")]
            public bool C04 { get; set; }

            [JsonProperty("c00")]
            public bool C00 { get; set; }

            [JsonProperty("c03")]
            public bool C03 { get; set; }

            [JsonProperty("631800")]
            public bool _631800 { get; set; }

            [JsonProperty("a500085")]
            public bool A500085 { get; set; }

            [JsonProperty("a6394a0")]
            public bool A6394a0 { get; set; }

            [JsonProperty("8024e0")]
            public bool _8024e0 { get; set; }

            [JsonProperty("2018c00")]
            public bool _2018c00 { get; set; }

            [JsonProperty("742400")]
            public bool _742400 { get; set; }

            [JsonProperty("6319400")]
            public bool _6319400 { get; set; }

            [JsonProperty("739c00")]
            public bool _739c00 { get; set; }

            [JsonProperty("c21")]
            public bool C21 { get; set; }

            [JsonProperty("6320000")]
            public bool _6320000 { get; set; }

            [JsonProperty("a010000")]
            public bool A010000 { get; set; }

            [JsonProperty("311040")]
            public bool _311040 { get; set; }

            [JsonProperty("10601c00")]
            public bool _10601c00 { get; set; }

            [JsonProperty("4219082")]
            public bool _4219082 { get; set; }

            [JsonProperty("4210000")]
            public bool _4210000 { get; set; }

            [JsonProperty("84a4e0")]
            public bool _84a4e0 { get; set; }

            [JsonProperty("a5294a0")]
            public bool A5294a0 { get; set; }

            [JsonProperty("a5294a5")]
            public bool A5294a5 { get; set; }

            [JsonProperty("2100c00")]
            public bool _2100c00 { get; set; }

            [JsonProperty("a428000")]
            public bool A428000 { get; set; }

            [JsonProperty("4000840")]
            public bool _4000840 { get; set; }

            [JsonProperty("108000e8")]
            public bool _108000e8 { get; set; }

            [JsonProperty("311000")]
            public bool _311000 { get; set; }

            [JsonProperty("6b560")]
            public bool _6b560 { get; set; }

            [JsonProperty("d6bc00")]
            public bool D6bc00 { get; set; }

            [JsonProperty("c80")]
            public bool C80 { get; set; }

            [JsonProperty("c81")]
            public bool C81 { get; set; }

            [JsonProperty("1ef88000")]
            public bool _1ef88000 { get; set; }

            [JsonProperty("739400")]
            public bool _739400 { get; set; }

            [JsonProperty("83a400")]
            public bool _83a400 { get; set; }

            [JsonProperty("f7bda0")]
            public bool F7bda0 { get; set; }

            [JsonProperty("839400")]
            public bool _839400 { get; set; }

            [JsonProperty("1ef78000")]
            public bool _1ef78000 { get; set; }

            [JsonProperty("a500005")]
            public bool A500005 { get; set; }

            [JsonProperty("a5294e0")]
            public bool A5294e0 { get; set; }

            [JsonProperty("531400")]
            public bool _531400 { get; set; }

            [JsonProperty("742520")]
            public bool _742520 { get; set; }

            [JsonProperty("639800")]
            public bool _639800 { get; set; }

            [JsonProperty("a52940")]
            public bool A52940 { get; set; }

            [JsonProperty("c6000a6")]
            public bool C6000a6 { get; set; }

            [JsonProperty("12a58000")]
            public bool _12a58000 { get; set; }

            [JsonProperty("6328000")]
            public bool _6328000 { get; set; }

            [JsonProperty("d62800")]
            public bool D62800 { get; set; }

            [JsonProperty("6228000")]
            public bool _6228000 { get; set; }

            [JsonProperty("531c00")]
            public bool _531c00 { get; set; }

            [JsonProperty("14a5318a")]
            public bool _14a5318a { get; set; }

            [JsonProperty("a5b000")]
            public bool A5b000 { get; set; }

            [JsonProperty("38000")]
            public bool _38000 { get; set; }

            [JsonProperty("31084")]
            public bool _31084 { get; set; }

            [JsonProperty("31080")]
            public bool _31080 { get; set; }

            [JsonProperty("6294a")]
            public bool _6294a { get; set; }

            [JsonProperty("62940")]
            public bool _62940 { get; set; }

            [JsonProperty("8220880")]
            public bool _8220880 { get; set; }

            [JsonProperty("a52900")]
            public bool A52900 { get; set; }

            [JsonProperty("2310000")]
            public bool _2310000 { get; set; }

            [JsonProperty("68160")]
            public bool _68160 { get; set; }

            [JsonProperty("a530000")]
            public bool A530000 { get; set; }

            [JsonProperty("530000")]
            public bool _530000 { get; set; }

            [JsonProperty("749c00")]
            public bool _749c00 { get; set; }

            [JsonProperty("6529460")]
            public bool _6529460 { get; set; }

            [JsonProperty("8400c0")]
            public bool _8400c0 { get; set; }

            [JsonProperty("10000000")]
            public bool _10000000 { get; set; }

            [JsonProperty("62900")]
            public bool _62900 { get; set; }

            [JsonProperty("1ef70000")]
            public bool _1ef70000 { get; set; }

            [JsonProperty("6520000")]
            public bool _6520000 { get; set; }

            [JsonProperty("214a0")]
            public bool _214a0 { get; set; }

            [JsonProperty("214a3")]
            public bool _214a3 { get; set; }

            [JsonProperty("4321040")]
            public bool _4321040 { get; set; }

            [JsonProperty("a52800")]
            public bool A52800 { get; set; }

            [JsonProperty("94ac00")]
            public bool _94ac00 { get; set; }

            [JsonProperty("10848000")]
            public bool _10848000 { get; set; }

            [JsonProperty("8840")]
            public bool _8840 { get; set; }

            [JsonProperty("6110c61")]
            public bool _6110c61 { get; set; }

            [JsonProperty("10600000")]
            public bool _10600000 { get; set; }

            [JsonProperty("e529c00")]
            public bool E529c00 { get; set; }

            [JsonProperty("8420c0")]
            public bool _8420c0 { get; set; }

            [JsonProperty("419400")]
            public bool _419400 { get; set; }
        }
        #endregion
        #region Lessons | NOT COMPLETE
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
        #region Nonstop Play | NOT COMPLETE
        public class NSP_PlayListRoot2
        {
            public double NSP_PlayListArrangements { get; set; }
            public double NSP_PlayListTuning { get; set; }
            public double NSP_PlayListSongList { get; set; }
            public double NSP_PlayListPlayOrder { get; set; }
            public double Duration { get; set; }
        }
        #endregion
        #region Options | NOT COMPLETE
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
        #region Guitarcade | NOT COMPLETE

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

        #region Dynamic Difficulty | NOT COMPLETE
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
        #region Songs
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
        #region Session Mode | NOT COMPLETE
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
        #region Achievements | NOT COMPLETE
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
        #region Stats | NOT COMPLETE
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
        #region Score Attack | NOT COMPLETE
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

        public class V_List
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