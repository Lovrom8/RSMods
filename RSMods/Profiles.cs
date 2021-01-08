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
            DirectoryInfo directory = new DirectoryInfo(BackupProfile.GetSaveDirectory());
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

            if (RSMods.ProfileClasses.CSharpArgumentInfo.callSite_2 == null)
            {
                RSMods.ProfileClasses.CSharpArgumentInfo.callSite_2 = CallSite<Func<CallSite, object, string, object, object>>.Create(BinderSetIndex(CSharpBinderFlags.None, Type.GetTypeFromHandle(typeof(Profiles).TypeHandle), new Microsoft.CSharp.RuntimeBinder.CSharpArgumentInfo[]
                {
                    CreateCSharpArgumentInfo(CSharpArgumentInfoFlags.None),
                    CreateCSharpArgumentInfo(CSharpArgumentInfoFlags.UseCompileTimeType),
                    CreateCSharpArgumentInfo(CSharpArgumentInfoFlags.None),
                }));
            }
            RSMods.ProfileClasses.CSharpArgumentInfo.callSite_2.Target(RSMods.ProfileClasses.CSharpArgumentInfo.callSite_2, profile, nameOfSection, argument);

            JsonSerializerSettings serializerSettings = new JsonSerializerSettings()
            {
                NullValueHandling = NullValueHandling.Ignore,
                StringEscapeHandling = StringEscapeHandling.EscapeNonAscii,
                Formatting = Formatting.Indented,
                Converters = new List<JsonConverter> { new RSMods.ProfileClasses.DecimalFormatJsonConverter(6) }
            };

            if (RSMods.ProfileClasses.CSharpArgumentInfo.callSite_3 == null)
                RSMods.ProfileClasses.CSharpArgumentInfo.callSite_3 = CallSite<Func<CallSite, object, string>>.Create(BinderConvert(CSharpBinderFlags.None, Type.GetTypeFromHandle(typeof(string).TypeHandle), Type.GetTypeFromHandle(typeof(Profiles).TypeHandle)));

            Func<CallSite, object, string> target = RSMods.ProfileClasses.CSharpArgumentInfo.callSite_3.Target;
            CallSite callsite_3_temp = RSMods.ProfileClasses.CSharpArgumentInfo.callSite_3;

            if (RSMods.ProfileClasses.CSharpArgumentInfo.callSite_4 == null)
            {
                RSMods.ProfileClasses.CSharpArgumentInfo.callSite_4 = CallSite<Func<CallSite, Type, object, JsonSerializerSettings, object>>.Create(BinderInvokeMember(CSharpBinderFlags.None, "SerializeObject", Type.GetTypeFromHandle(typeof(Profiles).TypeHandle), new Microsoft.CSharp.RuntimeBinder.CSharpArgumentInfo[] {
                    CreateCSharpArgumentInfo(CSharpArgumentInfoFlags.UseCompileTimeType | CSharpArgumentInfoFlags.IsStaticType),
                    CreateCSharpArgumentInfo(CSharpArgumentInfoFlags.None),
                    CreateCSharpArgumentInfo(CSharpArgumentInfoFlags.UseCompileTimeType)
                }));
            }
            return target(callsite_3_temp, RSMods.ProfileClasses.CSharpArgumentInfo.callSite_4.Target(RSMods.ProfileClasses.CSharpArgumentInfo.callSite_4, Type.GetTypeFromHandle(typeof(JsonConvert).TypeHandle), profile, serializerSettings));
        }

        private static CallSiteBinder BinderSetIndex(CSharpBinderFlags flags, Type userProfileType, IEnumerable<Microsoft.CSharp.RuntimeBinder.CSharpArgumentInfo> cSharpArgumentInfo) => Binder.SetIndex(flags, userProfileType, cSharpArgumentInfo);
        private static Microsoft.CSharp.RuntimeBinder.CSharpArgumentInfo CreateCSharpArgumentInfo(CSharpArgumentInfoFlags flags) => Microsoft.CSharp.RuntimeBinder.CSharpArgumentInfo.Create(flags, null);
        private static CallSiteBinder BinderConvert(CSharpBinderFlags flags, Type to, Type from) => Binder.Convert(flags, to, from);
        private static CallSiteBinder BinderInvokeMember(CSharpBinderFlags flags, string name, Type context, IEnumerable<Microsoft.CSharp.RuntimeBinder.CSharpArgumentInfo> argumentInfo) => Binder.InvokeMember(flags, name, null, context, argumentInfo);

        #endregion
    }
}
#region Extra Classes
namespace RSMods.ProfileClasses
{
    public static class CSharpArgumentInfo
    {
        // Token: 0x04000051 RID: 81
        public static CallSite<Func<CallSite, object, string, object, object>> callSite_0;

        // Token: 0x04000052 RID: 82
        public static CallSite<Func<CallSite, object, string, object>> callSite_1;

        // Token: 0x04000053 RID: 83
        public static CallSite<Func<CallSite, object, string, object, object>> callSite_2;

        // Token: 0x04000054 RID: 84
        public static CallSite<Func<CallSite, object, string>> callSite_3;

        // Token: 0x04000055 RID: 85
        public static CallSite<Func<CallSite, Type, object, JsonSerializerSettings, object>> callSite_4;
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

    public class FavoritesListRoot
    {
        public List<string> FavoritesList { get; set; }
    }

    public class SongListsRoot
    {
        public List<List<string>> SongLists { get; set; }
    }

}
#endregion