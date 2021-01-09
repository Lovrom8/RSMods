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
                string jsonHandling = JSON_Handling(profileJSON, NameOfSection, serializedSection);
                string jsonToUTF8 = FormatJSONForUTF8(jsonHandling);
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
        private static string FormatJSONForUTF8(string json)
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

                result = wantedSection.ToObject<T>();
            }
            return result;
        }

        // I'm gonna be 100% honest. I don't know what this does.... At all...
        private static string JSON_Handling(string jsonProfile, string nameOfSection, string serializedSection)
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
        #region Missions
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

        public class DemoTones { }

        #region Play Nexts
        public class Playnexts
        {
            public Histories Histories { get; set; }
            public PlayNexts_Songs Songs { get; set; }
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

        public class PlayNexts_Songs
        {
            public Dictionary<string, PlayNexts_Song> SongDescription { get; set; }

            public double Version { get; set; }
        }

        public class PlayNexts_Song
        {
            public double TimeStamp { get; set; }

            [JsonProperty(Required = Required.AllowNull)]
            public Slot Slot1 { get; set; }

            [JsonProperty(Required = Required.AllowNull)]
            public Slot Slot2 { get; set; }

            [JsonProperty(Required = Required.AllowNull)]
            public Slot Slot3 { get; set; }
        }
        #endregion
        #region Recently Played Venues
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

        #region Chords
        public class Chords
        {
            public bool _6110c00 { get; set; }
            public bool a310000 { get; set; }
            public bool a729ca0 { get; set; }
            public bool _50000 { get; set; }
            public bool _8800 { get; set; }
            public bool _731000 { get; set; }
            public bool _480e0 { get; set; }
            public bool _1400 { get; set; }
            public bool _73140 { get; set; }
            public bool _848000 { get; set; }
            public bool _28060 { get; set; }
            public bool _2126 { get; set; }
            public bool _1465 { get; set; }
            public bool _1460 { get; set; }
            public bool _1463 { get; set; }
            public bool _9 { get; set; }
            public bool _7 { get; set; }
            public bool _4 { get; set; }
            public bool _3 { get; set; }
            public bool _2 { get; set; }
            public bool _0 { get; set; }
            public bool c { get; set; }
            public bool a { get; set; }
            public bool _6528000 { get; set; }
            public bool ea49ce0 { get; set; }
            public bool _62d40 { get; set; }
            public bool e73a4e0 { get; set; }
            public bool _2106 { get; set; }
            public bool _2108 { get; set; }
            public bool b63140 { get; set; }
            public bool _781a0 { get; set; }
            public bool _62540 { get; set; }
            public bool _10638000 { get; set; }
            public bool _1442 { get; set; }
            public double TimeStamp { get; set; }
            public bool _1cd60000 { get; set; }
            public bool _73180 { get; set; }
            public bool _7318c { get; set; }
            public bool _73980 { get; set; }
            public bool _12b58000 { get; set; }
            public bool a600000 { get; set; }
            public bool _1ef7b000 { get; set; }
            public bool a49c00 { get; set; }
            public bool a731c00 { get; set; }
            public bool _201080 { get; set; }
            public bool _28000 { get; set; }
            public bool a531ce5 { get; set; }
            public bool _731084 { get; set; }
            public bool _6000860 { get; set; }
            public bool a8394a5 { get; set; }
            public bool d6b000 { get; set; }
            public bool _4314c0 { get; set; }
            public bool _394a0 { get; set; }
            public bool _394a5 { get; set; }
            public bool _120c00 { get; set; }
            public bool b53000 { get; set; }
            public bool _2988 { get; set; }
            public bool _28c20 { get; set; }
            public bool _8040 { get; set; }
            public bool d63140 { get; set; }
            public bool _394c0 { get; set; }
            public bool a53000 { get; set; }
            public bool _6318c { get; set; }
            public bool _63180 { get; set; }
            public bool _39ce8 { get; set; }
            public bool _39ce0 { get; set; }
            public bool _39ce7 { get; set; }
            public bool a638000 { get; set; }
            public bool _8431880 { get; set; }
            public bool _8431884 { get; set; }
            public bool _839ca5 { get; set; }
            public bool c838000 { get; set; }
            public bool _28c60 { get; set; }
            public bool _28c61 { get; set; }
            public bool _28c63 { get; set; }
            public bool _8000 { get; set; }
            public bool _7bda0 { get; set; }
            public bool _218c0 { get; set; }
            public bool _58120 { get; set; }
            public bool a529ce5 { get; set; }
            public bool _28860 { get; set; }
            public bool _39ca5 { get; set; }
            public bool _39ca0 { get; set; }
            public bool _5280a { get; set; }
            public bool a018005 { get; set; }
            public bool _28842 { get; set; }
            public bool _1184000 { get; set; }
            public bool b52108 { get; set; }
            public bool _2908 { get; set; }
            public bool _2906 { get; set; }
            public bool _2110c00 { get; set; }
            public bool a529ca5 { get; set; }
            public bool _6318ca3 { get; set; }
            public bool _4a560 { get; set; }
            public bool _6000000 { get; set; }
            public bool _6000003 { get; set; }
            public bool _7424c0 { get; set; }
            public bool _739ca0 { get; set; }
            public bool _2948 { get; set; }
            public bool _294a { get; set; }
            public bool _2110c41 { get; set; }
            public bool _20c63 { get; set; }
            public bool _539ca0 { get; set; }
            public bool _63 { get; set; }
            public bool _61 { get; set; }
            public bool _60 { get; set; }
            public bool _64 { get; set; }
            public bool _6301400 { get; set; }
            public bool _63140 { get; set; }
            public bool _6314a { get; set; }
            public bool _20861 { get; set; }
            public bool _20860 { get; set; }
            public bool _2110c60 { get; set; }
            public bool _2110c61 { get; set; }
            public bool _20c40 { get; set; }
            public bool _40 { get; set; }
            public bool _43 { get; set; }
            public bool _4a520 { get; set; }
            public bool _950100 { get; set; }
            public bool _6000040 { get; set; }
            public bool _6000043 { get; set; }
            public bool _4014c0 { get; set; }
            public bool _739ce0 { get; set; }
            public bool _20840 { get; set; }
            public bool _20842 { get; set; }
            public bool c63a106 { get; set; }
            public bool _23 { get; set; }
            public bool _4014a0 { get; set; }
            public bool _23198000 { get; set; }
            public bool _29420 { get; set; }
            public bool _84a400 { get; set; }
            public bool _8394a0 { get; set; }
            public bool _8394a5 { get; set; }
            public bool _639ca0 { get; set; }
            public bool _329460 { get; set; }
            public bool _63214a3 { get; set; }
            public bool _20800 { get; set; }
            public bool _4320000 { get; set; }
            public bool a639c00 { get; set; }
            public bool _29400 { get; set; }
            public bool _219000 { get; set; }
            public bool _10a49c00 { get; set; }
            public bool _73a400 { get; set; }
            public bool _4221040 { get; set; }
            public bool _4221042 { get; set; }
            public bool _301400 { get; set; }
            public bool _1c0 { get; set; }
            public bool a738000 { get; set; }
            public bool _29460 { get; set; }
            public bool _29463 { get; set; }
            public bool _6300000 { get; set; }
            public bool _6300002 { get; set; }
            public bool _6300003 { get; set; }
            public bool _8521800 { get; set; }
            public bool _1694ad69 { get; set; }
            public bool _11000 { get; set; }
            public bool _11002 { get; set; }
            public bool _7000e0 { get; set; }
            public bool _801d00 { get; set; }
            public bool _600100 { get; set; }
            public bool a3a400 { get; set; }
            public bool _6398e0 { get; set; }
            public bool _749ce0 { get; set; }
            public bool _12a48000 { get; set; }
            public bool _942900 { get; set; }
            public bool _300840 { get; set; }
            public bool _6300040 { get; set; }
            public bool _6300043 { get; set; }
            public bool _84a800 { get; set; }
            public bool e84a400 { get; set; }
            public bool a4a4e7 { get; set; }
            public bool a4a4e0 { get; set; }
            public bool _20880 { get; set; }
            public bool _52108 { get; set; }
            public bool _52100 { get; set; }
            public bool _300860 { get; set; }
            public bool _6300060 { get; set; }
            public bool _9024e0 { get; set; }
            public bool b5b400 { get; set; }
            public bool _398b { get; set; }
            public bool _21400 { get; set; }
            public bool _429460 { get; set; }
            public bool _8521884 { get; set; }
            public bool _8521880 { get; set; }
            public bool _11082 { get; set; }
            public bool a521000 { get; set; }
            public bool _429400 { get; set; }
            public bool _20040 { get; set; }
            public bool _10842108 { get; set; }
            public bool _510800 { get; set; }
            public bool e530000 { get; set; }
            public bool _21440 { get; set; }
            public bool _4218c0 { get; set; }
            public bool _2118000 { get; set; }
            public bool c0000a6 { get; set; }
            public bool e739c00 { get; set; }
            public bool _4418000 { get; set; }
            public bool _118800 { get; set; }
            public bool _2218c20 { get; set; }
            public bool _20000 { get; set; }
            public bool d53000 { get; set; }
            public bool e73a527 { get; set; }
            public bool _4631 { get; set; }
            public bool _521460 { get; set; }
            public bool _462f { get; set; }
            public bool _106000e8 { get; set; }
            public bool a528000 { get; set; }
            public bool _521800 { get; set; }
            public bool _8429800 { get; set; }
            public bool _107b800 { get; set; }
            public bool _10803 { get; set; }
            public bool _10800 { get; set; }
            public bool _4a0e0 { get; set; }
            public bool _18c60000 { get; set; }
            public bool _12738000 { get; set; }
            public bool _10860 { get; set; }
            public bool _10863 { get; set; }
            public bool c730000 { get; set; }
            public bool _110c00 { get; set; }
            public bool _110c01 { get; set; }
            public bool _300040 { get; set; }
            public bool _300043 { get; set; }
            public bool _420e { get; set; }
            public bool _10842 { get; set; }
            public bool _10840 { get; set; }
            public bool _1cc58000 { get; set; }
            public bool _10c61 { get; set; }
            public bool _10c60 { get; set; }
            public bool ca6 { get; set; }
            public bool ca0 { get; set; }
            public bool _74a4e0 { get; set; }
            public bool e73a400 { get; set; }
            public bool _300060 { get; set; }
            public bool _12049ce0 { get; set; }
            public bool _20a5 { get; set; }
            public bool _1ed60000 { get; set; }
            public bool _10c41 { get; set; }
            public bool _110c40 { get; set; }
            public bool _300000 { get; set; }
            public bool _300003 { get; set; }
            public bool _8398e0 { get; set; }
            public bool _20c6 { get; set; }
            public bool _8429884 { get; set; }
            public bool _418c0 { get; set; }
            public bool _418c6 { get; set; }
            public bool _218c20 { get; set; }
            public bool a529400 { get; set; }
            public bool _2208c20 { get; set; }
            public bool _107b400 { get; set; }
            public bool _601c00 { get; set; }
            public bool a510000 { get; set; }
            public bool _6418000 { get; set; }
            public bool e848000 { get; set; }
            public bool _1ad78000 { get; set; }
            public bool _110c60 { get; set; }
            public bool _110c61 { get; set; }
            public bool _4e71 { get; set; }
            public bool _218c00 { get; set; }
            public bool _19460 { get; set; }
            public bool _10c01 { get; set; }
            public bool _10c00 { get; set; }
            public bool _10c03 { get; set; }
            public bool _14a58000 { get; set; }
            public bool _511002 { get; set; }
            public bool _7420c0 { get; set; }
            public bool _10628000 { get; set; }
            public bool _939ce0 { get; set; }
            public bool _601d00 { get; set; }
            public bool _601d08 { get; set; }
            public bool _400c80 { get; set; }
            public bool _218000 { get; set; }
            public bool c529400 { get; set; }
            public bool e738000 { get; set; }
            public bool _4010440 { get; set; }
            public bool _63a000 { get; set; }
            public bool _400000 { get; set; }
            public bool _10004 { get; set; }
            public bool _10002 { get; set; }
            public bool _10003 { get; set; }
            public bool _10000 { get; set; }
            public bool _4a4e0 { get; set; }
            public bool _4a4e7 { get; set; }
            public bool a4a000 { get; set; }
            public bool _431800 { get; set; }
            public bool _23188000 { get; set; }
            public bool _8000004 { get; set; }
            public bool a529c00 { get; set; }
            public bool c632106 { get; set; }
            public bool _18c5a400 { get; set; }
            public bool _8530000 { get; set; }
            public bool _100860 { get; set; }
            public bool _110002 { get; set; }
            public bool _110000 { get; set; }
            public bool _6429460 { get; set; }
            public bool _60140 { get; set; }
            public bool _320c63 { get; set; }
            public bool _18020 { get; set; }
            public bool _100840 { get; set; }
            public bool a4ac00 { get; set; }
            public bool e740000 { get; set; }
            public bool f73180 { get; set; }
            public bool f7318c { get; set; }
            public bool _210000 { get; set; }
            public bool _18d70000 { get; set; }
            public bool a510800 { get; set; }
            public bool _520080 { get; set; }
            public bool _842000 { get; set; }
            public bool _8421000 { get; set; }
            public bool _18005 { get; set; }
            public bool _18000 { get; set; }
            public bool _431880 { get; set; }
            public bool _2318000 { get; set; }
            public bool _42100 { get; set; }
            public bool e720000 { get; set; }
            public bool _4194c0 { get; set; }
            public bool a4a900 { get; set; }
            public bool _6100800 { get; set; }
            public bool _308840 { get; set; }
            public bool _100800 { get; set; }
            public bool _110060 { get; set; }
            public bool _629c00 { get; set; }
            public bool _320c00 { get; set; }
            public bool a4a920 { get; set; }
            public bool _6100860 { get; set; }
            public bool _2318c20 { get; set; }
            public bool c63800 { get; set; }
            public bool _4311000 { get; set; }
            public bool _4211082 { get; set; }
            public bool _4211080 { get; set; }
            public bool _210880 { get; set; }
            public bool _42008 { get; set; }
            public bool _63000a5 { get; set; }
            public bool _500ca0 { get; set; }
            public bool a49ce0 { get; set; }
            public bool a49ce7 { get; set; }
            public bool _2318c00 { get; set; }
            public bool _8421084 { get; set; }
            public bool _8421884 { get; set; }
            public bool c63180 { get; set; }
            public bool _29ce0 { get; set; }
            public bool _29ce5 { get; set; }
            public bool _842908 { get; set; }
            public bool _8531800 { get; set; }
            public bool _100060 { get; set; }
            public bool c7420c0 { get; set; }
            public bool f73000 { get; set; }
            public bool e629c00 { get; set; }
            public bool _100c03 { get; set; }
            public bool _6ad6b { get; set; }
            public bool _318c20 { get; set; }
            public bool _18820 { get; set; }
            public bool _41cc5 { get; set; }
            public bool c732120 { get; set; }
            public bool _2000c60 { get; set; }
            public bool _210800 { get; set; }
            public bool c4a400 { get; set; }
            public bool _100c60 { get; set; }
            public bool _8421800 { get; set; }
            public bool _1084a948 { get; set; }
            public bool _318c00 { get; set; }
            public bool _18800 { get; set; }
            public bool _6628000 { get; set; }
            public bool _16b60000 { get; set; }
            public bool _2000c40 { get; set; }
            public bool _310800 { get; set; }
            public bool _310802 { get; set; }
            public bool a629c00 { get; set; }
            public bool _1084a800 { get; set; }
            public bool _318c60 { get; set; }
            public bool _6318c63 { get; set; }
            public bool _100000 { get; set; }
            public bool _6318000 { get; set; }
            public bool _200a0 { get; set; }
            public bool _210840 { get; set; }
            public bool c63140 { get; set; }
            public bool ea3a400 { get; set; }
            public bool _8531880 { get; set; }
            public bool _8531884 { get; set; }
            public bool _24a5 { get; set; }
            public bool _49c00 { get; set; }
            public bool c62d80 { get; set; }
            public bool c63000 { get; set; }
            public bool _9420c0 { get; set; }
            public bool _24c7 { get; set; }
            public bool a52a0a5 { get; set; }
            public bool _4a8e0 { get; set; }
            public bool _842 { get; set; }
            public bool _840 { get; set; }
            public bool _2ce7 { get; set; }
            public bool _8400004 { get; set; }
            public bool _24e5 { get; set; }
            public bool _24e6 { get; set; }
            public bool _24e7 { get; set; }
            public bool _24e0 { get; set; }
            public bool _24e9 { get; set; }
            public bool _952c00 { get; set; }
            public bool _28e5 { get; set; }
            public bool _28e7 { get; set; }
            public bool e948000 { get; set; }
            public bool a52108 { get; set; }
            public bool _14a60000 { get; set; }
            public bool _110380c { get; set; }
            public bool _9018c6 { get; set; }
            public bool _50100 { get; set; }
            public bool _318800 { get; set; }
            public bool _128cc00 { get; set; }
            public bool _860 { get; set; }
            public bool _863 { get; set; }
            public bool _862 { get; set; }
            public bool _865 { get; set; }
            public bool _107b5ad { get; set; }
            public bool _107b5a0 { get; set; }
            public bool _94ad60 { get; set; }
            public bool _298c4 { get; set; }
            public bool _49d00 { get; set; }
            public bool _18c63800 { get; set; }
            public bool _16c58000 { get; set; }
            public bool _952900 { get; set; }
            public bool _800 { get; set; }
            public bool d6294a { get; set; }
            public bool d62940 { get; set; }
            public bool b6b400 { get; set; }
            public bool _2108000 { get; set; }
            public bool _49d20 { get; set; }
            public bool _539c00 { get; set; }
            public bool e8 { get; set; }
            public bool e700000 { get; set; }
            public bool e700007 { get; set; }
            public bool _31c07 { get; set; }
            public bool _31c00 { get; set; }
            public bool _5a529 { get; set; }
            public bool _31806 { get; set; }
            public bool _31800 { get; set; }
            public bool _31480 { get; set; }
            public bool _5ad29 { get; set; }
            public bool _5ad20 { get; set; }
            public bool c0 { get; set; }
            public bool _94a400 { get; set; }
            public bool _200c00 { get; set; }
            public bool _3a106 { get; set; }
            public bool _3a100 { get; set; }
            public bool a0 { get; set; }
            public bool a6 { get; set; }
            public bool a5 { get; set; }
            public bool _639c00 { get; set; }
            public bool e83a400 { get; set; }
            public bool _5a569 { get; set; }
            public bool _200800 { get; set; }
            public bool a5b120 { get; set; }
            public bool _631400 { get; set; }
            public bool _194c3 { get; set; }
            public bool _742000 { get; set; }
            public bool _1294a400 { get; set; }
            public bool _40100 { get; set; }
            public bool _18c58000 { get; set; }
            public bool _7018e0 { get; set; }
            public bool _630080 { get; set; }
            public bool d6b56b { get; set; }
            public bool d6b560 { get; set; }
            public bool d6b400 { get; set; }
            public bool _294a5 { get; set; }
            public bool _12a5ad20 { get; set; }
            public bool _12a5ad29 { get; set; }
            public bool e84a4e0 { get; set; }
            public bool _31c80 { get; set; }
            public bool _194a3 { get; set; }
            public bool e03800 { get; set; }
            public bool _5a920 { get; set; }
            public bool _3a520 { get; set; }
            public bool _31884 { get; set; }
            public bool _31880 { get; set; }
            public bool c62c00 { get; set; }
            public bool _1cc60000 { get; set; }
            public bool _1842 { get; set; }
            public bool _1294a529 { get; set; }
            public bool _9424e0 { get; set; }
            public bool _200c80 { get; set; }
            public bool _318000 { get; set; }
            public bool _25290000 { get; set; }
            public bool d68160 { get; set; }
            public bool _200080 { get; set; }
            public bool _22003800 { get; set; }
            public bool _18c63000 { get; set; }
            public bool _831400 { get; set; }
            public bool _838000 { get; set; }
            public bool e83a540 { get; set; }
            public bool a501080 { get; set; }
            public bool _418c00 { get; set; }
            public bool _1c84 { get; set; }
            public bool _421800 { get; set; }
            public bool a628000 { get; set; }
            public bool _200040 { get; set; }
            public bool _801c0 { get; set; }
            public bool _5b1c0 { get; set; }
            public bool _74a008 { get; set; }
            public bool _1d29 { get; set; }
            public bool _1d20 { get; set; }
            public bool _501440 { get; set; }
            public bool _8628000 { get; set; }
            public bool _1900 { get; set; }
            public bool _1906 { get; set; }
            public bool _418400 { get; set; }
            public bool a538000 { get; set; }
            public bool _200000 { get; set; }
            public bool _730000 { get; set; }
            public bool _320ca0 { get; set; }
            public bool _1d08 { get; set; }
            public bool _1d07 { get; set; }
            public bool _1d05 { get; set; }
            public bool _1d00 { get; set; }
            public bool _1886 { get; set; }
            public bool _1884 { get; set; }
            public bool _1082 { get; set; }
            public bool _1080 { get; set; }
            public bool _1084 { get; set; }
            public bool _10852900 { get; set; }
            public bool _741c00 { get; set; }
            public bool _418420 { get; set; }
            public bool _418421 { get; set; }
            public bool _420c0 { get; set; }
            public bool _420c6 { get; set; }
            public bool _602106 { get; set; }
            public bool _638000 { get; set; }
            public bool _74000 { get; set; }
            public bool _1c00 { get; set; }
            public bool _6110800 { get; set; }
            public bool _501400 { get; set; }
            public bool c700000 { get; set; }
            public bool _421080 { get; set; }
            public bool _53180 { get; set; }
            public bool e739ce7 { get; set; }
            public bool e739ce0 { get; set; }
            public bool _1c63 { get; set; }
            public bool _1d40 { get; set; }
            public bool a539c00 { get; set; }
            public bool _1044 { get; set; }
            public bool _1042 { get; set; }
            public bool _1040 { get; set; }
            public bool _18e70000 { get; set; }
            public bool _4a108 { get; set; }
            public bool _30080 { get; set; }
            public bool _2110000 { get; set; }
            public bool _254a { get; set; }
            public bool _2547 { get; set; }
            public bool _2540 { get; set; }
            public bool _2549 { get; set; }
            public bool _1000 { get; set; }
            public bool _531800 { get; set; }
            public bool _10a42900 { get; set; }
            public bool _2529 { get; set; }
            public bool _2527 { get; set; }
            public bool a520000 { get; set; }
            public bool _421000 { get; set; }
            public bool _84218c4 { get; set; }
            public bool _2500 { get; set; }
            public bool _2507 { get; set; }
            public bool a50000 { get; set; }
            public bool _10ca0 { get; set; }
            public bool _14a5294a { get; set; }
            public bool _14c58000 { get; set; }
            public bool _421040 { get; set; }
            public bool _421042 { get; set; }
            public bool a629ca0 { get; set; }
            public bool e93a4e0 { get; set; }
            public bool _52d69 { get; set; }
            public bool ea4a800 { get; set; }
            public bool _41d00 { get; set; }
            public bool _49ca0 { get; set; }
            public bool _4421040 { get; set; }
            public bool _731c00 { get; set; }
            public bool _2400 { get; set; }
            public bool d73000 { get; set; }
            public bool _41d27 { get; set; }
            public bool c8420c0 { get; set; }
            public bool _8721884 { get; set; }
            public bool _314a0 { get; set; }
            public bool _73800 { get; set; }
            public bool _2d29 { get; set; }
            public bool _498e6 { get; set; }
            public bool _1484a800 { get; set; }
            public bool _429000 { get; set; }
            public bool _52948 { get; set; }
            public bool _49ce8 { get; set; }
            public bool _49ce7 { get; set; }
            public bool _49ce0 { get; set; }
            public bool _2d4b { get; set; }
            public bool _14a5b180 { get; set; }
            public bool _14a5b18a { get; set; }
            public bool _6300840 { get; set; }
            public bool _31ce0 { get; set; }
            public bool _31ce5 { get; set; }
            public bool _2d6b { get; set; }
            public bool _2d69 { get; set; }
            public bool _4201080 { get; set; }
            public bool _94a4e0 { get; set; }
            public bool _2219000 { get; set; }
            public bool _6519460 { get; set; }
            public bool _52900 { get; set; }
            public bool _52908 { get; set; }
            public bool _6300860 { get; set; }
            public bool _424c0 { get; set; }
            public bool _4a948 { get; set; }
            public bool _4a940 { get; set; }
            public bool _301000 { get; set; }
            public bool _10738000 { get; set; }
            public bool a730000 { get; set; }
            public bool d00000 { get; set; }
            public bool _408c61 { get; set; }
            public bool _7340b { get; set; }
            public bool _2118c00 { get; set; }
            public bool _4220000 { get; set; }
            public bool _1f188000 { get; set; }
            public bool _20440 { get; set; }
            public bool _23180000 { get; set; }
            public bool _2118c20 { get; set; }
            public bool a5a400 { get; set; }
            public bool _2118c41 { get; set; }
            public bool a6294a0 { get; set; }
            public bool a5ad20 { get; set; }
            public bool _3194a3 { get; set; }
            public bool _8021000 { get; set; }
            public bool _29d00 { get; set; }
            public bool a5010a0 { get; set; }
            public bool _18c70000 { get; set; }
            public bool _529400 { get; set; }
            public bool _8314e0 { get; set; }
            public bool _29c00 { get; set; }
            public bool b52900 { get; set; }
            public bool e548000 { get; set; }
            public bool _429800 { get; set; }
            public bool _6418400 { get; set; }
            public bool _108c400 { get; set; }
            public bool _380a0 { get; set; }
            public bool _8631880 { get; set; }
            public bool _6418420 { get; set; }
            public bool _531ce0 { get; set; }
            public bool _531ce5 { get; set; }
            public bool _529c00 { get; set; }
            public bool _118c400 { get; set; }
            public bool _300c40 { get; set; }
            public bool _529460 { get; set; }
            public bool _318a { get; set; }
            public bool _318c { get; set; }
            public bool _600000 { get; set; }
            public bool e73800 { get; set; }
            public bool _1ce70000 { get; set; }
            public bool _4110000 { get; set; }
            public bool e742527 { get; set; }
            public bool _12950000 { get; set; }
            public bool _702000 { get; set; }
            public bool c628000 { get; set; }
            public bool _314a { get; set; }
            public bool _528c00 { get; set; }
            public bool _21042 { get; set; }
            public bool _21043 { get; set; }
            public bool _21040 { get; set; }
            public bool _3a4e0 { get; set; }
            public bool _1cc5 { get; set; }
            public bool _1cc7 { get; set; }
            public bool _321400 { get; set; }
            public bool _2d6b0000 { get; set; }
            public bool e742400 { get; set; }
            public bool _1ca7 { get; set; }
            public bool _1ca5 { get; set; }
            public bool _1ca0 { get; set; }
            public bool _3108 { get; set; }
            public bool _21000 { get; set; }
            public bool e73400 { get; set; }
            public bool _8010004 { get; set; }
            public bool a518000 { get; set; }
            public bool _8520000 { get; set; }
            public bool _628000 { get; set; }
            public bool _1ce0 { get; set; }
            public bool _1ce7 { get; set; }
            public bool _1ce5 { get; set; }
            public bool _628c60 { get; set; }
            public bool _628c63 { get; set; }
            public bool _6310000 { get; set; }
            public bool _14a50000 { get; set; }
            public bool e839ce0 { get; set; }
            public bool _12948000 { get; set; }
            public bool c63a000 { get; set; }
            public bool _629460 { get; set; }
            public bool _839c0 { get; set; }
            public bool _881e0 { get; set; }
            public bool _528060 { get; set; }
            public bool _6318c0 { get; set; }
            public bool _51ce7 { get; set; }
            public bool _110803 { get; set; }
            public bool _110800 { get; set; }
            public bool _10a5 { get; set; }
            public bool _10a4 { get; set; }
            public bool _10a2 { get; set; }
            public bool e730000 { get; set; }
            public bool _63194a3 { get; set; }
            public bool _21082 { get; set; }
            public bool _6428000 { get; set; }
            public bool _83a4e0 { get; set; }
            public bool _321040 { get; set; }
            public bool e83a4e0 { get; set; }
            public bool _628c00 { get; set; }
            public bool _8328000 { get; set; }
            public bool _1ec60000 { get; set; }
            public bool _431400 { get; set; }
            public bool _318c0 { get; set; }
            public bool _110840 { get; set; }
            public bool c5a40b { get; set; }
            public bool c5a400 { get; set; }
            public bool _521040 { get; set; }
            public bool _10800000 { get; set; }
            public bool f74000 { get; set; }
            public bool _18e68000 { get; set; }
            public bool a739ca0 { get; set; }
            public bool _5a4e0 { get; set; }
            public bool _211080 { get; set; }
            public bool c8320c0 { get; set; }
            public bool c602000 { get; set; }
            public bool _19080 { get; set; }
            public bool _19082 { get; set; }
            public bool _70180 { get; set; }
            public bool _18e0 { get; set; }
            public bool _18e4 { get; set; }
            public bool _18e7 { get; set; }
            public bool _18e6 { get; set; }
            public bool c5a529 { get; set; }
            public bool _320800 { get; set; }
            public bool _18c6 { get; set; }
            public bool _18c4 { get; set; }
            public bool _3def { get; set; }
            public bool _3ded { get; set; }
            public bool _400c0 { get; set; }
            public bool _42527 { get; set; }
            public bool _3010a0 { get; set; }
            public bool _35ad { get; set; }
            public bool f03000 { get; set; }
            public bool _18a4 { get; set; }
            public bool _400e0 { get; set; }
            public bool _949c00 { get; set; }
            public bool _211000 { get; set; }
            public bool _8420000 { get; set; }
            public bool _111000 { get; set; }
            public bool _8210000 { get; set; }
            public bool _10400 { get; set; }
            public bool c83a000 { get; set; }
            public bool _5018e0 { get; set; }
            public bool _4310040 { get; set; }
            public bool _48000 { get; set; }
            public bool _73a4e0 { get; set; }
            public bool _129cc00 { get; set; }
            public bool _310000 { get; set; }
            public bool _310002 { get; set; }
            public bool _39ce { get; set; }
            public bool _39cc { get; set; }
            public bool _2000043 { get; set; }
            public bool a639ca0 { get; set; }
            public bool _520840 { get; set; }
            public bool _520842 { get; set; }
            public bool _4310000 { get; set; }
            public bool _5294a0 { get; set; }
            public bool _14a0 { get; set; }
            public bool _14a3 { get; set; }
            public bool _14a5 { get; set; }
            public bool _14a7 { get; set; }
            public bool e838000 { get; set; }
            public bool _20078000 { get; set; }
            public bool b4a0e0 { get; set; }
            public bool c6420c0 { get; set; }
            public bool c4ac00 { get; set; }
            public bool _18420 { get; set; }
            public bool _18421 { get; set; }
            public bool _831ce0 { get; set; }
            public bool _520800 { get; set; }
            public bool _280c6 { get; set; }
            public bool a4a800 { get; set; }
            public bool a4a807 { get; set; }
            public bool a4a808 { get; set; }
            public bool _529ce0 { get; set; }
            public bool _8320800 { get; set; }
            public bool a539ca5 { get; set; }
            public bool a539ca0 { get; set; }
            public bool _7b5ad { get; set; }
            public bool _7b5a0 { get; set; }
            public bool _5294e5 { get; set; }
            public bool _14e5 { get; set; }
            public bool a728000 { get; set; }
            public bool _4298c0 { get; set; }
            public bool _4298c4 { get; set; }
            public bool _10a50000 { get; set; }
            public bool e94a4e0 { get; set; }
            public bool _4000004 { get; set; }
            public bool a531c00 { get; set; }
            public bool _6329463 { get; set; }
            public bool _6329460 { get; set; }
            public bool _1084000 { get; set; }
            public bool _3a400 { get; set; }
            public bool e021000 { get; set; }
            public bool _8200c80 { get; set; }
            public bool a4a400 { get; set; }
            public bool _63a106 { get; set; }
            public bool _839c00 { get; set; }
            public bool c000000 { get; set; }
            public bool _732000 { get; set; }
            public bool _23298000 { get; set; }
            public bool _3214a3 { get; set; }
            public bool _3214a0 { get; set; }
            public bool _18c68000 { get; set; }
            public bool _4000043 { get; set; }
            public bool _4000040 { get; set; }
            public bool _31ca { get; set; }
            public bool c6b800 { get; set; }
            public bool _8200004 { get; set; }
            public bool _2108421 { get; set; }
            public bool _631880 { get; set; }
            public bool _4000064 { get; set; }
            public bool _108400 { get; set; }
            public bool _401 { get; set; }
            public bool _10a58000 { get; set; }
            public bool _39c05 { get; set; }
            public bool _39c00 { get; set; }
            public bool _93a4e0 { get; set; }
            public bool _18c63 { get; set; }
            public bool _18c60 { get; set; }
            public bool _39460 { get; set; }
            public bool _12a5ac00 { get; set; }
            public bool _631900 { get; set; }
            public bool _84298c0 { get; set; }
            public bool _84298c4 { get; set; }
            public bool _421 { get; set; }
            public bool _5a120 { get; set; }
            public bool c40 { get; set; }
            public bool c41 { get; set; }
            public bool c43 { get; set; }
            public bool _51d20 { get; set; }
            public bool _2108c00 { get; set; }
            public bool b5ad20 { get; set; }
            public bool b5ad29 { get; set; }
            public bool _38c63 { get; set; }
            public bool _12952d69 { get; set; }
            public bool c63 { get; set; }
            public bool c60 { get; set; }
            public bool c61 { get; set; }
            public bool _74a400 { get; set; }
            public bool _18c23 { get; set; }
            public bool _18c21 { get; set; }
            public bool _18c20 { get; set; }
            public bool _51d00 { get; set; }
            public bool _464 { get; set; }
            public bool _2108c61 { get; set; }
            public bool c740000 { get; set; }
            public bool _4290c0 { get; set; }
            public bool c04 { get; set; }
            public bool c00 { get; set; }
            public bool c03 { get; set; }
            public bool _631800 { get; set; }
            public bool a500085 { get; set; }
            public bool a6394a0 { get; set; }
            public bool _8024e0 { get; set; }
            public bool _2018c00 { get; set; }
            public bool _742400 { get; set; }
            public bool _6319400 { get; set; }
            public bool _739c00 { get; set; }
            public bool c21 { get; set; }
            public bool _6320000 { get; set; }
            public bool a010000 { get; set; }
            public bool _311040 { get; set; }
            public bool _10601c00 { get; set; }
            public bool _4219082 { get; set; }
            public bool _4210000 { get; set; }
            public bool _84a4e0 { get; set; }
            public bool a5294a0 { get; set; }
            public bool a5294a5 { get; set; }
            public bool _2100c00 { get; set; }
            public bool a428000 { get; set; }
            public bool _4000840 { get; set; }
            public bool _108000e8 { get; set; }
            public bool _311000 { get; set; }
            public bool _6b560 { get; set; }
            public bool d6bc00 { get; set; }
            public bool c80 { get; set; }
            public bool c81 { get; set; }
            public bool _1ef88000 { get; set; }
            public bool _739400 { get; set; }
            public bool _83a400 { get; set; }
            public bool f7bda0 { get; set; }
            public bool _839400 { get; set; }
            public bool _1ef78000 { get; set; }
            public bool a500005 { get; set; }
            public bool a5294e0 { get; set; }
            public bool _531400 { get; set; }
            public bool _742520 { get; set; }
            public bool _639800 { get; set; }
            public bool a52940 { get; set; }
            public bool c6000a6 { get; set; }
            public bool _12a58000 { get; set; }
            public bool _6328000 { get; set; }
            public bool d62800 { get; set; }
            public bool _6228000 { get; set; }
            public bool _531c00 { get; set; }
            public bool _14a5318a { get; set; }
            public bool a5b000 { get; set; }
            public bool _38000 { get; set; }
            public bool _31084 { get; set; }
            public bool _31080 { get; set; }
            public bool _6294a { get; set; }
            public bool _62940 { get; set; }
            public bool _8220880 { get; set; }
            public bool a52900 { get; set; }
            public bool _2310000 { get; set; }
            public bool _68160 { get; set; }
            public bool a530000 { get; set; }
            public bool _530000 { get; set; }
            public bool _749c00 { get; set; }
            public bool _6529460 { get; set; }
            public bool _8400c0 { get; set; }
            public bool _10000000 { get; set; }
            public bool _62900 { get; set; }
            public bool _1ef70000 { get; set; }
            public bool _6520000 { get; set; }
            public bool _214a0 { get; set; }
            public bool _214a3 { get; set; }
            public bool _4321040 { get; set; }
            public bool a52800 { get; set; }
            public bool _94ac00 { get; set; }
            public bool _10848000 { get; set; }
            public bool _8840 { get; set; }
            public bool _6110c61 { get; set; }
            public bool _10600000 { get; set; }
            public bool e529c00 { get; set; }
            public bool _8420c0 { get; set; }
            public bool _419400 { get; set; }
        }
        #endregion
        #region Lessons
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
        #region Nonstop Play
        public class NSPPlayListRoot2
        {
            public double NSP_PlayListArrangements { get; set; }
            public double NSP_PlayListTuning { get; set; }
            public double NSP_PlayListSongList { get; set; }
            public double NSP_PlayListPlayOrder { get; set; }
            public double Duration { get; set; }
        }
        #endregion
        #region Options
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
        #region Guitarcade
        public class GC_ActiveChallengesCumulative
        {
            public double _2 { get; set; }
            public double _1 { get; set; }
            public double _0 { get; set; }
        }

        public class GC_ActiveChallenges
        {
            public double _2 { get; set; }
            public double _1 { get; set; }
            public double _0 { get; set; }
        }

        public class GC_ActiveChallengesRandomIndex
        {
            public double _2 { get; set; }
            public double _1 { get; set; }
            public double _0 { get; set; }
        }

        public class GC_CompletedChallenges
        {
            public double _3 { get; set; }
            public double _2 { get; set; }
            public double _1 { get; set; }
            public double _6 { get; set; }
            public double _4 { get; set; }
        }

        public class GC_Guitar
        {
            public double LastTimePlayed { get; set; }
            public double HighScore { get; set; }
            public GC_ActiveChallengesCumulative ActiveChallengesCumulative { get; set; }
            public GC_ActiveChallenges ActiveChallenges { get; set; }
            public GC_ActiveChallengesRandomIndex ActiveChallengesRandomIndex { get; set; }
            public GC_CompletedChallenges CompletedChallenges { get; set; }
            public double PlayCount { get; set; }
            public double SkillLevel { get; set; }
            public double CurrentLevel { get; set; }
            public double LevelsUnlockedChapterScreen { get; set; }
            public double LevelsUnlocked { get; set; }
        }

        public class GC_Bass
        {
            public double LastTimePlayed { get; set; }
            public double HighScore { get; set; }
            public GC_ActiveChallengesCumulative ActiveChallengesCumulative { get; set; }
            public GC_ActiveChallenges ActiveChallenges { get; set; }
            public GC_ActiveChallengesRandomIndex ActiveChallengesRandomIndex { get; set; }
            public GC_CompletedChallenges CompletedChallenges { get; set; }
            public double PlayCount { get; set; }
            public double SkillLevel { get; set; }
            public double CurrentLevel { get; set; }
            public double LevelsUnlockedChapterScreen { get; set; }
            public double LevelsUnlocked { get; set; }
        }

        public class GC_TrackAndField
        {
            public GC_Guitar Guitar { get; set; }
            public GC_Bass Bass { get; set; }
        }

        public class GC_NinjaSlides
        {
            public GC_Guitar Guitar { get; set; }
            public GC_Bass Bass { get; set; }
        }

        public class GC_32925
        {
            public GC_Guitar Guitar { get; set; }
            public GC_Bass Bass { get; set; }
        }

        public class GC_32926
        {
            public GC_Guitar Guitar { get; set; }
            public GC_Bass Bass { get; set; }
        }

        public class GC_DucksPlus
        {
            public GC_Guitar Guitar { get; set; }
            public GC_Bass Bass { get; set; }
        }

        public class GC_TempleOfBends
        {
            public GC_Guitar Guitar { get; set; }
            public GC_Bass Bass { get; set; }
        }

        public class GC_0
        {
            public double _4 { get; set; }
            public double _3 { get; set; }
            public double _2 { get; set; }
            public double _1 { get; set; }
        }

        public class GC_ScaleRacer
        {
            public GC_Guitar Guitar { get; set; }
            public GC_Bass Bass { get; set; }
        }

        public class GC_StarChords
        {
            public GC_Guitar Guitar { get; set; }
            public GC_Bass Bass { get; set; }
        }

        public class GC_HarmonicHeist
        {
            public GC_Guitar Guitar { get; set; }
            public GC_Bass Bass { get; set; }
        }

        public class GC_WhaleRider
        {
            public GC_Guitar Guitar { get; set; }
            public GC_Bass Bass { get; set; }
        }

        public class GC_StringSkipSaloon
        {
            public GC_Guitar Guitar { get; set; }
            public GC_Bass Bass { get; set; }
        }

        public class GuitarcadeGames
        {
            public GC_TrackAndField GC_TrackAndField { get; set; }
            public GC_NinjaSlides GC_NinjaSlides { get; set; }
            [JsonProperty("$[32925]")]
            public GC_32925 _32925 { get; set; }
            [JsonProperty("$[32926]")]
            public GC_32926 _32926 { get; set; }
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

        #region Dynamic Difficulty
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

        #region Prizes
        public class Prizes
        {
            public double UnawardedPrizePoints { get; set; }
            public bool IsPrizeAwarded_40 { get; set; }
            public bool IsPrizeAwarded_41 { get; set; }
            public bool IsPrizeAwarded_42 { get; set; }
            public bool IsPrizeAwarded_43 { get; set; }
            public bool IsPrizeAwarded_44 { get; set; }
            public bool IsPrizeAwarded_46 { get; set; }
            public bool IsPrizeAwarded_47 { get; set; }
            public bool IsPrizeAwarded_48 { get; set; }
            public bool IsPrizeAwarded_49 { get; set; }
            public bool IsPrizeAwarded_53 { get; set; }
            public bool IsPrizeAwarded_52 { get; set; }
            public bool IsPrizeAwarded_50 { get; set; }
            public bool IsPrizeAwarded_55 { get; set; }
            public bool IsPrizeAwarded_54 { get; set; }
            public bool IsPrizeAwarded_19 { get; set; }
            public bool IsPrizeAwarded_18 { get; set; }
            public bool IsPrizeAwarded_17 { get; set; }
            public bool IsPrizeAwarded_16 { get; set; }
            public bool IsPrizeAwarded_15 { get; set; }
            public bool IsPrizeAwarded_13 { get; set; }
            public bool IsPrizeAwarded_12 { get; set; }
            public bool IsPrizeAwarded_11 { get; set; }
            public bool IsPrizeAwarded_10 { get; set; }
            public bool IsPrizeAwarded_23 { get; set; }
            public bool IsPrizeAwarded_20 { get; set; }
            public bool IsPrizeAwarded_21 { get; set; }
            public bool IsPrizeAwarded_26 { get; set; }
            public bool IsPrizeAwarded_27 { get; set; }
            public bool IsPrizeAwarded_24 { get; set; }
            public bool IsPrizeAwarded_25 { get; set; }
            public bool IsPrizeAwarded_28 { get; set; }
            public bool IsPrizeAwarded_35 { get; set; }
            public bool IsPrizeAwarded_34 { get; set; }
            public bool IsPrizeAwarded_37 { get; set; }
            public bool IsPrizeAwarded_31 { get; set; }
            public bool IsPrizeAwarded_30 { get; set; }
            public bool IsPrizeAwarded_33 { get; set; }
            public bool IsPrizeAwarded_32 { get; set; }
            public bool IsPrizeAwarded_39 { get; set; }
            public bool IsPrizeAwarded_38 { get; set; }
            public bool IsPrizeAwarded_4 { get; set; }
            public bool IsPrizeAwarded_5 { get; set; }
            public bool IsPrizeAwarded_6 { get; set; }
            public bool IsPrizeAwarded_1 { get; set; }
            public bool IsPrizeAwarded_2 { get; set; }
            public bool IsPrizeAwarded_3 { get; set; }
            public bool IsPrizeAwarded_8 { get; set; }
            public bool IsPrizeAwarded_9 { get; set; }
            public UPlayRewards UPlayRewards { get; set; }
            public bool HasPrizeDialogShown_50 { get; set; }
            public bool HasPrizeDialogShown_52 { get; set; }
            public bool HasPrizeDialogShown_53 { get; set; }
            public bool HasPrizeDialogShown_54 { get; set; }
            public bool HasPrizeDialogShown_55 { get; set; }
            public bool HasPrizeDialogShown_43 { get; set; }
            public bool HasPrizeDialogShown_42 { get; set; }
            public bool HasPrizeDialogShown_41 { get; set; }
            public bool HasPrizeDialogShown_40 { get; set; }
            public bool HasPrizeDialogShown_47 { get; set; }
            public bool HasPrizeDialogShown_46 { get; set; }
            public bool HasPrizeDialogShown_44 { get; set; }
            public bool HasPrizeDialogShown_49 { get; set; }
            public bool HasPrizeDialogShown_48 { get; set; }
            public bool HasPrizeDialogShown_18 { get; set; }
            public bool HasPrizeDialogShown_19 { get; set; }
            public bool HasPrizeDialogShown_15 { get; set; }
            public bool HasPrizeDialogShown_16 { get; set; }
            public bool HasPrizeDialogShown_17 { get; set; }
            public bool HasPrizeDialogShown_10 { get; set; }
            public bool HasPrizeDialogShown_11 { get; set; }
            public bool HasPrizeDialogShown_12 { get; set; }
            public bool HasPrizeDialogShown_13 { get; set; }
            public bool HasPrizeDialogShown_32 { get; set; }
            public bool HasPrizeDialogShown_33 { get; set; }
            public bool HasPrizeDialogShown_30 { get; set; }
            public bool HasPrizeDialogShown_31 { get; set; }
            public bool HasPrizeDialogShown_37 { get; set; }
            public bool HasPrizeDialogShown_34 { get; set; }
            public bool HasPrizeDialogShown_35 { get; set; }
            public bool HasPrizeDialogShown_38 { get; set; }
            public bool HasPrizeDialogShown_39 { get; set; }
            public bool HasPrizeDialogShown_25 { get; set; }
            public bool HasPrizeDialogShown_24 { get; set; }
            public bool HasPrizeDialogShown_27 { get; set; }
            public bool HasPrizeDialogShown_26 { get; set; }
            public bool HasPrizeDialogShown_21 { get; set; }
            public bool HasPrizeDialogShown_20 { get; set; }
            public bool HasPrizeDialogShown_23 { get; set; }
            public bool HasPrizeDialogShown_28 { get; set; }
            public bool UplayAction3 { get; set; }
            public bool UplayAction1 { get; set; }
            public bool UplayAction4 { get; set; }
            public bool UplayDlg4 { get; set; }
            public bool UplayDlg1 { get; set; }
            public bool UplayDlg3 { get; set; }
            public double NumPrizePoints { get; set; }
            public bool HasPrizeDialogShown_5 { get; set; }
            public bool HasPrizeDialogShown_4 { get; set; }
            public bool HasPrizeDialogShown_6 { get; set; }
            public bool HasPrizeDialogShown_1 { get; set; }
            public bool HasPrizeDialogShown_3 { get; set; }
            public bool HasPrizeDialogShown_2 { get; set; }
            public bool HasPrizeDialogShown_9 { get; set; }
            public bool HasPrizeDialogShown_8 { get; set; }
        }

        public class RS2Reward
        {
            public double value { get; set; }
            public bool purchased { get; set; }
            public string name { get; set; }
            public string description { get; set; }
        }

        public class UPlayRewards
        {
            public RS2Reward RS2REWARD04 { get; set; }
            public RS2Reward RS2REWARD03 { get; set; }
            public RS2Reward RS2REWARD02 { get; set; }
        }

        #endregion
        #region Songs
        public class Songs_Root
        {
            public Dictionary<string, SongDetails> Songs { get; set; }
        }

        public class SongDetails
        {
            public double TimeStamp { get; set; }
            public Song_DD DynamicDifficulty { get; set; }

            [JsonProperty(Required = Newtonsoft.Json.Required.AllowNull)]
            public Song_PlaynextStats PlaynextStats { get; set; }
        }

        public class Song_DD
        {
            public double Avg { get; set; }

            [JsonProperty(Required = Required.AllowNull)]
            public Dictionary<string, Song_DD_Phrases> Phrases { get; set; }

            [JsonProperty(Required = Required.AllowNull)]
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
        }

        public class Song_PlaynextStats_PhraseIterations
        {
            public Dictionary<string, Song_PlaynextStats_PhraseIteration> PhaseIteration { get; set; }
            public Dictionary<string, Song_PlaynextStats_Chords> Chords { get; set; }
            public Dictionary<string, Song_PlaynextStats_Articulations> Articulations { get; set; }
        }

        public class Song_PlaynextStats_PhraseIteration
        {
            [JsonProperty(Required = Required.AllowNull)]
            public double ChordsMissed { get; set; }

            [JsonProperty(Required = Required.AllowNull)]
            public double ArticulationsMissed { get; set; }

            [JsonProperty(Required = Required.AllowNull)]
            public double NotesMissed { get; set; }
            [JsonProperty(Required = Required.AllowNull)]
            public double ChordsHit { get; set; }

            public double NotesHit { get; set; }
            public double ArticulationsHit { get; set; }
        }

        public class Song_PlaynextStats_Chords
        {
            public double Hit { get; set; }

            [JsonProperty(Required = Required.AllowNull)]
            public double Missed { get; set; }
        }

        public class Song_PlaynextStats_Articulations
        {
            public double Hit { get; set; }

            [JsonProperty(Required = Required.AllowNull)]
            public double Missed { get; set; }
        }
        #endregion
        #region Session Mode
        public class SessionMode
        {
            [JsonProperty(Required = Required.AllowNull)]
            public SessionMode_CustomBands CustomBands { get; set; }

            public SessionMode_Missions Missions { get; set; }

            public SessionMode_Settings Settings { get; set; }

        }

        public class SessionMode_CustomBands { }

        public class SessionMode_Mission
        {
            [JsonProperty(Required = Required.AllowNull)]
            public double TimeStamp { get; set; }
            public double PlayCount { get; set; }
        }

        public class SessionMode_Tempos
        {
            public SessionMode_Mission _8 { get; set; }
            public SessionMode_Mission _3 { get; set; }
            public SessionMode_Mission _2 { get; set; }
        }

        public class SessionMode_Notes
        {
            public SessionMode_Mission _9 { get; set; }
            public SessionMode_Mission _6 { get; set; }
            public SessionMode_Mission _2 { get; set; }
        }


        public class SessionMode_Scales
        {
            public SessionMode_Mission _7 { get; set; }
            public SessionMode_Mission _1 { get; set; }
            public SessionMode_Mission _14 { get; set; }
            public SessionMode_Mission _15 { get; set; }
        }

        public class SessionMode_Rooms
        {
            public SessionMode_Mission _4 { get; set; }
            public SessionMode_Mission _1 { get; set; }
            public SessionMode_Mission _0 { get; set; }
        }

        public class SessionMode_Complexities
        {
            public SessionMode_Mission _1 { get; set; }
            public SessionMode_Mission _0 { get; set; }
        }

        public class SessionMode_Grooves
        {
            public SessionMode_Mission _2 { get; set; }
            public SessionMode_Mission _1 { get; set; }
        }

        public class SessionMode_Missions
        {
            public SessionMode_Mission BF96EB56102435A29C9A3D97C649165B { get; set; }
            public SessionMode_Mission _1219C8F1A6A735E8A7CE398322D96688 { get; set; }
            public SessionMode_Mission _1F76B8CF98CD343282F458D29903800F { get; set; }
            public SessionMode_Mission _010FE6D6292F3A8EB8258A802C57B230 { get; set; }
            public SessionMode_Mission DB67160C7BD43582BC8E785D60A447BE { get; set; }
            public SessionMode_Mission _411261DDF4FE3BD88A27712AE3986F9C { get; set; }
            public SessionMode_Mission F7B3D501A72134058FA09E5C49A22E75 { get; set; }
            public SessionMode_Mission C3C878CF9BA23CDD837E96495AA7A702 { get; set; }
            public SessionMode_Mission _9C0427E4DC4B370BB9F64A88A9CEFC56 { get; set; }
            public SessionMode_Mission _1A236BD1D8FA3FC8934283ADC5F0F773 { get; set; }
            public SessionMode_Mission E5501BDA8A593CAC8BA50B3EC78235EF { get; set; }
            public SessionMode_Mission _9F6A7AA159483AC1879EC0BFCAC2944B { get; set; }
            public SessionMode_Mission _03B13729D4F63F8FBA0C63767C9262B8 { get; set; }
            public SessionMode_Mission _03ADA6BA199E3222AB681CA78B477111 { get; set; }
            public SessionMode_Mission AA495F0A950535A18372E82159A3C972 { get; set; }
            public SessionMode_Mission DCFEFFB614AC3005BD72474099CE1C4E { get; set; }
            public SessionMode_Mission CC899076BE6134DABB467DEC8F569C4D { get; set; }
            public SessionMode_Mission DF2B1032F33132358DB8363654FFA000 { get; set; }
            public SessionMode_Mission _1A4795A54EAB3D21B8863164F98E0811 { get; set; }
            public SessionMode_Mission _48FB1981378B30FF8BECC9CA7A70D825 { get; set; }
            public SessionMode_Mission A0A96416DC3633609BCBD01E07C6025D { get; set; }
            public SessionMode_Mission _2DE905F2730A349A9DCE8C4A9B8B3901 { get; set; }
            public SessionMode_Mission DF17AF729F06377B9BAFA16B2CCA7753 { get; set; }
            public SessionMode_Tempos Tempos { get; set; }
            public SessionMode_Mission C3372A367862331BB1726140FA3D0A2B { get; set; }
            public SessionMode_Mission _06340C8536A73735BCCF07A5A6A19AE7 { get; set; }
            public SessionMode_Mission _0B4926A17B1E392E99C08E78D97280B6 { get; set; }
            public SessionMode_Mission B966FC32977730D2AE23164972804684 { get; set; }
            public SessionMode_Mission _847363FB782D31498EE6C2A4C0259660 { get; set; }
            public SessionMode_Mission EAB905E9D9C637AA8613C81B194D0789 { get; set; }
            public SessionMode_Mission _4E6B94366C0F34148757470B0889041D { get; set; }
            public SessionMode_Mission _4A344400452C39AC926066541F198209 { get; set; }
            public SessionMode_Mission _65998043CFA033798F419FBC0FB8E806 { get; set; }
            public SessionMode_Mission B95856593B4F34879D8959FE2BAFF438 { get; set; }
            public SessionMode_Mission FDB67B32BA4832098E553148B23B7623 { get; set; }
            public SessionMode_Mission _93BD5C3358B93387AB2598BB396DBFAB { get; set; }
            public SessionMode_Mission _48FD0185AB4A362AB7CF8B5DD041CA50 { get; set; }
            public SessionMode_Mission _013212529E333EFFB20116387AB01447 { get; set; }
            public SessionMode_Mission _4161F23998743F90A385C33ABA304C0A { get; set; }
            public SessionMode_Mission AD3C0F12ED7A3D229A893FB871CACB85 { get; set; }
            public SessionMode_Mission C8CC52ED91CF3BAC90E2AA3CBA5D8692 { get; set; }
            public SessionMode_Mission _6B357F63837E3B4599EA21E4189CFF22 { get; set; }
            public SessionMode_Mission _65DFDC5C5D8435F59D19D06D71AED4B7 { get; set; }
            public SessionMode_Mission _2558B65D503236F5892E7CF055F94317 { get; set; }
            public SessionMode_Mission AA7902F6BD7C368C8AE8C3ACE00E6354 { get; set; }
            public SessionMode_Mission B0E91AE437673DEA85B2EEE9173B36AB { get; set; }
            public SessionMode_Notes Notes { get; set; }
            public SessionMode_Mission F8174557419332FD9814EAD805C5E72F { get; set; }
            public SessionMode_Mission _9BA7702A70313CEAA03C6950FAACE821 { get; set; }
            public SessionMode_Mission E9B88B6CC74A31FBB6B610EA21F49E37 { get; set; }
            public SessionMode_Mission _09A5C099B5853C5DB44F9D7A9C8FC404 { get; set; }
            public SessionMode_Mission _539ECA429D4E333FA2B0D97E9A826EFA { get; set; }
            public SessionMode_Mission _6CBECBF7C5B0340DB001255E880359EC { get; set; }
            public SessionMode_Mission _7E24D00F50653AB0891470C10FE543D0 { get; set; }
            public SessionMode_Mission AA07ABD2545D398CBD3561D47961AACC { get; set; }
            public SessionMode_Mission _739C196158ED38DD9B58E534C7F9B17B { get; set; }
            public SessionMode_Mission FF55C9680E3C3355AA8EBE683F278C05 { get; set; }
            public SessionMode_Mission BE4DFDBA138B3C6282B5FC1914F6E6EC { get; set; }
            public SessionMode_Mission _9E852B48AA5437D88323F99811862416 { get; set; }
            public SessionMode_Mission EFCD8A9BE2CC3896B50C62A1885D8578 { get; set; }
            public SessionMode_Mission F7F0118DC5763B4784FF84E9CEE28CE9 { get; set; }
            public SessionMode_Mission _87C5D0B1DC0E326E90EC212D43F77DA5 { get; set; }
            public SessionMode_Mission DFC1F34FCFF23DF69B535DABE5AAEAD6 { get; set; }
            public SessionMode_Scales Scales { get; set; }
            public SessionMode_Mission _1A6B23287C703658ACF30D63D8EC0A27 { get; set; }
            public SessionMode_Mission _286D3F2E908330CC9FB04C734DCD3586 { get; set; }
            public SessionMode_Mission _2CCC26EF66E332C690E159661E1E2116 { get; set; }
            public SessionMode_Mission _53F60DC4BE133E4998389C0247AC3FA6 { get; set; }
            public SessionMode_Mission _64497E174E4933A58346AE5D325B3EE9 { get; set; }
            public SessionMode_Mission _5BAD0314D6A931C088E3F0CA2B57F59C { get; set; }
            public SessionMode_Mission _679B3EE913423F3EB20511AB43420BEA { get; set; }
            public SessionMode_Rooms Rooms { get; set; }
            public SessionMode_Mission _281E3623FB6B34A3B5DB99B69ED65B8F { get; set; }
            public SessionMode_Mission D71194C59C163842B0D06A15EBBAEB77 { get; set; }
            public SessionMode_Mission _2EE5059D6B213C188EA4BBF758A99004 { get; set; }
            public SessionMode_Mission _781CA376FACC385097ACEF8B83017227 { get; set; }
            public SessionMode_Mission E57F5A48AE0235BEAC4BA3FA26C81DE7 { get; set; }
            public SessionMode_Mission AB9B725327713916888DDBF80878946E { get; set; }
            public SessionMode_Complexities Complexities { get; set; }
            public SessionMode_Grooves Grooves { get; set; }
            public SessionMode_Mission AF5B84E33F153E4E9E0D2CFFE0AFA614 { get; set; }
            public SessionMode_Mission _05E7C2A820D83A67A64ABBF612CC4287 { get; set; }
            public SessionMode_Mission _8A1C59EA18EB33FF92B4933FC88396C1 { get; set; }
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
        #region Achievements
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
        #region Stats
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
            public Stats_DLCTag DLCTag { get; set; }
            public double TotalNumMissionCompletions { get; set; }
            public double SASongsPlayedMediumAndMore { get; set; }
            public double NumSongsMastered { get; set; }
            public double DLCPlayedCount { get; set; }
            public double FingerprintMastery_Bass { get; set; }
            public double SongsLeadPlayedCount { get; set; }
            public double SessionMissionTime { get; set; }
            public double TimePlayedAndLesson { get; set; }
            public string HitTechniques { get; set; }
            public List<Stats_GuitarcadePlayedTime> GuitarcadePlayedTime { get; set; }
            public double DiversityRating { get; set; }
            public List<Stats_LessonPlayedCount> LessonPlayedCount { get; set; }
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
            public Stats_Songs Songs { get; set; }
            public double LongestSession { get; set; }
            public double FingerprintAccuracy_Bass { get; set; }
            public string Achievements { get; set; }
            public double AccuracyRating { get; set; }
            public List<Stats_ArticulationAccuracy> ArticulationAccuracy { get; set; }
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
            public List<Stats_GuitarcadePlayedCnt> GuitarcadePlayedCnt { get; set; }
            public double FingerprintMastery_Lead { get; set; }
            public double SASongsPlayedEasyAndMore { get; set; }
            public double LeadSkillRating { get; set; }
            public double GuitarcadeScaleRacerHS { get; set; }
            public double TimePlayed { get; set; }
            public double TimeShop { get; set; }
            public double ConsecutiveDays { get; set; }
            public string TSMissCompleted { get; set; }
        }

        public class Stats_DLCTag
        {
            public Dictionary<string, bool> DLCTag { get; set; }
        }

        public class Stats_GuitarcadePlayedTime
        {
            public double V { get; set; }
        }

        public class Stats_LessonPlayedCount
        {
            public double V { get; set; }
        }

        public class Stats_Songs
        {
            public Dictionary<string, Stats_Song> Songs { get; set; }
        }

        public class Stats_Song
        {
            [JsonProperty(Required = Newtonsoft.Json.Required.AllowNull)]
            public double MasteryLast { get; set; }

            [JsonProperty(Required = Newtonsoft.Json.Required.AllowNull)]
            public double PlayedCount { get; set; }

            [JsonProperty(Required = Newtonsoft.Json.Required.AllowNull)]
            public double AccuracyGlobal { get; set; }

            [JsonProperty(Required = Newtonsoft.Json.Required.AllowNull)]
            public string DateLAS { get; set; }

            [JsonProperty(Required = Newtonsoft.Json.Required.AllowNull)]
            public double AccuracyChords { get; set; }

            [JsonProperty(Required = Newtonsoft.Json.Required.AllowNull)]
            public double MasteryPrevious { get; set; }

            [JsonProperty(Required = Newtonsoft.Json.Required.AllowNull)]
            public double MasteryPeak { get; set; }

            [JsonProperty(Required = Newtonsoft.Json.Required.AllowNull)]
            public double MasteryPreviousPeak { get; set; }

            [JsonProperty(Required = Newtonsoft.Json.Required.AllowNull)]
            public double Streak { get; set; }

            [JsonProperty(Required = Newtonsoft.Json.Required.AllowNull)]
            public List<Stats_ArticulationAccuracy> ArticulationAccuracy { get; set; }

            [JsonProperty(Required = Newtonsoft.Json.Required.AllowNull)]
            public List<Stats_ChordsAccuracies> ChordsAccuracies { get; set; }

        }

        public class Stats_ArticulationAccuracy
        {
            public double V { get; set; }
        }

        public class Stats_ChordsAccuracies
        {
            public double V { get; set; }
        }

        public class Stats_History
        {
            public string LastUpdate { get; set; }
        }

        public class Stats_GuitarcadePlayedCnt
        {
            public double V { get; set; }
        }
        #endregion
        #region Score Attack
        public class SongsSARoot
        {
            public Dictionary<string, SA_SongDetails> SongsSA { get; set; }
        }

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
    }

}
#endregion